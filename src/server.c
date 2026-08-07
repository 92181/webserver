#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <openssl/ssl.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fcntl.h>

#ifndef __linux__
  #include "./kqueue.c"
#else
  #include <liburing.h>
  #include <linux/stat.h>
  #include <sys/resource.h>
  #include <linux/tls.h>
  #include <poll.h>
#endif

#define THREAD_CLIENTS 65536
#define THREAD_CONC_CLIENTS 2048
#define PACKET_SIZE 16384

#define WR_QUEUE 8

// Define Structures;
typedef struct td td; typedef struct ud ud;

struct file_in {int file; struct statx st; ud *client; void *b;};

struct cx {const void *b; unsigned int l;};

typedef struct
{
  unsigned int ip[4];
  unsigned short port;
  td *ti, *te;

  SSL_CTX *ssl;
  EVP_PKEY *key;
  X509 *cert;
  
  void (*ev_open)(ud *t);
  void (*ev_close)(ud *t);
  void (*ev_recv)(ud *t, void *a, int u);
  void (*ev_io)(struct file_in *t, int u);
  void (*ev_write)(ud *t, int u);
} cz;

typedef struct ud
{
  int fd;
  td *thread;
  ud *prev, *next;
  void *data;
  SSL *ssl;
  
  struct cx wr_list[WR_QUEUE];
  unsigned char wr_tail, wr_out;
  long wr_pos;
} ud;

typedef struct td
{
  int fd;
  void *data;
  pthread_t id;
  ud *ud_free, *ud_active, *sl;
  cz *info;

  struct io_uring *r;
  void *b, *i;
} td;

void ev_p(ud *t){}; void ev_recv_p(ud *t, void *a, int u){}; void ev_write_p(ud *t, int u){};

void ev_io_p(struct file_in *f, int u){};

void (*ev_open)(ud *t);
void (*ev_close)(ud *t);
void (*ev_recv)(ud *t, void *a, int u);
void (*ev_io)(struct file_in *t, int u);
void (*ev_write)(ud *t, int u);

// Send Data (Direct, No Queue);
void socket_send(ud *x,const void *b,unsigned int l)
{
  struct io_uring_sqe *e=io_uring_get_sqe((x->thread)->r);

  if(e)
  {
    io_uring_prep_send(e,x->fd,b,l,0);io_uring_sqe_set_data64(e,((unsigned long)x|2UL<<60));
  };
};

// Send Safely (Write Queue);
static inline int server_send(ud *x,const void *b,unsigned int l)
{
  struct cx *s=x->wr_list,*a;

  char n=(x->wr_tail+1)%WR_QUEUE;

  // Queue Full?;
  if(n==x->wr_out)
  {
    return 1;
  };

  // Add To List;
  a=(s+x->wr_tail);a->b=b;a->l=l;
  
  // Send (Immediate When Queue Empty);
  if(x->wr_out==x->wr_tail)
  {
    x->wr_tail=n;socket_send(x,b,l);
  }
  else
  {
    x->wr_tail=n;
  };

  return 0;
};

// Server Write Callback & Drained;
static void write_queue(ud *x,int u)
{
  struct cx s=*(x->wr_list+x->wr_out);

  if(u>=0)
  {
    x->wr_pos+=u;

    // Finished Active?;
    if(x->wr_pos>=s.l)
    {
      // Write Callback;
      ev_write(x,u);if(x->fd==-1){return;};x->wr_pos=0;

      x->wr_out=(x->wr_out+1)%WR_QUEUE;

      // Queue Not Empty? Send Next;
      if(x->wr_out!=x->wr_tail)
      {
        s=*(x->wr_list+x->wr_out);socket_send(x,s.b,s.l);
      };
    };
  }
  else
  {
    // Partial Write (Pressure Drained);
    socket_send(x,s.b+x->wr_pos,s.l-x->wr_pos);
  };
};

