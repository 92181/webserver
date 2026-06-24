#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "../../src/server.c"

#ifndef __linux__
#include <dispatch/dispatch.h>

// 1. The Background Worker Function
void fetch_file_worker(void *c)
{
	//FetchContext *ctx = (FetchContext *)c;
	//unsigned char *p=(unsigned char*)c;printf("%s\n",p); // remove line after testing.

	int f=open((unsigned char*)c,O_RDONLY);
	
	if(f==-1) 
	{
		return 1;
	};

	// use stat get size!
	struct stat d;fstat(f,&d);
	unsigned char b=malloc(d.st_size); // was +1

	// Perform the blocking read on a background thread
	ssize_t bytesRead = read(f, b, d.st_size);
	
	if(bytesRead>=0) 
	{
		printf("Background: Read %zd bytes.\n---\n%s\n---\n", bytesRead, b);
	} 
	else
	{
		perror("Read error");
	};

	// Cleanup: Close file and free memory inside the worker or back on main
	close(f);
	free(b);
	
	printf("Done, handle data here or function callback..\n");
};

int main() 
{
	// Get Dispatch Queue;
	dispatch_queue_t queue=dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT,0);
	
	dispatch_async_f(queue,"image.jpg",fetch_file_worker);

	// 4. Keep main alive long enough to see the result
	printf("Main: Doing other things while file fetches...\n");
	sleep(2);

	return 0;
};
#else
// Linux IO_URING!
#endif

// gcc main.c -o main -O3 -lssl -lcrypto, https://[::1]:4433/

unsigned char *b,*c,*qw=(unsigned char*)"\033[31mMemory Allocation Failure!\033[0m\n",*qe=(unsigned char*)"\033[31mFile Operation Failed!\n\033[0m";struct stat d;

unsigned int i[4],s,a,f;//char vq[16384],*vz,*vw,*vy;

// Server Router;
static inline void router(ud *t,void *a,int u)
{
  unsigned char *vz=(unsigned char*)a,*vf=vz+1024,*vw,*vy,*vq;

  // Read Request And Extract URL;
  if(1)
  {
    // Parse URL;
    vw=vz;vq=vz+16384;while(vw<vq){if(*vw=='/'){vy=vw+1;while(vw<vq){if(*vw==' '){break;};vw+=1;};break;};vw+=1;};

    // Write Response To Client;
    if(vy==vw)
    {
      // server send from test to server.c!!!
      const unsigned char *r="Hello World!";server_send(t,r,sizeof(r)-1,0);
      //SSL_write(q,"Hello World;",12);
    }
    else if(*vy=='n')
    {
      const unsigned char *r="HTTP/2 200 OK\r\n\n<!doctype><html>This Is Another Page.</html>\r\n";server_send(t,r,sizeof(r)-1,0);
      //SSL_write(q,"HTTP/2 200 OK\r\n\n<!doctype><html>This Is Another Page.</html>\r\n",61);
    }
    else
    {
      const unsigned char *r="HTTP/2 200 OK\r\n\n<!doctype><html>This Is Yet Another Page.</html>\r\n";server_send(t,r,sizeof(r)-1,0);
      //SSL_write(q,"HTTP/2 200 OK\r\n\n<!doctype><html>This Is Yet Another Page.</html>\r\n",66);
    };
  };
};

// Main Logic;
/*int main(int e,char **r)
{
  // Load Certificates;
  if(load_cert("../src/cert.pem","../src/key.pem")!=0)
  {
    return 1;
  };

  // Start Server On Port 4433;
  *(i+3)=16777216;start_server(i,4433,router,b,s,c,a);pthread_join(*nu,0);

  // Return Success;
  desrv();return 0;
};
*/