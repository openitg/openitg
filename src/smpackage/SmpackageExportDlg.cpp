// SmpackageExportDlg.cpp : implementation file
//

#include "stdafx.h"
#include "smpackage.h"
#include "SmpackageExportDlg.h"
#include "RageUtil.h"
#include "ZipArchive\ZipArchive.h"	
#include "EnterName.h"	
#include "EnterComment.h"	
#include "smpackageUtil.h"	
#include "EditInsallations.h"	
#include "IniFile.h"	

#include <vector>
#include <algorithm>
#include <set>
using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSmpackageExportDlg dialog


CSmpackageExportDlg::CSmpackageExportDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSmpackageExportDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSmpackageExportDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CSmpackageExportDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSmpackageExportDlg)
	DDX_Control(pDX, IDC_COMBO_DIR, m_comboDir);
	DDX_Control(pDX, IDC_BUTTON_EXPORT_AS_INDIVIDUAL, m_buttonExportAsIndividual);
	DDX_Control(pDX, IDC_BUTTON_EXPORT_AS_ONE, m_buttonExportAsOne);
	DDX_Control(pDX, IDC_TREE, m_tree);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSmpackageExportDlg, CDialog)
	//{{AFX_MSG_MAP(CSmpackageExportDlg)
	ON_BN_CLICKED(IDC_BUTTON_EXPORT_AS_ONE, OnButtonExportAsOne)
	ON_BN_CLICKED(IDC_BUTTON_EXPORT_AS_INDIVIDUAL, OnButtonExportAsIndividual)
	ON_BN_CLICKED(IDC_BUTTON_PLAY, OnButtonPlay)
	ON_BN_CLICKED(IDC_BUTTON_EDIT, OnButtonEdit)
	ON_CBN_SELCHANGE(IDC_COMBO_DIR, OnSelchangeComboDir)
	ON_BN_CLICKED(IDC_BUTTON_OPEN, OnButtonOpen)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSmpackageExportDlg message handlers

BOOL CSmpackageExportDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	//

	RefreshInstallationList();
	
	RefreshTree();

	return TRUE;  // return TRUE  unless you set the focus to a control
}

CString ReplaceInvalidFileNameChars( CString sOldFileName )
{
	CString sNewFileName = sOldFileName;
	const char charsToReplace[] = { 
		' ', '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', 
		'+', '=', '[', ']', '{', '}', '|', ':', '\"', '\\',
		'<', '>', ',', '?', '/' 
	};
	for( int i=0; i<sizeof(charsToReplace); i++ )
		sNewFileName.Replace( charsToReplace[i], '_' );
	return sNewFileName;
}

void GetFilePaths( CString sDirOrFile, 	vector<CString> &asPathToFilesOut )
{
	vector<CString> asDirectoriesToExplore;

	// HACK:
	// Must use backslashes in the path, or else WinZip and WinRAR don't see the files.
	// Not sure if this is ZipArchive's fault.

	if( IsADirectory(sDirOrFile) && sDirOrFile.Right(1) != "\\" )
	{
		sDirOrFile += "\\";
		sDirOrFile += "*.*";
	}
	
	if( IsAFile(sDirOrFile) )
	{
		asPathToFilesOut.push_back( sDirOrFile );
		return;
	}


	GetDirListing( sDirOrFile, asPathToFilesOut, false, true );
	GetDirListing( sDirOrFile, asDirectoriesToExplore, true, true );
	while( asDirectoriesToExplore.size() > 0 )
	{
		GetDirListing( asDirectoriesToExplore[0] + "\\*.*", asPathToFilesOut, false, true );
		GetDirListing( asDirectoriesToExplore[0] + "\\*.*", asDirectoriesToExplore, true, true );
		asDirectoriesToExplore.erase( asDirectoriesToExplore.begin() );
	}
}

