#ifndef TFF_TESTSUPPORT_RQUEUE_RING_H_
#define TFF_TESTSUPPORT_RQUEUE_RING_H_

#include <vector>

namespace tff { namespace test {
	
class ring {
public:
	using frame_type = int&;
	using index_type = std::ptrdiff_t;
	using const_frame_type = const int&;
	using wraparound_view_type = std::vector<int*>;
	using const_wraparound_view_type = std::vector<const int*>;
	
private:
	std::vector<int> content_;
	
public:
	std::size_t capacity() const { return content_.size(); }
	
	const_frame_type operator[](index_type i) const { return content_[i]; }
	frame_type operator[](index_type i) { return content_[i]; }
	
	wraparound_view_type wraparound_view(index_type start, std::size_t dur) {
		wraparound_view_type out;
		for(index_type i = start; i < start + dur; ++i)
			out.push_back(content_.data() + (i % capacity()));
		return out;
	}
	
	const_wraparound_view_type wraparound_view(index_type start, std::size_t dur) const {
		const_wraparound_view_type out;
		for(index_type i = start; i < start + dur; ++i)
			out.push_back(content_.data() + (i % capacity()));
		return out;
	}
	
	void initialize_frame(frame_type fr) {
		fr = 0;
	}
};

};}

#endif