// Free Socket;
void free_socket(td *t,ud *x)
{
  if(x->ssl)
  {
    SSL_set_shutdown(x->ssl,SSL_SENT_SHUTDOWN|SSL_RECEIVED_SHUTDOWN);SSL_free(x->ssl);
    
    x->ssl=0;
  };

  if(x->data!=0)
  {
    ev_close(x);x->data=0;
  };

  // Reset Lists;
  x->wr_tail=x->wr_pos=x->wr_out=0;
  
  if(x->prev) 
  {
    x->prev->next=x->next;
  }
  else 
  {
    t->ud_active=x->next;
  };

  if(x->next){x->next->prev=x->prev;};

  x->next=t->ud_free;t->ud_free=x;
};

// Close Socket;
void close_socket(ud *x)
{
  td *t=x->thread;struct io_uring_sqe *e=io_uring_get_sqe(t->r);

  if(!e)
  {
    io_uring_submit(t->r);e=io_uring_get_sqe(t->r);
  };

  if(e)
  {
    io_uring_prep_close(e,x->fd);io_uring_sqe_set_data64(e,((unsigned long)x|4UL<<60));

    if(io_uring_submit(t->r)>0){x->fd=-1;return;};
  };

  // Sync Close (Ring Saturated);
  close(x->fd);x->fd=-1;free_socket(t,x);
};

// Define Queue & IO Functions, Variables;
const unsigned long tm=15UL<<60,am=(~tm);

static inline int reg_accept(struct io_uring *r,int s)
{
  struct io_uring_sqe *e=io_uring_get_sqe(r);
  
  if(!e)
  {
    io_uring_submit(r);e=io_uring_get_sqe(r);
    
    if(!e){return -1;};
  };
  
  io_uring_prep_multishot_accept(e,s,0,0,0);io_uring_sqe_set_data(e,0);

  return io_uring_submit(r);
};

// Register File I/O Read;
void async_load_file(ud *x,const char *path)
{
  struct io_uring *r=(x->thread)->r;struct io_uring_sqe *e=io_uring_get_sqe(r),*n=io_uring_get_sqe(r);

  if(e&&n)
  {
    struct file_in *q=malloc(sizeof(struct file_in));
    
    q->client=x;q->file=-1;

    // Register Open Event;
    io_uring_prep_openat(e,AT_FDCWD,path,O_RDONLY,0);e->flags|=IOSQE_IO_LINK;
    
    io_uring_sqe_set_data64(e,((unsigned long)q|5UL<<60));

    // Register StatX & Chain Events;
    io_uring_prep_statx(n,AT_FDCWD,path,0,STATX_SIZE,&q->st);

    io_uring_sqe_set_data64(n,((unsigned long)q|5UL<<60));
  };
};

// Register Socket Read;
static inline void reg_read(struct io_uring *r,ud *s)
{
  struct io_uring_sqe *e=io_uring_get_sqe(r);
  
  if(e)
  {
    io_uring_prep_read(e,s->fd,0,PACKET_SIZE,0);e->flags|=IOSQE_BUFFER_SELECT;

    io_uring_sqe_set_data64(e,((unsigned long)s|1UL<<60));
  };
};

// Register Write Ready Callback;
static inline void reg_out(struct io_uring *r,ud *s,unsigned long d)
{
  struct io_uring_sqe *e=io_uring_get_sqe(r);
  
  if(e)
  {
    io_uring_prep_poll_add(e,s->fd,POLLOUT);io_uring_sqe_set_data64(e,((unsigned long)s|d<<60));
  };
};

// Copy Memory (Destination, Source, Source + Size);
static inline void mem_copy(void *d,const void *i,const void *e)
{
  __builtin_memcpy(d,i,e-i);
};

