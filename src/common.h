#ifndef TFF_RQUEUE_COMMON_H_
#define TFF_RQUEUE_COMMON_H_

#ifdef TFF_RQUEUE_STANDALONE

#include <cstddef>

namespace tff {

using time_unit = std::ptrdiff_t;

struct time_span {
	time_unit begin;
	time_unit end;
	
	time_span() = default;
	time_span(time_unit b, time_unit e) : begin(b), end(e) { }
};

};

#else

#ifndef TFF_RQUEUE_HAS_DEPENDENCIES
#error TFF_RQUEUE_HAS_DEPENDENCIES not defined before including tff-rqueue
#endif

#endif

#endif