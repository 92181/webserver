#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdatomic.h>
#include <time.h>

#include "../src/server.c"
#include "../src/ws.h"

const char h[]="HTTP/1.1 200\r\nContent-Length: 11\r\n\r\nHello World";

typedef struct cl
{
  unsigned char *recv, *recv_pos;
  unsigned long recv_len;
} cl;

// Server Router;
static void c_recv(ud *t,void *a,int u)
{
  unsigned char *vz=(unsigned char*)a,*vf=vz+u;

  if(*vz=='G')
  {
    // Check For WebSocket Upgrade;
    if(*(vz+5)=='w'&&*(vz+6)=='s')
    {
      vz+=24;printf("ws upgr server!\n");

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
    cl *j=t->data;

    if(j->recv==0)
    {
      ws_header(&p,&l);h=(p-vz);printf("h: %d\n",h);

      // Allocate Receive Cache;
      j->recv=malloc(l);if(j->recv==0){close_socket(t);return;};j->recv_pos=j->recv+u-h;j->recv_len=l;
      
      if(u-h>l){u=l+h;};mem_copy(j->recv,p,p+u-h);

      printf("j->recv alloc %lu, copied: %d %d, target: %d\n",(long)(j->recv),u-h,h,j->recv_len);
    }
    else
    {
      // Add Chunk To Cache;
      unsigned long d=j->recv_pos-j->recv;

      if(d<j->recv_len)
      {
        if(u>j->recv_len-d){u=j->recv_len-d;};mem_copy(j->recv_pos,vz,vz+u);printf("Recv: chunk added.\n");
        
        j->recv_pos+=u;
      };
    };

    // Receive Complete?;
    if(j->recv_pos>=j->recv+j->recv_len)
    {
      printf("%d %d %d %d %d\n",(unsigned char)*(j->recv),(unsigned char)*(j->recv+1),(unsigned char)*(j->recv+2),(unsigned char)*(j->recv+3),(unsigned char)*(j->recv+4));
      
      ws_unmask(j->recv,j->recv_len);l=(j->recv_len-4);

      // Check Content ('Random' Sequence);
      unsigned char *z=j->recv+4,*i=z,*e=i+l,s=85;

      while(i<e)
      {
        s>>=1;if(s&1){s^=184;};

        if(*i!=s){break;};i+=1;
      };

      if(i==e)
      {
        // Mirror Message (-> Client);
        l=ws_set(&z,l);

        printf("Mirror To Client %lu \n",l);

        server_send(t,z,l);
      }
      else
      {
        close_socket(t);
      };
    };
  };
};

// Connect Event;
static void c_open(ud *t)
{
  // Allocate Client Memory;
  t->data=calloc(1,sizeof(cl));if(t->data==0){close_socket(t);return;};
  
  write(2,"t->d alloc!\n",12);
};

// Close Event;
static void c_close(ud *t)
{
  cl *j=t->data;

  if(j->recv!=0)
  {
    free(j->recv);
  };

  free(t->data);
};

// Write Callback;
static void c_write(ud *t,int u)
{
  // Free Buffer;
  cl *j=t->data;

  if(j->recv!=0)
  {
    free(j->recv);j->recv=0;printf("RECV FREE\n");
  };
};

// Start Server;
int main()
{
  // Start Server;
  cz p={.port=443,.ip={0},.ev_open=c_open,.ev_close=c_close,.ev_recv=c_recv,.ev_write=c_write};

  if(load_cert(&p,"../src/cert.pem","../src/key.pem")!=0)
  {
    return 1;
  };

  // Start Server;
  long x=start_server(&p);

  if(x>0)
  {
    sleep(1);system("su $SUDO_USER -c './test'");
  };

  // Return Success;
  destroy_server(&p);printf("✅ Server Terminated.\n");

  return 0;
};