// Load Certificates;
int load_cert(cz *z,const char *c,const char *k)
{
  BIO *b=BIO_new_file(c,"r");if(!b){return 1;};

  z->cert=PEM_read_bio_X509(b,0,0,0);BIO_free(b);

  if(!z->cert)
  {
    return 1;
  };

  b=BIO_new_file(k,"r");if(!b){return 1;};

  z->key=PEM_read_bio_PrivateKey(b,0,0,0);BIO_free(b);

  if(!z->key)
  {
    X509_free(z->cert);return 1;
  };

  return 0;
};

// Setup Socket;
static inline int setup_sock(unsigned int *z,unsigned short p)
{
  #if __APPLE__
  int s=socket(AF_INET6,SOCK_STREAM,0),o=1;fcntl(s,F_SETFL,O_NONBLOCK);
  #else
  int s=socket(AF_INET6,SOCK_STREAM|SOCK_NONBLOCK,0),o=1;
  #endif

  if(setsockopt(s,SOL_SOCKET,SO_REUSEPORT,&o,sizeof(o))!=0)
  {
    return 0;
  };

  struct sockaddr_in6 q;q.sin6_family=AF_INET6;q.sin6_port=(p>>8|p<<8)&65535;

  // Set IP (IPv6);
  unsigned int *v=(unsigned int*)&q.sin6_addr;

  *v=*z;
  *(v+1)=*(z+1);
  *(v+2)=*(z+2);
  *(v+3)=*(z+3);

  bind(s,(struct sockaddr*)&q,sizeof(q));listen(s,SOMAXCONN);

  return s;
};

// Destroy Server;
void destroy_server(cz *z)
{
  td *t=z->ti;

  while(t<z->te)
  {
    if(t->id!=0){pthread_cancel(t->id);pthread_join(t->id,0);};

    t+=1;
  };

  if(z->ssl){SSL_CTX_free(z->ssl);};
  
  if(z->key!=0)
  {
    X509_free(z->cert);EVP_PKEY_free(z->key);
  };

  if(z->ti!=0){free(z->ti);};
};

// Cleanup Thread;
static void free_thread(void *j)
{
  td *t=(td*)j;t->id=0;
  
  close(t->fd);

  if(t->r!=0) 
  {
    io_uring_unregister_buf_ring(t->r,0);
    io_uring_queue_exit(t->r);
  };

  if(t->i!=0){free(t->i);};
  if(t->b!=0){free(t->b);};

  // Cleanup Active Sockets;
  if(t->sl!=0)
  {
    ud *x=t->ud_active;

    while(x!=0)
    {
      close(x->fd);free_socket(t,x);

      x=t->ud_active;
    };

    free(t->sl);
  };
};

