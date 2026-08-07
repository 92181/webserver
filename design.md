## Server Design
For the client connection structure we per use a pre-allocated fixed per thread client list (linked), this list contains both a *prev and *next pointer for both way traversel, this way it can be used as both a free tracker and an active socket list. Every client object also has a thread reference.

## Load Balancing
As we have multiple threads, the work needs to get distributed between them, on Linux and FreeBSD we have support for SO_REUSEPORT and SO_REUSEPORT_LB, which effectively means the kernel handles thread wake-ups. On MacOS SO_REUSEPORT does not load balance (takes last or first socket), so we have a 'Round Robin' load balancer at the very start of the socket accept.

## Async File Loading
For MacOS the standard is the GCD (Dispatch API), aio_read is not supported for kqueue. Luckily FreeBSD supports GCD through a simple compatibility wrapper. GCD is a little weird as it occupies a thread, only to use it in a blocking manner, sadly a better alternative such as io_uring will not be implemented.

## Buffers & Write/Backpressure Callback
As we use HTTP/1.1 all writes are sequential (non-interleaved, linear). We use a buffer queue to write all outstanding data, this approuch seems the most efficient. It can be resized by changing WR_QUEUE. When the write queue is full the function will return 1.

## Improvements
- Implement io_uring_for_each_cqe(&r,head,e){}; + io_uring_cq_advance(&r,count);
- Io_uring wrapper for Kqueue.
