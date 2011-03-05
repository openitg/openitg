#if defined(WIN32)
	#include "global.h"
#endif
#include "StdString.h"
#include "ProductInfo.h"

CString ProductInfo::getName() {
	return CString("OpenITG");
}

CString ProductInfo::getVersion() {
	return CString(OITG_VERSION);
}

CString ProductInfo::getDate() {
	return CString(OITG_DATE);
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
