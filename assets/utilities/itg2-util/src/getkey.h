#ifndef GETKEY_H
#define GETKEY_H

#define KEYDUMP_GETKEY_ITG2 0
#define KEYDUMP_GETKEY_PPRO 1

extern int getKey(const unsigned char *subkey, unsigned char *output, int game, unsigned int subkeySize);
#endif /* GETKEY_H */
