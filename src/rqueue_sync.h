#ifndef TFF_RQUEUE_SYNC_H_
#define TFF_RQUEUE_SYNC_H_

#include "common.h"
#include "rqueue_base.h"

#include <utility>
#include <atomic>
#include <cassert>

namespace tff {

class rqueue_sync : public rqueue_base {
private:
	const std::size_t capacity_;

	time_unit head_time_;
	std::ptrdiff_t head_;
	std::ptrdiff_t tail_;
	bool full_;

	std::atomic<time_span> request_time_span_;
	unsigned int active_readers_;

	sync_writer_function sync_writer_;

	time_unit available_duration_() const {
		if(full_) return capacity_;
		else if(head_ <= tail_) return tail_ - head_;
		else return capacity_ - (head_ - tail_);
	}

	time_unit tail_time_() const {
		return head_time_ + available_duration_();
	}

	bool write_next_(time_span request_time_span);

public:
	explicit rqueue_sync(std::size_t capacity);
	~rqueue_sync() override;

	std::size_t capacity() const override { return capacity_; }

	void request(time_span) override;
	void stop() override;

	read_result begin_read(time_span) override;
	void end_read() override;

	void set_sync_writer(sync_writer_function) override;
	write_result begin_write() override;
	void end_write(bool commit) override;
};


}

#endif
