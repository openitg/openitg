#ifndef DEBUG_TIMER_H
#define DEBUG_TIMER_H

/* This is a simple struct, written with the intention of
 * testing I/O speed. It can be used for other purposes, though.
 * It's entirely inlined, so this should be very fast.
 *
 * StartUpdate() and EndUpdate() are to be called at the start
 * and end of a function you want to time, respectively.
 *
 */

#include "RageLog.h"
#include "RageTimer.h"
#include "RageUtil.h"

struct DebugTimer
{
	CString m_sName;

	// automatically Report() once the interval is matched
	bool m_bAutoReport;
	int m_iReportInterval;

	// amount of updates, total time for all of them
	int m_iUpdates;
	double m_fUpdateTime;

	// any values over this are discarded
	float m_fTolerance;

	RageTimer m_Timer;

	DebugTimer();

	void StartUpdate();
	void EndUpdate();

	// default reporter if m_bAutoReport is enabled
	void Report();
	void Reset();

	/* aliases for simple, but useful, functions */
	inline int GetUpdateRate()	{ return (1.0/m_fUpdateTime); }
	inline float GetUpdateTime()	{ return (m_fUpdateTime/m_iUpdates); }
	inline bool TimeToReport()	{ return (m_iUpdates >= m_iReportInterval); }

	/* more user-friendly assignment aliases */
	inline void AutoReport( bool bAuto )		{ m_bAutoReport = bAuto; } 
	inline void SetName( const CString &sName )	{ m_sName = sName; }
	inline void SetInterval( const int &iVal )	{ m_iReportInterval = iVal; }
	inline void SetTolerance( const float &fTol )	{ m_fTolerance = fTol; }
};

inline DebugTimer::DebugTimer()
{
	m_sName = "DebugTimer";
	m_bAutoReport = true;
	m_iReportInterval = 1000;
	m_fTolerance = 0.1f;

	Reset();
}

inline void DebugTimer::Report()
{
	LOG->Info( "%s: %i updates in %f seconds (average %f per update, %i updates per second)",
		m_sName.c_str(), m_iUpdates, m_fUpdateTime, GetUpdateTime(), GetUpdateRate() );

	Reset();
}

inline void DebugTimer::Reset()
{
	m_iUpdates = 0;
	m_fUpdateTime = 0;
}

inline void DebugTimer::StartUpdate()
{
	m_Timer.Touch();
}

inline void DebugTimer::EndUpdate()
{
	float fDelta = m_Timer.GetDeltaTime();

	// too big - ignore
	if( fDelta > m_fTolerance )
		return;

	m_iUpdates++;
	m_fUpdateTime += fDelta;

	if( m_bAutoReport && TimeToReport() )
		Report();
}

#endif // DEBUG_TIMER_H
