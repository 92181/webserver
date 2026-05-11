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

#define REDIR_HTTP 1
#define URL "https://localhost"



// fix tester ws_send_mask and finish ws_test!
// test backpressure.
// Look into fast http-redirection.
// get libkqueue to work under linux
// async image loading demo for mac and linux.
// 

// Define Globals (Sockets, Queue, SSL, Socket Struct);
unsigned int nr,nt;pthread_t *nu;SSL_CTX *ssl;X509 *cert;EVP_PKEY *key;

typedef struct ux
{
  int fd,wr;
  SSL *ssl;
  //unsigned int wr;
  void *d;

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

//void (*recv_func)(ud*,void*,int);void(*drain_func)(ud*);void(*close_func)(ud*);
void (*recv_func)(ud*,void*,int),(*drain_func)(ud*),(*close_func)(ud*);

_Atomic (ud*)sz;ud *sl;td *ti;

// Define Queue & IO Functions, Variables;
#ifndef __linux__
//int to=0;

static void server_send(ud *x,const void *b,unsigned int l) 
{
  int u=SSL_write(x->ssl,b,l);

  if(u<=0)
  {
    if(SSL_get_error(x->ssl,u)!=SSL_ERROR_WANT_WRITE)
    {
      x->wr=-1;return;
    };

    u=0;
  };

  if(l-u>0&&x->wr==0)
  {
    struct kevent k;EV_SET(&k,x->fd,EVFILT_WRITE,EV_ADD|EV_ONESHOT,0,0,x);x->wr=l-u;

    kevent(x->kq,&k,1,0,0,0);printf("Partial Send: %d/%d\n",u,l);
  };
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

static void server_send(ud *s,const void *b,unsigned int l)
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
static int load_cert(const char *i,const char *z)
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
static int setup_sock(unsigned int *z,unsigned short p)
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
  
  bind(s,(struct sockaddr*)&q,sizeof(q));listen(s,2500);

  return s;
};

// Close Socket;
static inline void close_socket(ud *x)
{
  printf("Start Close: %d\n",x->fd);close(x->fd);

  if(x->ssl!=0)
  {
    SSL_free(x->ssl);x->ssl=0;
  };

  if((long)x->d>2)
  {
    close_func(x);
  };
  

  x->d=0;printf("Closed: %d\n",x->fd);
  //x->d=atomic_exchange(&sz,x);
  free(x);// temp
};

// Destroy Server;
static void destroy_server()
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

// Redirect HTTP Requests;
static void *http_redirect(void *p)
{
  // Initialize Variables;
  struct sockaddr_in6 u;unsigned int f=(long)p,s,q=sizeof(u);
  
  // Prepare Output;
  const char x[]="\r\n\r\n";char r[512]="HTTP/1.1 301\r\nContent-Length: 0\r\nLocation: ",*a=r+42+sizeof(URL);
  
  mem_copy(r+43,URL,(char*)URL+sizeof(URL)-1);

  // Process Incoming Data;
  while(1)
  {
    printf("thr\n");
    s=accept(f,(struct sockaddr*)&u,&q);

    if(s>>31==0)
    {
      printf("thr\n");
      // HTTP Redirect;
      char *z=a,*e=a+256;(void)!read(s,a,256);

      while(z<e&&*z!='/')
      {
        z+=1;
      };

      while(z<e&&*z!=' ')
      { 
        *a=*z;z+=1;a+=1;
      };

      mem_copy(a,x,x+4);a+=4;
      
      (void)!write(s,r,a-r);close(s);
    };
  };
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
  
  socklen_t y=sizeof(u);int s,r=kqueue();t->kq=r;printf("l->kq: %d\n",r);

  int to=0;
  // Kqueue For Main Sockets;
  #if REDIR_HTTP
  /*EV_SET(&k,f,EVFILT_READ,EV_ADD|EV_DISPATCH,0,0,&x); // on each nr socket.

  if(r<0||kevent(r,&k,1,0,0,0)<0)
  {
    pthread_exit(0);
  };

  EV_SET(&k,f,EVFILT_READ,EV_ENABLE|EV_DISPATCH,0,0,&x);*/
  #endif // remove redirection?
  // reg_accept(&r,f); even support it, implement io redir, if too hard remove it all together.
  // best to implement..

  // just move this (below) to accept, for apple check if ==0 for bsd not. ifndef linux >> nested apple
  EV_SET(&k,f,EVFILT_READ,EV_ADD|EV_DISPATCH,0,0,&x);

  if(r<0||kevent(r,&k,1,0,0,0)<0)
  {
    pthread_exit(0);
  };

  // do not move this (below keep here)
  EV_SET(&k,f,EVFILT_READ,EV_ENABLE|EV_DISPATCH,0,0,&x);
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
      s=e[i].ident;ud *x=(ud*)e[i].udata;printf("Wakeup %d %d %ld %d!\n",s,x->fd,(long)x->d,e[i].filter); // 4 0 1 -1(READ_EVENT) but does not read perhaps so loop? check!

      if(e[i].flags&(EV_EOF|EV_ERROR))
      {
        if(s==f)
        {
          close(s);
        }
        else
        {
          close_socket(x);
        };
      }
      else if(e[i].filter==EVFILT_READ)
      {
        switch((long)(x->d))
        {
          case 0:
          {
            // Handle Accept (!Break) & Activate Main (>Accept);
            //if((((long)j-(long)ti)/sizeof(td))!=0){printf("Faulty\n\n");break;}
            s=accept(f,(struct sockaddr*)&u,&y);printf("New Client %d %ld\n",s,(long)(((long)j-(long)ti)/sizeof(td))); // log thread
            // -1 causes crashes, because register is in front of check? then 2 on 1 sock.
            // tried with 1 socket, does not solve it.

            kevent(r,&k,1,0,0,0);

            if(s<0||fcntl(s,F_SETFL,O_NONBLOCK)<0)
            {
              break;
            };

            // Get Client Buffer (Linked List);
            /*do 
            {
              x=atomic_load(&sz);

              if(x==0)
              {
                close(s);goto r;
              };
            }
            while(!atomic_compare_exchange_weak(&sz,&x,x->d));*/ // atomics not needed perse.
            x=calloc(1,sizeof(ud));

            // Create SSL Context;
            x->ssl=SSL_new(ssl);SSL_set_fd(x->ssl,s);x->fd=s;

            // Create Event;
            EV_SET(&z,s,EVFILT_READ,EV_ADD|EV_DISPATCH,0,0,x);
            /*
            // was __APPLE__ (now not for testing)
            #if 1
            // Distribute Clients (1 Accept, Apple Only); // Causes segm fault...
            to=(to+1)%nt;x->kq=((td*)ti+to)->kq;
            //x->kq=13;
            //to=(to+1)%(nt-1);x->kq=((td*)ti+to)->kq;
            
            if(kevent(x->kq,&z,1,0,0,0)<0)
            #else
            x->kq=r;

            if(kevent(r,&z,1,0,0,0)<0)
            #endif
            {
              close_socket(x);break;
            };*/

            // move to below? register when done.

            printf("Client Done %d %d\n",x->kq,s);
          }
          case 1:
          {
            // SSL Accept (Break Error);
            //printf("SSL Accept Thread: %ld, Sock %d\n",(long)(((long)j-(long)ti)/sizeof(td)),s);
            int u=SSL_accept(x->ssl);printf("SSL, %d\n",u); // Segmentation on accept.

            if(u<=0)
            {
              int e=SSL_get_error(x->ssl,u);

              if(e==SSL_ERROR_WANT_READ||e==SSL_ERROR_WANT_WRITE)
              {
                // Re-activate Read Listener;
                if(x->d==(char*)1)
                {
                  EV_SET(&z,s,EVFILT_READ,EV_ENABLE|EV_DISPATCH,0,0,x);

                  if(kevent(x->kq,&z,1,0,0,0)<0)
                  {
                    close_socket(x);printf("Failure\n");exit(0);
                  };
                }
                else // NEW
                {
                  to=(to+1)%nt;x->kq=((td*)ti+to)->kq; // ifdef apple
                  x->d=(char*)1;
                  if(kevent(x->kq,&z,1,0,0,0)<0)
                  {
                    close_socket(x);printf("Failure\n");exit(0);
                  };
                }
              }
              else
              {
                close_socket(x);
              };

              x->d=(char*)1;break;
            }
            else
            {
              x->d=(char*)2;printf("SSL Done %d\n",s);
            };
          }
          default:
          {
            int u=SSL_read(x->ssl,q,PACKET_SIZE);printf("Read %d\n",u);

            // Register Listener;
            EV_SET(&z,s,EVFILT_READ,EV_ENABLE|EV_DISPATCH,0,0,x);

            if(kevent(x->kq,&z,1,0,0,0)<0)
            {
              close_socket(x);break;
            };

            if(u>0)
            {
              // Process;
              //recv_func(x,q,u);
              const char h[]="HTTP/1.1 200\r\n\r\nHello World";server_send(x,h,sizeof(h)-1);
              printf("Send Done\n");close_socket(x);break;
            };

            break;
          }
        };
      }
      else if(e[i].filter==EVFILT_WRITE)
      {
        // Backpressure Handler (Drained);
        drain_func(x);

        // Reset;
        x->wr=0;
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
          reg_read(&r,x);
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
        if(x->wr-u>0) // check
        {
          reg_out(&r,x,((unsigned long)x|3UL<<60));printf("Partial Write: %d/%d\n",u,x->wr);
          x->wr-=u;
        };

        printf("Write Success: %d\n",u);

        break;
      }
      case 3: 
      {
        printf("Back-Pressure Drained!\n");
        drain_func(x);

        break;
      }
    };

    #endif
  };

  pthread_cleanup_pop(1);
};

