#ifndef TFF_RQUEUE_ASYNC_MPX_H_
#define TFF_RQUEUE_ASYNC_MPX_H_

#include "common.h"
#include "rqueue.h"
#include <condition_variable>
#include <shared_mutex>
#include <atomic>


namespace tff {

class rqueue_async_mpx {
private:
	const std::size_t capacity_;
	time_unit head_time_;
	std::ptrdiff_t head_, tail_;
	bool is_full_;
	
	time_span request_time_span_;
	
	std::mutex state_mutex_;
	std::shared_timed_mutex reader_mutex_;
	std::condition_variable state_cv_;

public:
	void request(time_span);
	
	
};
	
};

#endif