#include "global.h"
#include "MemoryCardDriverThreaded_Windows.h"
#include "RageUtil.h"
#include "RageLog.h"

#include "archutils/Win32/ErrorStrings.h"
#include "PlayerNumber.h"
#include "MemoryCardManager.h"

MemoryCardDriverThreaded_Windows::MemoryCardDriverThreaded_Windows()
{
	m_dwLastLogicalDrives = 0;
}

MemoryCardDriverThreaded_Windows::~MemoryCardDriverThreaded_Windows()
{
}

static bool TestReady( const CString &sDrive, CString &sVolumeLabelOut )
{
	TCHAR szVolumeNameBuffer[MAX_PATH];
	DWORD dwVolumeSerialNumber;
	DWORD dwMaximumComponentLength;
	DWORD lpFileSystemFlags;
	TCHAR szFileSystemNameBuffer[MAX_PATH];

	if( !GetVolumeInformation( 
		sDrive,
		szVolumeNameBuffer,
		sizeof(szVolumeNameBuffer),
		&dwVolumeSerialNumber,
		&dwMaximumComponentLength,
		&lpFileSystemFlags,
		szFileSystemNameBuffer,
		sizeof(szFileSystemNameBuffer)) )
		return false;

	sVolumeLabelOut = szVolumeNameBuffer;
	return true;
}

bool MemoryCardDriverThreaded_Windows::TestWrite( UsbStorageDevice* pDevice )
{
	/* Try to write a file, to check if the device is writable and that we have write permission.
	 * Use FILE_ATTRIBUTE_TEMPORARY to try to avoid actually writing to the device.  This reduces
	 * the chance of corruption if the user removes the device immediately, without doing anything. */
	for( int i = 0; i < 10; ++i )
	{
		HANDLE hFile = CreateFile( ssprintf( "%stmp%i", pDevice->sOsMountDir.c_str(), RandomInt(0,100000)),
			GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, CREATE_NEW, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, NULL );

		if( hFile == INVALID_HANDLE_VALUE )
		{
			DWORD iError = GetLastError();
			LOG->Warn( werr_ssprintf(iError, "Couldn't write to %s", pDevice->sOsMountDir.c_str()) );

			if( iError == ERROR_FILE_EXISTS )
				continue;
			break;
		}

		CloseHandle( hFile );
		return true;
	}

	pDevice->SetError( "TestFailed" );
	return false;
}

static bool IsFloppyDrive( const CString &sDrive )
{
	char szBuf[1024];

	int iRet = QueryDosDevice( sDrive, szBuf, 1024 );
	if( iRet == 0 )
	{
		LOG->Warn( werr_ssprintf(GetLastError(), "QueryDosDevice(%s)", sDrive.c_str()) );
		return false;
	}

	// Make sure szBuf is terminated with two nulls.  This only may be needed if the buffer filled.
	szBuf[iRet-2] = 0;
	szBuf[iRet-1] = 0;

	const char *p = szBuf;
	while( *p )
	{
		if( BeginsWith(p, "\\Device\\Floppy") )
			return true;

		p += strlen(p)+1;
	}
	return false;
}

void MemoryCardDriverThreaded_Windows::GetUSBStorageDevices( vector<UsbStorageDevice>& vDevicesOut )
{
	LOG->Trace( "MemoryCardDriverThreaded_Windows::GetUSBStorageDevices" );

	DWORD dwLogicalDrives = ::GetLogicalDrives();
	m_dwLastLogicalDrives = dwLogicalDrives;

	const int MAX_DRIVES = 26;
	for( int i=0; i<MAX_DRIVES; ++i )
	{
		DWORD mask = (1 << i);
		if( !(m_dwLastLogicalDrives & mask) )
			continue; // drive letter is invalid

		CString sDrive = ssprintf( "%c:", 'A'+i%26 );

		LOG->Trace( sDrive );

		if( IsFloppyDrive(sDrive) )
		{
			LOG->Trace( "IsFloppyDrive" );
			continue;
		}

		// Testing hack:  Allow non-removable drive letters to be used if that 
		// driver letter is specified as a m_sMemoryCardOsMountPoint.

		bool bIsSpecifiedMountPoint = false;
		FOREACH_PlayerNumber( p )
		{
			LOG->Trace("MemoryCardOsMountPointP%d: %s", (int)(p+1), MEMCARDMAN->m_sMemoryCardOsMountPoint[p].Get().c_str());
			bIsSpecifiedMountPoint |= bool( !(MEMCARDMAN->m_sMemoryCardOsMountPoint[p].Get().CompareNoCase(sDrive)) );
		}

		CString sDrivePath = sDrive + "\\";

		if( bIsSpecifiedMountPoint )
		{
			LOG->Trace( "'%s' is a specified mount point.  Allowing...", sDrive.c_str() );
		}
		else
		{
			if( GetDriveType(sDrivePath) != DRIVE_REMOVABLE )
			{
				LOG->Trace( "not DRIVE_REMOVABLE" );
				continue;	
			}
		}

		CString sVolumeLabel;
		if( !TestReady(sDrivePath, sVolumeLabel) )
		{
			LOG->Trace( "not TestReady" );
			continue;	
		}

		vDevicesOut.push_back( UsbStorageDevice() );
		UsbStorageDevice &usbd = vDevicesOut.back();
		usbd.SetOsMountDir( sDrive );
		usbd.sDevice = "\\\\.\\" + sDrive;
		usbd.sVolumeLabel = sVolumeLabel;
	}

	for( size_t i = 0; i < vDevicesOut.size(); ++i )
	{
		UsbStorageDevice &usbd = vDevicesOut[i];
		
		// TODO: fill in bus/level/port with this:
		// http://www.codeproject.com/system/EnumDeviceProperties.asp

		// find volume size
		DWORD dwSectorsPerCluster;
		DWORD dwBytesPerSector;
		DWORD dwNumberOfFreeClusters;
		DWORD dwTotalNumberOfClusters;
		if( GetDiskFreeSpace(
				usbd.sOsMountDir,
				&dwSectorsPerCluster,
				&dwBytesPerSector,
				&dwNumberOfFreeClusters,
				&dwTotalNumberOfClusters ) )
		{
			usbd.iVolumeSizeMB = (int)roundf( dwTotalNumberOfClusters * (float)dwSectorsPerCluster * dwBytesPerSector / (1024*1024) );
		}
	}
}

bool MemoryCardDriverThreaded_Windows::USBStorageDevicesChanged()
{
	return ::GetLogicalDrives() != m_dwLastLogicalDrives;
}

bool MemoryCardDriverThreaded_Windows::Mount( UsbStorageDevice* pDevice )
{
	// nothing to do here...
	return true;
}

void MemoryCardDriverThreaded_Windows::Unmount( UsbStorageDevice* pDevice )
{
	/* Try to flush the device before returning.  This requires administrator priviliges. */
	HANDLE hDevice = CreateFile( pDevice->sDevice, GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );

	if( hDevice == INVALID_HANDLE_VALUE )
	{
		LOG->Warn( werr_ssprintf(GetLastError(), "Couldn't open memory card device to flush (%s): CreateFile", pDevice->sDevice.c_str()) );
		return;
	}

	if( !FlushFileBuffers(hDevice) )
		LOG->Warn( werr_ssprintf(GetLastError(), "Couldn't flush memory card device (%s): FlushFileBuffers", pDevice->sDevice.c_str()) );
	CloseHandle( hDevice );
}

/*
 * (c) 2003-2004 Chris Danford
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
