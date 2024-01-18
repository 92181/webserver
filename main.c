#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <openssl/ssl.h>

// openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -sha256 -days 3560 -nodes -subj '/CN=127.0.0.1'
// gcc main.c -o main -lssl, ./main

unsigned int sa,sb,sc,sd,se,*sj,c[10000],i4,*z=c,*i=c+9999,*f=c,*n,*x;int t0;SSL *ssl[10000],*s2,**s0=ssl,**s3,**s4;

static const unsigned char qq[64]={65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,48,49,50,51,52,53,54,55,56,57,43,47},
tt[256]={65,65,65,65,66,66,66,66,67,67,67,67,68,68,68,68,69,69,69,69,70,70,70,70,71,71,71,71,72,72,72,72,73,73,73,73,74,74,74,74,75,75,75,75,76,76,76,76,77,77,77,77,78,78,78,78,79,79,79,79,80,80,80,80,81,81,81,81,82,82,82,82,83,83,83,83,84,84,84,84,85,85,85,85,86,86,86,86,87,87,87,87,88,88,88,88,89,89,89,89,90,90,90,90,97,97,97,97,98,98,98,98,99,99,99,99,100,100,100,100,101,101,101,101,102,102,102,102,103,103,103,103,104,104,104,104,105,105,105,105,106,106,106,106,107,107,107,107,108,108,108,108,109,109,109,109,110,110,110,110,111,111,111,111,112,112,112,112,113,113,113,113,114,114,114,114,115,115,115,115,116,116,116,116,117,117,117,117,118,118,118,118,119,119,119,119,120,120,120,120,121,121,121,121,122,122,122,122,48,48,48,48,49,49,49,49,50,50,50,50,51,51,51,51,52,52,52,52,53,53,53,53,54,54,54,54,55,55,55,55,56,56,56,56,57,57,57,57,43,43,43,43,47,47,47},
yy[256]={65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,48,49,50,51,52,53,54,55,56,57,43,47,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,48,49,50,51,52,53,54,55,56,57,43,47,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,48,49,50,51,52,53,54,55,56,57,43,47,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,48,49,50,51,52,53,54,55,56,57,43};

unsigned char r[125]="HTTP/1.1 101 Switching Protocols\nupgrade: websocket\nConnection: upgrade\nSec-WebSocket-Accept: 0000000000000000000000000000\n\n",v[36]="258EAFA5-E914-47DA-95CA-C5AB0DC85B11",a[65600],*yt,*iy,*i5,ir,i6,i7,i8,*i9,*io,*ip,*ia,*ix,*iq,*t,*o=a+576,hash[21];

unsigned char h0[]="HTTP/3\r\n\n<!DOCTYPE html><html><head><link rel='icon' href='data:,'></head><body><script>s=new WebSocket('wss://127.0.0.1:3000');s.onopen=function(){console.log('OPEN');s.send('Hello!');setTimeout(function(){s.close();console.log('CLOSED')},4000)};</script></body></html>";

#define rol(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))
#define blk0(i) (sj[i] = (rol(sj[i],24)&4278255360U)|(rol(sj[i],8)&16711935))
#define blk(i) (sj[i&15] = rol(sj[(i+13)&15]^sj[(i+8)&15]^sj[(i+2)&15]^sj[i&15],1))
#define R0(v,w,x,y,z,i) z+=((w&(x^y))^y)+blk0(i)+1518500249+rol(v,5);w=rol(w,30);
#define R1(v,w,x,y,z,i) z+=((w&(x^y))^y)+blk(i)+1518500249+rol(v,5);w=rol(w,30);
#define R2(v,w,x,y,z,i) z+=(w^x^y)+blk(i)+1859775393+rol(v,5);w=rol(w,30);
#define R3(v,w,x,y,z,i) z+=(((w|x)&y)|(w&x))+blk(i)+2400959708U+rol(v,5);w=rol(w,30);
#define R4(v,w,x,y,z,i) z+=(w^x^y)+blk(i)+3395469782U+rol(v,5);w=rol(w,30);

