#ifndef TFF_RQUEUE_H_
#define TFF_RQUEUE_H_

#include "common.h"
#include "rqueue_base.h"
#include "sync.h"
#include "async.h"
#include "rqueue_async_mpx.h"

#include <memory>
#include <functional>
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
	
	using sync_writer_function = std::function<void(write_handle&)>;
	
private:
	ring_type ring_;
	std::unique_ptr<rqueue_base> base_;
	
	rqueue(const rqueue&) = delete;
	rqueue& operator=(const rqueue&) = delete;
	
public:
	rqueue(rqueue_variant variant, std::size_t capacity) :
		ring_(capacity)
	{
		if(variant == rqueue_variant::sync) base_.reset(new rqueue_sync(capacity));
		else if(variant == rqueue_variant::async) base_.reset(new rqueue_async(capacity));
		else if(variant == rqueue_variant::async_multiplex) base_.reset(new rqueue_async_mpx(capacity));
		else throw std::invalid_argument("invalid rqueue variant");
	}
	
	const ring_type& ring() const { return ring_; }
	ring_type& ring() { return ring_; }
	std::size_t capacity() const { return ring_.capacity(); }

	void set_sync_writer(sync_writer_function func) {
		auto base_sync_writer = [this, func](const rqueue_base::write_result& res) -> bool {
			write_handle handle(*this, res, false);
			func(handle);
			return handle.was_committed();
		};
		base_->set_sync_writer(base_sync_writer);
	}
	
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
		return write_handle(*this, base_->begin_write(), true);
	}
};


template<typename Ring>
class rqueue<Ring>::read_handle {
private:
	rqueue* queue_;
	rqueue_base::read_result base_;
	const_wraparound_view_type view_;
	
public:
	read_handle(rqueue& q, const rqueue_base::read_result& base) :
		queue_(&q),
		base_(base)
	{
		if(base.flag == rqueue_base::read_result::normal)
			view_ = queue_->ring().const_wraparound_view(base.start_index, base.duration);
	}
	
	~read_handle() {
		if(queue_ != nullptr && valid()) queue_->base_->end_read();
	}
	
	read_handle(const read_handle&) = delete;
	read_handle& operator=(const read_handle&) = delete;

	read_handle(read_handle&& other) :
		queue_(other.queue_),
		base_(other.base_),
		view_(std::move(other.view_)) { other.queue_ = nullptr; }

	read_handle& operator=(read_handle&& other) {
		using std::swap;
		swap(queue_, other.queue_);
		swap(base_, other.base_);
		swap(view_, other.view_);
		return *this;
	}
	
	bool valid() const { return (base_.flag == rqueue_base::read_result::normal); }
	time_unit start_time() const { return base_.start_time; }
	time_unit end_time() const { return base_.start_time + base_.duration; }
	time_unit duration() const { return base_.duration; }
	const const_wraparound_view_type& view() const { return view_; }
};


template<typename Ring>
class rqueue<Ring>::write_handle {
private:
	rqueue* queue_;
	rqueue_base::write_result base_;
	frame_type frame_;
	
	bool committed_;
	
public:
	write_handle(rqueue& q, const rqueue_base::write_result& base, bool link_to_queue) :
		queue_(link_to_queue ? &q : nullptr),
		base_(base),
		committed_(true)
	{
		if(base.flag == rqueue_base::write_result::normal) {
			frame_ = q.ring()[base_.index];
			committed_ = false;
		}
	}
	
	~write_handle() {
		if(queue_ != nullptr && valid() && !committed_)
			queue_->base_->end_write(false);
	}
	
	write_handle(const write_handle&) = delete;
	write_handle& operator=(const write_handle&) = delete;

	write_handle(write_handle&& other) :
		queue_(other.queue_),
		base_(other.base_),
		frame_(std::move(other.frame_)) { other.queue_ = nullptr; }

	write_handle& operator=(write_handle&& other) {
		using std::swap;
		swap(queue_, other.queue_);
		swap(base_, other.base_);
		swap(frame_, other.frame_);
		return *this;
	}
	
	bool valid() const { return (base_.flag == rqueue_base::write_result::normal); }
	
	bool has_stopped() const { return (base_.flag == rqueue_base::write_result::stopped); }
	time_unit time() const { return base_.time; }
	const frame_type& frame() const { return frame_; }
	
	void commit() {
		if(queue_ != nullptr) queue_->base_->end_write(true);
		committed_ = true;
	}
	
	bool was_committed() const { return committed_; }
	bool linked_to_queue() const { return (queue_ != nullptr); }
};

}

#endif
