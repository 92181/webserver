#include <stdio.h>
#include <sys/socket.h>
#include <openssl/ssl.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#include "../src/server.c"

#if defined(__BSD__) && defined(SO_REUSEPORT_LB)
  #undef SO_REUSEPORT
  #define SO_REUSEPORT SO_REUSEPORT_LB
#endif

#ifndef __linux__
  #include <sys/event.h>
  #include <dispatch/dispatch.h>

  dispatch_queue_t dispatch_queue;
#else
  #include <kqueue/sys/event.h>
#endif

/* Work in progress. (WIP); */

// Define Structs;
struct io_uring_buf_ring {}; struct io_uring_sqe {}; struct io_uring_params {}; struct io_uring_buf_reg {};

struct io_uring {
	int kq;
};

// Empty Stubs;
static inline void io_uring_buf_ring_init(struct io_uring_buf_ring *br){};

static inline void io_uring_sqe_set_data64(struct io_uring_sqe *sqe, unsigned long long data){};

int io_uring_register_buf_ring(struct io_uring *ring, struct io_uring_buf_reg *reg, unsigned int flags){};

static inline void io_uring_sqe_set_data(struct io_uring_sqe *sqe, void *data){};

// Define Functions;
static inline struct io_uring_sqe *io_uring_get_sqe(struct io_uring *ring)
{
	return (struct io_uring_sqe*)ring;
};

int io_uring_queue_init_params(unsigned int entries, struct io_uring *ring, struct io_uring_params *p)
{
  #ifndef __linux__
  dispatch_queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
  #endif

	// Store Kqueue;
	ring->kq = kqueue();

	if(!ring->kq)
	{
		return -1;
	};

  return 0;
};

static inline void io_uring_buf_ring_init(struct io_uring_buf_ring *br)
{
  // Free Memory;
	//free();free();
  // t->b=b;
  // t->i=(void*)br;
};

static inline void io_uring_prep_accept(struct io_uring_sqe *sqe, int fd, struct sockaddr *addr, socklen_t *addrlen, int flags)
{
	struct kevent e[THREAD_CONC_CLIENTS],k,z;char q[PACKET_SIZE];ud x={0};struct sockaddr_in6 u;
  
  //socklen_t y=sizeof(u);int s,o=0,r=kqueue();t->kq=r;
  
	// SPLIT

  // Kqueue For Main Sockets;
  EV_SET(&k,fd,EVFILT_READ,EV_ADD,0,0,&x);

  if(r<0||kevent(r,&k,1,0,0,0)<0)
  {
    pthread_exit(0);
  };
};

#ifndef __linux__
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

	dispatch_async_f(queue,"image.jpg",fetch_file_worker);

	// 4. Keep main alive long enough to see the result
	printf("Main: Doing other things while file fetches...\n");
	sleep(2);

	return 0;
};
#endif