void SHA1Transform()
{
    unsigned int a,b,c,d,e;a=sa;b=sb;c=sc;d=sd;e=se;

    R0(a,b,c,d,e,0);R0(e,a,b,c,d,1);R0(d,e,a,b,c,2);R0(c,d,e,a,b,3);R0(b,c,d,e,a,4);R0(a,b,c,d,e,5);R0(e,a,b,c,d,6);R0(d,e,a,b,c,7);R0(c,d,e,a,b,8);R0(b,c,d,e,a,9);R0(a,b,c,d,e,10);R0(e,a,b,c,d,11);R0(d,e,a,b,c,12);R0(c,d,e,a,b,13);R0(b,c,d,e,a,14);R0(a,b,c,d,e,15);
    R1(e,a,b,c,d,16);R1(d,e,a,b,c,17);R1(c,d,e,a,b,18);R1(b,c,d,e,a,19);R2(a,b,c,d,e,20);R2(e,a,b,c,d,21);R2(d,e,a,b,c,22);R2(c,d,e,a,b,23);R2(b,c,d,e,a,24);R2(a,b,c,d,e,25);R2(e,a,b,c,d,26);R2(d,e,a,b,c,27);R2(c,d,e,a,b,28);R2(b,c,d,e,a,29);R2(a,b,c,d,e,30);R2(e,a,b,c,d,31);
    R2(d,e,a,b,c,32);R2(c,d,e,a,b,33);R2(b,c,d,e,a,34);R2(a,b,c,d,e,35);R2(e,a,b,c,d,36);R2(d,e,a,b,c,37);R2(c,d,e,a,b,38);R2(b,c,d,e,a,39);R3(a,b,c,d,e,40);R3(e,a,b,c,d,41);R3(d,e,a,b,c,42);R3(c,d,e,a,b,43);R3(b,c,d,e,a,44);R3(a,b,c,d,e,45);R3(e,a,b,c,d,46);R3(d,e,a,b,c,47);
    R3(c,d,e,a,b,48);R3(b,c,d,e,a,49);R3(a,b,c,d,e,50);R3(e,a,b,c,d,51);R3(d,e,a,b,c,52);R3(c,d,e,a,b,53);R3(b,c,d,e,a,54);R3(a,b,c,d,e,55);R3(e,a,b,c,d,56);R3(d,e,a,b,c,57);R3(c,d,e,a,b,58);R3(b,c,d,e,a,59);R4(a,b,c,d,e,60);R4(e,a,b,c,d,61);R4(d,e,a,b,c,62);R4(c,d,e,a,b,63);
    R4(b,c,d,e,a,64);R4(a,b,c,d,e,65);R4(e,a,b,c,d,66);R4(d,e,a,b,c,67);R4(c,d,e,a,b,68);R4(b,c,d,e,a,69);R4(a,b,c,d,e,70);R4(e,a,b,c,d,71);R4(d,e,a,b,c,72);R4(c,d,e,a,b,73);R4(b,c,d,e,a,74);R4(a,b,c,d,e,75);R4(e,a,b,c,d,76);R4(d,e,a,b,c,77);R4(c,d,e,a,b,78);R4(b,c,d,e,a,79);
    
    sa+=a;sb+=b;sc+=c;sd+=d;se+=e;
};

