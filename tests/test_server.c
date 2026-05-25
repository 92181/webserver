#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdatomic.h>
#include <time.h>

#include "../src/server.c"
#include "../src/ws.h"

unsigned int i[4];atomic_int z=0;const char h[]="HTTP/1.1 200\r\n\r\nHello World";

typedef struct ws_client
{
  unsigned long id;
  //unsigned int ws_op; // to cache pos!
  char *ws_cache,*ws_ps,*ws_pe;
} cl;

// Server Router;
static inline void client_recv(ud *t,void *a,int u)
{
  unsigned char *vz=(unsigned char*)a,*vf=vz+1024;

  if(*vz=='G')
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

      server_send(t,r,sizeof(r)-1);
    }
    else
    {
      // Send GET Response;
      server_send(t,h,sizeof(h)-1);//printf("RES Send!\n");
    };
  }
  else //if(0) // disabled
  {
    // Mirror WebSocket Message To Client;
    unsigned char *p=vz,h;long int l,s;

    // Process WebSocket Frame;
    cl *j=t->d;

    if(j->ws_cache==0)
    {
      unsigned long l;ws_header(&p,&l);h=(p-vz);

      // Allocate Message Cache;
      if(l+h>u)
      {
        j->ws_cache=malloc(l);mem_copy(j->ws_cache,p,p+u-h);

        j->ws_ps=j->ws_cache+u-h;j->ws_pe=j->ws_cache+l;
      };
    }
    else
    {
      // Add Chunk To Cache;
      mem_copy(j->ws_ps,vz,vz+u);j->ws_ps+=u;

      // Message Complete;
      if(j->ws_ps==j->ws_pe)
      {
        unsigned long l=(j->ws_pe-j->ws_cache);ws_unmask(j->ws_cache,l);

        // print decrypted content here
        printf("Content:\n");
        char *ii=j->ws_cache,*v=ii+l;while(ii<v)
        {
          printf("%c",*ii);ii+=1;
        };
        printf("\n");

        // Medium MSG;
        unsigned char g[2048],*y=g,d[]="Hello Client! Hail To Victory!";
        mem_copy(g+10,d,d+sizeof(d)-1);ws_set(&y,sizeof(d)-1);

        //server_send(t,y,sizeof(d)-1+(g+10-y));

        // Free;
        free(j->ws_cache);j->ws_cache=0;
      };

      printf("Chunk::Consumed: %d\n",u);
    };
    
    // fix wsread length first. seems wrong.

    printf("u length: %d\n",u);
    printf("Server WSREAD LENGTH: %ld\n",l); // 16384*4+11319=76855

    ws_set(&p,l);server_send(t,p,l+p-vz);

    atomic_fetch_add(&z,1);
  };
};

// Server Close Handler;
static void client_close(ud *t)
{
  // Not Used;
};

// Server Pressure Release (Socket Writable);
static void client_drain(ud *t)
{
  printf("Drained -- Write: %d.\n",t->wr);
};

// Start Server;
int main()
{
  // Load Certificates;
  if(load_cert("../src/cert.pem","../src/key.pem")!=0)
  {
    return 1;
  };

  // Start Server On Port;
  start_server(i,443,client_recv,client_drain,client_close);

  // Run Tests
  sleep(1);system("./test");printf("✅ Server Terminating, Back-pressure: %d, ?: %d.\n",z,1);
  //sleep(1000);

  // Return Success;
  destroy_server();

  return 0;
};