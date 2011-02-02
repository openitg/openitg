//---------------------------------------------------------------------------
// Copyright (C) 2000 Dallas Semiconductor Corporation, All Rights Reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY,  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL DALLAS SEMICONDUCTOR BE LIABLE FOR ANY CLAIM, DAMAGES
// OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
//
// Except as contained in this notice, the name of Dallas Semiconductor
// shall not be used except as stated in the Dallas Semiconductor
// Branding Policy.
//--------------------------------------------------------------------------
//
//  owFile.h - Contains the types and constants for file I/O. 
//
//  version 1.00
//

#include "ownet.h"

#ifndef OWFILE_H
#define OWFILE_H
#define MAX_NUM_PGS   256    // this is for max pages part might have
                             // increase if you are going to use parts
                             // with more than 256 pages.
#define PAGE_TYPE     uchar  // change if the page number can be > 256
#define FILE_NAME_LEN 7      // the length of a new file
#define FIRST_FOUND   204    //  don't know how many now so return max
// Used in internal function Read_Page and page cache
#define STATUSMEM     0x80
#define REDIRMEM      0x40
#define REGMEM        0 
#define DEPTH         254
#define CACHE_TIMEOUT 8000
// Directory options
#define SET_DIR            0
#define READ_DIR           1
// program Job constants
#define PJOB_NONE               0
#define PJOB_WRITE              1
#define PJOB_REDIR              2
#define PJOB_ADD                3
#define PJOB_TERM               4
#define PJOB_START              0x80   
#define PJOB_MASK               0x7F
#define APPEND                  1

// external file information structure
typedef struct 
{
   uchar Name[4];          // 4
   uchar Ext;              // 1
   uchar Spage;            // 1
   uchar NumPgs;           // 1
   uchar Attrib;           // 1
   uchar BM[32];           // 32
} FileEntry;

// structure use when reading or changing the current directory               
typedef struct
{
   uchar NumEntries;      // number of entries in path 0-10 
   char  Ref;             // reference character '\' or '.' 
   char  Entries[10][4];  // sub-directory entry names                                           
} DirectoryPath;

// internal file information structure
typedef struct 
{
   uchar Name[4];          // 4  File/Directory name
   uchar Ext;              // 1  File/Directory extension
   uchar Spage;            // 1  Start page
   uchar NumPgs;           // 1  Number of pages
   uchar Attrib;           // 1  Attribute 0x01 for readonly 0x02 for hidden
   uchar Dpage;            // 1  Directory page this entry is in
   uchar Offset;           // 1  Byte offset into directory page of entry
   uchar Read;             // 1  Read = 1 if opened and 0 if created 
   uchar PDpage;           // 1  Previous directory page number
} FileInfo;

// current directory structure
typedef struct 
{
   uchar ne;          // number of entries
   struct {
      uchar name[4];  // name and extention of the sub-directory
      PAGE_TYPE page; // page the subdirectory starts on  
      uchar attrib;   // attribute of the directory
   } Entry [10];
   uchar DRom[8];     // rom that this directory is valid for
} CurrentDirectory;  

// structure to hold the program job list ~ 7977 bytes                            
typedef struct
{                    
   uchar ERom[8];          // rom of the program job target
   uchar EBitmap[32];      // bit map kept for 
   uchar OrgBitmap[32];    // original bitmap
   struct {
      uchar     wr;        // flag to indicate this page needs to be written
      PAGE_TYPE len;       // length of data
      uchar     data[29];  // data to write 
      uchar     bm[4];     // bitmap for bytes to write 
   } Page[MAX_NUM_PGS];    // 256 possible jobs
} ProgramJob;   

// type to hold a data entry in the DHash
typedef struct
{
   uchar     ROM[8];        // rom of device page is from
   ulong     Tstamp;        // time stamp when page void
   uchar     Fptr;          // forward pointer to next entry in chain
   uchar     Bptr;          // back pointer to previous entry in chain
   uchar     Hptr;          // hash pointer to hash position
   PAGE_TYPE Page;          // page number   
   uchar     Data[32];      // page data including length
   uchar     Redir;         // redirection page
}  Dentry;
#endif //OWFILE_H

