#ifndef MISC_ITG_H
#define MISC_ITG_H
#include "StdString.h"

int GetNumCrashLogs();
int GetNumMachineEdits();
int GetIP();
int GetRevision();
int GetNumMachineScores();
CString GetSerialNumber();

static CString g_SerialNum;
#endif

