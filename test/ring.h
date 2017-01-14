#ifndef TFF_TESTSUPPORT_RQUEUE_RING_H_
#define TFF_TESTSUPPORT_RQUEUE_RING_H_

#include <vector>

namespace tff { namespace test {

template<typename T>
class ring {
public:
	using index_type = std::ptrdiff_t;
	using frame_type = T*;
	using const_frame_type = const T*;
	using wraparound_view_type = std::vector<T*>;
	using const_wraparound_view_type = std::vector<const T*>;
	
private:
	std::vector<T> content_;
	
public:
	explicit ring(std::size_t cap) : content_(cap) { }
	
	std::size_t capacity() const { return content_.size(); }
	
	frame_type operator[](index_type i) { return &content_.at(i); }
	const_frame_type operator[](index_type i) const { return &content_.at(i); }
	
	wraparound_view_type wraparound_view(index_type start, std::size_t dur) {
		wraparound_view_type out;
		for(index_type i = start; i < start + dur; ++i)
			out.push_back(content_.data() + (i % capacity()));
		return out;
	}
	
	const_wraparound_view_type const_wraparound_view(index_type start, std::size_t dur) const {
		const_wraparound_view_type out;
		for(index_type i = start; i < start + dur; ++i)
			out.push_back(content_.data() + (i % capacity()));
		return out;
	}
	
	const_wraparound_view_type wraparound_view(index_type start, std::size_t dur) const {
		return const_wraparound_view(start, dur);
	}

};

}}

#endif