// function prototypes for owcache.c
void     InitDHash(void);  
uchar    AddPage(int portnum, uchar *SNum, PAGE_TYPE pg, uchar *buf, int len);        
SMALLINT FindPage(int portnum, uchar *SNum, PAGE_TYPE *page, uchar mflag, uchar time, 
                  uchar *buf, int *len, uchar *space_num);
uchar FreePage(uchar ptr);

// function prototypes for owfile.c
SMALLINT      owFirstFile(int, uchar *, FileEntry *); 
SMALLINT      owNextFile(int, uchar *, FileEntry *);
SMALLINT      owOpenFile(int, uchar *, FileEntry *,short *);
SMALLINT      owCreateFile(int, uchar *, int *, short *, FileEntry *);
SMALLINT      owCloseFile(int, uchar *, short);
SMALLINT      owReadFile(int, uchar *, short, uchar *, int,int *);
SMALLINT      owRemoveDir(int, uchar *, FileEntry *);
SMALLINT      owWriteFile(int, uchar *, short , uchar *, int );
SMALLINT      owDeleteFile(int, uchar *, FileEntry *);
SMALLINT      owFormat(int, uchar *);
SMALLINT      owAttribute(int, uchar *, short, FileEntry *);
SMALLINT      owReNameFile(int, uchar *, short, FileEntry *); 
SMALLINT      owChangeDirectory(int, uchar *, DirectoryPath *); 
SMALLINT      owCreateDir(int, uchar *, FileEntry *);
DirectoryPath owGetCurrentDir(int, uchar *);
SMALLINT      owWriteAddFile(int,uchar *,short,short,short,uchar *,int *);
SMALLINT      owTerminateAddFile(int,uchar *,FileEntry *);
SMALLINT      ReadBitMap(int, uchar *, uchar *);
SMALLINT      WriteBitMap(int, uchar *, uchar *);
SMALLINT      ReadNumEntry(int, uchar *, short , FileEntry *,
                           PAGE_TYPE *);
SMALLINT      GetMaxWrite(int, uchar *, uchar *);
SMALLINT      FindDirectoryInfo(int, uchar *, PAGE_TYPE *, FileInfo *); 
SMALLINT      ValidRom(int, uchar *);  
SMALLINT      BitMapChange(int, uchar *, uchar, uchar, uchar *);
SMALLINT      Valid_FileName(FileEntry *);
void          FindEmpties(int, uchar *, uchar *, PAGE_TYPE *, 
                          PAGE_TYPE *);
int           maxPages(int, uchar *);
void          ChangeRom(int portnum, uchar *SNum);
PAGE_TYPE     getLastPage(int, uchar *);

// function prototypes for owpgrw.c
SMALLINT Read_Page(int portnum, uchar *SNum, uchar *buff, uchar flag, 
                   PAGE_TYPE *pg, int *len);
SMALLINT Write_Page(int portnum, uchar *SNum, uchar *buff, PAGE_TYPE page, int len);
SMALLINT ExtWrite(int portnum, uchar *SNum, uchar strt_page, uchar *file, int fllen,
                  uchar *BM);
SMALLINT ExtRead(int portnum, uchar *SNum, uchar *file, int start_page, 
						int maxlen, uchar del, uchar *BM, int *fl_len);
SMALLINT ExtendedRead_Page(int portnum, uchar *SNum, uchar *buff, PAGE_TYPE pg);

// function prototypes for owprgm.c
SMALLINT owCreateProgramJob(int, uchar *); 
SMALLINT owDoProgramJob(int, uchar *); 
SMALLINT isJob(int,uchar *);
SMALLINT isJobWritten(int,uchar *,PAGE_TYPE);
SMALLINT getJobData(int,uchar *,PAGE_TYPE,uchar *,int *);
SMALLINT setJobData(int,uchar *,PAGE_TYPE,uchar *,int);
SMALLINT setJobWritten(int,uchar *,PAGE_TYPE,SMALLINT);
SMALLINT getOrigBM(int,uchar *,uchar *);
SMALLINT setProgramJob(int,uchar *,uchar,int);
SMALLINT getProgramJob(int,uchar *,uchar *,int);
SMALLINT WriteJobByte(int, uchar *, PAGE_TYPE, short, uchar, short);
SMALLINT TerminatePage(int, uchar *, short, uchar *);
