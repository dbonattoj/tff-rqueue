#include <catch.hpp>
#include "../src/rqueue_async_mpx.h"
#include "ring.h"
#include "flow.h"

using namespace tff;
using namespace tff::test;


TEST_CASE("rqueue sync: basic") {
	sync_node A("A", 15);
	sink_node S("S");

	auto test = [&]() {
		REQUIRE_FALSE(A.test_failure.load());
		REQUIRE_FALSE(S.test_failure.load());
	};
	
	A.eof_time = 100;

	S.request_connections.push_back({&A, 1, 1});

	S.read_connections.push_back({&A, 1, 1});

	thread_name("S");
	for(int i = 0; i < 1; ++i) {
		time_unit t = 0;
		bool eof = false;
		while(!eof) { S.process(t++, eof); test(); }
		
		t = 0 - 2;
		eof = false;
		while(!eof) { S.process(t += 2, eof); test(); }
	}

	S.stop();
	test();
}




TEST_CASE("rqueue sync: multiplex") {
	sync_node M("M", 15);
	sync_node A("A", 15);
	sync_node B("B", 15);
	sink_node S("S");
	
	auto test = [&]() {
		REQUIRE_FALSE(M.test_failure.load());
		REQUIRE_FALSE(A.test_failure.load());
		REQUIRE_FALSE(B.test_failure.load());
		REQUIRE_FALSE(S.test_failure.load());
	};

	M.eof_time = 100;

	S.request_connections.push_back({&M, 0, 0});
	S.request_connections.push_back({&A, 0, 0});
	S.request_connections.push_back({&B, 0, 0});

	S.read_connections.push_back({&A, 0, 0});
	S.read_connections.push_back({&B, 0, 0});
	A.read_connections.push_back({&M, 0, 0});
	B.read_connections.push_back({&M, 0, 0});

	thread_name("S");

	for(int i = 0; i < 1; ++i) {
		time_unit t = 0;
		bool eof = false;
		while(!eof) { S.process(t++, eof); test(); }
		
		t = 0 - 2;
		eof = false;
		while(!eof) { S.process(t += 2, eof); test(); }
	}

	S.stop();
	test();
}




TEST_CASE("rqueue sync: double multiplex") {
	sync_node M("N", 15);
 	sync_node N("M", 15);
 	sync_node A("A", 15);
 	sync_node B("B", 15);
 	sync_node C("C", 15);
	sink_node S("S");
	
	auto test = [&]() {
		REQUIRE_FALSE(M.test_failure.load());
		REQUIRE_FALSE(N.test_failure.load());
		REQUIRE_FALSE(A.test_failure.load());
		REQUIRE_FALSE(B.test_failure.load());
		REQUIRE_FALSE(C.test_failure.load());
		REQUIRE_FALSE(S.test_failure.load());
	};

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
	for(int i = 0; i < 100; ++i) {
		time_unit t = 0;
		bool eof = false;
		while(!eof) { S.process(t++, eof); test(); }
		t = 0 - 2;
		eof = false;
		while(!eof) { S.process(t += 2, eof); test(); }
	}

	S.stop();
	test();
}
