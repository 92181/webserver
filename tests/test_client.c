#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <sys/socket.h>
#include <pthread.h>
#include <fcntl.h>
#include <netdb.h>
#include <time.h>

#ifdef __linux__
  #include <kqueue/sys/event.h>
#else
  #include <sys/event.h>
#endif

#define SERVER_IP "::1"
#define SERVER_PORT "443"

#define SINGLE_THREADED 0 // implement!
#define THREAD_CONNECTIONS 1
#define HTTP_LOAD_TIME 14
#define WS_LOAD_TIME 16

// Define Globals;
const unsigned long tm=15UL<<60,am=(~tm);int gg=0;struct addrinfo hi,*rs;

typedef struct cl
{
  SSL *ssl;
  int fd, kq, recv_pos, wr_pos;
  unsigned char state;
} cl;

typedef struct rt
{
  pthread_t thread;
  unsigned int ok, failed; 
} rt;

const char d[]="\n\nGerman Car Fabrication (1900-2025)\n\n1901 - Benz launches the Velo, the first modest German automobile.  1902 - DMG's Mercedes 35-HP sets new speed standards.  Post-WWI, BMW pivots from aircraft engines to cars.\n"
  "1924 - Horch's 12/50 offers affordable refinement.  193 - Audi's Front pioneers front-wheel drive.  1937 - Volkswagen's Beetle (designed by Porsche) becomes the “people's car.”\n"
  "WWII forces factories to produce military hardware, but advances in alloy casting and aerodynamics emerge.\n"
  "1949 - Mercedes-Benz's 300 SL Gullwing debuts fuel injection; Porsche's 356 shows lightweight sports engineering.  The 1950s-60s “Wirtschaftswunder” fuels mass-production: VW's Microbus, Audi's safety-cell 100, BMW's driver-focused E30 3-Series.\n"
  "1970s - Mercedes adds ABS; Audi rolls out quattro all-wheel-drive (1980).  Digital ECUs, catalytic converters, and OBD-II become standard by the 1990s.\n"
  "2000s - BlueTEC diesel (Mercedes), aluminium A2 (Audi), and plug-in hybrids (VW Golf, BMW i3/i8) push efficiency.  2016 - VW's ID. Series announces a full electric shift; the MEB platform underpins the ID.3/ID.4.\n"
  "2021-2025 - Electrification dominates: 70% of VW Group sales are electric, solid-state batteries give 500 km range in 15 min.  Mercedes's autonomous S-Class Vision EQ, Audi's e-tronic quattro, and BMW's iX M showcase AI, Lidar, and carbon-fiber monocoques.  Factories run on cobots and digital twins, producing a customized vehicle every 12 hours.\n"
  "From the humble Velo to autonomous electric sedans, German carmaking has continuously fused performance, precision, and sustainability.";

