#ifndef DBGUTIL_H
#define DBGUTIL_H

void printbuffer(const char *name, const unsigned char *buf);
void printKey(const unsigned char *aesKey);
void printScratchpad(const unsigned char *scratchPad);

#define CHECKPOINT printf("CHECKPOINT@line %d\n", __LINE__)

#define CHECKPOINT_I(x,i) printf("CHECKPOINT@line %d: %s = %d\n", __LINE__, x, i)

#endif /* DBGUTIL_H */