CString GetDesktopPath()
{
    static TCHAR strNull[2] = _T("");
    static TCHAR strPath[MAX_PATH];
    DWORD dwType;
    DWORD dwSize = MAX_PATH;
    HKEY  hKey;

    // Open the appropriate registry key
    LONG lResult = RegOpenKeyEx( HKEY_CURRENT_USER,
                                _T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders"),
                                0, KEY_READ, &hKey );
    if( ERROR_SUCCESS != lResult )
        return strNull;

    lResult = RegQueryValueEx( hKey, _T("Desktop"), NULL,
                              &dwType, (BYTE*)strPath, &dwSize );
    RegCloseKey( hKey );

    if( ERROR_SUCCESS != lResult )
        return strNull;

    return strPath;
}

bool ExportPackage( CString sPackageName, const CStringArray& asDirectoriesToExport, CString sComment )	
{
	CZipArchive zip;
	
	//
	// Create the package zip file
	//
	const CString sPackagePath = GetDesktopPath() + "\\" + sPackageName;
	try
	{
		zip.Open( sPackagePath, CZipArchive::zipCreate );
	}
	catch( CException* e )
	{
		e->ReportError();
		zip.Close();
		e->Delete();
		return false;
	}


	zip.SetGlobalComment( sComment );

	/* Find files to add to zip. */
	unsigned i;
	vector<CString> asFilePaths;
	for( i=0; i<asDirectoriesToExport.size(); i++ )
		GetFilePaths( asDirectoriesToExport[i], asFilePaths );

	{
		IniFile ini;
		ini.SetValueI( "SMZIP", "Version", 1 );

		set<CString> Directories;
		for( i=0; i<asFilePaths.size(); i++ )
		{
			const CString name = GetPackageDirectory( asFilePaths[i] );
			if( name != "" )
				Directories.insert( name );
		}

		set<CString>::const_iterator it;
		int num = 0;
		for( it = Directories.begin(); it != Directories.end(); ++it )
			ini.SetValue( "Packages", ssprintf("%i", num++), *it );
		ini.SetValueI( "Packages", "NumPackages", num );

		CString buf;
		ini.WriteBuf(buf);

		CZipMemFile control;
		control.Write( buf.GetBuffer(0), buf.GetLength() );

		control.Seek( 0, CZipAbstractFile::begin );
		zip.AddNewFile( control, "smzip.ctl" );
	}

	//
	// Add files to zip
	//
	for( unsigned j=0; j<asFilePaths.size(); j++ )
	{
		CString sFilePath = asFilePaths[j];
		
		// don't export "thumbs.db" files or "CVS" folders
		if( sFilePath.Find("CVS")!=-1 )
			continue;	// skip
		if( sFilePath.Find("Thumbs.db")!=-1 )
			continue;	// skip

		CString sDir, sFName, sExt;
		splitrelpath( sFilePath, sDir, sFName, sExt );
		bool bUseCompression = true;
		if( sExt.CompareNoCase("avi")==0 ||
			sExt.CompareNoCase("mpeg")==0 ||
			sExt.CompareNoCase("mpg")==0 ||
			sExt.CompareNoCase("ogg")==0 ||
			sExt.CompareNoCase("gif")==0 ||
			sExt.CompareNoCase("jpg")==0 ||
			sExt.CompareNoCase("png")==0 )
			bUseCompression = false;

		try
		{
			zip.AddNewFile( sFilePath, bUseCompression?Z_BEST_COMPRESSION:Z_NO_COMPRESSION, true );
		}
		catch (CException* e)
		{
			AfxMessageBox( ssprintf("Error adding file '%s'.", sFilePath) );
			zip.Close();
			e->Delete();
			return false;
		}
	}

	zip.Close();
	return true;
}

bool CSmpackageExportDlg::MakeComment( CString &comment )
{
	bool DontAskForComment;
	if( GetPref("DontAskForComment", DontAskForComment) && DontAskForComment )
	{
		comment = "";
		return true;
	}

	EnterComment commentDlg;
	int nResponse = commentDlg.DoModal();
	if( nResponse != IDOK )
		return false;	// cancelled

	comment = commentDlg.m_sEnteredComment;
	if( commentDlg.m_bDontAsk )
		SetPref( "DontAskForComment", true );

	return true;
}

