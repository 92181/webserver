#include "../../server.c"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>

// gcc main.c -o main -O3 -lssl -lcrypto, https://[::1]:3000/

unsigned char *b,*c,*qw=(unsigned char*)"\033[31mMemory Allocation Failure!\033[0m\n",*qe=(unsigned char*)"\033[31mFile Operation Failed!\n\033[0m",*u,*t,*j,**gh,**sf,**se;struct stat d;unsigned int gl,gj=0;

unsigned int i[4],s,a,f;char vq[16384],*vz,*vw,*vy,*gi="index.html",*gu;

// Hash Bytes Using FNV Hashing Algorithm;
static inline unsigned int fnv(char *i,char *u)
{
  // Hash In Groups Of 4 Bytes;
  unsigned int h=2166136261;while(i+4<u){h=(h^*i)*16777619;h=(h^*(i+1))*16777619;h=(h^*(i+2))*16777619;h=(h^*(i+3))*16777619;i+=4;};
  
  // Deal With Remaining Bytes And Return;
  while(i<u){h=(h^*i)*16777619;i+=1;};return h;
};

// Load All Files In Given Directory And Create Hashmap;
static inline void trd(char *g,unsigned int b)
{
  // Allocate Variables (May Overflow);
  char st[131072],*y=st,*pt[2048],**jx=pt,cd[8192];t=u=malloc(1024*1024);j=u+1024*1024;
  
  // Write Name To Stack;
  *jx=y;while(*g!=0){*y=*g;y+=1;g+=1;};*y=0;y+=1;

  // Loop Through Saved Directories;
  while(jx>=pt)
  {
    // Write Current Directory To Seperate Buffer;
    g=cd;y=*jx;while(*y!=0){*g=*y;y+=1;g+=1;};*g=0;y=*jx;jx-=1;

    // Open Directory And Make Ready For Path Fill;
    struct dirent *en;DIR *dr=opendir((const char*)cd);if(dr==0){continue;};*g='/';g+=1;

    while((en=readdir(dr))!=0)
    {
      // Get Name And Skip Hidden Files;
      char *v=en->d_name,*tu=g,*ty=tu+en->d_namlen,*lr;if(*v=='.'){continue;};

      // Write Extension Of Path;
      while(tu<ty){*tu=*v;v+=1;tu+=1;};*tu=0;
      
      // Save Directories To Stack, Else Load Files Into Memory;
      if(en->d_type==4)
      {
        // Save To Stack (May Overflow);
        lr=cd;jx+=1;*jx=y;while(*lr!=0){*y=*lr;y+=1;lr+=1;};*y=0;y+=1;
      }
      else
      {
        // Open The File And Check It's Size;
        unsigned int f=open(cd,0),s;if(f>>31){write(1,qe,33);return;};stat(cd,&d);s=d.st_size;gj+=1;

        // Detect Index Files And Remove Slashes;
        lr=g;gu=gi;while(lr<ty&&*lr==*gu){lr+=1;gu+=1;}unsigned int hj;if(lr==ty){hj=tu-cd-b-10;ty-=10;if(hj!=0){hj-=1;ty-=1;};}else{hj=tu-cd-b;};

        // Increase Size Of Content Buffer If It Does Not Fit;
        unsigned char *r=t+sizeof(int)*2+(hj+s);if(r>j){unsigned char *l=u;unsigned int x=r-u+1024*1024;u=realloc(u,x);t=u+(t-l);j=u+x;};

        // Write Size Of Name And Content, Then Write Name And File Content To Buffer;
        *(unsigned int*)t=hj;*(unsigned int*)(t+sizeof(int))=s;t+=sizeof(int)*2;lr=cd+b;while(lr<ty){*t=*lr;t+=1;lr+=1;};read(f,t,s);close(f);t+=s;
      };
    };

    closedir(dr);
  };

  // Allocate And Fill Hash Table With Zero's; // gj*16
  gl=gj*4;gh=sf=malloc(gl*sizeof(char*));se=gh+gl;while(sf<se){*sf=0;sf+=1;};unsigned char *gr=u;

  while(gr<t)
  {
    // Hash The Index Name;
    unsigned int fr=*(unsigned int*)gr,gg=*(unsigned int*)(gr+sizeof(int));char *of=(char*)(gr+sizeof(int)*2);unsigned int h=fnv(of,of+fr)%gl;

    // Prevent Collisions And Insert Into Table;
    unsigned char **y=gh+h;while(*y!=0){if(y>=se){y=0;};y+=1;if(*y==0){break;};};*y=gr;gr+=fr+gg+sizeof(int)*2;
  };
};

// Server Router;
static inline void router(SSL *q)
{
  // Read Request And Extract URL;
  SSL_read(q,vq,16384);vw=vq;vz=vq+16384;while(vw<vz){if(*vw=='/'){vy=vw+1;while(vw<vz){if(*vw==' '){break;};vw+=1;};break;};vw+=1;};

  // Retrieve Item Using Hash;
  unsigned int l=vw-vy,h=fnv(vy,vw)%gl,rr,rs;unsigned char **y=gh+h,*z=*y;

  while(z!=0)
  {
    // Check If Size Matches;
    rr=*(unsigned int*)z;

    if(l==rr)
    {
      // Compare Keys, Break On Hit;
      rs=0;z+=sizeof(int)*2;while(rs<rr&&*z==*vy){z+=1;vy+=1;rs+=1;};

      // Process Match;
      if(rs==rr){z=*y;SSL_write(q,"HTTP/2 200 OK\r\n\n",16);SSL_write(q,z+l+sizeof(int)*2,*(unsigned int*)(z+sizeof(int)));SSL_write(q,"\r\n",2);break;};
    };

    // Run Through Table On Collision;
    y+=1;if(y>=se){y=gh;};z=*y;
  };
};

// Main Logic;
int main(int e,char **r)
{
  // Load Files Into Memory And Create References;
  trd("./public",8+1);

  // Load Certificates;
  f=open("cert.pem",0);if(f>>31){write(1,qe,33);return 1;};fstat(f,&d);s=d.st_size;b=malloc(s);read(f,b,s);close(f);
  f=open("key.pem",0);if(f>>31){write(1,qe,33);return 1;};fstat(f,&d);a=d.st_size;c=malloc(a);read(f,c,a);close(f);

  // Start Server On Port 3000;
  *(i+3)=16777216;insrv(i,3000,router,b,s,c,a);free(b);free(c);pthread_join(*nu,0);

  // Return Success;
  free(u);free(gh);desrv();return 0;
};
