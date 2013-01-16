/* ProductInfo - Branding strings and Lua globals. */

#ifndef PRODUCT_INFO_H
#define PRODUCT_INFO_H

namespace ProductInfo
{
	/* Binary name + release (e.g. "OpenITG", "beta3") */
	const CString& GetName();
	const CString& GetVersion();

	/* Binary name + build type + release (e.g. "OpenITG AC beta3") */
	const CString& GetFullVersion();

	/* Build data (e.g. "2012-12-31", "beta3-105-g63a1100") */
	const CString& GetBuildDate();
	const CString& GetBuildRevision();

	/* Crash report URL */
	const CString& GetCrashReportURL();
};

#endif

/*
 * (c) 2003-2013 Chris Danford, Marc Cannon
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

