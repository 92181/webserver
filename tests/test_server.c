#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdatomic.h>
#include <time.h>

#include "../src/server.c"
#include "../src/ws.h"

#define WRITE_QUEUE 8
#define TEST_WRITE_QUEUE 1

unsigned int i[4];atomic_int z=0;const char h[]="HTTP/1.1 200\r\nContent-Length: 11\r\n\r\nHello World";

typedef struct cl
{
  unsigned char *recv,*recv_p;
  unsigned long recv_len;
   
  unsigned long id;
} cl;

// Server Router;
static inline void recv_handler(ud *t,void *a,int u)
{
  unsigned char *vz=(unsigned char*)a,*vf=vz+1024;

  if(t->d==(char*)3)
  {
    // Allocate Client Memory;
    t->d=calloc(1,sizeof(cl));if(t->d==0){close_socket(t);return;};

    // Allocate Send Buffer;
    //cl *j=t->d;j->wr_list=malloc(sizeof(cx)*8);if(j->wr_list==0){close_socket(t);return;};//j->wr_max=8;

    // Init Linked List;
    /*cl *j=t->d;cx *s=j->wr_list;

    int g=0;
    while(g<WRITE_QUEUE-1)
    {
      s->next=g+1;s+=1;printf("Entry: %d, Next: %d\n",g,g+1);g+=1; // 0 to 7, last -1
    };
    s->next=-1;
    */

  };

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

      // Hash Key & Send Upgrade Response;
      unsigned char r[]="HTTP/1.1 101\r\nupgrade: websocket\r\nConnection: upgrade\r\nSec-WebSocket-Accept: xxxxxxxxxxxxxxxxxxxxxxxxxxxx\r\n\r\n";

      unsigned char *h=r+sizeof(r)-33;ws_key(vz,&h);*(r+sizeof(r)-5)='\r';printf("Server Upgrade\n");

      server_send(t,r,sizeof(r)-1);
    }
    else
    {
      // Send HTTP/GET;
      server_send(t,h,sizeof(h)-1);
    };
  }
  else
  {
    unsigned char *p=vz,h;unsigned long l;

    // Process WebSocket Frame;
    cl *j=t->d;//printf("client ws msg received %u\n",u);

    if(j->recv==0)
    {
      ws_header(&p,&l);h=(p-vz);printf("h: %d\n",h);

      // Allocate Receive Cache;
      j->recv=malloc(l);if(j->recv==0){close_socket(t);return;};j->recv_p=j->recv+u-h;j->recv_len=l;
      
      if(u-h>l){u=l+h;};mem_copy(j->recv,p,p+u-h);

      printf("j->recv alloc %lu, copied: %d %d\n",(long)(j->recv),u-h,h);
    }
    else
    {
      // Add Chunk To Cache;
      unsigned long d=j->recv_p-j->recv;

      if(d<j->recv_len)
      {
        if(u>j->recv_len-d){u=j->recv_len-d;};mem_copy(j->recv_p,vz,vz+u);printf("Add to Cache\n");
        
        j->recv_p+=u;
      };
    };

    // Receive Complete?;
    if(j->recv_p>=j->recv+j->recv_len)
    {
      printf("%d %d %d %d %d\n",(unsigned char)*(j->recv),(unsigned char)*(j->recv+1),(unsigned char)*(j->recv+2),(unsigned char)*(j->recv+3),(unsigned char)*(j->recv+4));
      ws_unmask(j->recv,j->recv_len);l=(j->recv_len-4);

      // Check Content ('Random' Sequence);
      unsigned char *i=j->recv+4,*z=i,*e=i+l,s=85;

      while(i<e)
      {
        s>>=1;if(s&1){s^=184;};//printf("%d %d\n",(int)(unsigned char)*i,(int)(unsigned char)s);

        if(*i!=s){break;};i+=1;
      };

      if(i==e)
      {
        // Mirror Message (-> Client);
        printf("Mirror To Client %lu\n",l);server_send(t,z,l);
      }
      else
      {
        //close_socket(t);atomic_fetch_add(&z,1); // should close, testing only.
        
        printf("content no match %lu %lu\n",(long)i,(long)e); 


      };
    };
  };
};

// Server Close Handler;
static void close_handler(ud *t)
{
  cl *j=t->d;

  if(j->recv!=0)
  {
    free(j->recv);
  };

  free(t->d);
};

// Write & Drained Callback;
static void write_handler(ud *t,int u)
{
  // Get Current Buffer;
  cl *j=t->d;cx *s=(t->wr_list+t->wr_tail);

  if(j->recv==s->b-4)
  {
    free(j->recv);j->recv=0;
    
    printf("RECV FREE\n");
  };
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
  start_server(i,443,recv_handler,write_handler,close_handler);

  // Run Tests
  sleep(1);system("./test");printf("✅ Server Terminating.\n");

  // Return Success;
  destroy_server();

  return 0;
};