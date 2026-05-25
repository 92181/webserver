#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>

#include "src/server.c"
#include "src/ws.h"

unsigned int i[4];

typedef struct ws_client
{
  unsigned long id;
  //unsigned int ws_op; // to cache pos!
  char *ws_cache,*ws_ps,*ws_pe;
} cl;

// Server Router;
static void client_recv(ud *t,void *a,int u)
{
  char *vz=(char*)a,*vf=vz+1024;

  // Handle Request (/GET);
  if(*vz=='G')
  {
    const char *resp = "HTTP/1.1 200 OK\r\nContent-Length: 12\r\n\r\nHello HTTPS!";
    printf("Received: res: %d, text: %.*s\n",u,u,vz);
    server_send(t,resp,strlen(resp));
  }
  else if(*vz!='P')
  {
    // Process WebSocket Frame;
    /*cl *j=t->d;

    if(j->ws_cache==0)
    {
      char *p=vz,h;unsigned long l;ws_header(&p,&l);h=(p-vz);

      // Allocate Message Cache;
      if(l+h>u)
      {
        j->ws_cache=malloc(l);mem_copy(j->ws_cache,p,p+u-h); // free on close... (on close function/handler?)

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



    // WRITE BIG RESPONSE!


    //unsigned char demo[]={0x81,0x05,'H','e','l','l','o'};
    //int s=SSL_write(q,demo,sizeof(demo));printf("Write :: %d\n",s);


    // handle ping pong;*/
    // close_socket(s,t); // on close code
  };
};

// Server Close Handler;
static void client_close(ud *x)
{
  if(x->d>(char*)12)
  {
    free(x->d);
  };
};

// Server Pressure Release (Socket Writable);
static void client_drain(ud *x)
{
  printf("Backpressure Drained, WR: %d\n",x->wr);
};

// Main Logic;
int main()
{
  // Start Server;
  if(load_cert("src/cert.pem","src/key.pem")!=0)
  {
    return 1;
  };

  start_server(i,443,client_recv,client_drain,client_close);
  
  printf("Server Started, Threads: %d.\n",nt);

  // Return;
  pthread_join(*nu,0);destroy_server();
  
  return 0;
};