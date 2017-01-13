#ifndef FLOW_H_
#define FLOW_H_

#include <vector>
#include <thread>
#include "utility.h"
#include "queue.h"
#include "queue_sync.h"

struct frame {
	enum { normal, error, eof } type;
	int value;
};


using queue_type = queue<frame>;
using queue_sync_type = queue_sync<frame>;
using read_handle_type = read_handle_base<frame>;
using read_handles_array_type = std::vector<std::unique_ptr<read_handle_type>>;
using write_view_type = write_view<frame>;


class node {
public:
	struct connection {
		node* predecessor_node;
		time_unit past_window;
		time_unit future_window;
	};
	
	std::string name;
	std::vector<connection> request_connections;
	std::vector<connection> read_connections;

	time_unit prefetch = 0;
	time_unit eof_time = -1;
	
	explicit node(const std::string& nm) : name(nm) { }
	
	virtual ~node() = default;
	
	template<typename... Args>
	void nlog(const std::string& fmt, const Args&... args) {
		std::string full_fmt = "(%s) " + fmt;
		log(full_fmt.c_str(), name.c_str(), args...);
	}

	read_handles_array_type read_predecessors(time_unit t, bool& eof) {
		read_handles_array_type handles;
		handles.reserve(read_connections.size());
		
		for(const connection& conn : read_connections) {
			time_span req_span { t - conn.past_window, t + conn.future_window + 1 };
			if(req_span.begin < 0) req_span.begin = 0;
			
			auto hd = conn.predecessor_node->read(req_span);
			
			if(! hd->valid) {
				// tfail
				//require(false, "tfail");
				handles.clear();
				return handles;
			}
			if(hd->frame(hd->view.begin_index).type == frame::eof) {
				eof = true;
				handles.clear();
				return handles;
			}
			
			time_unit frame_t = hd->view.begin_time;
			for(std::ptrdiff_t off = 0; off < hd->view.length; ++off, ++frame_t) {
				const frame& frame = hd->frame(hd->view.begin_index + off);
				require(frame.type != frame::normal || frame.value == frame_t,
						"incorrect frame value (got %d, type %d, expected %d)", frame.value, frame.type, frame_t);
			}
		
			handles.push_back(std::move(hd));			
		}
		
		return handles;
	}
	
	void process(const write_view_type& vw) {
		//nlog("process %d...", vw.time);
		vw.frame->type = frame::normal;
		vw.frame->value = vw.time;
	}
	
	void request(time_span span) {
		for(const connection& conn : request_connections) {
			time_span req_span { span.begin - conn.past_window, span.end + conn.future_window };
			if(req_span.begin < 0) req_span.begin = 0;
			conn.predecessor_node->request(req_span);
		}
		span.end += prefetch;
		this->do_request(span);
	}

	void stop() {
		for(const connection& conn : request_connections) {
			conn.predecessor_node->stop();
		}
		this->do_stop();
	}

	virtual void do_request(time_span span) { }
	virtual void do_stop() { }
	virtual std::unique_ptr<read_handle_type> read(time_span) { }
	virtual void end_read() { }
};


class async_node : public node {
private:
	queue_type queue_;
	std::thread thread_;
	
	void thread_main_() {
		thread_name(name);
		for(;;) {
			auto vw = queue_.begin_write_next();
			if(vw.should_stop) break;

			if(vw.frame == nullptr) continue;
			if(eof_time != -1 && vw.time >= eof_time) {
				vw.frame->type = frame::eof;
				queue_.end_write();
				//nlog("frame %d (eof)", vw.time);
				continue;
			}
			{
				bool eof = false;
				auto read_handles = read_predecessors(vw.time, eof);
				if(eof) {
					vw.frame->type = frame::eof;
					queue_.end_write();
					//nlog("frame %d (eof from prec)", vw.time);
					continue;
				} else if(read_handles.size() == 0 && read_connections.size() > 0) {
					queue_.end_write_tfail();
					continue;
				}
				process(vw);
			}
			queue_.end_write();
		}
	}
	
public:
	async_node(const std::string& name) :
		node(name),
		queue_(name, 15),
		thread_(&async_node::thread_main_, this) { thread_name(name); }

	~async_node() override {
		require(! thread_.joinable());
	}

	void do_request(time_span span) override {
		queue_.request(span);
	}

	void do_stop() override {
		queue_.request(time_span(-1, -1));
		thread_.join();
	}

	std::unique_ptr<read_handle_type> read(time_span span) override {
		return std::make_unique<queue_type::read_handle>(queue_.read(span));
	}
};


class sync_node : public node {
private:
	queue_sync_type queue_;

	bool write_(const write_view_type& vw) {
		if(eof_time != -1 && vw.time >= eof_time) {
			vw.frame->type = frame::eof;
			return true;
		}
		{
			bool eof = false;
			auto read_handles = read_predecessors(vw.time, eof);
			if(eof) {
				vw.frame->type = frame::eof;
				return true;

			} else if(read_handles.size() == 0 && read_connections.size() > 0) {
				return false;
			}
			process(vw);
			return true;
		}
	}

public:
	sync_node(const std::string& name) :
			node(name),
			queue_(name, 15, std::bind(&sync_node::write_, this, std::placeholders::_1)) { }

	void do_request(time_span span) override {
		queue_.request(span);
	}

	void do_stop() override { }

	std::unique_ptr<read_handle_type> read(time_span span) override {
		return std::make_unique<queue_sync_type::read_handle>(queue_.read(span));
	}

};


class sink_node : public node {
public:
	using node::node;

	void process(time_unit t, bool& eof) {
		time_span span{t, t+1};
		request(span);
		read_predecessors(t, eof);
		//sleep_ms(10);
		nlog("sink got %d %s", t, eof ? "EOF" : "");
	}
};

#endif
