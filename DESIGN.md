# Server Design
For the client connection structure we use atomics so a connection can be safely closed from other threads.
There is a performance benefit to removing them, and for io_uring it would be highly possible, for MacOS it complicates things in relation to async networking sadly. I also add the kqueue/ring to the structure for added flexiblity (albeit we could do without, pass directly).

# Load Balancing
As we have multiple threads, the work needs to get distributed between them, on Linux and FreeBSD we have support for SO_REUSEPORT and SO_REUSEPORT_LB, which effectively means the kernel handles thread wake-ups. On MacOS SO_REUSEPORT does not load balance (takes last or first socket), so we have a 'Round Robin' load balancer at the very start of the socket accept.

# Async File Loading
For MacOS the standard is the GCD (Dispatch API), aio_read is not supported for kqueue atleast. Luckily FreeBSD also supports it through a simple compatibility wrapper. GCD is a little weird as it occupies a thread, only to use it in a blocking manner, sadly a superior alternative such as io_uring will 'never' be implemented.

# Backpressure Handler
A backpressure handler is exposed, for both kqueue and io_uring.