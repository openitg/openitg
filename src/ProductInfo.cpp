#include "global.h"
#include "ProductInfo.h"

CString ProductInfo::getName() {
	return CString("OpenITG");
}

CString ProductInfo::getVersion() {
#if defined(_WIN32)
	return CString("b3-win32");
#else
	return CString(OITG_VERSION);
#endif
}

CString ProductInfo::getDate() {
#if defined(_WIN32)
	return CString("04-08-2011");
#else
	return CString(OITG_DATE);
#endif
}

CString ProductInfo::getPlatform() {
#if defined(ITG_ARCADE)
	return CString("AC");
#elif defined(XBOX)
	return CString("CS");
#else
	return CString("PC");
#endif
}

CString ProductInfo::getCrashReportUrl() {
	return CString("http://wush.net/bugzilla/terabyte/");
}

CString ProductInfo::getFullVersionString() {
	return getVersion() + "-" + getPlatform();
}

CString ProductInfo::getSerial() {
	return CString("OITG-") + getVersion() + "-" + getPlatform();
}