void CSmpackageExportDlg::OnButtonExportAsOne() 
{
	CStringArray asPaths;
	GetCheckedPaths( asPaths );

	if( asPaths.size() == 0 )
	{
		AfxMessageBox( "No items are checked" );
		return;
	}
	else if( asPaths.size() == 1 )
	{
		OnButtonExportAsIndividual();
		return;
	}

	// Generate a package name
	CString sPackageName;
	EnterName nameDlg;
	int nResponse = nameDlg.DoModal();
	if( nResponse != IDOK )
		return;	// cancelled
	sPackageName = nameDlg.m_sEnteredName;
	sPackageName = ReplaceInvalidFileNameChars( sPackageName+".smzip" );

	// Generate a comment
	CString sComment;
	if( !MakeComment(sComment) )
		return;		// cancelled

	if( ExportPackage( sPackageName, asPaths, sComment ) )
		AfxMessageBox( ssprintf("Successfully exported package '%s' to your Desktop.",sPackageName) );
}

void CSmpackageExportDlg::OnButtonExportAsIndividual() 
{
	CStringArray asPaths;
	GetCheckedPaths( asPaths );

	if( asPaths.size() == 0 )
	{
		AfxMessageBox( "No items are checked" );
		return;
	}
	
	// Generate a comment
	CString sComment;
	if( !MakeComment(sComment) )
		return;		// cancelled

	bool bAllSucceeded = true;
	CStringArray asExportedPackages;
	CStringArray asFailedPackages;
	for( unsigned i=0; i<asPaths.size(); i++ )
	{		
		// Generate a package name for every path
		CString sPath = asPaths[i];

		CString sPackageName;
		CStringArray asPathBits;
		split( sPath, "\\", asPathBits, true );
		sPackageName = asPathBits[ asPathBits.size()-1 ] + ".smzip";
		sPackageName = ReplaceInvalidFileNameChars( sPackageName );

		CStringArray asPathsToExport;
		asPathsToExport.push_back( sPath );
		
		if( ExportPackage( sPackageName, asPathsToExport, sComment ) )
			asExportedPackages.push_back( sPackageName );
		else
			asFailedPackages.push_back( sPackageName );
	}

	CString sMessage;
	if( asFailedPackages.size() == 0 )
		sMessage = ssprintf("Successfully exported the package%s '%s' to your Desktop.", asFailedPackages.size()>1?"s":"", join("', '",asExportedPackages) );
	else
		sMessage = ssprintf("  The packages %s failed to export.", join(", ",asFailedPackages) );
	AfxMessageBox( sMessage );
}

void CSmpackageExportDlg::OnButtonPlay() 
{
	// TODO: Add your control notification handler code here

	PROCESS_INFORMATION pi;
	STARTUPINFO	si;
	ZeroMemory( &si, sizeof(si) );

	CreateProcess(
		NULL,		// pointer to name of executable module
		"stepmania.exe",		// pointer to command line string
		NULL,  // process security attributes
		NULL,   // thread security attributes
		false,  // handle inheritance flag
		0, // creation flags
		NULL,  // pointer to new environment block
		NULL,   // pointer to current directory name
		&si,  // pointer to STARTUPINFO
		&pi  // pointer to PROCESS_INFORMATION
	);

	exit(0);
}

void CSmpackageExportDlg::GetTreeItems( CArray<HTREEITEM,HTREEITEM>& aItemsOut )
{
	CArray<HTREEITEM,HTREEITEM> aRootsToExplore;	
	
	// add all top-level roots
	HTREEITEM item = m_tree.GetRootItem();
	while( item != NULL )
	{
		aRootsToExplore.Add( item );
		item = m_tree.GetNextSiblingItem( item );
	}

	while( aRootsToExplore.GetSize() > 0 )
	{
		HTREEITEM item = aRootsToExplore[0];
		aRootsToExplore.RemoveAt( 0 );
		aItemsOut.Add( item );

		HTREEITEM child = m_tree.GetChildItem( item );
		while( child != NULL )
		{
			aRootsToExplore.Add( child );
			child = m_tree.GetNextSiblingItem( child );
		}
	}
}

