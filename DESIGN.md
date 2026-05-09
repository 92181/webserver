# Server Design
The server uses Atomics instead of 1 array containg all io_uring rings accessed through thread_id's.
This seems about as fast and less complex to keep a mental model of.

# Async File Loading
For MacOS the standard is the GCD (Dispatch API), aio_read is vaguely supported. Luckily FreeBSD also supports it through a simple compatibility wrapper.
GCD is a little weird as it occupies a thread, only to use it in a blocking manner, sadly a superior alternative such as io_uring will 'never' be implemented.

# Backpressure Handler
We expose a backpressure handler to the user, for both Kqueue and io_uring.