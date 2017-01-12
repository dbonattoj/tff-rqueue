#include "utility.h"

#include <cstdarg>
#include <mutex>
#include <cstdio>
#include <thread>
#include <chrono>
#include <pthread.h>

static std::mutex output_mutex_;

void require(bool condition, const char* msg, ...) {
	if(! condition) {
		std::lock_guard<std::mutex> lock(output_mutex_);
		va_list args;
		va_start(args, msg);
		std::printf("condition violated: ");
		std::vprintf(msg, args);
		std::putchar('\n');
		va_end(args);
		std::terminate();
	}
}

const std::string& thread_name(const std::string& new_name) {
	static thread_local std::string name;
	if(! new_name.empty()) {
		name = new_name;
		pthread_setname_np(pthread_self(), new_name.c_str());
	}
	return name;
}

void log(const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	std::lock_guard<std::mutex> lock(output_mutex_);
	std::printf("%s: ", thread_name().c_str());
	std::vprintf(fmt, args);
	std::putchar('\n');
	va_end(args);
}

void sleep_ms(unsigned ms) {
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
