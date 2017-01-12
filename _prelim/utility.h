#ifndef UTILITY_H_
#define UTILITY_H_

#include <string>

void require(bool condition, const char* msg = "", ...);

const std::string& thread_name(const std::string& new_name = "");

void log(const char* fmt, ...);

void sleep_ms(unsigned ms);

using time_unit = std::ptrdiff_t;
struct time_span {
	time_unit begin;
	time_unit end;

	time_span() { }

	time_span(time_unit b, time_unit e) :
			begin(b), end(e) { }

	bool contains(time_unit t) const {
		return (t >= begin) && (t < end);
	}

	bool contains(time_span span) const {
		return (span.begin >= begin) && (span.end <= end);
	}
};


struct read_view {
	time_unit begin_time;
	std::ptrdiff_t begin_index;
	std::ptrdiff_t length;
};

template<typename Frame>
struct read_handle_base {
	bool valid;
	read_view view;

	virtual ~read_handle_base() { }

	read_handle_base() : valid(false) { }
	read_handle_base(bool val, const read_view& vw) : valid(val), view(vw) { }
	read_handle_base(const read_handle_base&) = default;

	virtual const Frame& frame(std::ptrdiff_t i) = 0;
};


template<typename Frame>
struct write_view {
	bool should_stop;
	time_unit time;
	Frame* frame;
};



#endif
