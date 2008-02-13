//shaib.h

#ifndef SHAIBUTTON_H
#define SHAIBUTTON_H

// for ANSI memory commands - memcpy,memcmp,memset
#include <string.h>
#include <time.h>
#include "ownet.h"
#include "owfile.h"

// ********************************************************************** //
// DS1963S SHA iButton commands
// ********************************************************************** //
#define SHA_COMPUTE_FIRST_SECRET 0x0F
#define SHA_COMPUTE_NEXT_SECRET  0xF0
#define SHA_VALIDATE_DATA_PAGE   0x3C
#define SHA_SIGN_DATA_PAGE       0xC3
#define SHA_COMPUTE_CHALLENGE    0xCC
#define SHA_AUTHENTICATE_HOST    0xAA
#define CMD_READ_MEMORY          0xF0
#define CMD_MATCH_SCRATCHPAD     0x3C
#define CMD_WRITE_SCRATCHPAD     0x0F
#define CMD_READ_SCRATCHPAD      0xAA
#define CMD_ERASE_SCRATCHPAD     0xC3
#define CMD_COPY_SCRATCHPAD      0x55
#define CMD_READ_AUTH_PAGE       0xA5
#define CMD_COMPUTE_SHA          0x33
#define ROM_CMD_SKIP             0x3C
#define ROM_CMD_RESUME           0xA5

// ********************************************************************** //
// DS1961S SHA iButton commands - rest are shared with above
// ********************************************************************** //
#define SHA33_LOAD_FIRST_SECRET     0x5A
#define SHA33_COMPUTE_NEXT_SECRET   0x33
#define SHA33_REFRESH_SCRATCHPAD    0xA3

// ********************************************************************** //
// other constants
#define SHA_FAMILY_CODE        0x18
#define SHA33_FAMILY_CODE      0x33
// maximum number of buttons to track on the port
#define MAX_SHA_IBUTTONS       16

#define SHACoprFilename       "shacopr.cnf"

// ********************************************************************** //
// SHA Device Structures
// ********************************************************************** //

// struct SHACopr
//   Holds all information pertinent to SHA coprocessors, as
//   well as all of the system parameters for this account
//   transaction system.
typedef struct
{
   // portnum and address of the device
   int   portnum;
   uchar devAN[8];

   // name of the account file stored on the user token
   uchar serviceFilename[5];
   // memory page used for signing certificate data (0 or 8)
   uchar signPageNumber;
   // memory page used for storing master authentication secret
   // and recreating user's unique secret
   uchar authPageNumber;
   // memory page used for storing user's unique secret
   // and verifying the user's authentication response
   uchar wspcPageNumber;
   // version number of the system
   uchar versionNumber;
   // Binding information for producing unique secrets
   uchar bindCode[7];
   uchar bindData[32];
   // challenge and signature used when signing account data
   uchar signChlg[3];
   uchar initSignature[20];
   // name of the transaction system provider
   uchar* providerName; //variable length
   // any other pertinent information
   uchar* auxilliaryData; //variable length
   // encryption code, used to specify additional encryption
   uchar encCode;
   // indicates that the master authentication secret was
   // padded to match the secret of a DS1961S
   uchar ds1961Scompatible;

} SHACopr;

// struct SHAUser
//   Holds all information pertinent to SHA user tokens.
//   Maintains state between routines for verifying the
//   user's authentication response, the account signature,
//   and updating the account data.
typedef struct
{
   // portnum and address of the device
   int   portnum;
   uchar devAN[8];

   // page the user's account file is stored on
   uchar accountPageNumber;
   // Write cycle counter for account page
   int   writeCycleCounter;
   // MAC from Read Authenticated Page command
   uchar responseMAC[20];

   // 32-byte account file
   uchar accountFile[32];

} SHAUser;

// struct DebitFile
//   Holds user token account data.  Byte-ordering
//   matters, so that this is a valid certificate
//   as specified by Application Note 151.
typedef struct
{
   uchar fileLength;
   uchar dataTypeCode;
   uchar signature[20];
   uchar convFactor[2];
   uchar balanceBytes[3];
   uchar transID[2];
   uchar contPtr;
   uchar crc16[2];

} DebitFile;