// Start Server;
static inline int start_server(unsigned int *z,unsigned short p,void(*v)(ud*,void*,int),void(*b)(ud*),void(*n)(ud*))
{
  // Increase RAM Pin Limit;
  if(setrlimit(RLIMIT_MEMLOCK,&(struct rlimit){.rlim_cur=RLIM_INFINITY,.rlim_max=RLIM_INFINITY})!=0) 
  {
    return 1;
  };

  // Setup Server Functions;
  int i=sysconf(_SC_NPROCESSORS_ONLN);nu=malloc(i*sizeof(pthread_t));recv_func=v;drain_func=b;close_func=n;

  if(nu==0)
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
    //sz->next=sz+1;sz+=1;
    sz->d=(void*)(sz+1);
    //sz+=1;
    atomic_fetch_add(&sz,1);
  };

  sz=sl;

  // Setup Event Queues;
  #if __APPLE__
  //int ns=setup_sock(z,p);
  /*ep=kqueue();
  
  ud x={0};EV_SET(&kl,ns,EVFILT_READ,EV_ADD|EV_DISPATCH,0,0,&x);

  if(kevent(ep,&kl,1,0,0,0)<0)
  {
    destroy_server();return 1; // not ok..
  };

  EV_SET(&kl,ns,EVFILT_READ,EV_ENABLE|EV_DISPATCH,0,0,&x);*/
  #endif

  ti=malloc(i*sizeof(td));td *q=ti;

  // Spawn Virtual Threads (As Many As Physical);
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

  // Ignore Broken Pipe;
  signal(SIGPIPE,SIG_IGN);

  return 0;
};