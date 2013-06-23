#ifndef	_ENDIAN_H
#define	_ENDIAN_H

#define	__LITTLE_ENDIAN	1234
#define	__BIG_ENDIAN	4321

#if defined(__i386__) || defined(__x86_64__) || defined(__ia64__)
#define __BYTE_ORDER		__LITTLE_ENDIAN
#define __FLOAT_WORD_ORDER	__BYTE_ORDER
#endif

#ifdef __sparc__
#define __BYTE_ORDER		__BIG_ENDIAN
#define __FLOAT_WORD_ORDER	__BYTE_ORDER
#endif

#ifdef __hppa__
/* I hope this is correct...? */
#define __BYTE_ORDER		__BIG_ENDIAN
#define __FLOAT_WORD_ORDER	__BYTE_ORDER
#endif

#ifdef __mips__
#ifdef __MIPSEB__
#define __BYTE_ORDER		__BIG_ENDIAN
#define __FLOAT_WORD_ORDER	__BYTE_ORDER
#endif
#endif

#ifndef __BYTE_ORDER
#ifdef __BIG_ENDIAN__
#define __BYTE_ORDER		__BIG_ENDIAN
#define __FLOAT_WORD_ORDER	__BYTE_ORDER
#else
#define __BYTE_ORDER		__LITTLE_ENDIAN
#define __FLOAT_WORD_ORDER	__BYTE_ORDER
#endif
#endif

#define LITTLE_ENDIAN		__LITTLE_ENDIAN
#define BIG_ENDIAN		__BIG_ENDIAN
#define BYTE_ORDER		__BYTE_ORDER

#if __BYTE_ORDER == __LITTLE_ENDIAN
# define __LONG_LONG_PAIR(HI, LO) LO, HI
#elif __BYTE_ORDER == __BIG_ENDIAN
# define __LONG_LONG_PAIR(HI, LO) HI, LO
#endif

#if defined(__alpha__) || defined(__mips64) || defined(__sparc_v9__) || defined(__x86_64__) || defined(__ia64__)
#define __WORDSIZE 64
#endif

#if defined(__x86_64__) || defined(__powerpc64__) || defined(__sparc_v9__)
#define __WORDSIZE_COMPAT32 1
#endif

#if defined(__sparc__) && (__arch64__)
#define __WORDSIZE 64
#endif

#ifndef __WORDSIZE
#define __WORDSIZE 32
#endif

#endif
