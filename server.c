#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <openssl/ssl.h>
#include <openssl/evp.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fcntl.h>

#ifndef __linux__
#include <sys/event.h>
struct kevent l[2];
#else
#include <sys/epoll.h>
#endif

#define MAV 256
#define MAX_THREAD_CLIENTS 524288
#define REDIR_HTTP 1
#define URL "https://localhost"

// 0.5. Implement Tests: 
// 3. Test with load tester;


// 1. Implement websocket send. (did but need tests!)

// 2. Implement websocket backpressure handling (epollout = socket writable but only when send fails for websockets)
// 2. Implement in client demo only?

// 4. Test on MacOS

// 5. Commit SERVER to Git.
// Add to git doc "Kqueue & Epolling implemented for increased throughput"
// Add to git doc "Works on: FreeBSD, MacOS, Linux."

// Define Globals (Sockets, Epoll, SSL, Socket Struct);
unsigned int ns,nr,ep,nt;char it=0;pthread_t *nu;SSL_CTX *ssl;

typedef struct sockd // to main.c?
{
  //#if __linux__ // SAFE REGARDLESS;
  int fd;
  //#endif

  SSL *ssl;
  char *d;
} ud;

void (*fu)(int,ud*);//ud *sl,*sz;

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

// Setup Socket;
static inline int spk(unsigned int *z,unsigned short p)
{
  #if __APPLE__
  int s=socket(AF_INET6,SOCK_STREAM,0);fcntl(s,F_SETFL,O_NONBLOCK);
  #else
  int s=socket(AF_INET6,SOCK_STREAM|SOCK_NONBLOCK,0);
  #endif
  
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
static inline void close_socket(int f,ud *s)
{
  //printf("Close, %d\n",f);
  if(s->ssl!=0)
  {
    SSL_shutdown(s->ssl);SSL_free(s->ssl);s->ssl=0;
    //printf("Y, %d ",f);
  };

  if(s->d>(char*)2)
  {
    free(s->d);
    //printf("X, %d ",f);
  };

  s->d=0;close(f);
  printf("CloseYX, %d\n",f);
};

// Destroy Server;
static inline void destroy_server()
{
  // shutdown/terminate threads (nu)

  unsigned int x=0;it=1;

  //close(ep);
   
  while(x<nt)
  {
    printf("Killing: %d\n",x);
    //pthread_kill(*(nu+x),SIGUSR1); // SIGCONT
    pthread_cancel(*(nu+x));

    pthread_join(*(nu+x),0);
    x+=1;
  };
  printf("Z");

  #if REDIR_HTTP
  close(nr);
  #endif

  printf("Y");

  close(ns);SSL_CTX_free(ssl);close(ep);free(nu);

  printf(" Cleanup Done\n");
};

// SHA-1 Hash & Base64 Encode;
char *ws_str="258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

static void ws_key(const char *i,char **o,int s)
{
  EVP_MD_CTX *mdctx=EVP_MD_CTX_new();unsigned char h[EVP_MAX_MD_SIZE];unsigned int l;

  EVP_DigestInit_ex(mdctx,EVP_sha1(),0);EVP_DigestUpdate(mdctx,i,s);EVP_DigestFinal_ex(mdctx,h,&l);

  EVP_MD_CTX_free(mdctx);

  // Base64 Encode;
  EVP_EncodeBlock((unsigned char*)*o,h,l);
};

// https://developer.mozilla.org/en-US/docs/Web/API/WebSockets_API/Writing_WebSocket_servers

// Unmask WebSocket Frame;
static void ws_read(char **j,unsigned long *k)
{
  printf("Frame Func\n");

  char *i=*j;

  // Get Payload Length;
  unsigned long l=*(i+1)&127;

  if(l<=125)
  {
    i+=2;
  }
  else if(l==126)
  {
    l=(*(i+2)<<8)|*(i+3); // test it with big msg. >126 && <65536?
    i+=4;
  }
  else
  {
    i+=2;char *e=i+8;l=0;

    while(i<e)
    {
      l=(l<<8)|*i;i+=1; // test it with big msg.
    };
  };

  // Get Mask & Decode;
  unsigned char *mask_key = i; i+=4;

  *k=l;*j=i;
  
  for (size_t z = 0; z < l; ++z) 
  {
    *i = *i ^ mask_key[z % 4];
    i+=1;
  };
};

// Create WS Frame (Type, Size) (Front: 10 Bytes Needed);
static void ws_set(char **j,unsigned long l)
{
  char *i=*j+10;

  if(l<126)
  {
    *(i-2)=(1<<7)+1;
    *(i-1)=l;

    *j=i-2;
  }
  else if(l<65536)
  {
    *(i-4)=(1<<7)+1;
    *(i-3)=126;

    *(unsigned short*)(i-2)=*((unsigned char*)&l)<<8|*(((unsigned char*)&l)+1);

    *j=i-4;
  }
  else
  {
    **j=(1<<7)+1;*(*j+1)=127;

    unsigned char *x=(unsigned char*)&l;x+=8;char *k=*j+2;

    while(k<i) // untested;
    {
      *k=*x;

      k+=1;x-=1;
    };
  };

  // FIN (1) + bullshit (7)
  // mask? (1 to 0!) + payload length (7) (dy)
  // content (x)
};

// Redirect HTTP Requests;
static void *http_redirect(void *p)
{
  // Initialize Variables;
  struct sockaddr_in6 u;unsigned int f=(long)p,s,q=sizeof(u);
  
  // Prepare Output;
  const char x[]="\r\n\r\n";char r[512]="HTTP/1.1 301\r\nContent-Length: 0\r\nLocation: ",*a=r+42+sizeof(URL);
  
  mem_copy(r+43,URL,URL+sizeof(URL));

  // Process Incoming Data;
  while(1)
  {
    s=accept(f,(struct sockaddr*)&u,&q);

    if(s>>31==0)
    {
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
    }
    else if(it==1) // can be removed if using cancel.
    {
      // Thread Termination;
      return 0;
    };
  };
};

// Cleanup Thread;
static void free_thread(void *sl)
{
  ud *t=(ud*)sl;printf("Thread Terminating\n");

  while(t<(ud*)sl+MAX_THREAD_CLIENTS)
  {
    if(t->d!=0)
    {
      close_socket(t->fd,t);
    };

    t+=1;
  };

  return;
};

// Server Processing Function;
static void *ths(void *j)
{
  struct sockaddr_in6 u;ud *sl,*sz;unsigned int q=sizeof(u),v=(long)j;int s;

  sl=sz=calloc(MAX_THREAD_CLIENTS,sizeof(ud));

  pthread_cleanup_push(free_thread,sl);

  #ifndef __linux__
  struct kevent e[MAV];
  #else
  struct epoll_event e[MAV];
  #endif

  // Process Incoming Data;
  while(1)
  {
    #ifndef __linux__
    int n=kevent(ep,0,0,e,MAV,0),i=0;
    #else
    int n=epoll_wait(ep,e,MAV,-1),i=0;
    #endif
    printf("epoll unblock :: n=%d %d\n",i,i<n);

    while(i<n)
    {
      #ifndef __linux__
      s=e[i].ident;ud *x=(ud*)e[i].udata;

      if(e[i].flags&EV_EOF) // add ||e[i].flags&EV_ERROR? lookup.
      {
        printf("Closed Socket.\n");close_socket(s,x);
      }
      else if(e[i].filter&EVFILT_READ)
      #else
      ud *x=(ud*)e[i].data.ptr;
      s=x->fd; // ..

      if(e[i].events&(EPOLLERR|EPOLLHUP))
      {
        if(s==ns)
        {
          close(s);
        }
        else
        {
          //printf("Hello Crash X0!\n");
          close_socket(s,x);
        };
      }
      else if(e[i].events&EPOLLIN)
      #endif
      {
        if(s==ns)
        {
          int c=accept(s,(struct sockaddr*)&u,&q); // use accept4 as seen in other demo file! CAN work!

          if(c<0||fcntl(c,F_SETFL,O_NONBLOCK)<0)
          {
            i+=1;continue;
          };

          printf("Accept, Thread: %d, Sock: %u\n",v,c);

          // Store Client Structure In List (Thread Local);
          if(sz+MAX_THREAD_CLIENTS/5000>sl+MAX_THREAD_CLIENTS){sz=sl;};

          while(sz<sl+MAX_THREAD_CLIENTS)
          {
            if(sz->d==0)
            {
              x=sz;x->d=(char*)1;sz+=1;break;
            };

            sz+=1;
          };

          if(sz>=sl+MAX_THREAD_CLIENTS)
          {
            close(c);i+=1;continue;
          };

          // Create SSL Context & Store It;
          x->ssl=SSL_new(ssl);SSL_set_fd(x->ssl,c);

          #ifndef __linux__
          // Re-activate Current Socket (NR/NS);
          EV_SET(&l[0],f,EVFILT_READ,EV_ENABLE|EV_DISPATCH,0,0,0);kevent(ep,l,1,0,0,0);x->fd=c;

          // Add Client Socket To Queue;
          struct kevent z;EV_SET(&z,c,EVFILT_READ,EV_ADD|EV_DISPATCH,0,0,x);

          if(kevent(ep,&z,1,0,0,0)<0)
          {
            close(c);x->d=0;
          };
          #else
          // Add Client Socket To Queue;
          struct epoll_event e;e.events=EPOLLIN|EPOLLONESHOT;e.data.ptr=x;x->fd=c;

          if(epoll_ctl(ep,EPOLL_CTL_ADD,c,&e)!=0)
          {
            close(c);x->d=0;
          };
          #endif

          //printf("Added Client! %d Sock: %d\n",v,c);
        }
        else
        {
          // Initiate HTTPS Connection;
          if(x->d==(char*)1)
          {
            int u=SSL_accept(x->ssl);

            if(u<=0)
            {
              int e=SSL_get_error(x->ssl,u);

              if(e!=SSL_ERROR_WANT_READ&&e!=SSL_ERROR_WANT_WRITE)
              {
                close_socket(s,x);
                
                i+=1;continue;
              };
            }
            else
            {
              x->d=(char*)2;fu(s,x); // TESTING (ACTIV LATER)
              //char vq[65536];int u=SSL_read(x->ssl,vq,65536);SSL_write(x->ssl,"HTTP/1.1 200\r\n\r\n",16);close_socket(s,x);
            };
          }
          else
          {
            fu(s,x); // TESTING (ACTIV LATER)
            //char vq[65536];int u=SSL_read(x->ssl,vq,65536);SSL_write(x->ssl,"HTTP/1.1 200\r\n\r\n",16);close_socket(s,x);
          };

          // Re-activate Client Socket (Not Closed);
          if(x->ssl!=0)
          {
            #ifndef __linux__
            EV_SET(&l[0],f,EVFILT_READ,EV_ENABLE|EV_DISPATCH,0,0,x);kevent(ep,l,1,0,0,0);
            #else
            struct epoll_event e;e.events=EPOLLIN|EPOLLONESHOT;e.data.ptr=x;epoll_ctl(ep,EPOLL_CTL_MOD,s,&e);
            #endif
          };
        };
      };

      i+=1;
    };

    // Thread Termination & Cleanup;
    /*if(it==1)
    {
      ud *t=sl;printf("Thread Terminating\n");

      while(t<sl+MAX_THREAD_CLIENTS)
      {
        if(t->d!=0)
        {
          close_socket(t->fd,t);
        };

        t+=1;
      };

      // Return Succes;
      return 0;
    };*/
  };

  pthread_cleanup_pop(1);
};

// Start Server;
static int start_server(unsigned int *z,unsigned short p,void (*fc)(int,ud*),char *b,int s,char *c,int a)
{
  // Create SSL Method And Context;
  fu=fc;ssl=SSL_CTX_new(TLS_server_method());

  // Load Certificates Into OpenSSL Context;
  BIO *crt=BIO_new_mem_buf(b,s);X509 *cert=PEM_read_bio_X509(crt,0,0,0);free(crt);crt=BIO_new_mem_buf(c,a);EVP_PKEY *key=PEM_read_bio_PrivateKey(crt,0,0,0);free(crt);

  SSL_CTX_use_certificate(ssl,cert);SSL_CTX_use_PrivateKey(ssl,key);X509_free(cert);EVP_PKEY_free(key);

  // Setup Socket & Allocate List;
  ns=spk(z,p);//sl=sz=calloc(MAX_CLIENTS,sizeof(ud));

  // Setup Queue (Epoll);
  #ifndef __linux__
  ep=kqueue();
  
  EV_SET(&l[0],ns,EVFILT_READ,EV_ADD|EV_DISPATCH,0,0,0);int k=1; // was |EV_ONESHOT

  if(kevent(ep,l,1,0,0,0)<0)
  {
    destroy_server();return 1;
  };
  #else
  ep=epoll_create1(0);

  struct epoll_event ev;ev.events=EPOLLIN|EPOLLEXCLUSIVE;

  // Setup Sockets;
  ud x;ev.data.ptr=&x;x.fd=ns;int k=epoll_ctl(ep,EPOLL_CTL_ADD,ns,&ev);

  if(k!=0)
  {
    destroy_server();return 1;
  };
  #endif

  // Query Max Amount Of Threads On CPU;
  unsigned int i=0;nt=sysconf(_SC_NPROCESSORS_ONLN);char *j=0;nu=malloc(nt*sizeof(pthread_t));

  // Ignore Broken Pipe;
  signal(SIGPIPE,SIG_IGN);

  #if REDIR_HTTP
  nr=spk(z,80);fcntl(nr,F_SETFL,0);printf("Sock (ns,nr): %u %u\n",ns,nr);

  // Calculate Ratio Between Threads, (/16);
  int ni=nt/16;

  while(i<ni)
  {
    pthread_create(&nu[i],0,http_redirect,(char*)(long)nr);i+=1;
  };
  #endif

  // Set ID And Create Threads;
  while(i<nt)
  {
    pthread_create(&nu[i],0,ths,j+i);i+=1;
  };
  printf("Total Threads: %d\n",i);

  return 0;
};
