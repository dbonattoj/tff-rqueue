#include <catch.hpp>
#include "../src/rqueue_async_mpx.h"
#include "flow.h"

using namespace tff;
using namespace tff::test;


TEST_CASE("rqueue async: basic") {
	async_node A("A", 15, false);
	sink_node S("S");

	auto test = [&]() {
		REQUIRE_FALSE(A.test_failure.load());
		REQUIRE_FALSE(S.test_failure.load());
	};
	
	A.prefetch = 10;
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




TEST_CASE("rqueue async: multiplex") {
	async_node M("M", 15, true);
	async_node A("A", 15, false);
	async_node B("B", 15, false);
	sink_node S("S");
	
	auto test = [&]() {
		REQUIRE_FALSE(M.test_failure.load());
		REQUIRE_FALSE(A.test_failure.load());
		REQUIRE_FALSE(B.test_failure.load());
		REQUIRE_FALSE(S.test_failure.load());
	};

	A.prefetch = 10;
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




TEST_CASE("rqueue async: double multiplex") {
	async_node M("N", 15, true);
 	async_node N("M", 15, true);
 	async_node A("A", 15, false);
 	async_node B("B", 15, false);
 	async_node C("C", 15, false);
	sink_node S("S");
	
	auto test = [&]() {
		REQUIRE_FALSE(M.test_failure.load());
		REQUIRE_FALSE(N.test_failure.load());
		REQUIRE_FALSE(A.test_failure.load());
		REQUIRE_FALSE(B.test_failure.load());
		REQUIRE_FALSE(C.test_failure.load());
		REQUIRE_FALSE(S.test_failure.load());
	};

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
