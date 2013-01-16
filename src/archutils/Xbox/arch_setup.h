#ifndef ARCH_SETUP_XBOX_H
#define ARCH_SETUP_XBOX_H

#include <xtl.h>
#include <xgraphics.h>
#include <stdio.h>

// Xbox base path
#define SYS_BASE_PATH "D:\\"

/* Stubs: */
inline HRESULT CoInitialize( LPVOID pvReserved ) { return S_OK; }
inline void CoUninitialize() { }

#endif
