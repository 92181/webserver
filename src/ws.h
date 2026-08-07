//#include <stdio.h>
#include <openssl/evp.h>
//#include <unistd.h>

// Generate Handshake Key;
static void ws_key(const unsigned char *i,unsigned char **o)
{
  // Prepend Client Key To 'Magic' String;
  char s[24+36]={'x','x','x','x','x','x','x','x','x','x','x','x','x','x','x','x','x','x','x','x','x','x','x','x','2','5','8','E','A','F','A','5','-','E','9','1','4','-','4','7','D','A','-','9','5','C','A','-','C','5','A','B','0','D','C','8','5','B','1','1'},*t=s,*e=s+24;

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

// Parse Initial WebSocket Frame;
static unsigned char ws_header(unsigned char **j,unsigned long *k)
{
  unsigned char *i=*j;

  // Get Payload Length;
  unsigned long l=*(i+1)&127;

  if(l<=125)
  {
    *j=i+2;
  }
  else if(l==126)
  {
    l=(*(i+2)<<8)|*(i+3);

    *j=i+4;
  }
  else
  {
    unsigned long v=*(unsigned long*)(i+2);l=0;

    l|=((v>>56)&255)<<0;
    l|=((v>>48)&255)<<8;
    l|=((v>>40)&255)<<16;
    l|=((v>>32)&255)<<24;
    l|=((v>>24)&255)<<32;
    l|=((v>>16)&255)<<40;
    l|=((v>>8)&255)<<48;
    l|=(v&255)<<56;
    
    *j=i+10;
  };

  *k=l+4;

  return *i&15;
};

static void ws_unmask(unsigned char *i,unsigned long l)
{
  unsigned char *j=(i+4);unsigned long z=0;
  
  while(z<l-4)
  {
    *j=*j^i[z%4];
    
    j+=1;z+=1;
  };
};

// Create WS Frame (Source, Size) (Start: 10 Free Bytes Needed);
static unsigned long ws_set(unsigned char **j,unsigned long l)
{
  unsigned char *i=(*j)+10;

  if(l<126)
  {
    *(i-2)=(1<<7)+1;*(i-1)=l;

    *j=i-2;

    return l+2;
  }
  else if(l<65536)
  {
    *(i-4)=(1<<7)+1;*(i-3)=126;

    *(unsigned short*)(i-2)=*((unsigned char*)&l)<<8|*(((unsigned char*)&l)+1);

    *j=i-4;

    return l+4;
  }
  else
  {
    **j=(1<<7)+1;*(*j+1)=127;

    unsigned char *x=(unsigned char*)&l,*k=*j+2;x+=sizeof(long);

    while(k<i)
    {
      *k=*x;

      k+=1;x-=1;
    };

    return l+10;
  };
};