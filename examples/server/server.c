#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include "../../src/server.c"

#define DIR_STACK 2056
#define MAX_FILES 1024*64
#define TABLE_SIZE 1024*24

#define URL "https://[::1]:8000"

typedef struct cd
{
  struct cd *next;
  long key, data, data_size;
} cd;

unsigned int i[4];cd *ht[TABLE_SIZE],hd[MAX_FILES];char *b;

// Redirect HTTP Requests;
static void *http_redirect(void *p)
{
  // Initialize Variables;
  struct sockaddr_in6 u;unsigned int f=(long)p,s,q=sizeof(u);

  fcntl(f,F_SETFL,fcntl(f,F_GETFL,0)&~O_NONBLOCK);
  
  // Prepare Output;
  const char x[]="\r\n\r\n";char r[512]="HTTP/1.1 301\r\nContent-Length: 0\r\nLocation: ",*a=r+42+sizeof(URL);
  
  mem_copy(r+43,URL,(char*)URL+sizeof(URL)-1);

  // Process Incoming Data;
  while(1)
  {
    s=accept(f,(struct sockaddr*)&u,&q);

    if(s>>31==0)
    {
      // HTTP Redirect;
      char *z=a,*e=a+256;(void)!read(s,a,256);

      while(z<e&&*z!='/')
      {
        z+=1;
      };

      while(z<e&&*z!=' ')
      { 
        *a=*z;z+=1;a+=1;
      };

      mem_copy(a,x,x+4);a+=4;
      
      (void)!write(s,r,a-r);close(s);
    };
  };
};

// Convert Integer To String;
static inline char *itc(unsigned int n,char *s)
{
  char *t=s,c,*u,*e=s;

  while(n>0) 
  {
    unsigned int d=n%10;*t=d+'0';

    t+=1;n/=10;
  };

  // Reverse Result;
  u=t;t-=1;

  while(e<t)
  {
    c=*e;*e=*t;*t=c;e+=1;t-=1;
  };

  return u;
};

// Hash Bytes Using FNV Hashing Algorithm;
static inline unsigned int fnv(char *i,char *u)
{
  // Hash In Groups Of 4 Bytes;
  unsigned int h=2166136261;
  
  while(i+4<u)
  {
    h=(h^*i)*16777619;
    h=(h^*(i+1))*16777619;
    h=(h^*(i+2))*16777619;
    h=(h^*(i+3))*16777619;
    
    i+=4;
  };
  
  // Deal With Remaining Bytes And Return;
  while(i<u)
  {
    h=(h^*i)*16777619;i+=1;
  };
  
  return h;
};