int main() // Main;
{
    unsigned int y,s=socket(2,1,0),l=16;ioctl(s,21537,&l);struct sockaddr_in q;q.sin_family=2;q.sin_addr.s_addr=0;q.sin_port=htons(3000);bind(s,(struct sockaddr*)&q,l);struct sockaddr u;listen(s,1000);
    const SSL_METHOD *method=TLS_server_method();SSL_CTX *ctx=SSL_CTX_new(method);SSL_CTX_use_certificate_file(ctx,"cert.pem",1);SSL_CTX_use_PrivateKey_file(ctx,"key.pem",1);

    while(1)
    {
        y=accept(s,&u,&l);

        if(y>>31==0)
        {
            s2=SSL_new(ctx);SSL_set_fd(s2,y);SSL_accept(s2);SSL_read(s2,a,576);

            t=a;do // Initial WebSocket handshake;
            {
                if(*t==101)
                {
                    if(*(t+1)==121)
                    {
                        sa=1732584193;sb=4023233417;sc=2562383102;sd=271733878;se=3285377520;t+=28;yt=v,iy=v+36;while(yt<iy){*t=*yt;t+=1;yt+=1;}
                        sj=(unsigned int*)(t-60);sj[15]=128;SHA1Transform();sj[15]=3758161920;sj[14]=sj[13]=sj[12]=sj[11]=sj[10]=sj[9]=sj[8]=sj[7]=sj[6]=sj[5]=sj[4]=sj[3]=sj[2]=sj[1]=sj[0]=0;SHA1Transform();
                
                        unsigned char i0=sa>>24,i1=sa>>16,i2=sa>>8;*(r+94)=tt[i0];*(r+95)=qq[i1>>4|(i0&3)<<4];*(r+96)=qq[i2>>6|(i1&15)<<2];*(r+97)=yy[i2];i0=sa,i1=sb>>24,i2=sb>>16;*(r+98)=tt[i0];*(r+99)=qq[i1>>4|(i0&3)<<4];*(r+100)=qq[i2>>6|(i1&15)<<2];*(r+101)=yy[i2];
                        i0=sb>>8,i1=sb,i2=sc>>24;*(r+102)=tt[i0];*(r+103)=qq[i1>>4|(i0&3)<<4];*(r+104)=qq[i2>>6|(i1&15)<<2];*(r+105)=yy[i2];i0=sc>>16,i1=sc>>8,i2=sc;*(r+106)=tt[i0];*(r+107)=qq[i1>>4|(i0&3)<<4];*(r+108)=qq[i2>>6|(i1&15)<<2];*(r+109)=yy[i2];
                        i0=sd>>24,i1=sd>>16,i2=sd>>8;*(r+110)=tt[i0];*(r+111)=qq[i1>>4|(i0&3)<<4];*(r+112)=qq[i2>>6|(i1&15)<<2];*(r+113)=yy[i2];i0=sd,i1=se>>24,i2=se>>16;*(r+114)=tt[i0];*(r+115)=qq[i1>>4|(i0&3)<<4];*(r+116)=qq[i2>>6|(i1&15)<<2];*(r+117)=yy[i2];
                        i0=se>>8,i1=se;*(r+118)=tt[i0];*(r+119)=qq[i1>>4|(i0&3)<<4];*(r+120)=qq[(i1&15)<<2];*(r+121)=61;
                        
                        SSL_write(s2,r,125);ioctl(y,21537,&l);*z=y;*s0=s2;z+=1;break;
                    };
                };

                t+=1;
            }
            while(t<o);

            if(t==o) // Initial HTTP get;
            {
                SSL_write(s2,h0,271);SSL_shutdown(s2);SSL_free(s2);close(y);
            };
        }
        else // WebSocket proccess;
        {
            while(f<z)
            {
                t0=SSL_read(*s0,a,65600);if(t0>0) // Handle received data;
                {
                    if(*a==136) // Handle WebSocket disconnect;
                    {
                        SSL_shutdown(*s0);SSL_free(*s0);close(*f);n=f;x=f+1;s3=s0;s4=s0+1;while(x<z){*n=*x;*s0=*s4;n+=1;x+=1;s3+=1;s4+=1;}z-=1;printf("Shutdown!\n");continue;
                    };

                    i5=a+2;i4=*(a+1)&127;if(i4>125){if(i4==126){i4=*(a+2)|*(a+3)<<8;i5+=2;}else{i4=65536;i5+=7;};};ir=*i5;i6=*(i5+1);i7=*(i5+2);i8=*(i5+3);ix=iq=i5+4;i9=ix+i4;io=ix+1;ip=io+1;ia=ip+1;
                    do{*ix=*ix^ir;*io=*io^i6;*ip=*ip^i7;*ia=*ia^i8;ix+=4;io+=4;ip+=4;ia+=4;}while(ix<i9);
                    
                    unsigned char *i=iq;while(i<i9){printf("%c",*i);i+=1;};printf("\n");
                }
                else if(t0==0) // Handle TCP disconnect;
                {
                    SSL_shutdown(*s0);SSL_free(*s0);close(*f);n=f;x=f+1;s3=s0;s4=s0+1;while(x<z){*n=*x;*s0=*s4;n+=1;x+=1;s3+=1;s4+=1;}z-=1;printf("Disconnect!\n");continue;
                };

                f+=1;s0+=1;
            };
            
            f=c;s0=ssl;
        };
    };
};