void CSmpackageExportDlg::GetCheckedTreeItems( CArray<HTREEITEM,HTREEITEM>& aCheckedItemsOut )
{
	CArray<HTREEITEM,HTREEITEM> aItems;	

	GetTreeItems( aItems );
	for( int i=0; i<aItems.GetSize(); i++ )
		if( m_tree.GetCheck(aItems[i]) )
			aCheckedItemsOut.Add( aItems[i] );
}

void CSmpackageExportDlg::GetCheckedPaths( CStringArray& aPathsOut )
{
	CArray<HTREEITEM,HTREEITEM> aItems;	

	GetCheckedTreeItems( aItems );
	for( int i=0; i<aItems.GetSize(); i++ )
	{
		HTREEITEM item = aItems[i];

		CString sPath;
		
		while( item )
		{
			sPath = m_tree.GetItemText(item) + '\\' + sPath;
			item = m_tree.GetParentItem(item);
		}

		sPath.TrimRight('\\');	// strip off last slash

		aPathsOut.push_back( sPath );
	}
}


void CSmpackageExportDlg::OnButtonEdit() 
{
	// TODO: Add your control notification handler code here
	EditInsallations dlg;
	int nResponse = dlg.DoModal();
	if( nResponse == IDOK )
	{
		WriteStepManiaInstallDirs( dlg.m_asReturnedInstallDirs );
		RefreshInstallationList();
		RefreshTree();
	}
}

void CSmpackageExportDlg::RefreshInstallationList() 
{
	m_comboDir.ResetContent();

	CStringArray asInstallDirs;
	GetStepManiaInstallDirs( asInstallDirs );
	for( unsigned i=0; i<asInstallDirs.size(); i++ )
	{
		m_comboDir.AddString( asInstallDirs[i] );
	}
	m_comboDir.SetCurSel( 0 );	// guaranteed to be at least one item
}

void CSmpackageExportDlg::OnSelchangeComboDir() 
{
	// TODO: Add your control notification handler code here
	RefreshTree();
}

