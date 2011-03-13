#include "global.h"
#include "USBDriver_Impl.h"
#include "arch/arch_default.h"

USBDriver_Impl::USBDriver_Impl()
{
	/* no-op */
}

USBDriver_Impl::~USBDriver_Impl()
{
	/* no-op */
}

/* XXX: this is a little dumb. hopefully there's a better way? */

USBDriver_Impl* USBDriver_Impl::Create()
{
#if defined(WINDOWS)
	return new USBDriver_Impl_WinUSB;
#elif defined(UNIX)
	return new USBDriver_Impl_Libusb;
#else
	return NULL;
#endif
}
