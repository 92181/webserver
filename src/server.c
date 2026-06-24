#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <openssl/ssl.h>
#include <stdatomic.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fcntl.h>

#ifndef __linux__
  #include <sys/event.h>
#else
  #include <liburing.h>
  #include <sys/resource.h>
  #include <linux/tls.h>
  #include <poll.h>
#endif

#if defined(__BSD__) && defined(SO_REUSEPORT_LB)
  #undef SO_REUSEPORT
  #define SO_REUSEPORT SO_REUSEPORT_LB
#endif

#define THREAD_CLIENTS 65536
#define THREAD_CONC_CLIENTS 2048
#define PACKET_SIZE 16384

#define WR_QUEUE 8

#if WR_QUEUE > 0
typedef struct cx
{
  const void *b;
  unsigned char next;
  unsigned int l;
} cx;
#endif


// finish ws_test! implement large ws send!
// implement write queue in server but keep optional.
// test backpressure.

// get libkqueue to work under linux
// async image loading demo for mac and linux.
// 

// Define Globals (Sockets, Queue, SSL, Socket Struct);
unsigned int nr,nt;pthread_t *nu;SSL_CTX *ssl;X509 *cert;EVP_PKEY *key;

typedef struct
{
  int fd;
  SSL *ssl;
  void *d;

  #if WR_QUEUE > 0
  unsigned char wr_head,wr_tail;
  cx wr_list[WR_QUEUE];
  unsigned int wr_pos;
  #endif

  #if __linux__
  struct io_uring *r;
  #else
  int kq;
  #endif
} ud;

typedef struct
{
  int s;

  #if __linux__
  struct io_uring *r;
  void *b,*i;
  #else
  int kq;
  #endif
} td;

void (*recv_func)(ud*,void*,int),(*wr_func)(ud*,int),(*close_func)(ud*);

_Atomic (ud*)sz;ud *sl;td *ti;

#if WR_QUEUE > 1
void socket_send(ud *x,const void *b,unsigned int l);

// Send Safely (Write Queue);
static inline int server_send(ud *x,const void *b,unsigned int l)
{
  //cl *j=x->d;
  cx *s=x->wr_list,*a;printf("Send Start\n");

  char n=x->wr_head;
  //cx *z=s+n;
  // ,n=(s+h)->next

  // Queue Full?;
  if(n==-1)
  {
    return 1;
  };

  // Add To List;
  a=(s+n);a->b=b;a->l=l;//a->f=f;
  printf("len %u\n",a->l);
  
  // j->head should be first free entry! instead of last. DONE.
  x->wr_head=a->next;
  
  // Send;
  if(n==x->wr_tail) // OK.
  {
    printf("RealSend %d %d\n",x->wr_tail,x->wr_head);socket_send(x,b,l); // OK???
  }
  else
  {
    printf("%d %d\n",x->wr_head,x->wr_tail);
  };

  return 0;
};

// Server Write Callback & Drained;
static void write_queue(ud *x,int u)
{
  cx s=*(x->wr_list+x->wr_tail);char n=x->wr_tail;

  if(u>=0)
  {
    x->wr_pos+=u;

    printf("Write: %u-%u, %d %d\n",x->wr_pos,s.l,(char)(x->wr_tail),(char)(x->wr_head));

    // Finished Active?;
    if(x->wr_pos>=s.l)
    {
      // Write Callback;
      wr_func(x,u);

      // Free?;
      //if(s.f){free(s.b);s.b=0;};

      x->wr_pos=0;

      // Queue Not Empty?;
      if(s.next!=x->wr_head)
      {
        // Move Free To Head;
        char z=s.next;s.next=x->wr_head;x->wr_head=n;

        x->wr_tail=z;
        //s=*(x->wr_list+z); // Fix?
        s=*(x->wr_list+z);

        // Send Next;
        socket_send(x,s.b,s.l);
      }
      else
      {
        // Move Free To Head;
        s.next=x->wr_head;x->wr_head=n;
      };
    };
  }
  else
  {
    // Partial Write (Pressure Drained);
    socket_send(x,s.b+x->wr_pos,s.l-x->wr_pos);
  };
};
#endif

// Close Socket;
void close_socket(ud *x)
{
  close(x->fd);

  // Free & Reset;
  SSL_free(x->ssl);x->ssl=0;

  if((long)x->d>3)
  {
    close_func(x);
  };

  printf("Closed: %d\n",x->fd);
  
  x->d=0;x->d=atomic_exchange(&sz,x);
};

