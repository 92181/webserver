#include <stdio.h>
#include <openssl/evp.h>
#include <unistd.h>

// SHA-1 Hash & Base64 Encode;
char *ws_str="258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

static void ws_key(const char *i,char **o,int s)
{
  EVP_MD_CTX *mdctx=EVP_MD_CTX_new();unsigned char h[EVP_MAX_MD_SIZE];unsigned int l;

  EVP_DigestInit_ex(mdctx,EVP_sha1(),0);EVP_DigestUpdate(mdctx,i,s);EVP_DigestFinal_ex(mdctx,h,&l);

  EVP_MD_CTX_free(mdctx);

  // Base64 Encode;
  EVP_EncodeBlock((unsigned char*)*o,h,l);
};

// https://developer.mozilla.org/en-US/docs/Web/API/WebSockets_API/Writing_WebSocket_servers

// Unmask WebSocket Frame;
static void ws_read(char **j,unsigned long *k)
{
  printf("Frame Func\n");

  char *i=*j;

  // Get Payload Length;
  unsigned long l=*(i+1)&127;

  if(l<=125)
  {
    i+=2;
  }
  else if(l==126)
  {
    l=(*(i+2)<<8)|*(i+3); // test it with big msg. >126 && <65536?
    i+=4;
  }
  else
  {
    i+=2;char *e=i+8;l=0;

    while(i<e)
    {
      l=(l<<8)|*i;i+=1; // test it with big msg.
    };
  };

  // Get Mask & Decode;
  unsigned char *mask_key=(unsigned char*)i;i+=4;

  *k=l;*j=i;
  
  for (size_t z = 0; z < l; ++z) 
  {
    *i = *i ^ mask_key[z % 4];
    i+=1;
  };
};

// Create WS Frame (Type, Size) (Front: 10 Bytes Needed);
static void ws_set(char **j,unsigned long l)
{
  char *i=*j+10;

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

    unsigned char *x=(unsigned char*)&l;x+=8;char *k=*j+2;

    while(k<i) // untested;
    {
      *k=*x;

      k+=1;x-=1;
    };
  };

  // FIN (1) + bullshit (7)
  // mask? (1 to 0!) + payload length (7) (dy)
  // content (x)
};