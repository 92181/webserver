#include <stdio.h>
#include "../../src/server.c"

// Server Router;
static void c_recv(ud *t,void *a,int u)
{
  // Extract URL (HTTP GET);
  unsigned char *s=(unsigned char*)a,*z=s+4,*e=s+u;s=z;

  if(*z!='/'){close_socket(t);return;};

  while(z<e&&*z!=' ')
  {
    z+=1;
  };

  if(z==e){close_socket(t);return;}else if(*(z-1)=='/'){z-=1;};

  // Route;
  switch(*(s+1))
  {
    case 'h':
    {
      // Send "Hello";
      char q[]="HTTP/1.1 200\r\nContent-Length: 12\r\n\r\nHello Client";
      
      server_send(t,q,sizeof(q));

      break;
    }  
    default:
    {
      // Send Redirect;
      char q[]="HTTP/1.1 301 Moved Permanently\r\nLocation: https://[::1]:8000/home\r\n\r\n";
      
      server_send(t,q,sizeof(q));
    }
  };
};

// Close (Write Complete);
static void c_write(ud *t,int u)
{
  close_socket(t);
};

int main()
{
  // Start Server;
  cz p={.port=8000,.ip={0},.ev_recv=c_recv,.ev_write=c_write};

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