// Define Queue & IO Functions, Variables;
#ifndef __linux__
#include <openssl/err.h> // TEMP

void socket_send(ud *x,const void *b,unsigned int l)
{
  int u=SSL_write(x->ssl,b,l);

  if(u<=0)
  {
    int e=SSL_get_error(x->ssl,u);

    if(e!=SSL_ERROR_WANT_READ&&e!=SSL_ERROR_WANT_WRITE)
    {
      printf("Closing Client: u: %d, err: %d\n",u,e);ERR_print_errors_fp(stderr); // SSL_ERROR_SYSCALL 5
      printf("Syscall error: %s (errno: %d)\n", strerror(errno), errno); // errno means client EOF!
      close_socket(x);return;
    };

    u=0;
  };//printf("Server Write OK %d\n",l-u>0);  // TEMP

  // Register Write Ready;
  if(l-u>0)
  {
    struct kevent k;EV_SET(&k,x->fd,EVFILT_WRITE,EV_ADD|EV_ONESHOT,0,0,x);//x->wr=1;

    kevent(x->kq,&k,1,0,0,0);
  };

  // Write Callback;
  #if WR_QUEUE > 0
  write_queue(x,u); // perhaps do this in main loop at write event receive?.
  #else
  wr_func(x,u);
  #endif
};
#else
const unsigned long tm=15UL<<60,am=(~tm);

static inline void reg_accept(struct io_uring *r,int s)
{
  struct io_uring_sqe *q=io_uring_get_sqe(r);io_uring_prep_accept(q,s,0,0,0);

  io_uring_sqe_set_data(q,0);io_uring_submit(r);
};

static inline void reg_read(struct io_uring *r,ud *s)
{
  struct io_uring_sqe *q=io_uring_get_sqe(r);io_uring_prep_read(q,s->fd,0,PACKET_SIZE,0);q->flags|=IOSQE_BUFFER_SELECT;

  io_uring_sqe_set_data64(q,((unsigned long)s|1UL<<60));
  io_uring_submit(r);
};

static inline void reg_out(struct io_uring *r,ud *s,unsigned long d)
{
  struct io_uring_sqe *e=io_uring_get_sqe(r);io_uring_prep_poll_add(e,s->fd,POLLOUT);
  
  io_uring_sqe_set_data64(e,d);io_uring_submit(r);
};

void socket_send(ud *s,const void *b,unsigned int l)
{
  struct io_uring_sqe *e=io_uring_get_sqe(s->r);io_uring_prep_send(e,s->fd,b,l,0);s->wr=l;

  io_uring_sqe_set_data64(e,((unsigned long)s|2UL<<60));
  io_uring_submit(s->r);
};
#endif

// Copy Memory (Destination, Source, Source + Size);
static inline void mem_copy(void *d,const void *i,const void *e)
{
  while(i+4<=e)
  {
    *(unsigned int*)d=*(unsigned int*)i;
    
    i+=4;d+=4;
  };

  // Faster Nested Loop (~Wolf);
  if(i+1<e)
  {
    *(unsigned char*)d=*(unsigned char*)i;
    *(unsigned char*)(d+1)=*(unsigned char*)(i+1);

    if(i+2<e)
    {
      *(unsigned char*)(d+2)=*(unsigned char*)(i+2);
    };
  }
  else if(i+1==e)
  {
    *(unsigned char*)d=*(unsigned char*)i;
  };
};

