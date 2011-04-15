/*
 * ProfilingTimer class to track timing for profiling information
 *
 * This is a high resolution timer which should have sub-millisecond resolution
 * on both Linux and Windows.  As of this writing, of course, the windows part
 * is completely untested.  Yeah, someone should get on that shit.
 *
 * Partially inspired by:
 * http://www.songho.ca/misc/timer/timer.html
 * http://tdistler.com/2010/06/27/high-performance-timing-on-linux-windows
 *
 * -cmyers
 */

#ifndef _TIMER_H
#define _TIMER_H

#ifdef _WINDOWS
#include <windows.h>
#else
#include <time.h>
#endif

class ProfilingTimer {

	public:
		ProfilingTimer();
		~ProfilingTimer();

//                ProfilingTimer &operator=(ProfilingTimer *t);
                //const ProfilingTimer &operator=(const ProfilingTimer &t) const  { return t; }

		void Start(); // start the clock
		void Stop(); // pause the clock
		void Reset(); // reset the clock to zero

		double GetElapsedSoFarInS() const; // get the total elapsed so far (works while stopped or going), in seconds
		double GetElapsedSoFarInNS() const; // get the total elapsed so far (works while stopped or going), in nanoseconds

	private:

		bool isRunning;

#ifdef _WINDOWS
		LONGLONG frequency;
		LONG elapsedSoFarInNS;
		LARGE_INTEGER lastStart;
#else // UNIX platforms
		long elapsedSoFarInNS;
		long lastStart; // in nanosec
#endif // _WINDOWS
};

#endif // _TIMER_H
