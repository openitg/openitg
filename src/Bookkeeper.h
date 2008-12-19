/* Bookkeeper - coin tracking. */

#ifndef Bookkeeper_H
#define Bookkeeper_H

#include "DateTime.h"
#include <map>
struct XNode;

class Bookkeeper
{
public:
	Bookkeeper();
	~Bookkeeper();

	void ClearAll();

	void CoinInserted();
	void ServiceCoinInserted();

	int GetCoinsTotal( bool bService = false ) const;
	void GetCoinsLastDays( int coins[NUM_LAST_DAYS], bool bService = false ) const;
	void GetCoinsLastWeeks( int coins[NUM_LAST_WEEKS], bool bService = false ) const;
	void GetCoinsByDayOfWeek( int coins[DAYS_IN_WEEK], bool bService = false ) const;
	void GetCoinsByHour( int coins[HOURS_IN_DAY], bool bService = false ) const;

	void ReadFromDisk();
	void WriteToDisk();
private:
	struct Date
	{
		int m_iHour; // 0 = midnight
		int m_iDayOfYear; // 0 = Jan 1
		int m_iYear; // eg. 2005
		Date() { m_iHour = m_iDayOfYear = m_iYear = 0; }
		Date( tm time ) { Set(time); }
		void Set( time_t t );
		void Set( tm pTime );
		bool operator<( const Date &rhs ) const;
	};

	XNode *CreateXML() const;
	XNode* CreateNode( const CString name, const map<Date,int> &mCoinMap ) const;
	void LoadFromNode( const XNode *pNode, map<Date,int> &mCoinMap );

	int GetNumCoins( Date beginning, Date ending, bool bService = false ) const;
	int GetNumCoinsInRange( map<Date,int>::const_iterator begin, map<Date,int>::const_iterator end ) const;

	const map<Date,int> *GetCoinMap( bool bService ) const
		{ return bService ? &m_mapServiceCoinsForHour : &m_mapCoinsForHour; }

	int m_iLastSeenTime;
	map<Date,int> m_mapCoinsForHour;
	map<Date,int> m_mapServiceCoinsForHour;
};


extern Bookkeeper*	BOOKKEEPER;	// global and accessable from anywhere in our program

#endif

/*
 * (c) 2003-2004 Chris Danford
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
