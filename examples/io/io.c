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
      char q[]="HTTP/1.1 200\r\nContent-Length: 16\r\n\r\nHello I/O Tester";
      
      server_send(t,q,sizeof(q));

      break;
    }  
    default:
    {
      // Load Image;
      async_load_file(t,"./image.jpg");
    }
  };
};

// File I/O (Read Complete);
static void c_io(struct file_in *f,int u)
{ 
  unsigned long s=f->st.stx_size;

  char q[]="HTTP/1.1 200\r\nContent-Length: ",z[]="\r\n\r\n",*b=malloc(512+s),*i=b;
  
  if(b==0)
  {
    return;
  };

  // Built HTTP Header;
  mem_copy(i,q,q+sizeof(q)-1);i+=sizeof(q)-1;

  i+=sprintf(i,"%ld",s);

  mem_copy(i,z,z+sizeof(z)-1);i+=sizeof(z)-1;

  // Image Copy & Send;
  mem_copy(i,f->b,f->b+s);i+=s;
  
  server_send(f->client,b,i-b);
};

// Close (Write Complete);
static void c_write(ud *t,int u)
{
  close_socket(t);
};

// Main Logic;
int main()
{
  // Start Server;
  cz p={.port=443,.ip={0},.ev_recv=c_recv,.ev_write=c_write,.ev_io=c_io};

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