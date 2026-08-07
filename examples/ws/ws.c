#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include "../../src/server.c"
#include "../../src/ws.h"

const char k[]="Hello Client! (WS Connection).",h[]=

"HTTP/1.1 200\r\nContent-Length: 449\r\n\r\n"
"<!doctype html>\n"
"<html>\n"
"  <head>\n"
"    <meta charset=\"utf-8\">\n"
"    <link rel=\"icon\" href=\"data:,\" type=\"image/x-icon\">\n"
"  </head>\n"
"  <body>\n"
"    See console (F12).\n"
"    <script>\n"
"      // Setup Websocket;\n"
"      let s=new WebSocket(\"/ws\");\n"
"\n"
"      s.onopen=function()\n"
"      {\n"
"        setTimeout(function(){s.send(\"Hello Server (WS)!\");},1500);console.log(\"Connected.\");\n"
"      };\n"
"\n"
"      s.onmessage=function(m){console.log(m);};\n"
"    </script>\n"
"  </body>\n"
"</html>\n";

typedef struct cl
{
  time_t open_time;
  unsigned char *recv, *recv_pos;
  unsigned long recv_len;
} cl;

// Server Router;
static void c_recv(ud *t,void *a,int u)
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

      // Hash Key & Send Upgrade Response;
      unsigned char r[]="HTTP/1.1 101\r\nupgrade: websocket\r\nConnection: upgrade\r\nSec-WebSocket-Accept: xxxxxxxxxxxxxxxxxxxxxxxxxxxx\r\n\r\n";

      unsigned char *h=r+sizeof(r)-33;ws_key(vz,&h);*(r+sizeof(r)-5)='\r';

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
      ws_header(&p,&l);h=(p-vz);

      // Allocate Receive Cache;
      j->recv=malloc(l);
      
      if(j->recv==0)
      {
        close_socket(t);return;
      };
      
      j->recv_pos=j->recv+u-h;j->recv_len=l;
      
      if(u-h>l)
      {
        u=l+h;
      };
      
      mem_copy(j->recv,p,p+u-h);
    }
    else
    {
      // Add Chunk To Cache;
      unsigned long d=j->recv_pos-j->recv;

      if(d<j->recv_len)
      {
        if(d+u>j->recv_len){u=j->recv_len-d;};mem_copy(j->recv_pos,vz,vz+u);
        
        j->recv_pos+=u;
      };
    };

    // Receive Complete?
    if(j->recv_pos>=j->recv+j->recv_len)
    {
      ws_unmask(j->recv,j->recv_len);l=(j->recv_len-4);printf("Message: %.*s\n",l,j->recv+4); 

			free(j->recv);j->recv=0;

      // Build & Send Message;
      unsigned char z[10+sizeof(k)],*u=z;mem_copy(z+10,k,k+sizeof(k));

      l=ws_set(&u,l);server_send(t,u,l);
    };
  };
};

// Connect Event;
static void c_open(ud *x)
{
  // Allocate Client Memory;
  x->data=calloc(1,sizeof(cl));
  
  if(x->data==0)
  {
    close_socket(x);return;
  };

  time_t k=time(0);((cl*)(x->data))->open_time=k;

  td *t=x->thread;t->data+=1;

  // Disconnect Clients (Timed, Every 10 Connects);
  if((int)t->data>10)
  {
    t->data=0;

    ud *y=t->ud_active,*n;

    while(y!=0)
    {
      cl *j=y->data;n=y->next;

      if(k-20>j->open_time)
      {
        close_socket(y);
      };
      
      y=n;
    };
  };
};

// Close Handler;
static void c_close(ud *x)
{
  cl *j=(cl*)x->data;

  if(j->recv)
  {
    free(j->recv);j->recv=0;
  }

  free(x->data);
};

// Main Logic;
int main()
{
  // Start Server;
  cz p={.port=443,.ip={0},.ev_recv=c_recv,.ev_open=c_open,.ev_close=c_close};

  if(load_cert(&p,"../../src/cert.pem","../../src/key.pem")!=0)
  {
    return 1;
  };

  // Start Server;
  long x=start_server(&p);

  if(x>0)
  {
    pthread_join(x,0);destroy_server(&p);
  };
  
  return 0;
};