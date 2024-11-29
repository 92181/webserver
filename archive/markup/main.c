#include "../../server.c"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

// gcc main.c -o main -O3 -lssl -lcrypto, https://[::1]:3000/

unsigned char *b,*c,*e,*qe=(unsigned char*)"\033[31mFile Operation Failed!\n\033[0m";struct stat d;unsigned int i[4],s,a,l,f;

// Process Markup;
static inline void mkr(unsigned char *e)
{
  //malloc(1024*1024);
  //<!doctype><html>Markup</html>
};

// Server Router;
static inline void rtr(SSL *q)
{
  // Write Response To Client;
  SSL_write(q,"HTTP/2 200 OK\r\n\nContent\r\n",49);
};

// Main Logic;
int main(int e,char **r)
{
  // Load Certificates;
  f=open("cert.pem",0);if(f>>31){write(1,qe,33);return 1;};fstat(f,&d);s=d.st_size;b=malloc(s);read(f,b,s);close(f);
  f=open("key.pem",0);if(f>>31){write(1,qe,33);return 1;};fstat(f,&d);a=d.st_size;c=malloc(a);read(f,c,a);close(f);

  // Load Page And Process It As A Markup File;
  f=open("page.md",0);if(f>>31){write(1,qe,33);return 1;};fstat(f,&d);l=d.st_size;e=malloc(l);read(f,e,l);mkr(e);close(f);close(f);

  // Start Server On Port 3000;
  *(i+3)=16777216;insrv(i,3000,rtr,b,s,c,a);free(b);free(c);pthread_join(*nu,0);

  // Return Success;
  desrv();return 0;
};
