# Simplistic C Web Server
**WIP AGAIN - 2024**

Performant and minimal web server, capable of providing full-duplex communication via WS. This web server template has built-in support for the WebSocket protocol. This small server depends on the OpenSSL library, it also uses some native system libraries, wich are included by default on linux. 
It is single-threaded and capable of handling both IPv4 & Ipv6 connections. 

Various things it can do include...
* Handling HTTPS connection requests in a safe and efficient manner.
* Upgrading an HTTPS connection to a Websocket connection (WSS).
* Full-Duplex communication over the Websockets.
* Keeping track of various clients that are connected to the server.

# Usage
The server makes uses of non-blocking sockets to continuously check for incoming connections. There are multiple sections, the WebSocket handshake and initial HTTP requests are grouped together, this server also loops through all previously connected sockets, to check for any transferred data and to reply. 
I suggest to take a closer look at main.c for a better understanding on how the whole proccess works.

Self-signed certificates can be generated with OpenSSL.
```bash
openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -sha256 -days 3560 -nodes -subj '/CN=127.0.0.1'
```

Project can be compiled and ran with gcc using this bash command.
```bash
gcc main.c -o main
./main
```

# License
MIT
