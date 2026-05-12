#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <openssl/ssl.h>
#include <sys/socket.h>
#include <pthread.h>
#include <sys/event.h>
#include <fcntl.h>
#include <netdb.h>
#include <time.h>
#include "../src/ws.h"

#define SERVER_IP "::1"
#define SERVER_PORT "443"

#define THREAD_CONNECTIONS 12
#define HTTP_LOAD_TIME 14
#define WS_LOAD_TIME 16

// Define Globals;
struct addrinfo hi,*rs;pthread_t *nu;

const char d[]="HTTP/1.1 200\r\nContent-Length: 1597\r\n\r\nGerman Car Fabrication (1900-2025)\n\n1901 - Benz launches the Velo, the first modest German automobile.  1902 - DMG's Mercedes 35-HP sets new speed standards.  Post-WWI, BMW pivots from aircraft engines to cars.\n"
  "1924 - Horch's 12/50 offers affordable refinement.  193 - Audi's Front pioneers front-wheel drive.  1937 - Volkswagen's Beetle (designed by Porsche) becomes the “people's car.”\n"
  "WWII forces factories to produce military hardware, but advances in alloy casting and aerodynamics emerge.\n"
  "1949 - Mercedes-Benz's 300 SL Gullwing debuts fuel injection; Porsche's 356 shows lightweight sports engineering.  The 1950s-60s “Wirtschaftswunder” fuels mass-production: VW's Microbus, Audi's safety-cell 100, BMW's driver-focused E30 3-Series.\n"
  "1970s - Mercedes adds ABS; Audi rolls out quattro all-wheel-drive (1980).  Digital ECUs, catalytic converters, and OBD-II become standard by the 1990s.\n"
  "2000s - BlueTEC diesel (Mercedes), aluminium A2 (Audi), and plug-in hybrids (VW Golf, BMW i3/i8) push efficiency.  2016 - VW's ID. Series announces a full electric shift; the MEB platform underpins the ID.3/ID.4.\n"
  "2021-2025 - Electrification dominates: 70% of VW Group sales are electric, solid-state batteries give 500 km range in 15 min.  Mercedes's autonomous S-Class Vision EQ, Audi's e-tronic quattro, and BMW's iX M showcase AI, Lidar, and carbon-fiber monocoques.  Factories run on cobots and digital twins, producing a customized vehicle every 12 hours.\n"
  "From the humble Velo to autonomous electric sedans, German carmaking has continuously fused performance, precision, and sustainability.";

// Client WS Helper Function;
static void ws_client_set()
{

  // fix ws tester client_send (add mask 18,52,86,120) (14 bytes total) (tester only)
};

// Processing Function;
static void *thread(void *j)
{
  // Thread Setup;
  int kq=kqueue(),i=0;
  struct kevent change, event; // would need to use the same kqueue per thread for multiple sockets.

  SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());

  // Launch Sockets;
  while(0) // 1?
  {
    while(i<THREAD_CONNECTIONS)
    {
      // connect logic
      /*
      int s=socket(rs->ai_family,rs->ai_socktype,rs->ai_protocol);
      fcntl(s,F_SETFL,O_NONBLOCK);

      connect(s,rs->ai_addr,rs->ai_addrlen);

      SSL *ssl = SSL_new(ctx);SSL_set_fd(ssl, s);

      EV_SET(&change, s, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, NULL);
      kevent(kq, &change, 1, NULL, 0, NULL);
      */

      i+=1; // on disconnect -=1;
    };

    // wait
    // int nev = kevent(kq, NULL, 0, &event, 1, NULL);

    // receive logic
  };

  // test first before moving!

  int f=socket(rs->ai_family,rs->ai_socktype,rs->ai_protocol);fcntl(f,F_SETFL,O_NONBLOCK);

  connect(f,rs->ai_addr,rs->ai_addrlen); // Will likely return EINPROGRESS

  SSL *ssl = SSL_new(ctx);SSL_set_fd(ssl, f);
  //SSL_set_tlsext_host_name(ssl, hostname);


  // Initially, we want to know when the socket is writable to start the handshake
  EV_SET(&change, f, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, NULL);
  kevent(kq, &change, 1, NULL, 0, NULL);

  printf("Starting event loop...\n");

  int connected = 0;
  while (1) 
  {
    int nev = kevent(kq, NULL, 0, &event, 1, NULL);
    if (nev < 0) break;

    // check time here 
    // if http time done ELSE ws
    int s=event.ident;

    if (s == f) 
    {
      if (!connected) 
      {
        int ret = SSL_connect(ssl);

        if (ret == 1) 
        {
          printf("SSL Connection Established!\n");
          connected = 1;
          
          const char t[]="GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n";SSL_write(ssl,t,sizeof(t)-1);
                    
          EV_SET(&change, f, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
          kevent(kq, &change, 1, NULL, 0, NULL);
          EV_SET(&change, f, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
          kevent(kq, &change, 1, NULL, 0, NULL);
        } 
        else 
        {
          int err = SSL_get_error(ssl, ret);
          if (err == SSL_ERROR_WANT_READ) {
            EV_SET(&change, f, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
            kevent(kq, &change, 1, NULL, 0, NULL);
          } else if (err == SSL_ERROR_WANT_WRITE) {
            EV_SET(&change, f, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, NULL);
            kevent(kq, &change, 1, NULL, 0, NULL);
          } else {
            printf("Fucked.\n");
            //ERR_print_errors_fp(stderr);
            break;
          }
        }
      } 
      else 
      {
        // Read the response
        char buf[1024];
        int bytes = SSL_read(ssl, buf, sizeof(buf) - 1);
        if (bytes > 0) {
          buf[bytes] = 0;
          printf("Received %d bytes:\n%s\n", bytes, buf);
        } else {
          printf("Connection closed or error.\n");
          break;
        }
      }
    }
  }

  // Cleanup
  SSL_free(ssl);
  close(f);
  SSL_CTX_free(ctx);
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

  // Launch Threads;
  int i=sysconf(_SC_NPROCESSORS_ONLN);nu=malloc(i*sizeof(pthread_t));

  if(nu==0)
  {
    return 1;
  };

  unsigned int x=0,r=0,f=0,*q;char *j=0;

  x=0;f=0;r=0;

  while(x<i)
  {
    pthread_create(&nu[x],0,thread,j+x);x+=1;
  };

  x=0;

  while(x<i)
  {
    pthread_join(*(nu+x),(void**)&q);x+=1;

    r+=*q;f+=*(q+1);free(q);
  };


};