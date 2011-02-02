#ifndef KEYDUMP_H
#define KEYDUMP_H

#include "config.h"

#ifdef KD_DEBUG
#include "dbgutil.h"
#else
#define CHECKPOINT ;
#define CHECKPOINT_I(x,i) ;
#define printbuffer(name, buf) ;
#define printKey(aesKey) ;
#define printScratchpad(scratchPad) ;
#endif /* KD_DEBUG */

void printIntro(const char *game);

#endif /* KEYDUMP_H */
