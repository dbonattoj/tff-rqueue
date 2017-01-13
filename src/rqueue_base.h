#ifndef TFF_RQUEUE_BASE_H_
#define TFF_RQUEUE_BASE_H_

namespace tff {

class rqueue_base {
public:
	struct write_result {
		enum flag_type { normal, stopped };

		flag_type flag;
		std::ptrdiff_t index;
		time_unit time;
		
		explicit write_result(flag_type fg, std::ptrdiff_t idx = -1, time_unit t = -1) :
			flag(fg),
			index(idx),
			time(t) { }
	};
	
	struct read_result {
		enum flag_type { normal, unavailable };
		
		flag_type flag;
		std::ptrdiff_t start_index;
		time_unit start_time;
		std::size_t duration;

		explicit read_result(flag_type fg, std::ptrdiff_t start_idx = -1, time_unit start_t = -1, std::size_t dur = 0) :
			flag(fg),
			start_index(start_idx),
			start_time(start_t),
			duration(dur) { }
	};
	
public:
	virtual ~rqueue_base() { }
	
	virtual std::size_t capacity() const = 0;
	
	virtual void request(time_span) = 0;
	virtual void stop() = 0;
	
	virtual read_result begin_read(time_span) = 0;
	virtual void end_read() = 0;
	
	virtual write_result begin_write() = 0;
	virtual void end_write(bool commit) = 0;
};


}

#endif
