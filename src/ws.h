//#include <stdio.h>
#include <openssl/evp.h>
//#include <unistd.h>

// Generate Handshake Key;
static void ws_key(const char *i,char **o)
{
  // Prepend Client Key To 'Magic' String;
  char s[24+36]="xxxxxxxxxxxxxxxxxxxxxxxx258EAFA5-E914-47DA-95CA-C5AB0DC85B11",*t=s,*e=s+24;

  while(t<e)
  {
    *t=*i;t+=1;i+=1;
  };

  // SHA-1 Hash;
  EVP_MD_CTX *m=EVP_MD_CTX_new();unsigned char h[EVP_MAX_MD_SIZE];unsigned int l;

  EVP_DigestInit_ex(m,EVP_sha1(),0);EVP_DigestUpdate(m,s,24+36);EVP_DigestFinal_ex(m,h,&l);

  EVP_MD_CTX_free(m);

  // Base64 Encode;
  EVP_EncodeBlock((unsigned char*)*o,h,l);
};

// https://developer.mozilla.org/en-US/docs/Web/API/WebSockets_API/Writing_WebSocket_servers

// Unmask WebSocket Frame;
static void ws_header(char **j,unsigned long *k) // return OPCODE
{
  char *i=*j;

  // Get Payload Length;
  unsigned long l=*(i+1)&127;

  if(l<=125)
  {
    i+=2;
  }
  else if(l==126)
  {
    l=(*(i+2)<<8)|*(i+3);
    i+=4;
  }
  else
  {
    printf("Big READ USED\n");
    i+=2;char *e=i+8;l=0;

    while(i<e)
    {
      l=(l<<8)|*i;i+=1; // test it with big msg.
    };
  };

  *k=l+4;*j=i;

  // HANDLE PING PONG (opcode of 9 and 10)
  // HANDLE CLOSE CODE (opcode of 8)
};

static void ws_unmask(char *i,unsigned long l)
{
  unsigned char *k=(unsigned char*)(i);

  unsigned char *j=(unsigned char*)(i+4); // use i directly (test later when all else OK)...
  
  for (size_t z = 0; z < l; ++z)
  {
    *j = *j ^ k[z%4];
    j+=1;
  };
};

// Create WS Frame (Source, Size) (Start: 10 Free Bytes Needed);
static void ws_set(unsigned char **j,unsigned long l) // was char **j
{
  //unsigned char *i=*(unsigned char*)j+10;
  unsigned char *i=*j+10;

  if(l<126)
  {
    *(i-2)=(1<<7)+1;
    *(i-1)=l;

    *j=i-2;
  }
  else if(l<65536)
  {
    *(i-4)=(1<<7)+1;
    *(i-3)=126;

    *(unsigned short*)(i-2)=*((unsigned char*)&l)<<8|*(((unsigned char*)&l)+1);

    *j=i-4;
  }
  else
  {
    **j=(1<<7)+1;*(*j+1)=127;

    unsigned char *x=(unsigned char*)&l,*k=*j+2;x+=8;//char *k=*j+2;

    while(k<i) // untested; WRONG?
    {
      *k=*x;

      k+=1;x-=1;
    };
  };

  // FIN (1) + bullshit (7)
  // mask? (1 to 0!) + payload length (7) (dy)
  // content (x)
};