#include "rqueue_sync.h"

namespace tff {

rqueue_sync::rqueue_sync(std::size_t capacity) :
	capacity_(capacity),
	head_time_(0),
	head_(0),
	tail_(0),
	full_(false),
	request_time_span_{time_span(0, 0)},
	active_readers_(0) { }


rqueue_sync::~rqueue_sync() {}


bool rqueue_sync::write_next_(time_span request_time_span) {
	if(request_time_span.begin >= head_time_ && request_time_span.begin <= tail_time_()) {
		if(tail_time_() >= request_time_span.end) {
			return false;
			
		} else if(full_) {
			assert(head_ == tail_);
			if(active_readers_ > 0) return false;
			head_ = (tail_ + 1) % capacity_;
			head_time_++;
			full_ = false;
		}

	} else {
		if(active_readers_ > 0) return false;
		head_time_ = request_time_span.begin;
		tail_ = head_;
		full_ = false;
	}
	
	write_result write_res(write_result::normal, tail_, tail_time_());
	bool write_success = sync_writer_(write_res);
	
	if(write_success) {
		tail_ = (tail_ + 1) % capacity();
		if(tail_ == head_) full_ = true;
	}
	
	return write_success;
}


void rqueue_sync::request(time_span span) {
	assert(span.end - span.begin <= capacity_);
	request_time_span_.store(span);
}

void rqueue_sync::stop() {}


auto rqueue_sync::begin_read(time_span span) -> read_result {
	time_span request_time_span = request_time_span_.load();

	if(span.begin < request_time_span.begin || span.end <= tail_time_())
		return read_result(read_result::unavailable);

	while(!(span.begin >= head_time_ && span.end <= tail_time_())) {
		bool write_success = write_next_(request_time_span);
		if(! write_success) return read_result(read_result::unavailable);
	}

	active_readers_++;

	return read_result(
		read_result::normal,
		head_ + (span.begin - head_time_),
		span.begin,
		span.end - span.begin
	);
}


void rqueue_sync::end_read() {
	active_readers_--;
}


void rqueue_sync::set_sync_writer(sync_writer_function func) {
	sync_writer_ = func;
}


auto rqueue_sync::begin_write() -> write_result {
	throw std::logic_error("use write callback");
}


void rqueue_sync::end_write(bool) {
	throw std::logic_error("use write callback");
}


}
