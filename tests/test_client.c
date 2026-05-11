#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/event.h>
#include "../src/ws.h"

// make sure libkqueue is there
  // -lkqueue
  // kqueue lib compatible (use linux to write it!)

#define SERVER_IP "::1"
#define SERVER_PORT "443"
#define THREADS 1 // 32

#define HTTP_LOAD_TIME 1400 // 14
#define WS_LOAD_TIME 16

struct addrinfo hi,*rs;pthread_t *t;

const char d[]="HTTP/1.1 200\r\nContent-Length: 1597\r\n\r\nGerman Car Fabrication (1900-2025)\n\n1901 - Benz launches the Velo, the first modest German automobile.  1902 - DMG's Mercedes 35-HP sets new speed standards.  Post-WWI, BMW pivots from aircraft engines to cars.\n"
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

// Client WS Helper Function;
static void ws_client_set()
{

  // fix ws tester client_send (add mask 18,52,86,120) (14 bytes total) (tester only)
};

// HTTP Test Function;
static void *http_test(void *j)
{
  char b[65536];unsigned int f=0,r=0;SSL_CTX *c=SSL_CTX_new(TLS_client_method());
  
  struct timespec ts,te;timespec_get(&ts,1);

  while(1)
  {
    // Check Time;
    timespec_get(&te,1);

    if(te.tv_sec-ts.tv_sec>HTTP_LOAD_TIME)
    {
      unsigned int *s=malloc(sizeof(unsigned int)*2);*s=r;*(s+1)=f;

      SSL_CTX_free(c);pthread_exit(s);
    };

    // Create Socket;
    int s=socket(rs->ai_family,rs->ai_socktype,0);

    SSL *ssl=SSL_new(c);SSL_set_fd(ssl,s);//printf("XX0\n");

    if(connect(s,rs->ai_addr,rs->ai_addrlen)>=0)
    {
      if(SSL_connect(ssl)>0)
      {
        //printf("XX1\n");
        // Send HTTP Request (GET);
        const char s[]="GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n";SSL_write(ssl,s,sizeof(s)-1);

        // Check Response;
        int x=SSL_read(ssl,b,sizeof(b));

        if(x<24)
        {
          f+=1;
        }
        else
        {
          r+=1;
        };

        SSL_shutdown(ssl);
      }
      else
      {
        SSL_free(ssl);close(s);f+=1;continue;
      };
    }
    else
    {
      SSL_free(ssl);close(s);f+=1;continue;
    };

    SSL_free(ssl);close(s);
  };

  return 0;
};