void CSmpackageExportDlg::RefreshTree()
{
	m_tree.DeleteAllItems();

	CString sDir;
	m_comboDir.GetWindowText( sDir );

	SetCurrentDirectory( sDir );

	// Add announcers
	{
		CStringArray as1;
		HTREEITEM item1 = m_tree.InsertItem( "Announcers" );
		GetDirListing( "Announcers\\*.*", as1, true, false );
		for( unsigned i=0; i<as1.size(); i++ )
			m_tree.InsertItem( as1[i], item1 );
	}

	// Add characters
	{
		CStringArray as1;
		HTREEITEM item1 = m_tree.InsertItem( "Characters" );
		GetDirListing( "Characters\\*.*", as1, true, false );
		for( unsigned i=0; i<as1.size(); i++ )
			m_tree.InsertItem( as1[i], item1 );
	}

	// Add themes
	{
		CStringArray as1;
		HTREEITEM item1 = m_tree.InsertItem( "Themes" );
		GetDirListing( "Themes\\*.*", as1, true, false );
		for( unsigned i=0; i<as1.size(); i++ )
			m_tree.InsertItem( as1[i], item1 );
	}

	// Add BGAnimations
	{
		CStringArray as1;
		HTREEITEM item1 = m_tree.InsertItem( "BGAnimations" );
		GetDirListing( "BGAnimations\\*.*", as1, true, false );
		for( unsigned i=0; i<as1.size(); i++ )
			m_tree.InsertItem( as1[i], item1 );
	}

	// Add RandomMovies
	{
		CStringArray as1;
		HTREEITEM item1 = m_tree.InsertItem( "RandomMovies" );
		GetDirListing( "RandomMovies\\*.avi", as1, false, false );
		GetDirListing( "RandomMovies\\*.mpg", as1, false, false );
		GetDirListing( "RandomMovies\\*.mpeg", as1, false, false );
		for( unsigned i=0; i<as1.size(); i++ )
			m_tree.InsertItem( as1[i], item1 );
	}

	// Add visualizations
	{
		CStringArray as1;
		HTREEITEM item1 = m_tree.InsertItem( "Visualizations" );
		GetDirListing( "Visualizations\\*.avi", as1, false, false );
		GetDirListing( "Visualizations\\*.mpg", as1, false, false );
		GetDirListing( "Visualizations\\*.mpeg", as1, false, false );
		for( unsigned i=0; i<as1.size(); i++ )
			m_tree.InsertItem( as1[i], item1 );
	}

	// Add courses
	{
		CStringArray as1;
		HTREEITEM item1 = m_tree.InsertItem( "Courses" );
		GetDirListing( "Courses\\*.crs", as1, false, false );
		for( unsigned i=0; i<as1.size(); i++ )
		{
			as1[i] = as1[i].Left(as1[i].GetLength()-4);	// strip off ".crs"
			m_tree.InsertItem( as1[i], item1 );
		}
	}


	//
	// Add NoteSkins
	//
	{
		CStringArray as1;
		HTREEITEM item1 = m_tree.InsertItem( "NoteSkins" );
		GetDirListing( "NoteSkins\\*.*", as1, true, false );
		for( unsigned i=0; i<as1.size(); i++ )
		{
			CStringArray as2;
			HTREEITEM item2 = m_tree.InsertItem( as1[i], item1 );
			GetDirListing( "NoteSkins\\" + as1[i] + "\\*.*", as2, true, false );
			for( unsigned j=0; j<as2.size(); j++ )
				m_tree.InsertItem( as2[j], item2 );
		}
	}

	//
	// Add Songs
	//
	{
		CStringArray as1;
		HTREEITEM item1 = m_tree.InsertItem( "Songs" );
		GetDirListing( "Songs\\*.*", as1, true, false );
		for( unsigned i=0; i<as1.size(); i++ )
		{
			CStringArray as2;
			HTREEITEM item2 = m_tree.InsertItem( as1[i], item1 );
			GetDirListing( "Songs\\" + as1[i] + "\\*.*", as2, true, false );
			for( unsigned j=0; j<as2.size(); j++ )
				m_tree.InsertItem( as2[j], item2 );
		}
	}


	// Strip out any "CVS" items
	CArray<HTREEITEM,HTREEITEM> aItems;
	GetTreeItems( aItems );
	for( int i=0; i<aItems.GetSize(); i++ )
		if( m_tree.GetItemText(aItems[i]).CompareNoCase("CVS")==0 )
			m_tree.DeleteItem( aItems[i] );
}

void CSmpackageExportDlg::OnButtonOpen() 
{
	// TODO: Add your control notification handler code here
	
	char szCurDir[MAX_PATH];
	GetCurrentDirectory( MAX_PATH, szCurDir );

	char szCommandToExecute[MAX_PATH] = "explorer ";
	strcat( szCommandToExecute, szCurDir );


	PROCESS_INFORMATION pi;
	STARTUPINFO	si;
	ZeroMemory( &si, sizeof(si) );

	CreateProcess(
		NULL,		// pointer to name of executable module
		szCommandToExecute,		// pointer to command line string
		NULL,  // process security attributes
		NULL,   // thread security attributes
		false,  // handle inheritance flag
		0, // creation flags
		NULL,  // pointer to new environment block
		NULL,   // pointer to current directory name
		&si,  // pointer to STARTUPINFO
		&pi  // pointer to PROCESS_INFORMATION
	);
}