// Load Certificates;
int load_cert(const char *i,const char *z)
{
  BIO *b=BIO_new_file(i,"r");if(!b){return 1;};

  cert=PEM_read_bio_X509(b,0,0,0);BIO_free(b);

  if(!cert)
  {
    return 1;
  };

  b=BIO_new_file(z,"r");if(!b){return 1;};

  key=PEM_read_bio_PrivateKey(b,0,0,0);BIO_free(b);

  if(!key)
  {
    X509_free(cert);return 1;
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
void destroy_server()
{
  unsigned int x=0;

  while(x<nt)
  {
    pthread_cancel(*(nu+x));printf("Killing thread %d\n",x);pthread_join(*(nu+x),0);

    x+=1;
  };

  ud *t=sl,*e=sl+(THREAD_CLIENTS*nt);

  while(t<e)
  {
    if(t->d!=0)
    {
      close_socket(t);
    };

    t+=1;
  };

  #if REDIR_HTTP
  //close(nr);
  #endif

  free(sl);SSL_CTX_free(ssl);free(nu);free(ti);
};

// Cleanup Thread;
static void free_thread(void *j)
{
  td t=*(td*)j;

  close(t.s);printf("Free Thread: %d\n",t.s);

  #if __linux__
  /*td *t=(td*)j; // cast
  io_uring_unregister_buf_ring(t->r,0);free(t->i);
  io_uring_queue_exit(t->r);free(t->b);*/

  io_uring_unregister_buf_ring(t.r,0);free(t.i);io_uring_queue_exit(t.r);free(t.b);
  #else
  if(t.kq)
  {
    close(t.kq);
  };
  #endif
};

// Server Processing Function;
static void *thread(void *j)
{
  td *t=(td*)j;int f=t->s;pthread_cleanup_push(free_thread,(void*)&t);

  #ifndef __linux__
  struct kevent e[THREAD_CONC_CLIENTS],k,z;char q[PACKET_SIZE];ud x={0};struct sockaddr_in6 u;
  
  socklen_t y=sizeof(u);int s,o=0,r=kqueue();t->kq=r;printf("l->kq: %d\n",r);
  
  #if REDIR_HTTP
  /*EV_SET(&k,f,EVFILT_READ,EV_ADD|EV_DISPATCH,0,0,&x); // on each nr socket.

  if(r<0||kevent(r,&k,1,0,0,0)<0)
  {
    pthread_exit(0);
  };

  EV_SET(&k,f,EVFILT_READ,EV_ENABLE|EV_DISPATCH,0,0,&x);*/
  #endif // remove redirection?
  // reg_accept(&r,f); even support it, implement io redir, if too hard remove it all together.

  // Kqueue For Main Sockets;
  EV_SET(&k,f,EVFILT_READ,EV_ADD,0,0,&x);

  if(r<0||kevent(r,&k,1,0,0,0)<0)
  {
    pthread_exit(0);
  };
  #else
  int u,f=0;struct io_uring_cqe *e;ud *x;struct io_uring r;

  if(io_uring_queue_init_params(THREAD_CONC_CLIENTS,&r,&(struct io_uring_params){})<0)  // add IORING_SETUP_ATTACH_WQ?
  {
    pthread_exit(0);
  };

  t->r=&r;

  // Allocate Buffer;
  struct io_uring_buf_ring *br;void *buffer_data; // rename both!

  if(posix_memalign(&buffer_data,4096,THREAD_CONC_CLIENTS*PACKET_SIZE)!=0)
  {
    pthread_exit(0);
  };

  t->b=buffer_data;
  
  // Allocate Pointer List;
  if(posix_memalign((void**)(&br),4096,THREAD_CONC_CLIENTS*sizeof(struct io_uring_buf))!=0)
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
  
  while(f<THREAD_CONC_CLIENTS)
  {
    void *a=buffer_data+(f*PACKET_SIZE);

    io_uring_buf_ring_add(br,a,PACKET_SIZE,f,io_uring_buf_ring_mask(THREAD_CONC_CLIENTS),f);f+=1;
  };
  
  io_uring_buf_ring_advance(br,THREAD_CONC_CLIENTS);

  // Prime;
  reg_accept(&r,f);
  #endif

  // Process Incoming Data;
  while(1)
  {
    #ifndef __linux__
    int n=kevent(r,0,0,e,THREAD_CONC_CLIENTS,0),i=0;

    while(i<n)
    {
      s=e[i].ident;ud *x=(ud*)e[i].udata;

      if(e[i].flags&(EV_EOF|EV_ERROR))
      {
        if(s==f)
        {
          close(s);
        }
        else if(x->ssl!=0)
        {
          close_socket(x);
        };
      }
      else
      {
        switch((long)(x->d))
        {
          case 0:
          {
            // Handle Accept (!Break);
            #if __APPLE__
            if(s==f){
            #endif

            s=accept(f,(struct sockaddr*)&u,&y);

            if(s<0)
            {
              break;
            }
            else if(fcntl(s,F_SETFL,O_NONBLOCK)<0)
            {
              close(s);break;
            };

            // Get Client Buffer (Linked List);
            do 
            {
              x=atomic_load(&sz);

              if(x==0)
              {
                close(s);goto r;
              };
            }
            while(!atomic_compare_exchange_weak(&sz,&x,x->d));

            // Get SSL & Fill Struct;
            x->ssl=SSL_new(ssl);SSL_set_fd(x->ssl,s);x->fd=s;x->d=(char*)1;

            // Register Event;
            EV_SET(&z,s,EVFILT_READ,EV_ADD|EV_CLEAR,0,0,x); // NEW
            
            #if __APPLE__
              o=((o+1)%(nt-1))+1;

              if(kevent(((td*)ti+o)->kq,&z,1,0,0,0)<0)
              {
                close_socket(x);
              };

              break;
            };
            #else
            if(kevent(r,&z,1,0,0,0)<0)
            {
              close_socket(x);break;
            };
            #endif

            x->kq=r;
          }
          case 1:
          {
            // SSL Accept (Break Error);
            int u=SSL_accept(x->ssl);

            if(u<=0)
            {
              int e=SSL_get_error(x->ssl,u);

              if(e==SSL_ERROR_WANT_WRITE)
              {
                EV_SET(&z,s,EVFILT_WRITE,EV_ADD|EV_ONESHOT,0,0,x);

                if(kevent(x->kq,&z,1,0,0,0)<0)
                {
                  close_socket(x);
                };
              }
              else if(e!=SSL_ERROR_WANT_READ)
              {
                close_socket(x);break;
              };

              break;
            }
            else
            {
              x->d=(char*)3;

              #if WR_QUEUE > 0
              // Init Linked List;
              int g=0;cx *s=x->wr_list; // also ADD for io_uring!

              while(g<WR_QUEUE-1)
              {
                s->next=g+1;s+=1;printf("Entry: %d, Next: %d\n",g,g+1);g+=1; // 0 to 7, last -1
              };

              s->next=-1;
              #endif
            };
          }
          default:
          {
            if(e[i].filter!=EVFILT_WRITE)
            {
              while(x->ssl!=0)
              {
                int u=SSL_read(x->ssl,q,PACKET_SIZE);

                // Process;
                if(u>0)
                {
                  recv_func(x,q,u);
                }
                else
                {
                  int e=SSL_get_error(x->ssl,u);

                  if(e!=SSL_ERROR_WANT_READ&&e!=SSL_ERROR_WANT_WRITE)
                  {
                    close_socket(x);
                  };

                  break;
                };
              };
            }
            else
            {
              // Write Ready (Drained Pressure);
              //x->wr=-1;wr_func(x,-1);if(x->wr<0){x->wr=0;}; // was (x,-1)

              #if WR_QUEUE > 0
              write_queue(x,-1);
              #else
              wr_func(x,-1);
              #endif
            };

            break;
          }
        };
      };

      r:
      i+=1;
    };
    #else
    io_uring_wait_cqe(&r,&e);

    // Consume CQE & Mark;
    unsigned long o=e->user_data;f=e->flags>>IORING_CQE_BUFFER_SHIFT;x=(ud*)(o&am);u=e->res;

    io_uring_cqe_seen(&r,e);

    if(u<0)
    {
      if(u==-EAGAIN)
      {
        // reg pollout ready lis
        reg_out(&r,x,((unsigned long)x|3UL<<60));
        continue;
      }
      else
      {
        close_socket(x);continue;
      };
    };

    // Switched Based On Event Code;
    switch((o&tm)>>60)
    {
      case 0:
      {
        // Handle Accept (!Break);
        if(x==0)
        {
          reg_accept(&r,f);

          // Get Client Structure (Thread, Linked List);
          if(sz==0||fcntl(u,F_SETFL,O_NONBLOCK)<0)
          {
            close(u);break;
          };

          //x=atomic_load(&sz);atomic_store(&sz,x->next);
          //x=atomic_load(&sz);atomic_store(&sz,x->d);x->d=0;
          do 
          {
            x=atomic_load(&sz);

            if(x==0)
            {
              close(s);goto r;
            };
          }
          while(!atomic_compare_exchange_weak(&sz,&x,x->d));

          x->d=0;
          // Save Client Data;
          x->fd=u;x->ssl=SSL_new(ssl);SSL_set_fd(x->ssl,u);SSL_set_accept_state(x->ssl);

          // Enable kTLS;
          SSL_set_options(x->ssl,SSL_OP_ENABLE_KTLS);
        };
      }
      default:
      {
        // Handle SSL Handshake;
        int h=SSL_do_handshake(x->ssl);

        if(h==1)
        {
          // Handshake Done;
          reg_read(&r,x);

          #if WR_QUEUE > 0
          // Init Linked List;
          int g=0;cx *s=x->wr_list;

          while(g<WR_QUEUE-1)
          {
            s->next=g+1;s+=1;printf("Entry: %d, Next: %d\n",g,g+1);g+=1; // 0 to 7, last -1
          };
          
          s->next=-1;
          #endif
        }
        else
        {
          int e=SSL_get_error(x->ssl,h);

          if(e==SSL_ERROR_WANT_READ) 
          {
            struct io_uring_sqe *e=io_uring_get_sqe(&r);

            io_uring_prep_poll_add(e,x->fd,POLLIN);io_uring_sqe_set_data(e,x);

            io_uring_submit(&r);
          }
          else if(e==SSL_ERROR_WANT_WRITE)
          {
            reg_out(&r,x,0);
          } 
          else 
          {
            close_socket(x);break;
          };
        };

        break;
      }
      case 1:
      {
        reg_read(&r,x);

        // Get Buffer & Call Handler;
        void *a=buffer_data+(f*PACKET_SIZE);

        x->r=&r;recv_func(x,a,u);

        // Recycle Read Buffer;
        io_uring_buf_ring_add(br,a,PACKET_SIZE,f,io_uring_buf_ring_mask(THREAD_CONC_CLIENTS),0);
        
        io_uring_buf_ring_advance(br,1);

        break;
      }
      case 2:
      {
        // Register Write Ready;
        if(x->wr-u>0)
        {
          reg_out(&r,x,((unsigned long)x|3UL<<60));x->wr=1;printf("Partial Write: %d, %d.\n",u,x->wr);
        };

        // Write Callback;
        #if WR_QUEUE > 0
        WR_QUEUE(x,u);
        #else
        wr_func(x,u);
        #endif
        //wr_func(x,u);

        break;
      }
      case 3:
      {
        // Write Ready (Pressure Drained);
        //x->wr=-1;wr_func(x,-1);if(x->wr<0){x->wr=0;};
        #if WR_QUEUE > 0
        WR_QUEUE(x,-1);
        #else
        wr_func(x,-1);
        #endif

        break;
      }
    };

    #endif
  };

  pthread_cleanup_pop(1);
};

// Start Server;
// can use __weak function links with extern instead.. also add ssl init function call.
int start_server(unsigned int *z,unsigned short p,void(*v)(ud*,void*,int),void(*b)(ud*,int),void(*n)(ud*))
{
  // Ignore Broken Pipe (Crucial);
  signal(SIGPIPE,SIG_IGN);

  // Increase RAM Pin Limit;
  if(setrlimit(RLIMIT_MEMLOCK,&(struct rlimit){.rlim_cur=RLIM_INFINITY,.rlim_max=RLIM_INFINITY})!=0) 
  {
    return 1;
  };

  // Setup Server Functions;
  int i=sysconf(_SC_NPROCESSORS_ONLN);nu=malloc(i*sizeof(pthread_t));recv_func=v;wr_func=b;close_func=n;

  if(nu==0||i<2)
  {
    return 1;
  };

  // Create SSL & Load Certificates;
  ssl=SSL_CTX_new(TLS_server_method());

  SSL_CTX_use_certificate(ssl,cert);SSL_CTX_use_PrivateKey(ssl,key);
  
  X509_free(cert);EVP_PKEY_free(key);

  // Build Client List;
  sl=calloc(THREAD_CLIENTS*i,sizeof(ud));sz=sl;ud *se=sl+THREAD_CLIENTS*i-1;

  if(sl==0)
  {
    free(nu);return 1;
  };

  while(sz<se)
  {
    sz->d=(void*)(sz+1);atomic_fetch_add(&sz,1);
  };

  sz=sl;

  // Spawn Virtual Threads (As Many As Physical);
  ti=malloc(i*sizeof(td));td *q=ti;

  while(nt<i)
  {
    q->s=setup_sock(z,p);pthread_create(&nu[nt],0,thread,q);
    
    nt+=1;q+=1;
  };

  // Setup Redirection;
  /*#if REDIR_HTTP
  nr=setup_sock(z,80);fcntl(nr,F_SETFL,0);

  // Calculate Ratio Between Threads, (/16);
  unsigned int ni=i/16;//char *j=0;

  while(nt<ni)
  {
    pthread_create(&nu[nt],0,http_redirect,(char*)(long)nr);nt+=1;
  };
  #endif*/

  return 0;
};