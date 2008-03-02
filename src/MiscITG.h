#ifndef MISC_ITG_H
#define MISC_ITG_H

#include "StdString.h"

int GetNumCrashLogs();
int GetNumMachineEdits();
int GetNumMachineScores();

int GetIP();
int GetRevision();

CString GetSerialNumber();

static CString g_SerialNum;

/* Copyright notice, etc. -- Vyhd */

#endif
