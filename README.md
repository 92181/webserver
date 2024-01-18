# Compact C Web Server
Performant and minimal web server, capable of providing full-duplex communication via WS. This web server template has built-in support for the WebSocket protocol. This small server depends on the OpenSSL library, it also uses some native system libraries, wich are included by default on linux. It is single-threaded.

# Usage
The server makes uses of non-blocking sockets to continuously check for incoming connections. There are multiple sections, the WebSocket handshake and initial HTTP requests are grouped together, this server also loops through all previously connected sockets, to check for any transferred data and to reply. I suggest to take a closer look at main.c for a better understanding on how the whole proccess works.
