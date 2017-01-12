#ifndef QUEUE_SYNC_H_
#define QUEUE_SYNC_H_

#include <vector>
#include <utility>
#include <atomic>
#include <functional>
#include "utility.h"

// span: [begin, end[


template<typename Frame>
class queue_sync {
public:
	struct read_handle : read_handle_base<Frame> {
		queue_sync* q = nullptr;

		read_handle() { }

		read_handle(bool valid_, const read_view& view_, queue_sync* q_) :
			read_handle_base<Frame>(valid_, view_),
			q(q_) { q->active_readers_++; }

		read_handle(read_handle&& other) :
			read_handle_base<Frame>(other),
			q(other.q) { other.q = nullptr; }

		~read_handle() { if(q) q->active_readers_--; }

		const Frame& frame(std::ptrdiff_t i) { return q->frame(i); }
	};

	using write_view_type = write_view<Frame>;
	using write_callback_function = std::function<bool(const write_view_type&)>;

	friend struct read_handle;

private:
	std::string name_;

	std::vector<Frame> buffer_;

	time_unit head_time_;
	std::ptrdiff_t head_;
	std::ptrdiff_t tail_;
	bool is_full_;

	std::atomic<time_span> request_time_span_;

	write_callback_function write_callback_;

	unsigned active_readers_;

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
		log(full_fmt.c_str(), name_.c_str(), head_, capacity_(), tail_, capacity_(), head_time_, head_time_+available_duration_(), request_time_span_.load().begin, request_time_span_.load().end, args...);
	}


public:
	explicit queue_sync(const std::string& nm, std::size_t length, write_callback_function func) :
		name_(nm),
		buffer_(length),
		head_time_(0),
		head_(0),
		tail_(0),
		is_full_(false),
		request_time_span_(time_span(0, 0)),
		write_callback_(func),
		active_readers_(0) { }

	Frame& frame(std::ptrdiff_t ind) {
		return buffer_[ind % buffer_.size()];
	}


	// request thread
	//(
	void request(time_span span) {
		require(span.end - span.begin <= capacity_());
		request_time_span_.store(span);
	}
	//)
	
	// reader+write thread
	//(
	bool write_next_(time_span request_time_span) {
		bool write_success;

		time_unit tail_time = tail_time_();

		if(request_time_span.begin >= head_time_ && request_time_span.begin <= tail_time) {
			if(tail_time >= request_time_span.end) {
				return false;

			} else if(! is_full_) {
				write_success = write_callback_(write_view_type{false, tail_time, &frame(tail_)});

			} else {
				require(head_ == tail_);
				if(active_readers_ > 0) return false;
				head_ = (tail_ + 1) % capacity_();
				head_time_++;
				is_full_ = false;
				write_success = write_callback_(write_view_type{false, tail_time, &frame(tail_)});
			};

		} else {
			if(active_readers_ > 0) return false;
			head_time_ = request_time_span.begin;
			tail_ = head_;
			is_full_ = false;
			write_success = write_callback_(write_view_type{false, head_time_, &frame(head_)});
		};

		if(write_success) {
			tail_ = (tail_ + 1) % capacity_();
			if(tail_ == head_) is_full_ = true;
			return true;
		} else {
			return false;
		}
	}


	read_handle read(time_span span) {
		time_span request_time_span = request_time_span_.load();

		if(span.begin < request_time_span.begin || span.end > request_time_span.end)
			return read_handle();

		while(!(span.begin >= head_time_ && span.end <= tail_time_())) {
			bool write_success = write_next_(request_time_span);
			if(! write_success) return read_handle();
		}

		read_view vw{span.begin, head_ + (span.begin - head_time_), span.end - span.begin};
		return read_handle(true, vw, this);
	}
	//)
};

#endif

