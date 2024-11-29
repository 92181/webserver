#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <openssl/ssl.h>
#include <pthread.h>

// Define Safe Globals;
unsigned int *ny,nt,ns,nl=28;pthread_t *nu;SSL_CTX *ctx;

void (*funcPtr)(SSL*);

// Server Processing Function;
static inline void *thsrv(void *j)
{
    unsigned int x=*((unsigned int*)j),y,u[7];SSL *h;

    // Process Incoming Data;
    while(1)
    {
        y=accept(ns,(struct sockaddr*)&u,&nl);

        if(y>>31==0)
        {
            h=SSL_new(ctx);SSL_set_fd(h,y);SSL_accept(h);funcPtr(h);SSL_free(h);close(y);
        };
    };

    // Return Succes;
    return 0;
};

// Start Server;
static inline void insrv(unsigned int *z,unsigned short p,void (*func)(SSL*),unsigned char *b,unsigned int s,unsigned char *c,unsigned int a)
{
    funcPtr=func;

    // Create SSL Method And Context;
    const SSL_METHOD *method=TLS_server_method();ctx=SSL_CTX_new(method);

    // Parse Certificates And Load The Certificates Into The OpenSSL Context;
    BIO *crt=BIO_new_mem_buf(b,s),*kye=BIO_new_mem_buf(c,a);X509 *cert=PEM_read_bio_X509(crt,0,0,0);EVP_PKEY *key=PEM_read_bio_PrivateKey(kye,0,0,0);free(crt);free(kye);
    SSL_CTX_use_certificate(ctx,cert);SSL_CTX_use_PrivateKey(ctx,key);EVP_PKEY_free(key);X509_free(cert);

    // Set Port And IP Then Bind And Listen;
    ns=socket(AF_INET6,SOCK_STREAM,0);unsigned char q[28];*(q+1)=AF_INET6;*(unsigned short*)(q+2)=(p>>8|p<<8)&65535;
    *(unsigned int*)(q+8)=*z;*(unsigned int*)(q+12)=*(z+1);*(unsigned int*)(q+16)=*(z+2);*(unsigned int*)(q+20)=*(z+3);bind(ns,(struct sockaddr*)&q,nl);listen(ns,1000);

    // Query Max Amount Of Threads On CPU;
    nt=sysconf(58);ny=malloc(nt*sizeof(int));unsigned int i=0,*x=ny;nu=malloc(nt*sizeof(pthread_t));

    // Set ID And Create Thread For Each;
    while(i<nt){*x=i;pthread_create(&nu[i],0,thsrv,x);i+=1;x+=1;};
};

// Destroy Server;
static inline void desrv()
{
    free(ny);free(nu);
};