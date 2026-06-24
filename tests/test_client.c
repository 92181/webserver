#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <sys/socket.h>
#include <pthread.h>
#include <sys/event.h>
#include <fcntl.h>
#include <netdb.h>
#include <time.h>

#include "../src/ws.h"

#define SERVER_IP "::1"
#define SERVER_PORT "443"

#define THREAD_CONNECTIONS 1
#define HTTP_LOAD_TIME 0 // 14 (temp) to 0
#define WS_LOAD_TIME 16

// Define Globals;
const unsigned long tm=15UL<<60,am=(~tm);int gg=0;struct addrinfo hi,*rs;pthread_t *nu;

typedef struct cl
{
  SSL *ssl;
  unsigned char state;
  int fd,wr;
} cl;

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

// Close Socket;
static void close_socket(cl *m) // was cl **b,cl *m
{
  printf("Client Close %d\n",m->fd);
  
  close(m->fd);SSL_free(m->ssl);free(m);
};

// Client WS Send Function (Static Mask);
static int ws_client_send(cl *x,unsigned char *b,unsigned long l) // cl was SSL *s
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

  // Write Safe;
  int u=SSL_write(x->ssl,i,l+14-(i-b)); // ok.
  
  printf("Client MSG Send %d\n",u);
  
  if(u<0)
  {
    int e=SSL_get_error(x->ssl,u);printf("u: %d, err: %d\n",u,e);ERR_print_errors_fp(stderr);

    if(e!=SSL_ERROR_WANT_READ&&e!=SSL_ERROR_WANT_WRITE)
    {
      // might need to be adjusted for big writes eg, write again (just keep simple pos and length per socket nothing major).
      close_socket(x);
    };
  };

  return u;
};

// Processing Function;
static void *thread(void *j)
{
  // Thread Setup;
  struct kevent z,x,e;int r=kqueue(),k=0,v=0,i=0;unsigned char q[16384],b[77824]; // unsigned char b[77824]

  SSL_CTX *ctx=SSL_CTX_new(TLS_client_method());

  // Start;
  struct timespec o,p;timespec_get(&o,1);

  while(1)
  {
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
      
      m->fd=f;m->ssl=ssl;

      // Register Socket Events;
      EV_SET(&z,f,EVFILT_WRITE,EV_ADD|EV_ONESHOT,0,0,m);

      if(kevent(r,&z,1,0,0,0)<0)
      {
        close_socket(m);
      };

      i+=1;
    };

    // Check Time;
    timespec_get(&p,1);

    if(p.tv_sec-o.tv_sec>HTTP_LOAD_TIME)
    {
      if(p.tv_sec-o.tv_sec>HTTP_LOAD_TIME+WS_LOAD_TIME)
      {
        // Save Statistics;
        unsigned int *s=malloc(sizeof(int)*4);*s=k;*(s+1)=v;

        // Thread Cleanup;
        SSL_CTX_free(ctx);close(r);pthread_exit(s);
      }
      else if(!gg)
      {
        printf("\n\nSwitch\n\n"); // temp
        gg=1; // switch
      };
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
      close_socket(m);i-=1;//printf("Closed from Ev_err %d %d\n",e.flags&EV_EOF,e.flags&EV_ERROR);ERR_print_errors_fp(stderr);
    }
    else
    {
      //printf("Client EV Code: %lu, Socket: %d, Max: %d, Event: %d\n",(o&tm)>>60,s,i,e.filter);
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
              close_socket(m);i-=1;
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

              SSL_write(m->ssl,t,sizeof(t)-1);printf("Client Upgrade Send\n");
            };
          }
          else 
          {
            int e=SSL_get_error(m->ssl,u);

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
            int e=SSL_get_error(m->ssl,u);

            if(e!=SSL_ERROR_WANT_READ&&e!=SSL_ERROR_WANT_WRITE)
            {
              close_socket(m);i-=1;printf("Closed from read_err\n");
            };

            break;
          };
          printf("Received: %d, From Server;",u);

          if(*q=='H'&&*(q+9)=='2')
          {
            // Response HTTP;
            q[u]=0;//printf("Received: %d, Content:\n%s\n",u,q);

            close_socket(m);i-=1;
          }
          else
          {
            // Initialize Send Buffer ('Random' Sequence);
            unsigned char *a=b+14,s=85;

            while(a<b+sizeof(b))
            {
              s>>=1;if(s&1){s^=184;};

              *a=s;a+=1;
            };

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

                // Send Message (96 Bytes);
                if(ws_client_send(m,b,96)!=96){v+=1;};
              }
              else
              {
                v+=1;
              };

              break;
            };
            
            if(m->state!=0)
            {
              printf("Received from server (ws msg): %d\n",u);

              if(u==96)
              {
                // Send Message (1597 Bytes);
                printf("Send Second! %d\n",(unsigned char)(*b));

                if(ws_client_send(m,b,1597)!=1597){v+=1;};

                m->state+=1;
              }
              else if(u==1597)
              {
                // Send Message (>65526 Bytes);
                printf("Send Third!\n");

                int uu=ws_client_send(m,b,69632);
                //if(ws_client_send(m->ssl,b,69632)!=69632){v+=1;};
                //close_socket(m);i-=1;printf("OK OK\n"); // TEMP;

                m->state+=1;printf("OK %d\n",uu);
              }
              else
              {
                // Succes?;
                if(u==69632){k+=1;}else{v+=1;};printf("Other %d\n",u);

                close_socket(m);i-=1;
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
  int i=sysconf(_SC_NPROCESSORS_ONLN);nu=malloc(i*sizeof(pthread_t));

  if(nu==0)
  {
    return 1;
  };

  unsigned int x=0,r=0,f=0,*q;char *j=0;

  // Launch Threads;
  while(x<i)
  {
    pthread_create(&nu[x],0,thread,j+x);x+=1;

    break; // temp
  };

  x=0;

  while(x<i)
  {
    pthread_join(*(nu+x),(void**)&q);x+=1;

    r+=*q;f+=*(q+1);free(q); // statistics...
  };

  printf("✅ HTTPS Tests, RPS: %d, Failed: %d.\n",(r/HTTP_LOAD_TIME),f);
  printf("✅ WS Tests, RPS: %d, Failed: %d.\n",(r/HTTP_LOAD_TIME),f);

  return 0;
};