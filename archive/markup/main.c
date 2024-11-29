#include "../../server.c"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

// gcc main.c -o main -O3 -lssl -lcrypto, https://[::1]:3000/

unsigned char *b,*c,*qw=(unsigned char*)"\033[31mMemory Allocation Failure!\033[0m\n",*qe=(unsigned char*)"\033[31mFile Operation Failed!\n\033[0m";struct stat d;

unsigned int i[4],s,a,f;char vq[16384],*vz,*vw,*vy;

// Server Router;
static inline void router(SSL *q)
{
  // Read Request And Extract URL;
  SSL_read(q,vq,16384);vw=vq;vz=vq+16384;while(vw<vz){if(*vw=='/'){vy=vw+1;while(vw<vz){if(*vw==' '){break;};vw+=1;};break;};vw+=1;};

  // Write Response To Client;
  if(vy==vw)
  {
    SSL_write(q,"HTTP/2 200 OK\r\n\n<!doctype><html>Homepage!</html>\r\n",49);
  }
  else if(*vy=='n')
  {
    SSL_write(q,"HTTP/2 200 OK\r\n\n<!doctype><html>This Is Another Page.</html>\r\n",61);
  }
  else
  {
    SSL_write(q,"HTTP/2 200 OK\r\n\n<!doctype><html>This Is Yet Another Page.</html>\r\n",66);
  };
};

// Main Logic;
int main(int e,char **r)
{
  // Load Certificates;
  f=open("cert.pem",0);if(f>>31){write(1,qe,33);return 1;};fstat(f,&d);s=d.st_size;b=malloc(s);read(f,b,s);close(f);
  f=open("key.pem",0);if(f>>31){write(1,qe,33);return 1;};fstat(f,&d);a=d.st_size;c=malloc(a);read(f,c,a);close(f);

  // Start Server On Port 3000;
  *(i+3)=16777216;insrv(i,3000,router,b,s,c,a);free(b);free(c);pthread_join(*nu,0);

  // Return Success;
  desrv();return 0;
};