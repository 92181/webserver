# WpXI - Performant & Minimal HTTP Server
WpXI is a lightweight, high-performance web server library engineered for extreme efficiency and extensibility. 
By leveraging modern kernel primitives such as io_uring on Linux and Kqueue on macOS. WpXI achieves near-native I/O throughput with minimal CPU overhead.

Built in C and powered by OpenSSL, it serves as a robust "drop-in" replacement for `python3 -m http.server` when performance, security, and customization are non-negotiable.
Uses Kqueue for FreeBSD and MacOS, kTLS with io_uring for Linux (>5.13).

Why WpXI?
* Event-Driven Architecture: Fully multi-threaded and non-blocking I/O ensures the server scales with your hardware.
* Modern Networking: Native support for IPv6 out of the box.
* Minimalist Core: No bloated dependencies—just pure C and system-native libraries alongside OpenSSL.

Included Examples:
* Basic: A demonstration of efficient server-side routing and request handling.
* Websockets: Implementation of duplex communication channels for real-time apps with backpressure handling.
* Static: A high-speed HTTPS static server capable of serving cached assets from any provided directory.
* Disk: HTTPS example containg the neccessary tools to facilitate serving in an async manner from the disk.

## Usage
More information about the makeup and design choices of this project can be found in `DESIGN.md`.
But essentially a project using this library would consist of a `main.c` file, which then links to the `server.c`.

Self-signed certificates can be generated with OpenSSL.
```bash
openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -sha256 -days 3560 -nodes -subj '/CN=127.0.0.1'
```

The router example can be run with the following command:
```bash
sudo sh run.sh
```

## Limitations
- IPv6 only.
- Maximum of 1 server instance per process.
- HTTP/1.1 only (For now).

## About & Licensing
This project is licensed under the permissive MIT license. Please consider starring the project if you like it.
