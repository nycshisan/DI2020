#pragma once

#include <iostream>
#include <string>
#include <chrono>
#include <thread>

template <class TimeUnit, const char *_TimeUnitStr>
class Timer {
	typedef std::chrono::steady_clock Clock;

	Clock::time_point _start, _end;
	Clock::duration _duration;

public:
	std::string name;

	Timer(const std::string &name) : name(name), _duration(0) {}

	void start() {
		_start = Clock::now();
	}

	void end() {
		_end = Clock::now();
		_duration += std::chrono::duration_cast<Clock::duration>(_end - _start);
	}

	void clear() {
		_duration = Clock::duration(0);
	}

	typename TimeUnit::rep duration() {
		auto convertedDuration = std::chrono::duration_cast<TimeUnit>(_duration);
		return convertedDuration.count();
	}

	void print() {
		auto convertedDuration = std::chrono::duration_cast<TimeUnit>(_duration);
		std::cout << name << ": " << convertedDuration.count() << _TimeUnitStr << std::endl;
	}

	void sleep(typename TimeUnit x) {
		std::this_thread::sleep_for(x);
	}
};

static const char MillisecondsTimeUnitStr[] = "ms";
typedef Timer<std::chrono::milliseconds, MillisecondsTimeUnitStr> MillisecondsTimer;
static const char SecondsTimeUnitStr[] = "s";
typedef Timer<std::chrono::seconds, SecondsTimeUnitStr> SecondsTimer;