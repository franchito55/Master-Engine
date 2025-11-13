#pragma once
#include "Globals.h"
#include <chrono>

class Timer {
	typedef std::chrono::time_point<std::chrono::steady_clock, std::chrono::duration<long long, std::micro>> REP;
public:
	Timer() {
		clock = std::chrono::steady_clock();
	}
	REP now() const {
		return std::chrono::time_point_cast<std::chrono::duration<long long, std::micro>>(clock.now());
	}
	void start() {
		t = now();
	}
	long long stop() {
		return (now() - t).count();
	}
private:
	std::chrono::steady_clock clock;
	REP t;
};