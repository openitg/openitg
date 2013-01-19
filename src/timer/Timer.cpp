#include "global.h"
#include "Timer.h"

ProfilingTimer::ProfilingTimer() {
	isRunning = false;
	elapsedSoFarInNS = 0;

#ifdef _WINDOWS
	// This initializes the frequency of the ticks, which does not change.
	LARGE_INTEGER frequencyInfo;
	QueryPerformanceFrequency(&frequencyInfo);
	frequency = frequencyInfo.QuadPart;
#else
	lastStart = 0;
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
	elapsedSoFarInNS += (endTime.QuadPart - lastStart.QuadPart) / frequency * 1000000000;
	lastStart.QuadPart = 0;
#else
	timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	elapsedSoFarInNS += ts.tv_nsec - lastStart;
	lastStart = 0;
#endif // _WINDOWS

	isRunning = false;
}

void ProfilingTimer::Reset() {
	// if running, stop
	isRunning = false;
#ifdef _WINDOWS
	lastStart.QuadPart = 0;
#else
	lastStart = 0;
#endif
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
	LONG curElapsed = elapsedSoFarInNS + ( (curTime.QuadPart - lastStart.QuadPart) / frequency * 1000000000 );
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
		return (double) elapsedSoFarInNS / 1000000000UL;
	}

#ifdef _WINDOWS
	LARGE_INTEGER curTime;
	QueryPerformanceCounter(&curTime);
	// frequency is ticks per second, 1 sec = 1 000 000 000 ns
	// ticks / (tics/sec) * (ns / sec) = ns
	LONG curElapsed = elapsedSoFarInNS + ( (curTime.QuadPart - lastStart.QuadPart) / frequency * 1000000000UL );
#else
	timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	long curElapsed = elapsedSoFarInNS + (ts.tv_nsec - lastStart);
#endif // _WINDOWS
	
	return (double) curElapsed / 1000000000;
}