// Load & Hash Files (Public Directory);
int cache_files()
{
  // Allocate Directory Stack & CD Buffer;
  char d[DIR_STACK*512],*o,p[2056]={0},*h[DIR_STACK]={0},**j=h;cd *l=hd;long m=1024*1024*8;
  
  struct stat x;b=malloc(m);char *i=b;

  if(b==0)
  {
    return 1;
  };

  *d='.';*(d+1)=0;o=d+2; *j=d;*(j+1)=o;j+=2;

  // Loop Directory Stack;
  while(j-1>h)
  {
    j-=1;o=*(j-1);

    // Cache Directory Path;
    int g=(*j-*(j-1));mem_copy(p,*(j-1),*j);*(p+g-1)='/';*(p+g)=0;

    struct dirent *e;DIR *r=opendir(p);if(r==0){continue;};

    while((e=readdir(r))!=0)
    {
      int s;char *v=e->d_name,*y=v;
      
      if(*v=='.'){continue;};
      
      while(*y!=0){y+=1;};s=y-v+1;

      // Save Directory;
      if(e->d_type==4)
      {
        if(g+s>=2056||o+g+s>=d+DIR_STACK*512)
        {
          continue;
        };

        mem_copy(o,p,p+g);mem_copy(o+g,v,v+s);o+=(g+s);

        *j=o;j+=1;
      }
      else
      {
        if(p+g+s>=p+2056)
        {
          continue;
        };
        
        mem_copy(p+g,v,v+s);

        // Get FD;
        int z=g+s-2,f=open(p,O_RDONLY);if(f<0){continue;}else if(l>=hd+MAX_FILES){close(f);};

        fstat(f,&x);

        // Check Buffer Limit;
        if(i+x.st_size+z+512>=b+m)
        {
          long p=m;if(m*2<m+x.st_size+z){m+=x.st_size+z;}else{m*=2;};

          char *x=realloc(b,m);

          if(x!=0)
          {
            i=x+(i-b);b=x;
          }
          else
          {
            m=p;continue;
          };
        };

        // Add Mime-Type (Tag Size: <512);
        char *r,*q=i+z;

        do{y-=1;}while((y>v&&*y!='.'));

        switch(*(y+1))
        {
          case 'w':
          {
            // Handle Fonts (Woff2);
            const char a[]="HTTP/1.1 200\r\nContent-Type: font/woff2\r\nContent-Length: ",b[]="\r\n\r\n";

            mem_copy(q,a,a+sizeof(a)-1);

            // Add Length & HTTP Termination;
            r=itc(x.st_size,q+sizeof(a)-1);

            mem_copy(r,b,b+sizeof(b)-1);r+=sizeof(b)-1;

            break;
          }
          case 'h':
          {
            // Clean Path (Index);
            if(s==11&&*v=='i'&&*(v+1)=='n'&&*(v+4)=='x'&&*(v+5)=='.')
            {
              z=g-2;q=i+z;
            };
          }
          default:
          {
            const char a[]="HTTP/1.1 200\r\nContent-Length: ",b[]="\r\n\r\n";
            
            mem_copy(q,a,a+sizeof(a)-1);

            // Add Length & HTTP Termination;
            r=itc(x.st_size,q+sizeof(a)-1);

            mem_copy(r,b,b+sizeof(b)-1);r+=sizeof(b)-1;
          }
        };

        // Copy Path (Key) & File Data;
        mem_copy(i,p+1,p+1+z);read(f,i+z+(r-q),x.st_size);

        // Set Sizes;
        l->key=i-b; l->data=l->key+z; l->data_size=x.st_size+(r-q);

        i+=l->data_size+z;

        // Hash & Insert;
        unsigned int n=fnv(p+1,p+1+z)%TABLE_SIZE;

        if(*(ht+n)!=0)
        {
          cd *h=*(ht+n);l->next=h;
        };

        *(ht+n)=l;

        close(f);l+=1;
      };
    };

    closedir(r);
  };

  return 0;
};

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

  // Retrieve Item Using Hash;
  unsigned int n=fnv(s,z)%TABLE_SIZE;cd *h=*(ht+n);

  while(h!=0)
  {
    int key_size=h->data-h->key;

    // Compare Key (Name);
    if(key_size==z-s)
    {
      char *u=b+h->key,*e=u+key_size,*v=s;

      while(u<e&&*u==*v){u+=1;v+=1;};

      if(u==e)
      {
        server_send(t,b+h->data,h->data_size);

        return;
      };
    };

    // Check Next;
    h=h->next;
  };
  
  // Send 404;
  char q[]="HTTP/1.1 404\r\nContent-Length: 15\r\n\r\nNot Found - 404";
  
  server_send(t,q,sizeof(q));
};

// Close (Write Complete);
static void c_write(ud *t,int u)
{
  close_socket(t);
};

// Main Logic;
int main(int e,char **r)
{
  // Setup HTTP Redirection;
  long f=setup_sock(i,8080);pthread_t th;

  if(f<=0)
  {
    return 1;
  };

  pthread_create(&th,0,http_redirect,(void*)f);

  // Load Certificates;
  cz p={.port=8000,.ip={0},.ev_recv=c_recv,.ev_write=c_write};

  if(load_cert(&p,"../../src/cert.pem","../../src/key.pem")!=0)
  {
    goto destroy;
  };

  if(chdir("./public")<0)
  {
    goto destroy;
  };
  
  if(cache_files()!=0){goto destroy;};

  // Start Server;
  long x=start_server(&p);

  if(x>0)
  {
    pthread_join(x,0);destroy_server(&p);
  };

  destroy:
  if(b!=0){free(b);};close(f);
  
  return 0;
};