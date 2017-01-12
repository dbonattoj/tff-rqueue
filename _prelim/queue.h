#ifndef QUEUE_H_
#define QUEUE_H_

#include <vector>
#include <condition_variable>
#include <shared_mutex>
#include <atomic>
#include <thread>
#include <utility>
#include "utility.h"


template<typename Frame>
class queue {
public:
	struct read_handle : read_handle_base<Frame> {
		std::shared_lock<std::shared_timed_mutex> lock;
		queue* q;

		read_handle() { }

		read_handle(bool valid_, std::shared_lock<std::shared_timed_mutex>&& lock_, const read_view& view_, queue* q_) :
			read_handle_base<Frame>(valid_, view_),
			lock(std::move(lock_)),
			q(q_) { }

		read_handle(read_handle&& other) :
			read_handle_base<Frame>(other),
			lock(std::move(other.lock)),
			q(other.q) { other.q = nullptr; }


		const Frame& frame(std::ptrdiff_t i) { return q->frame(i); }
	};

	using write_view_type = write_view<Frame>;

private:
	std::string name_;

	std::vector<Frame> buffer_;

	time_unit head_time_;
	std::ptrdiff_t head_;
	std::ptrdiff_t tail_;
	bool is_full_;

	time_span request_time_span_;

	time_unit lowest_reader_head_time_;

	std::mutex state_mutex_;
	std::shared_timed_mutex reader_mutex_;
	std::condition_variable state_cv_;

	std::size_t capacity_() const { return buffer_.size(); }

	time_unit available_duration_() const {
		if(is_full_) return capacity_();
		else if(head_ <= tail_) return tail_ - head_;
		else return buffer_.size() - (head_ - tail_);
	}

	time_unit tail_time_() const {
		return head_time_ + available_duration_();
	}

	template<typename... Args>
	void qlog(const std::string& fmt, const Args&... args) {
		std::string full_fmt = "(%s) H: %d/%d,  T: %d/%d,  avail=[%d,%d[, req=[%d,%d[ - " + fmt;
		log(full_fmt.c_str(), name_.c_str(), head_, capacity_(), tail_, capacity_(), head_time_, head_time_+available_duration_(), request_time_span_.begin, request_time_span_.end, args...);
	}


public:
	explicit queue(const std::string& nm, std::size_t length) :
		name_(nm),
		buffer_(length),
		head_time_(0),
		head_(0),
		tail_(0),
		is_full_(false),
		request_time_span_(0, 0),
		lowest_reader_head_time_{-1} { }

	Frame& frame(std::ptrdiff_t ind) {
		return buffer_[ind % buffer_.size()];
	}


	// request thread
	//(
	void request(time_span span) {
		require(span.end - span.begin <= capacity_());
		{
			std::lock_guard<std::mutex> lock(state_mutex_);
			request_time_span_ = span;
		}
		state_cv_.notify_all();
	}
	//)


	// writer thread
	//(
	write_view_type begin_write_next() {
		std::unique_lock<std::mutex> state_lock(state_mutex_);

		time_unit tail_time = tail_time_();

	retry:
		if(request_time_span_.begin == -1 && request_time_span_.end == -1) {
			return write_view_type{true, -1, nullptr};

		} else if(request_time_span_.begin >= head_time_ && request_time_span_.begin <= tail_time) {
			if(tail_time >= request_time_span_.end) {
				//qlog("writer wait");
				state_cv_.wait(state_lock);
				goto retry;

			} else if(! is_full_) {
				//qlog("writer append");
				return write_view_type{false, tail_time, &frame(tail_)};

			} else {
				require(head_ == tail_);
				std::unique_lock<std::shared_timed_mutex> reader_lock(reader_mutex_, std::defer_lock);
				if(lowest_reader_head_time_ == head_time_) {
					reader_lock.lock();
					//qlog("writer append + head move (lock)");
				} else {
					//qlog("writer append + head move");
				}
				lowest_reader_head_time_ = -1;
				head_ = (tail_ + 1) % capacity_();
				head_time_++;
				is_full_ = false;
				return write_view_type{false, tail_time, &frame(tail_)};
			};

		} else {
			//qlog("writer jump");
			std::lock_guard<std::shared_timed_mutex> reader_lock(reader_mutex_);
			head_time_ = request_time_span_.begin;
			tail_ = head_;
			is_full_ = false;
			lowest_reader_head_time_ = -1;
			return write_view_type{false, head_time_, &frame(head_)};
		};
	}

	void end_write() {
		{
			std::lock_guard<std::mutex> state_lock(state_mutex_);
			tail_ = (tail_ + 1) % capacity_();
			if(tail_ == head_) is_full_ = true;

			if(reader_mutex_.try_lock()) {
				//qlog("lowest head reset");
				std::lock_guard<std::shared_timed_mutex> reader_lock(reader_mutex_, std::adopt_lock);
				lowest_reader_head_time_ = -1;
			}
		}
		state_cv_.notify_all();
	}


	void cancel_write() {

	}
	
	
	void end_write_tfail() {
		std::this_thread::yield();
	}
	//)
	
	
	// reader threads
	//(
	read_handle read(time_span span) {
		std::unique_lock<std::mutex> state_lock(state_mutex_);

	retry:
		if(span.begin < request_time_span_.begin || span.end > request_time_span_.end)
			return read_handle();

		time_unit tail_time = tail_time_();

		if(span.begin >= head_time_ && span.end <= tail_time) {
			if(lowest_reader_head_time_	== -1 || lowest_reader_head_time_ > span.begin)
				lowest_reader_head_time_ = span.begin;

			read_view vw{span.begin, head_ + (span.begin - head_time_), span.end - span.begin};
			return read_handle(true, std::shared_lock<std::shared_timed_mutex>(reader_mutex_), vw, this);

		} else {
			state_cv_.wait(state_lock);
			goto retry;
		}
	}
	//)
};

#endif

