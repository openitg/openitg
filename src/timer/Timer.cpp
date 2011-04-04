#include "Timer.h"

ProfilingTimer::ProfilingTimer() {
	elapsedSoFarInNS = 0;
	lastStart = 0;
	isRunning = false;
#ifdef _WINDOWS
	// This initializes the frequency of the ticks, which does not change.
	QueryPerformanceFrequency(&frequency)
#endif // _WINDOWS
}

ProfilingTimer::~ProfilingTimer() {
}

void ProfilingTimer::Start() {
	if (isRunning) {
		// Oops...already running.  Do nothing I guess.  Error maybe?
		return;
	}
#ifdef _WINDOWS
	QueryPerformanceCounter(&lastStart);
#else
	timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	lastStart = ts.tv_nsec;
#endif // _WINDOWS
}

void ProfilingTimer::Stop() {
	if (!isRunning) {
		// Oops...not running.  Do nothing I guess.  Error maybe?
		return;
	}
#ifdef _WINDOWS
	LARGE_INTEGER endTime;
	QueryPerformanceCounter(&endTime);
	// frequency is ticks per second, 1 sec = 1 000 000 000 ns
	// ticks / (tics/sec) * (ns / sec) = ns
	elapsedSoFarInNS += (endTime - lastStart) / frequency * 1000000000;
#else
	timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	elapsedSoFarInNS += ts.tv_nsec - lastStart;
#endif // _WINDOWS

	lastStart = 0;
	isRunning = false;
}

void ProfilingTimer::Reset() {
	// if running, stop
	isRunning = 0;
	lastStart = 0;
}

double ProfilingTimer::GetElapsedSoFarInNS() const {
	if (!isRunning) {
		// then we just return the current total
		return elapsedSoFarInNS;
	}

#ifdef _WINDOWS
	LARGE_INTEGER curTime;
	QueryPerformanceCounter(&curTime);
	// frequency is ticks per second, 1 sec = 1 000 000 000 ns
	// ticks / (tics/sec) * (ns / sec) = ns
	LARGE_INTEGER curElapsed = elapsedSoFarInNS + ( (curTime - lastStart) / frequency * 1000000000 );
#else
	timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	long curElapsed = elapsedSoFarInNS + (ts.tv_nsec - lastStart);
#endif // _WINDOWS
	
	return (double) curElapsed;
}

double ProfilingTimer::GetElapsedSoFarInS() const {
	if (!isRunning) {
		// then we just return the current total
		return (double) elapsedSoFarInNS / 1000000000;
	}

#ifdef _WINDOWS
	LARGE_INTEGER curTime;
	QueryPerformanceCounter(&curTime);
	// frequency is ticks per second, 1 sec = 1 000 000 000 ns
	// ticks / (tics/sec) * (ns / sec) = ns
	LARGE_INTEGER curElapsed = elapsedSoFarInNS + ( (curTime - lastStart) / frequency * 1000000000 );
#else
	timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	long curElapsed = elapsedSoFarInNS + (ts.tv_nsec - lastStart);
#endif // _WINDOWS
	
	return (double) curElapsed / 1000000000;
}

