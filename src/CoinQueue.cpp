/* This oughtta take care of the dropped coin woes --infamouspat */
int CoinQueue::CoinQueue_Start( void *p )
{
	((CoinQueue*)p)->CoinQueue_ThreadMain();
	return 0;
}

void CoinQueue::CoinQueue_ThreadMain()
{
	while( !m_bShutdown )
	{
		if (m_iCoins > 0)
		{
			// TODO: make this go through INPUTFILTER?
			InsertCoin(1, true);
			m_iCoins--;
		}
		// possible thread issues?  better safe than sorry :/
		if (m_iCoins < 0) m_iCoins = 0;
		usleep(1000);
	}
}

void CoinQueue::Stop()
{
	m_bShutdown = true;
}

// XXX: make this not suck
void CoinQueue::Run()
{
	CQThread.Create( CoinQueue_Start, this );
}

/*
 * Copyright (c) 2008 BoXoRRoXoRs
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
