#ifndef TFF_TESTSUPPORT_RQUEUE_FLOW_H_
#define TFF_TESTSUPPORT_RQUEUE_FLOW_H_


#include "ring.h"
#include "../src/common.h"
#include "../src/rqueue.h"
#include "../src/rqueue_async_mpx.h"

#include <memory>
#include <vector>
#include <thread>
#include <string>
#include <atomic>
#include <iostream>
//#include <pthread.h>

namespace tff { namespace test {

struct frame {
	enum { ok, eof } flag = ok;
	int value = 0;
};

using ring_type = ring<frame>;

using queue_type = rqueue<ring_type>;
using read_handle_type = typename queue_type::read_handle;
using write_handle_type = typename queue_type::write_handle;
using read_handles_array_type = std::vector<std::unique_ptr<read_handle_type>>;


inline const std::string& thread_name(const std::string& new_name) {
	static thread_local std::string name;
	if(! new_name.empty()) {
		name = new_name;
		//pthread_setname_np(pthread_self(), new_name.c_str());
	}
	return name;
}


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
	std::atomic<bool> test_failure {false};
	
	explicit node(const std::string& nm) : name(nm) { }
	
	virtual ~node() = default;

	read_handles_array_type read_predecessors(time_unit t, bool& eof) {
		read_handles_array_type handles;
		handles.reserve(read_connections.size());
		
		for(const connection& conn : read_connections) {
			time_span req_span { t - conn.past_window, t + conn.future_window + 1 };
			if(req_span.begin < 0) req_span.begin = 0;
			
			auto hd = conn.predecessor_node->read(req_span);
			
			if(! hd->valid()) {
				handles.clear();
				return handles;
			}
			if(hd->view()[t - hd->start_time()]->flag == frame::eof) {
				eof = true;
				handles.clear();
				return handles;
			}
			
			time_unit frame_t = hd->start_time();
			for(std::ptrdiff_t off = 0; off < hd->view().size(); ++off, ++frame_t) {
				const frame& frame = *hd->view().at(off);
				if(frame.flag == frame::ok && frame.value != frame_t) test_failure.store(true);
			}
		
			handles.push_back(std::move(hd));
		}
		
		return handles;
	}
	
	void process(const write_handle_type& hd) {
		hd.frame()->flag = frame::ok;
		hd.frame()->value = hd.time();
	}
	
	void request(time_span span) {
		for(const connection& conn : request_connections) {
			time_span req_span(span.begin - conn.past_window, span.end + conn.future_window);
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

	virtual void do_request(time_span) { }
	virtual void do_stop() { }
	virtual std::unique_ptr<read_handle_type> read(time_span) { }
	virtual void end_read() { }
};



class async_node : public node {
private:
	rqueue<ring_type> queue_;
	std::thread thread_;
	
	void thread_main_() {
		thread_name(name);
		for(;;) {
			auto hd = queue_.write();
			if(hd.has_stopped()) break;

			if(eof_time != -1 && hd.time() >= eof_time) {
				hd.frame()->flag = frame::eof;
				hd.commit();
				continue;
			}
			{
				bool eof = false;
				auto read_handles = read_predecessors(hd.time(), eof);
				if(eof) {
					hd.frame()->flag = frame::eof;
					hd.commit();
					continue;
				} else if(read_handles.size() == 0 && read_connections.size() > 0) {
					// tfail
					continue;
				}
				process(hd);
			}
			hd.commit();
		}
	}
	
public:
	async_node(const std::string& name, std::size_t capacity = 15, bool mpx = false) :
		node(name),
		queue_(mpx ? rqueue_variant::async_multiplex : rqueue_variant::async, capacity),
		thread_(&async_node::thread_main_, this) { thread_name(name); }

	~async_node() override {
		assert(! thread_.joinable());
	}

	void do_request(time_span span) override {
		queue_.request(span);
	}

	void do_stop() override {
		queue_.stop();
		thread_.join();
	}

	std::unique_ptr<read_handle_type> read(time_span span) override {
		return std::make_unique<queue_type::read_handle>(queue_.read(span));
	}
};



class sync_node : public node {
private:
	rqueue<ring_type> queue_;
	
	void write_(write_handle_type& hd) {
		assert(! hd.has_stopped());
		if(eof_time != -1 && hd.time() >= eof_time) {
			hd.frame()->flag = frame::eof;
			hd.commit();
			return;
		}
		{
			bool eof = false;
			auto read_handles = read_predecessors(hd.time(), eof);
			if(eof) {
				hd.frame()->flag = frame::eof;
				hd.commit();
				return;
			} else if(read_handles.size() == 0 && read_connections.size() > 0) {
				// tfail
				return;
			}
			process(hd);
		}
		hd.commit();
	}
	
public:
	sync_node(const std::string& name, std::size_t capacity = 15) :
		node(name),
		queue_(rqueue_variant::sync, capacity)
	{
		queue_.set_sync_writer(std::bind(&sync_node::write_, this, std::placeholders::_1));
	}


	void do_request(time_span span) override {
		queue_.request(span);
	}
	
	std::unique_ptr<read_handle_type> read(time_span span) override {
		return std::make_unique<queue_type::read_handle>(queue_.read(span));
	}
};



class sink_node : public node {
public:
	using node::node;

	void process(time_unit t, bool& eof) {
		time_span span{t, t+1};
		request(span);
		read_predecessors(t, eof);
		//std::cout << t << std::endl;
	}
};



}}

#endif
