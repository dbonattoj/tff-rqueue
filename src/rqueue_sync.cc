#include "rqueue_sync.h"

namespace tff {

rqueue_sync::rqueue_sync(std::size_t capacity) :
	capacity_(capacity),
	head_time_(0),
	head_(0),
	tail_(0),
	full_(false),
	request_time_span(time_span(0, 0)),
	active_readers_(0) { }


rqueue_sync::~rqueue_sync() {}


bool rqueue_sync::write_next_(time_span request_time_span) {
	time_unit tail_time = tail_time_();

	write_result write(write_result::normal, -1, -1);

	if(request_time_span.begin >= head_time_ && request_time_span.begin <= tail_time) {
		if(tail_time >= request_time_end) {
			return false;

		} else if(! full_) {

		}

	} else {

	}
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


auto rqueue_sync::begin_write() -> rqueue_sync {
	throw std::logic_error("use write callback");
}


void rqueue_sync::end_write(bool) {
	throw std::logic_error("use write callback");
}


}
