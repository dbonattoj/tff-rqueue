# TFF rqueue
**Request/Ring queue**, base for timed queueing system with 3 kinds of
threads. Manages reading and writing into a _circular buffer_. Handles
thread synchronization only. The ring buffer is implemented externally.
Formally verified using PROMELA/Spin model.

Data consists of _frames_, each having sequential _time indices_. The
queue is accessed in three ways:

 * **Requester**: Sends non-blocking request for given _time span_ to
   queue. Can also stop the queue.
   Requests must come from one single thread.

 * **Reader**: _Acquires_ a given _time span_ from the queue. When that span
   is not within the _requested time span_, fails. ("Transitional failure")
   Otherwise, blocks until it becomes available, and then allows reader to
   access that span in buffer. After it is done, the reader must _release_
   the span.

   There may be one or multiple readers, possibly on different threads.
   May or may not be same as requester thread.

 * **Writer**: Controlled by queue, _writes_ frame with given time index
   into buffer when asked. Always writer one frame at a time.

   May be on different thread than reader(s). In that case it will _prefetch_
   sequential frames in advance, before they are read.


Polymorphic class with 3 variants:

 * `rqueue_sync`: Writer and reader(s) are one same thread. Can have multiple
   readers (sequential). Writer gets invoked synchronously when needed through
   callback.

   Requester can be on a different thread or on same thread.

 * `rqueue_async`: Writer and reader on different threads. One reader only.
   Reader waits for frames to become available. Writer is invoked asynchrously,
   and is running and waiting on separate thread.

   Requester may be on reader thread, or on a third thread.

 * `rqueue_async_mpx`: Like `rqueue_async`, but can have multiple concurrent
   readers, one different threads or on same threads.

