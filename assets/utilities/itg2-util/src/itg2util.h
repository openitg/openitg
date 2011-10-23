#ifndef ITG2UTIL_H
#define ITG2UTIL_H

#define KEYDUMP_ITG2_FILE_DATA 0
#define KEYDUMP_ITG2_FILE_PATCH 1
#define KEYDUMP_ITG2_FILE_STATIC 2

int keydump_itg2_encrypt_file(const char *srcFile, const char *destFile, const unsigned char *subkey, const unsigned int subkeySize, const unsigned char *aesKey, const int type, char *keyFile);
int keydump_itg2_decrypt_file(const char *srcFile, const char *destFile, int type, const char *keyFile);
int keydump_itg2_retrieve_aes_key(const unsigned char *subkey, const unsigned int subkeySize, unsigned char *out, int type, int direction, const char *keyFile);

#endif /* ITG2UTIL_H */