// Server Processing Function;
static void *thread(void *j)
{
  td *t=(td*)j;cz *z=t->info;int f=t->fd,u,q=0,l=0;
  
  pthread_cleanup_push(free_thread,(void*)t);

  // Init Client List;
  t->sl=t->ud_free=calloc(THREAD_CLIENTS,sizeof(ud));

  if(t->sl==0)
  {
    pthread_exit(0);
  };

  ud *c=t->sl;

  while(c<t->sl+THREAD_CLIENTS)
  {
    c->fd=-1;c->next=(void*)(c+1);c+=1;
  };

  c-=1;c->next=0;

  struct io_uring_cqe *e;ud *x;struct io_uring r;

  if(io_uring_queue_init_params(THREAD_CONC_CLIENTS,&r,&(struct io_uring_params){})<0)
  {
    pthread_exit(0);
  };

  t->r=&r;

  // Allocate Buffer;
  struct io_uring_buf_ring *br;void *b;

  if(posix_memalign(&b,4096,THREAD_CONC_CLIENTS*PACKET_SIZE)!=0)
  {
    pthread_exit(0);
  };

  t->b=b;

  // Allocate Pointer List;
  if(posix_memalign((void**)(&br),4096,THREAD_CONC_CLIENTS*sizeof(br->bufs[0]))!=0)
  {
    pthread_exit(0);
  };

  t->i=(void*)br;

  // Register & Fill Buffer Ring;
  if(io_uring_register_buf_ring(&r,&(struct io_uring_buf_reg){.ring_addr=(unsigned long)br,.ring_entries=THREAD_CONC_CLIENTS,.bgid=0},0)!=0)
  {
    pthread_exit(0);
  };

  io_uring_buf_ring_init(br);
  
  while(q<THREAD_CONC_CLIENTS)
  {
    void *a=b+(q*PACKET_SIZE);

    io_uring_buf_ring_add(br,a,PACKET_SIZE,q,io_uring_buf_ring_mask(THREAD_CONC_CLIENTS),q);q+=1;
  };
  
  io_uring_buf_ring_advance(br,THREAD_CONC_CLIENTS);

  // Prime;
  reg_accept(&r,f);

  // Process Incoming Data;
  while(1)
  {
    if(io_uring_submit_and_wait(&r,1)==-EINTR)
    {
      pthread_testcancel();
      
      continue; 
    };

    /*
    unsigned head;

    io_uring_for_each_cqe(&r,head,e)
    {
      // Look into! performance benfit
    }

    io_uring_cq_advance(&r,count);
    */

    // Consume CQE & Mark;
    io_uring_peek_cqe(&r,&e);

    unsigned long o=e->user_data;q=e->flags>>IORING_CQE_BUFFER_SHIFT;x=(ud*)(o&am);u=e->res;

    io_uring_cqe_seen(&r,e); // not needed when using io_uring_for_each_cqe.

    // Keep-Alive (Multi-Shot);
    if(l||((o&tm)>>60==0&&!(e->flags&IORING_CQE_F_MORE)))
    {
      if(reg_accept(&r,f)<0){l=1;}else{l=0;};
    };

    // Switched Based On Event Code;
    switch((o&tm)>>60)
    {
      case 0:
      {
        if(u<=0)
        {
          if(x!=0&&x->fd!=-1){close_socket(x);};break; 
          // do we always need to check ->fd??? then move indeed to close_sock
        };

        // Handle Accept;
        if(x==0)
        {
          // Get Client Structure (Thread, Linked List);
          if(t->ud_free==0||fcntl(u,F_SETFL,O_NONBLOCK)<0)
          {
            close(u);break;
          };

          x=t->ud_free;t->ud_free=x->next;

          // Insert (Active Double List);
          x->next=t->ud_active;x->prev=0;
          
          if(t->ud_active!=0)
          {
            t->ud_active->prev=x;
          };

          t->ud_active=x;

          // Save Client Data & Call Event;
          x->thread=t;x->fd=u;x->ssl=SSL_new(z->ssl);SSL_set_fd(x->ssl,u);
          
          SSL_set_accept_state(x->ssl);

          // Enable kTLS;
          SSL_set_options(x->ssl,SSL_OP_ENABLE_KTLS);
          
          ev_open(x);
        }
        else if(x->fd==-1)
        {
          break;
        };

        // Handle SSL Handshake;
        int h=SSL_do_handshake(x->ssl);

        if(h==1)
        {
          // Handshake Done;
          reg_read(&r,x);
        }
        else
        {
          int e=SSL_get_error(x->ssl,h);

          if(e==SSL_ERROR_WANT_READ)
          {
            struct io_uring_sqe *e=io_uring_get_sqe(&r);
            
            if(!e)
            {
              close_socket(x);
            }
            else
            {
              io_uring_prep_poll_add(e,x->fd,POLLIN);io_uring_sqe_set_data64(e,((unsigned long)x|0UL<<60));
            };
          }
          else if(e==SSL_ERROR_WANT_WRITE)
          {
            reg_out(&r,x,0UL);
          } 
          else 
          {
            close_socket(x);
          };
        };

        break;
      }
      case 1:
      {
        void *a=b+(q*PACKET_SIZE);
        
        if(x->fd!=-1)
        {
          if(u>0)
          {
            // Read Event;
            reg_read(&r,x);ev_recv(x,a,u);
          }
          else
          {
            close_socket(x);
          };
        };

        // Recycle Read Buffer;
        io_uring_buf_ring_add(br,a,PACKET_SIZE,q,io_uring_buf_ring_mask(THREAD_CONC_CLIENTS),0);
        
        io_uring_buf_ring_advance(br,1);

        break;
      }
      case 2:
      {
        // Write Callback;
        if(x->fd==-1){break;}else if(u<=0&&u!=-EAGAIN){close_socket(x);break;};

        // Register Write Ready (For Partial Writes);
        if(u==-EAGAIN||x->wr_pos!=0)
        {
          reg_out(&r,x,3UL);
        };

        write_queue(x,u);

        break;
      }
      case 3:
      {
        if(x->fd!=-1)
        {
          if(u<=0)
          {
            close_socket(x);break;
          };

          // Write Ready (Pressure Drained);
          write_queue(x,-1);
        };

        break;
      }
      case 4:
      {
        // Free Socket;
        free_socket(t,x);

        break;
      }
      case 5:
      {
        struct file_in *q=(struct file_in*)(o&am);

        // Register Read;
        if(q->file!=-1)
        {
          struct io_uring_sqe *e=io_uring_get_sqe(&r);

          if(e!=0&&u>=0)
          {
            q->b=malloc(q->st.stx_size);

            if(q->b!=0)
            {
              io_uring_prep_read(e,q->file,q->b,q->st.stx_size,0);

              io_uring_sqe_set_data64(e,((unsigned long)q|(6UL<<60)));

              break;
            };
          };

          // ev_io(q,-1);
          close(q->file);free(q); 
        }
        else
        {
          if(u>0){q->file=u;}else{free(q);}; // ev_io(q,-1);
        };

        break;
      }
      case 6:
      {
        struct file_in *q=(struct file_in*)(o&am);

        // Process File;
        ev_io(q,0);

        // Cleanup Memory;
        free(q->b);close(q->file);free(q);

        break;
      }
    };
  };

  pthread_cleanup_pop(1);
};

