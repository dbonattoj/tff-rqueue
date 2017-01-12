#include <iostream>
#include "queue.h"
#include "flow.h"

void double_mpx() {
	async_node M("N"), N("M"), A("A"), B("B"), C("C");
	sink_node S("S");

	M.prefetch = 3;
	N.prefetch = 3;
	A.prefetch = 3;
	B.prefetch = 3;
	C.prefetch = 3;
	N.eof_time = 100;

	S.request_connections.push_back({&N, 1, 0});
	S.request_connections.push_back({&M, 1, 0});
	S.request_connections.push_back({&A, 1, 0});
	S.request_connections.push_back({&B, 0, 0});
	B.request_connections.push_back({&C, 0, 0});

	S.read_connections.push_back({&A, 1, 0});
	S.read_connections.push_back({&B, 0, 0});
	A.read_connections.push_back({&M, 0, 0});
	B.read_connections.push_back({&M, 0, 0});
	B.read_connections.push_back({&C, 0, 0});
	M.read_connections.push_back({&N, 0, 0});
	C.read_connections.push_back({&N, 0, 0});

	thread_name("S");
	for(int i = 0; 1||i < 100; ++i) {
		time_unit t = 0;
		bool eof = false;
		while(!eof) S.process(t++, eof);
		t = 0 - 2;
		eof = false;
		while(!eof) S.process(t += 2, eof);
		log("\n\nREACHED EOF\n\n\n\n");
	}

	S.stop();
}

void mpx_join() {
	async_node M("M");
	sync_node A("A");
	async_node B("B");
	sink_node S("S");
	
	//A.prefetch = 10;
	M.prefetch = 10;
	B.prefetch = 10;
	M.eof_time = 100;
	
	S.request_connections.push_back({&M, 0, 0});
	S.request_connections.push_back({&A, 0, 0});
	S.request_connections.push_back({&B, 0, 0});

	S.read_connections.push_back({&A, 0, 0});
	S.read_connections.push_back({&B, 0, 0});
	A.read_connections.push_back({&M, 0, 0});
	B.read_connections.push_back({&M, 0, 0});
		
	thread_name("S");

	for(int i = 0; 1||i < 1; ++i) {
		time_unit t = 0;
		bool eof = false;
		while(!eof) S.process(t++, eof);
		t = 0 - 2;
		eof = false;
		while(!eof) S.process(t += 2, eof);
		log("\n\nREACHED EOF\n\n\n\n");
	}

	S.stop();
}

void basic() {
	sync_node A("A");
	sink_node S("S");
	
	//A.prefetch = 10;
	A.eof_time = 100;
	
	S.request_connections.push_back({&A, 1, 1});
	
	S.read_connections.push_back({&A, 1, 1});
		
	thread_name("S");
	time_unit t = 0;
	bool eof = false;
	while(!eof) S.process(t++, eof);

	S.stop();
}


int main() {
	double_mpx();
	//mpx_join();
	//basic();
	return 0;
}
