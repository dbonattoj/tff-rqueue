#include "rqueue_async_mpx.h"
#include <iostream>

namespace tff {

rqueue_async_mpx::rqueue_async_mpx(std::size_t capacity) :
	capacity_(capacity),
	head_time_(0),
	head_(0),
	tail_(0),
	full_(false),
	request_time_span_(0, 0),
	lowest_reader_head_time_(-1) { }


rqueue_async_mpx::~rqueue_async_mpx() { }


void rqueue_async_mpx::request(time_span span) {
	assert(span.end - span.begin <= capacity());
	{
		std::lock_guard<std::mutex> lock(state_mutex_);
		request_time_span_ = span;
	}
	state_cv_.notify_all();
}


void rqueue_async_mpx::stop() {
	request(time_span(-1, -1));
}
	

auto rqueue_async_mpx::begin_read(time_span span) -> read_result {
	std::unique_lock<std::mutex> state_lock(state_mutex_);

retry:
	if(span.begin < request_time_span_.begin || span.end > request_time_span_.end)
		return read_result(read_result::unavailable);
	
	time_unit tail_time = tail_time_();
	
	if(span.begin >= head_time_ && span.end <= tail_time) {
		if(lowest_reader_head_time_ == -1 || lowest_reader_head_time_ > span.begin)
			lowest_reader_head_time_ = span.begin;
		
		reader_mutex_.lock_shared();
		
		return read_result(
			read_result::normal,
			head_ + (span.begin - head_time_),
			span.begin,
			span.end - span.begin
		);
		
	} else {
		state_cv_.wait(state_lock);
		goto retry;
	}
}


void rqueue_async_mpx::end_read() {
	reader_mutex_.unlock_shared();
}
	

auto rqueue_async_mpx::begin_write() -> write_result {
	std::unique_lock<std::mutex> state_lock(state_mutex_);
	
	time_unit tail_time = tail_time_();

retry:
	if(request_time_span_.begin == -1 && request_time_span_.end == -1) {
		return write_result(write_result::stopped);
		
	} else if(request_time_span_.begin >= head_time_ && request_time_span_.begin <= tail_time) {
		if(tail_time >= request_time_span_.end) {
			state_cv_.wait(state_lock);
			goto retry;
			
		} else if(! full_) {
			return write_result(write_result::normal, tail_, tail_time);
			
		} else {
			assert(head_ == tail_);
			std::unique_lock<shared_mutex_type> reader_lock(reader_mutex_, std::defer_lock);
			if(lowest_reader_head_time_ == head_time_) reader_lock.lock();
			lowest_reader_head_time_ = -1;
			head_ = (tail_ + 1) % capacity();
			head_time_++;
			full_ = false;
			return write_result(write_result::normal, tail_, tail_time);
		}
		
	} else {
		std::lock_guard<shared_mutex_type> reader_lock(reader_mutex_);
		head_time_ = request_time_span_.begin;
		tail_ = head_;
		full_ = false;
		lowest_reader_head_time_ = -1;
		return write_result(write_result::normal, head_, head_time_);
	}

}


void rqueue_async_mpx::end_write(bool commit) {
	if(! commit) return;
	{
		std::lock_guard<std::mutex> state_lock(state_mutex_);
		tail_ = (tail_ + 1) % capacity();
		if(tail_ == head_) full_ = true;
		
		if(reader_mutex_.try_lock()) {
			std::lock_guard<shared_mutex_type> reader_lock(reader_mutex_, std::adopt_lock);
			lowest_reader_head_time_ = -1;
		}
	}
	state_cv_.notify_all();
}


}