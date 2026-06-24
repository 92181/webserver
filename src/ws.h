//#include <stdio.h>
#include <openssl/evp.h>
//#include <unistd.h>

// Generate Handshake Key;
static void ws_key(const unsigned char *i,unsigned char **o)
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

// Unmask WebSocket Frame;
static void ws_header(unsigned char **j,unsigned long *k) // return OPCODE
{
  unsigned char *i=*j;

  // Get Payload Length;
  unsigned long l=*(i+1)&127;

  if(l<=125)
  {
    //i+=2;
    *j=i+2;
  }
  else if(l==126)
  {
    l=(*(i+2)<<8)|*(i+3);

    //i+=4;
    *j=i+4;
  }
  else
  {
    printf("Big READ USED\n");
    /*i+=2;unsigned char *e=i+8;l=0;

    while(i<e)
    {
      l=(l<<8)|*i;i+=1; // test it with big msg.
    };*/

    /*
    unsigned char* v = (unsigned char*)(i + 2);l=0;
    l |= ((uint64_t)v[0]) << 56;
    l |= ((uint64_t)v[1]) << 48;
    l |= ((uint64_t)v[2]) << 40;
    l |= ((uint64_t)v[3]) << 32;
    l |= ((uint64_t)v[4]) << 24;
    l |= ((uint64_t)v[5]) << 16;
    l |= ((uint64_t)v[6]) << 8;
    l |= ((uint64_t)v[7]);*/

    unsigned long v=*(unsigned long*)(i+2);l=0;

    // better?
    l |= ((v >> 56) & 255) << 0; // 255 was 0xFFULL
    l |= ((v >> 48) & 255) << 8;
    l |= ((v >> 40) & 255) << 16;
    l |= ((v >> 32) & 255) << 24;
    l |= ((v >> 24) & 255) << 32;
    l |= ((v >> 16) & 255) << 40;
    l |= ((v >> 8) & 255) << 48;
    l |= (v & 255) << 56;
    
    //i+=10;
    *j=i+10;
  };
  printf("ws len: %lu\n",l);

  *k=l+4;//*j=i;

  // HANDLE PING PONG (opcode of 9 and 10)
  // HANDLE CLOSE CODE (opcode of 8)
};

static void ws_unmask(unsigned char *i,unsigned long l)
{
  unsigned char *j=(i+4); // use i directly (test later when all else OK)...
  
  for (size_t z = 0; z < l; ++z)
  {
    *j = *j ^ i[z%4];
    j+=1;
  };
};

/*static void ws_unmask(char *i,unsigned long l)
{
  unsigned char *k=(unsigned char*)(i),*j=(unsigned char*)(i+4); // use i directly (test later when all else OK)...
  
  for (size_t z = 0; z < l; ++z)
  {
    *j = *j ^ k[z%4];
    j+=1;
  };
};*/

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

    unsigned char *x=(unsigned char*)&l,*k=*j+2;x+=sizeof(long);//char *k=*j+2;

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