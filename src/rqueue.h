#ifndef TFF_RQUEUE_H_
#define TFF_RQUEUE_H_

#include "common.h"

namespace tff {

template<typename Ring>
class rqueue {
public:
	using ring_type = Ring;
	using index_type = typename ring_type::index_type;
	using frame_type = typename ring_type::frame_type;
	using const_frame_type = typename ring_type::const_frame_type;
	using wraparound_view_type = typename ring_type::wraparound_view_type;
	using const_wraparound_view_type = typename ring_type::const_wraparound_view_type;
	
	class read_handle;
	class write_handle;
	
private:
	ring_type ring_;
	
	virtual void end_write_(bool success) = 0;
	
public:
	explicit rqueue(std::size_t cap) : ring_(cap) { }
		
	virtual ~rqueue() { }
	
	const ring_type& ring() const { return ring_; }
	ring_type& ring() { return ring_; }
	std::size_t capacity() const { return ring_.capacity(); }

	virtual void request(time_span) = 0;
	virtual void stop() = 0;
	
	virtual read_handle read(time_span) = 0;
	
	virtual write_handle write() = 0;
};


template<typename Ring>
class rqueue<Ring>::read_handle {
private:
	bool available_;
	time_unit start_time_;
	const_wraparound_view_type view_;
	
public:
	read_handle() :
		available_(false),
		start_time_(-1),
		view_() { }
	
	read_handle(time_unit start_t, const const_wraparound_view_type& vw) :
		available_(true),
		start_time(start_t),
		view_(vw) { }
	
	read_handle(const read_handle&) = delete;
	read_handle(read_handle&&) = default;
	read_handle& operator=(const read_handle&) = delete;
	read_handle& operator=(read_handle&&) = default;
	
	virtual ~read_handle() { }
	
	bool available() const { return available_; }
	time_unit start_time() const { return start_time_; }
	const const_wraparound_view_type& view() const { return view_; }
};


template<typename Ring>
class rqueue<Ring>::write_handle {
private:
	rqueue* queue_;
	const bool stopped_;
	
	time_unit time_;
	frame_type frame_;
	
	bool committed_;
	
public:
	write_handle() :
		queue_(nullptr),
		stopped_(true),
		time_(-1),
		frame_(),
		committed_(true) { }

	write_handle(rqueue& q, time_unit t, const frame_type& fr) :
		queue_(&q),
		stopped_(false),
		time_(t),
		frame_(fr),
		committed_(false) { }
	
	write_handle(const write_handle&) = delete;
	write_handle(write_handle&&) = default;
	write_handle& operator=(const write_handle&) = delete;
	write_handle& operator=(write_handle&&) = default;

	virtual ~write_handle() {
		if(queue_ != nullptr && !committed_)
			queue_->end_write(false);
	}
	
	bool has_stopped() const { return stopped_; }
	
	void commit() {
		queue_.end_write(true);
		committed_ = true;
	}
	
	time_unit time() const { return time_; }
	const frame_type& frame() const { return frame_; }
};

}

#endif
