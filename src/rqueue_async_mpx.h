#ifndef TFF_RQUEUE_ASYNC_MPX_H_
#define TFF_RQUEUE_ASYNC_MPX_H_

#include "common.h"
#include "rqueue_base.h"

#include <utility>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <cassert>

namespace tff {

class rqueue_async_mpx : public rqueue_base {
private:
	#if __cplusplus >= 201500
	using shared_mutex_type = std::shared_mutex;
	#else
	using shared_mutex_type = std::shared_timed_mutex;
	#endif

	const std::size_t capacity_;

	time_unit head_time_;
	std::ptrdiff_t head_;
	std::ptrdiff_t tail_;
	bool full_;
	
	time_span request_time_span_;
	
	time_unit lowest_reader_head_time_;
	
	std::mutex state_mutex_;
	shared_mutex_type reader_mutex_;
	std::condition_variable state_cv_;

	time_unit available_duration_() const {
		if(full_) return capacity_;
		else if(head_ <= tail_) return tail_ - head_;
		else return capacity_ - (head_ - tail_);
	}
	
	time_unit tail_time_() const {
		return head_time_ + available_duration_();
	}
	
public:
	explicit rqueue_async_mpx(std::size_t capacity);
	~rqueue_async_mpx() override;
	
	std::size_t capacity() const override { return capacity_; }
	
	void request(time_span) override;
	void stop() override;
	
	read_result begin_read(time_span) override;
	void end_read() override;

	write_result begin_write() override;
	void end_write(bool commit) override;
};


}

#endif