// Copy Memory (Destination, Source, Source + Size);
static inline void mem_copy(void *d,const void *i,const void *e)
{
  while(i+4<=e)
  {
    *(unsigned int*)d=*(unsigned int*)i;
    
    i+=4;d+=4;
  };

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

// Close Socket;
static void close_socket(cl *m)
{
  write(2,"cc\n",3);printf("Client Close %d\n",m->fd);

  // Close Explicitly (LibKqueue);
  #ifdef __linux__
    struct kevent k[2];
    
    EV_SET(&k[0],m->fd,EVFILT_READ,EV_DELETE,0,0,0);
    EV_SET(&k[1],m->fd,EVFILT_WRITE,EV_DELETE,0,0,0);
    
    kevent(m->kq,k,2,0,0,0);
  #endif
  
  close(m->fd);SSL_free(m->ssl);free(m);
};

// Client WS Send Function (Static Mask);
static void ws_client_frame(cl *x,unsigned char *b,unsigned long l)
{
  unsigned char *i,*c=b+10,m[]={18,52,86,120};

  if(l<126)
  {
    *(b+8)=(1<<7)+1;*(b+9)=l|1<<7;

    i=b+8;
  }
  else if(l<65536)
  {
    *(b+6)=(1<<7)+1;*(b+7)=126|1<<7;

    *(unsigned short*)(b+8)=*((unsigned char*)&l)<<8|*(((unsigned char*)&l)+1);

    i=b+6;
  }
  else
  {
    *b=(1<<7)+1;*(b+1)=255;i=b;

    unsigned char *x=(unsigned char*)&l,*k=b+2;x+=sizeof(long);

    while(k<c)
    {
      x-=1;*k=*x;k+=1;
    };
  };

  // Add Mask (4);
  *(unsigned int*)c=*(unsigned int*)m;c+=sizeof(char)*4;

  // Mask Data (B);
  unsigned long z=0;

  while(z<l)
  {
    *c=*c^m[z%4];c+=1;z+=1;
  };
  
  return;
};

// Processing Function;
static void *thread(void *j)
{
  // Thread Setup;
  struct kevent z,x,e;int r=kqueue(),i=0;rt *k=(rt*)j;
  
  SSL_CTX *ctx=SSL_CTX_new(TLS_client_method());

  if(ctx==0)
  {
    pthread_exit(0);
  };
  
  unsigned char q[16384],b[77824];

  // Start;
  struct timespec o,p;timespec_get(&o,1);

  while(1)
  {
    // Check Time;
    timespec_get(&p,1);

    if(p.tv_sec-o.tv_sec>=HTTP_LOAD_TIME)
    {
      if(p.tv_sec-o.tv_sec>=HTTP_LOAD_TIME+WS_LOAD_TIME)
      {
        // Save Statistics;
        //unsigned int *s=malloc(sizeof(int)*4);*s=k;*(s+1)=v;

        printf("Thread TERM %lu\n",k->thread);

        // Thread Cleanup;
        SSL_CTX_free(ctx);close(r);pthread_exit(0);
      }
      else if(!gg)
      {
        printf("\n\nSwitch\n\n"); // temp
        gg=1; // switch
      };
    };

    // Launch Sockets;
    while(i<THREAD_CONNECTIONS)
    {
      int f=socket(rs->ai_family,rs->ai_socktype,rs->ai_protocol);printf("New Client: %d\n",f);

      if(f<0)
      {
        continue;
      }
      else if(fcntl(f,F_SETFL,O_NONBLOCK)<0)
      {
        close(f);
      };

      SSL *ssl=SSL_new(ctx);SSL_set_fd(ssl,f);connect(f,rs->ai_addr,rs->ai_addrlen);

      // Store Info (Linked List);
      cl *m=calloc(1,sizeof(cl));
      
      m->fd=f;m->ssl=ssl;m->kq=r;

      // Register Socket Events;
      EV_SET(&z,f,EVFILT_WRITE,EV_ADD|EV_ONESHOT,0,0,m);

      if(kevent(r,&z,1,0,0,0)<0)
      {
        printf("Socket Reg Failed: Closed %d\n",f); // err here!
        close_socket(m);
      };

      i+=1;
    };

    // Wait;
    if(kevent(r,0,0,&e,1,0)<0)
    {
      exit(1);
    };

    // Process;
    unsigned long o=(long)e.udata;int s=e.ident;cl *m=(cl*)(o&am); // ok?

    if(e.flags&(EV_EOF|EV_ERROR))
    {
      printf("Closed from Ev_err %d %d\n",e.flags&EV_EOF,e.flags&EV_ERROR);ERR_print_errors_fp(stderr);

      close_socket(m);i-=1;
    }
    else
    {
      switch((o&tm)>>60)
      {
        case 0:
        {
          int u=SSL_connect(m->ssl);

          if(u==1)
          {
            // Register Read;
            EV_SET(&x,s,EVFILT_READ,EV_ADD,0,0,(void*)((unsigned long)m|1UL<<60));
            
            if(kevent(r,&x,1,0,0,0)<0)
            {
              close_socket(m);i-=1;printf("fff0\n");
            };

            if(!gg)
            {
              // Write HTTP;
              const char t[]="GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n";SSL_write(m->ssl,t,sizeof(t)-1);
              printf("Write HTTP\n");
            }
            else
            {
              // Write WS;
              const char t[]="GET /ws HTTP/1.1\r\nHost: localhost\r\nSec-WebSocket-Key: dwO8+1t9V6bheeWPWdC8mg==\r\nConnection: Upgrade\r\nUpgrade: websocket\r\n\r\n";

              //sleep(1); // temp
              int u=SSL_write(m->ssl,t,sizeof(t)-1);printf("Client WS Upgrade: %d\n",u); // works.
            };
          }
          else 
          {
            int e=SSL_get_error(m->ssl,u);printf("client handshake err: %d\n",e);

            if(e==SSL_ERROR_WANT_READ)
            {
              EV_SET(&z,s,EVFILT_READ,EV_ADD|EV_ONESHOT,0,0,m);

              if(kevent(r,&z,1,0,0,0)<0)
              {
                close_socket(m);i-=1;
              };
            }
            else if(e==SSL_ERROR_WANT_WRITE)
            {
              EV_SET(&z,s,EVFILT_WRITE,EV_ADD|EV_ONESHOT,0,0,m);

              if(kevent(r,&z,1,0,0,0)<0)
              {
                close_socket(m);i-=1;
              };
            }
            else
            {
              close_socket(m);i-=1;
            };
          };

          break;
        }
        case 1:
        {
          int u=SSL_read(m->ssl,q,sizeof(q));

          if(u<=0)
          {
            int e=SSL_get_error(m->ssl,u);printf("client read_err %d\n",e);

            if(e!=SSL_ERROR_WANT_READ&&e!=SSL_ERROR_WANT_WRITE)
            {
              close_socket(m);i-=1;
            };

            break;
          };

          printf("Received: %d, From Server;\n",u);

          if(*q=='H'&&*(q+9)=='2')
          {
            // Response HTTP;
            k->ok+=1;close_socket(m);i-=1;
          }
          else
          {
            // Initialize Send Buffer ('Random' Sequence);
            /*unsigned char *a=b+14,s=85;

            while(a<b+sizeof(b))
            {
              s>>=1;if(s&1){s^=184;};

              *a=s;a+=1;
            };*/

            // WS Upgrade;
            if(m->state==0)
            {
              unsigned char *x=q;

              // Check Match (T);
              const unsigned char t[]="HTTP/1.1 101\r\nupgrade: websocket\r\nConnection: upgrade\r\nSec-WebSocket-Accept: fpKENP7NB/nR0atevrsq+XuWAis=\r\n\r\n",*y=t;

              while(x<q+sizeof(t))
              {
                if(*y!=*x){break;};

                x+=1;y+=1;
              };

              if(x>=q+sizeof(t)-1)
              {
                m->state=1;printf("WS Upgrade!\n");
              }
              else
              {
                break;
              };
            };

            // client send, target: 96 (see 2 times)
            // FIX
            
            if(m->state!=0)
            {
              // Set Target;
              int t,o;

              set_target:
              switch(m->state)
              {
                case 1:
                case 2:
                {
                  t=96;o=6;break;
                }
                case 3:
                case 4:
                {
                  t=1597;o=8;break;
                }
                case 5:
                case 6:
                {
                  t=69632;o=14;break;
                }
              };

              // Read (Simulated);
              if(m->state%2==0)
              {
                m->recv_pos+=u;

                printf("client read, target: %d %d\n",t,m->recv_pos);

                if(m->recv_pos>t)
                {
                  m->recv_pos=0;m->state+=1;

                  goto set_target; // NEW
                };
              };

              // Send;
              if(m->state%2)
              {
                if(m->wr_pos==0)
                {
                  // Initialize Send Buffer ('Random' Sequence);
                  unsigned char *a=b+14,s=85;

                  while(a<b+sizeof(b))
                  {
                    s>>=1;if(s&1){s^=184;};

                    *a=s;a+=1;
                  };

                  ws_client_frame(m,b,t);printf("ws read init\n"); // only do 1 time! check
                };

                // Write Safe;
                int u=SSL_write(m->ssl,b+(14-o)+m->wr_pos,t+o-m->wr_pos); // ok.

                if(u<0)
                {
                  int e=SSL_get_error(m->ssl,u);printf("u: %d, err: %d\n",u,e);ERR_print_errors_fp(stderr);

                  if(e!=SSL_ERROR_WANT_READ&&e!=SSL_ERROR_WANT_WRITE)
                  {
                    close_socket(m);i-=1;k->failed+=1;
                  };
                };

                // SSL;
                m->wr_pos+=u;

                if(m->wr_pos>t)
                {
                  m->wr_pos=0;m->state+=1; printf("Send Done! %d\n",m->state);

                  if(0) // ????
                  {
                    k->ok+=1;close_socket(m);i-=1;
                  };
                }
                else
                {
                  // Add Write-Ready;
                  EV_SET(&x,m->fd,EVFILT_WRITE,EV_ADD|EV_ONESHOT,0,0,m);
                  
                  kevent(m->kq,&x,1,0,0,0);
                };
              };
            };
          };

          break;
        }
      };
    };
  };

  return 0;
};

int main() 
{
  // Retrieve Server Info;
  hi.ai_family=AF_UNSPEC;hi.ai_socktype=SOCK_STREAM;hi.ai_flags=AI_NUMERICHOST;

  if(getaddrinfo(SERVER_IP,SERVER_PORT,&hi,&rs)!=0)
  {
    freeaddrinfo(rs);return 1;
  };

  // Get CPU Info;
  int i=0,o=0,f=0;

  #if !SINGLE_THREADED
    i=sysconf(_SC_NPROCESSORS_ONLN);
  #endif

  rt *q=calloc(i,sizeof(rt)),*x=q,*z;

  if(q==0)
  {
    return 1;
  };

  // Launch Threads;
  while(x<q+i)
  {
    pthread_create(&(x->thread),0,thread,x);
    
    x+=1;
  };

  x=q;

  while(x<q+i)
  {
    pthread_join(x->thread,(void**)&z);
    
    o+=x->ok;f+=x->failed;
    
    x+=1;
  };

  free(q);

  printf("✅ Tester Done, Ok: %d, Failed: %d\n",o,f);

  return 0;
};