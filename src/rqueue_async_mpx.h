#ifndef TFF_RQUEUE_ASYNC_MPX_H_
#define TFF_RQUEUE_ASYNC_MPX_H_

#include "rqueue.h"

#include <utility>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <cassert>

namespace tff {

template<typename Ring>
class rqueue_async_mpx : public rqueue<Ring> {
	using base = rqueue<Ring>;
	
public:
	using typename base::ring_type;
	using typename base::index_type;
	using typename base::frame_type;
	using typename base::const_frame_type;
	using typename base::wraparound_view_type;
	using typename base::const_wraparound_view_type;
	
	class read_handle;
	using write_handle = typename base::write_handle;
	
private:
	#if __cplusplus >= 201500
	using shared_mutex_type = std::shared_mutex;
	#else
	using shared_mutex_type = std::shared_timed_mutex;
	#endif

	time_unit head_time_;
	index_type head_;
	index_type tail_;
	bool full_;
	
	time_span request_time_span_;
	
	time_unit lowest_reader_head_time_;
	
	std::mutex state_mutex_;
	shared_mutex_type reader_mutex_;
	std::condition_variable state_cv_;
	
	time_unit available_duration_() const {
		if(full_) return base::capacity();
		else if(head_ <= tail_) return tail_ - head_;
		else return base::capacity() - (head_ - tail_);
	}
	
	time_unit tail_time_() const {
		return head_time_ + available_duration_();
	}
	
public:
	explicit rqueue_async_mpx(std::size_t capacity) :
		base(capacity),
		head_time_(0),
		head_(0),
		tail_(0),
		full_(false),
		request_time_span_(0, 0),
		lowest_reader_head_time_(-1) { }
		
	~rqueue_async_mpx() override { }
	
	void request(time_span span) override {
		{
			std::lock_guard<std::mutex> lock(state_mutex_);
			request_time_span_ = span;
		}
		state_cv_.notify_all();
	}
	
	void stop() override {
		request(time_span(-1, -1));
	}
	
	
	read_handle read(time_span span) override {
		std::unique_lock<std::mutex> state_lock(state_mutex_);
		
	retry:
		if(span.begin < request_time_span_.begin || span.end > request_time_span_.end)
			return read_handle();
		
		time_unit tail_time = tail_time_();
		
		if(span.begin >= head_time_ && span.end <= tail_time) {
			if(lowest_reader_head_time_ == -1 || lowest_reader_head_time_ > span.begin)
				lowest_reader_head_time_ = span.begin;
			
			index_type start_index = head_ + (span.begin - head_time_);
			std::size_t duration = span.end - span.begin;
			
			return read_handle(
				span.begin,
				base::ring().wraparound_view(start_index, duration),
				std::shared_lock<shared_mutex_type>(reader_mutex_)
			);
			
		} else {
			state_cv_.wait(state_lock);
			goto retry;
		}
	}
	
	
	write_handle write() override {
		std::unique_lock<std::mutex> state_lock(state_mutex_);
		
		time_unit tail_time = tail_time_();
		
	retry:
		if(request_time_span_.begin == -1 && request_time_span_.end == -1) {
			return write_handle();
			
		} else if(request_time_span_.begin >= head_time_ && request_time_span_.begin <= tail_time) {
			if(tail_time >= request_time_span_.end) {
				state_cv_.wait(state_lock);
				goto retry;
				
			} else if(! full_) {
				return write_handle(tail_time, base::ring()[tail_]);
				
			} else {
				assert(head_ == tail_);
				std::unique_lock<shared_mutex_type> reader_lock(reader_mutex_, std::defer_lock);
				if(lowest_reader_head_time_ == head_time_) reader_lock.lock();
				lowest_reader_head_time_ = -1;
				head_ = (tail_ + 1) % base::capacity();
				head_time_++;
				full_ = false;
				return write_handle(tail_time, base::ring()[tail_]);
			}
			
		} else {
			std::lock_guard<shared_mutex_type> reader_lock(reader_mutex_);
			head_time_ = request_time_span_.begin;
			tail_ = head_;
			full_ = false;
			lowest_reader_head_time_ = -1;
			return write_handle(*this, head_time_, base::ring()[head_]);
		}
	}
	
private:
	void end_write_(bool success) override {
		if(! success) return;
		
		{
			std::lock_guard<std::mutex> state_lock(state_mutex_);
			tail_ = (tail_ + 1) % base::capacity();
			if(tail_ == head_) full_ = true;
			
			if(reader_mutex_.try_lock()) {
				std::lock_guard<shared_mutex_type> reader_lock(reader_mutex_, std::adopt_lock);
				lowest_reader_head_time_ = -1;
			}
		}
		state_cv_.notify_all();
	}
};


template<typename Ring>
class rqueue_async_mpx<Ring>::read_handle : public rqueue<Ring>::read_handle {
	using base = rqueue<Ring>::read_handle;
	
public:
	using lock_type = std::shared_lock<shared_mutex_type>;
	
private:
	lock_type lock_;
	
public:
	read_handle() : base() { }
	
	read_handle(time_unit start_t, const const_wraparound_view_type& vw, lock_type&& lock) :
		base(start_t, vw),
		lock(std::move(lock)) { }

	virtual ~read_handle() { }
	
	bool available() const { return available_; }
	time_unit start_time() const { return start_time_; }
	const const_wraparound_view_type& view() const { return view_; }
};


}

#endif
