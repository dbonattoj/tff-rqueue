# TFF rqueue
**Request/Ring queue**, base for timed queueing system with 3 kinds of
threads. Manages reading and writing into a _circular buffer_. Does not
buffer data by itself, but instead handles thread synchronization.
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
   into buffer when asked.

   May be on different thread than reader(s). In that case it will _prefetch_
   sequential frames in advance, before they are read.



