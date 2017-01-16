#include <catch.hpp>
#include "../src/rqueue_async_mpx.h"
#include "ring.h"
#include "flow.h"

using namespace tff;
using namespace tff::test;

/*
TEST_CASE("async_mpx: basic") {
	sync_node A("A");
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
*/