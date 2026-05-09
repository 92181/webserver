#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdatomic.h>
#include <time.h>

#include "../src/cc20.h"
#include "../src/server.c"
#include "../src/ws.h"

unsigned int i[4],s,a,f;struct stat n;

const char h[]="HTTP/1.1 200\r\n\r\nHello World";

atomic_int z=0;

// Server Router;
static inline void rtr(int s,ud *t)
{
  SSL *q=t->ssl;char vq[77824],*vt=vq+77824;

  // Read Request;
  int u=SSL_read(q,vq,77824);

  if(u>0)
  {
    char *vf=vq+1024,*vz=vq;

    if(*vq=='G')
    {
      // Check For WebSocket Upgrade;
      if(*(vz+5)=='w'&&*(vz+6)=='s')
      {
        vz+=24;

        // Skip To Key;
        while(vz<vf&&(*vz!='K'||*(vz+1)!='e'||*(vz+2)!='y'))
        {
          vz+=1;
        };

        vz+=5;

        // Allocate Client Memory;
        t->d=malloc(128);
        
        // Hash Key & Send Upgrade Response;
        unsigned char r[]="HTTP/1.1 101\r\nupgrade: websocket\r\nConnection: upgrade\r\nSec-WebSocket-Accept: xxxxxxxxxxxxxxxxxxxxxxxxxxxx\r\n\r\n";

        char *h=r+sizeof(r)-33;ws_key((char*)vz,&h);*(r+sizeof(r)-5)='\r';

        SSL_write(q,r,sizeof(r)-1);
      }
      else
      {
        // Send GET Response;
        SSL_write(q,h,sizeof(h)-1);
        
        close_socket(s,t);
      };
    }
    else
    {
      // Mirror WebSocket Message To Client;
      char *p=vq;long int l,s;ws_read(&p,&l);
      
      // fix wsread length first. seems wrong.

      printf("u length: %d\n",u);
      printf("Server WSREAD LENGTH: %d\n",l); // 16384*4+11319=76855

      ws_set(&p,l);s=SSL_write(q,p,l+p-vq);

      if(s<0)
      {
        atomic_fetch_add(&z,1);
      };
    };
  }
  else
  {
    int e=SSL_get_error(q,u);

    if(e!=SSL_ERROR_WANT_READ&&e!=SSL_ERROR_WANT_WRITE)
    {
      close_socket(s,t);
    };
  };
};

// Start Server;
int main()
{
  // Load Certificates;
  char *b,*c;

  f=open("../src/cert.pem",0);fstat(f,&n);s=n.st_size;b=malloc(s);(void)!read(f,b,s);close(f);
  f=open("../src/key.pem",0);fstat(f,&n);a=n.st_size;c=malloc(a);(void)!read(f,c,a);close(f);

  // Start Server On Port;
  start_server(i,443,rtr,b,s,c,a);free(b);free(c);

  // Run Tests
  sleep(1);system("./test");printf("✅ Server Terminating, Backpressure Blocks: %d.\n",z);

  // Return Success;
  destroy_server();

  return 0;
};