// Initialize Server;
long start_server(cz *z)
{
  // Ignore Broken Pipe (Crucial);
  signal(SIGPIPE,SIG_IGN);

  // Increase RAM Pin Limit;
  if(setrlimit(RLIMIT_MEMLOCK,&(struct rlimit){.rlim_cur=RLIM_INFINITY,.rlim_max=RLIM_INFINITY})!=0) 
  {
    destroy_server(z);
    
    return -1;
  };

  // Ready Functions (Placeholder Fallback);
  ev_open=ev_p;ev_close=ev_p;ev_recv=ev_recv_p;ev_write=ev_write_p;ev_io=ev_io_p;

  if(z->ev_open){ev_open=z->ev_open;};if(z->ev_close){ev_close=z->ev_close;};
  if(z->ev_recv){ev_recv=z->ev_recv;};if(z->ev_write){ev_write=z->ev_write;};

  if(z->ev_io){ev_io=z->ev_io;};

  // Get Thread Count;
  int i=sysconf(_SC_NPROCESSORS_ONLN);if(i<2){return -1;};
  
  // Create SSL & Load Certificates;
  z->ssl=SSL_CTX_new(TLS_server_method());

  if(z->ssl==0||SSL_CTX_use_certificate(z->ssl,z->cert)<=0||SSL_CTX_use_PrivateKey(z->ssl,z->key)<=0)
  {
    destroy_server(z);
    
    return -1;
  };

  X509_free(z->cert);EVP_PKEY_free(z->key);z->cert=0;z->key=0;

  // Spawn Virtual Threads (Max Physical);
  td *q;z->ti=calloc(i,sizeof(td));if(z->ti==0){return -1;};q=z->ti;

  while(q<(z->ti+i))
  {
    q->fd=setup_sock(z->ip,z->port);q->info=z;

    pthread_create(&(q->id),0,thread,q);

    q+=1;
  };

  z->te=q;

  return z->ti[0].id;
};