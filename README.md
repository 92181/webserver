# WpXI - Simple & Performant HTTPS Server
Performant and minimal web server, capable of being used simply and also highly extensible. This small web server library depends on OpenSSL, it also uses some native system libraries, wich are included by default on Linux and MacOS. 
It serves all of it's files in a multi-threaded way and is capable of handling both IPv4 & Ipv6 connections. 

It was written because I was frustated by the lack of functionality of `python3 -m http.server` and wanted a 'drop in' replacement which was highly extensible, written in C and which you can modify as you see fit.

Various things that it can do are...
* Handling HTTPS connection requests in a safe and efficient manner.
* Serving static and dynamic files using various AJAX methods.
* Keeping track of various clients that are connected to the server.

It contains two examples...
* Basic, an example illustrating a simple server side routing mechanism.
* Static, a fully operational HTTPS static file server which serves files from any directory.

# Usage
The server makes uses of non-blocking sockets to continuously check for incoming connections. 
There are multiple sections to the code, such as certificate handling, serving, threading, etc.

Self-signed certificates can be generated with OpenSSL.
```bash
openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -sha256 -days 3560 -nodes -subj '/CN=127.0.0.1'
```

The examples can be compiled and ran with gcc using these bash commands.
```bash
gcc main.c -o main -O3 -lssl -lcrypto
./main
```

# About & Licensing
This project is licensed under the permissive MIT license. Please consider starring the project if you like it.

This project originally used to contain a WebSocket implementation, however it was removed when I decided to reprogram a part of the library.