// struct DebitFile33
//   Holds user token account data for DS1961S.
//   Usese Double octa-byte scheme for dual money
//   bytes and a pointer to the valid record.
typedef struct
{
   // header
   uchar fileLength;
   uchar dataTypeCode;
   uchar convFactor[2];
   // dont cares
   uchar dontCareBytes1[4];
   // record A
   uchar balanceBytes_A[3];
   uchar transID_A[2];
   uchar contPtr_A;
   uchar crc16_A[2];
   // record B
   uchar balanceBytes_B[3];
   uchar transID_B[2];
   uchar contPtr_B;
   uchar crc16_B[2];
   // dont cares
   uchar dontCareBytes2[8];

} DebitFile33;

// file length used to point at Record A
#define RECORD_A_LENGTH 13
// file length used to point at Record B
#define RECORD_B_LENGTH 21


// ********************************************************************** //
// DS1963S Low-level Functions - defined in sha18.c
// ********************************************************************** //
// General I/O
extern SMALLINT CopySecretSHA18(int portnum, SMALLINT secretnum);
extern SMALLINT ReadScratchpadSHA18(int portnum, int* address,
                                    uchar* es, uchar* data,
                                    SMALLINT resume);
extern SMALLINT WriteScratchpadSHA18(int portnum, int address,
                                     uchar *data, SMALLINT data_len,
                                     SMALLINT resume);
extern SMALLINT CopyScratchpadSHA18(int portnum, int address,
                                    SMALLINT len, SMALLINT resume);
extern SMALLINT MatchScratchpadSHA18(int portnum, uchar* data,
                                     SMALLINT resume);
extern SMALLINT EraseScratchpadSHA18(int portnum, int address,
                                     SMALLINT resume);
extern int ReadAuthPageSHA18(int portnum, SMALLINT pagenum, uchar* data,
                             uchar* sign, SMALLINT resume);
extern SMALLINT ReadMemoryPageSHA18(int portnum, SMALLINT pagenum, uchar* data,
                                    SMALLINT resume);
extern SMALLINT WriteDataPageSHA18(int portnum, SMALLINT pagenum,
                                   uchar* data, SMALLINT resume);
extern SMALLINT SHAFunction18(int portnum, uchar control_byte,
                              int address, SMALLINT resume);
// Secret Installation
extern SMALLINT InstallSystemSecret18(int portnum, SMALLINT pagenum,
                                      SMALLINT secretnum, uchar* secret,
                                      int secret_length, SMALLINT resume);
extern SMALLINT BindSecretToiButton18(int portnum, SMALLINT pagenum,
                                      SMALLINT secretnum,
                                      uchar* bindData, uchar* bindCode,
                                      SMALLINT resume);
// ********************************************************************** //


// General Util
extern void ReformatSecretFor1961S(uchar* auth_secret, int secret_length);
extern void ComputeSHAVM(uchar* MT, long* hash);
extern void HashToMAC(long* hash, uchar* MAC);
// ********************************************************************** //

// ********************************************************************** //
// Protocol-Level Functions - defined in shaibutton.c
// ********************************************************************** //
// Finding and accessing SHA iButtons
extern SMALLINT SelectSHA(int portnum);
extern SMALLINT FindNewSHA(int portnum, uchar* devAN, SMALLINT forceFirst);
//extern SMALLINT FindUserSHA(SHAUser* user, FileEntry* fe, SMALLINT doBlocking);
//extern SMALLINT FindCoprSHA(SHACopr* copr, FileEntry* fe);
extern int GetCoprFromRawData(SHACopr* copr, uchar* raw, int len);
// General Protocol functions for 1963S
extern SMALLINT CreateChallenge(SHACopr* copr, SMALLINT pageNum,
                                uchar* chlg, SMALLINT offset);
//extern int AnswerChallenge(SHAUser* user, uchar* chlg);
extern SMALLINT VerifyAuthResponse(SHACopr* copr, SHAUser* user,
                                   uchar* chlg, SMALLINT doBind);
extern SMALLINT CreateDataSignature(SHACopr* copr, uchar* data,
                                    uchar* scratchpad, uchar* signature,
                                    SMALLINT readSignature);
// Useful utility functions
extern SMALLINT IntToBytes(uchar* byteArray, int len, unsigned int val);
extern int BytesToInt(uchar* byteArray, int len);
// ********************************************************************** //

// ********************************************************************** //
// Global - defined in shaibutton.c
extern SMALLINT in_overdrive[MAX_PORTNUM];
// ********************************************************************** //

extern void PrintHexLabeled(char* label,uchar* buffer, int cnt);

extern void ReadChars(uchar* buffer, int len);

#endif //SHAIBUTTON_H