// Websocket Test Function;
static void *ws_test(void *j)
{
  char b[77824],*u=b;unsigned int f=0,r=0;SSL_CTX *c=SSL_CTX_new(TLS_client_method());

  // Create Socket;
  int s=socket(rs->ai_family,rs->ai_socktype,0);

  SSL *ssl=SSL_new(c);SSL_set_fd(ssl,s);

  if(connect(s,rs->ai_addr,rs->ai_addrlen)>=0&&SSL_connect(ssl)>0)
  {
    // Send WS Upgrade Request (GET);
    const char s[]="GET /ws HTTP/1.1\r\nHost: localhost\r\nSec-WebSocket-Key: dwO8+1t9V6bheeWPWdC8mg==\r\nConnection: Upgrade\r\nUpgrade: websocket\r\n\r\n",
      r[]="HTTP/1.1 101\r\nupgrade: websocket\r\nConnection: upgrade\r\nSec-WebSocket-Accept: fpKENP7NB/nR0atevrsq+XuWAis=\r\n\r\n",*q=r;
    
    SSL_write(ssl,s,sizeof(s));

    // Print Response;
    int x=SSL_read(ssl,b,sizeof(b)),p;

    // Check Response (Match);
    while(u<b+x)
    {
      if(*q!=*u)
      {
        break;
      };

      u+=1;q+=1;
    };

    // Message Test (102 Bytes);
    //u=b;mem_copy(b+10,d,d+102);ws_set(&u,102);SSL_write(ssl,u,102+(b+10-u));printf("Written: %d\n",p); // Seems OK?

    //int p=SSL_read(ssl,b,sizeof(b));
    // CHECK RESPONSE SIZE (Should match!)

    // Message Test (1597 Bytes);
    //u=b;mem_copy(b+10,d,d+sizeof(d)-1);ws_set(&u,sizeof(d)-1);SSL_write(ssl,u,sizeof(d)-1+(b+10-u));printf("Written: %d\n",p); // Seems OK?
  
    // Message Test (>65526 Bytes);
    char *o=b+10;u=b;
    
    while(o+sizeof(d)<b+sizeof(b))
    {
      mem_copy(o,d,d+sizeof(d)-1);o+=sizeof(d)-1;
    };

    // USE ws_client_set()
    ws_set(&u,o-b+10);p=SSL_write(ssl,u,o-b);printf("Written: %d\n",p); // TEST

    p=SSL_read(ssl,b,sizeof(b));

    long int l;u=b;//ws_read(&u,&l);

    printf("Received: %d\n",p);printf("Length: %ld\n",l);
  }
  else
  {
    SSL_free(ssl);close(s);printf("❌ WebSocket Handshake Failed\n");
  };

  struct timespec ts,te;timespec_get(&ts,1);

  while(0) // 1
  {
    // Time Check;
    int p;timespec_get(&te,1);

    if(te.tv_sec-ts.tv_sec>WS_LOAD_TIME)
    {
      SSL_shutdown(ssl);SSL_free(ssl);close(s);SSL_CTX_free(c);

      unsigned int *s=malloc(sizeof(unsigned int)*2);*s=r;*(s+1)=f;

      pthread_exit(s); //if 0 failed altogether.. or something like that
    };

    // Message Loop;
    u=b;mem_copy(b+10,d,d+sizeof(d)-1);ws_set(&u,sizeof(d)-1);p=SSL_write(ssl,u,sizeof(d)-1+(b+10-u)); // Ok?

    p=SSL_read(ssl,b,sizeof(b));

    if(p<(sizeof(d)/2))
    {
      f+=1;
    }
    else
    {
      r+=1;
    };
  };
};

int main(int a,char **p)
{
  unsigned int x=0,r=0,f=0,*q;char *j=0;t=malloc(THREADS*sizeof(pthread_t));

  // Retrieve Server Info;
  hi.ai_family=AF_UNSPEC;hi.ai_socktype=SOCK_STREAM;hi.ai_flags=AI_NUMERICHOST;

  if(getaddrinfo(SERVER_IP,SERVER_PORT,&hi,&rs)!=0)
  {
    freeaddrinfo(rs);return 1;
  };

  // Launch Threads (HTTP Test);
  while(x<THREADS)
  {
    pthread_create(&t[x],0,http_test,j+x);x+=1;
  };

  x=0;

  while(x<THREADS)
  {
    pthread_join(*(t+x),(void**)&q);x+=1;

    r+=*q;f+=*(q+1);free(q);
  };
  
  printf("✅ HTTPS Test Passed, RPS: %d, Failed: %d.\n",(r/HTTP_LOAD_TIME),f);
  
  // Launch Threads (WebSocket Test);
  /*x=0;f=0;r=0;

  while(x<THREADS)
  {
    pthread_create(&t[x],0,ws_test,j+x);x+=1;
  };

  x=0;

  while(x<THREADS)
  {
    pthread_join(*(t+x),(void**)&q);x+=1;

    r+=*q;f+=*(q+1);free(q);
  };

  printf("✅ Websocket Test Passed, RPS: %d, Failed: %d.\n",(r/WS_LOAD_TIME),f);*/ // te.tv_sec-ts.tv_sec,(1000000000-te.tv_nsec)+ts.tv_nsec      << MOVE TO BENCHMARK backup in b64?
  
  // Free;
  freeaddrinfo(rs);
  
  return 0;
};