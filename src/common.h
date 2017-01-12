#ifndef TFF_RQUEUE_COMMON_H_
#define TFF_RQUEUE_COMMON_H_

#include <cstddef>

namespace tff {

using time_unit = std::ptrdiff_t;

struct time_span {
	time_unit begin;
	time_unit end;
	
	time_span(time_unit b, time_unit e) : begin(b), end(e) { }
};

};

#endif