#ifndef TFF_RQUEUE_H_
#define TFF_RQUEUE_H_

#include "common.h"
#include "rqueue_base.h"
#include "rqueue_async_mpx.h"

#include <memory>
#include <utility>

namespace tff {

enum class rqueue_variant {
	sync,
	async,
	async_multiplex
};

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
	std::unique_ptr<rqueue_base> base_;
	
	rqueue(const rqueue&) = delete;
	rqueue& operator=(const rqueue&) = delete;
	
public:
	rqueue(rqueue_variant variant, std::size_t capacity) :
		ring_(capacity)
	{
		if(variant == rqueue_variant::async_multiplex)
			base_.reset(new rqueue_async_mpx(capacity));
		else throw 1;
	}
	
	const ring_type& ring() const { return ring_; }
	ring_type& ring() { return ring_; }
	std::size_t capacity() const { return ring_.capacity(); }

	void request(time_span span) {
		base_->request(span);
	}
	
	void stop() {
		base_->stop();
	}
	
	read_handle read(time_span span) {
		return read_handle(*this, base_->begin_read(span));
	}
	
	write_handle write() {
		return write_handle(*this, base_->begin_write());
	}
};


template<typename Ring>
class rqueue<Ring>::read_handle {
private:
	rqueue& queue_;
	rqueue_base::read_result base_;
	const_wraparound_view_type view_;
	
public:
	read_handle(rqueue& q, const rqueue_base::read_result& base) :
		queue_(q),
		base_(base)
	{
		if(base.flag == rqueue_base::read_result::normal)
			view_ = queue_.ring().const_wraparound_view(base.start_index, base.duration);
	}
	
	~read_handle() {
		if(valid()) queue_.base_->end_read();
	}
	
	read_handle(const read_handle&) = delete;
	read_handle(read_handle&&) = default;
	read_handle& operator=(const read_handle&) = delete;
	read_handle& operator=(read_handle&&) = default;
	
	bool valid() const { return (base_.flag == rqueue_base::read_result::normal); }
	time_unit start_time() const { return base_.start_time; }
	time_unit end_time() const { return base_.start_time + base_.duration; }
	time_unit duration() const { return base_.duration; }
	const const_wraparound_view_type& view() const { return view_; }
};


template<typename Ring>
class rqueue<Ring>::write_handle {
private:
	rqueue& queue_;
	rqueue_base::write_result base_;
	frame_type frame_;
	
	bool committed_;
	
public:
	write_handle(rqueue& q, const rqueue_base::write_result& base) :
		queue_(q),
		base_(base),
		committed_(true)
	{
		if(base.flag == rqueue_base::write_result::normal) {
			frame_ = queue_.ring()[base_.index];
			committed_ = false;
		}
	}
	
	~write_handle() {
		if(valid() && !committed_)
			queue_.base_->end_write(false);
	}
	
	write_handle(const write_handle&) = delete;
	write_handle(write_handle&&) = default;
	write_handle& operator=(const write_handle&) = delete;
	write_handle& operator=(write_handle&&) = default;
	
	bool valid() const { return (base_.flag == rqueue_base::write_result::normal); }
	
	bool has_stopped() const { return (base_.flag == rqueue_base::write_result::stopped); }
	time_unit time() const { return base_.time; }
	const frame_type& frame() const { return frame_; }
	
	void commit() {
		queue_.base_->end_write(true);
		committed_ = true;
	}
};

}

#endif
