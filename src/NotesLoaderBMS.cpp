#include "global.h"
#include "NotesLoaderBMS.h"
#include "RageLog.h"
#include "GameManager.h"
#include "RageFile.h"
#include "StepsUtil.h"
#include "song.h"
#include "Steps.h"
#include "RageUtil_CharConversions.h"

/*	BMS encoding:     tap-hold
	4&8panel:   Player1     Player2
	Left		11-51		21-61
	Down		13-53		23-63
	Up			15-55		25-65
	Right		16-56		26-66

	6panel:	   Player1
	Left		11-51
	Left+Up		12-52
	Down		13-53
	Up			14-54
	Up+Right	15-55
	Right		16-56

	Notice that 15 and 25 have double meanings!  What were they thinking???
	While reading in, use the 6 panel mapping.  After reading in, detect if 
	only 4 notes are used.  If so, shift the Up+Right column back to the Up
	column

	Hey, folks, BMSes are used for things BESIDES DDR steps,
	and so we're borking up BMSes that are for pnm/bm/etc.

	pnm-nine:   11-15,22-25
	pnm-five:   13-15,21-22
	bm-single5: 11-16
	bm-double5: 11-16,21-26
	bm-single7: 11-16,18-19
	bm-double7: 11-16,18-19,21-26,28-29

	So the magics for these are:
	pnm-nine: nothing >5, with 12, 14, 22 and/or 24
	pnm-five: nothing >5, with 14 and/or 22
	bm-*: can't tell difference between bm-single and dance-solo
		18/19 marks bm-single7, 28/29 marks bm-double7
		bm-double uses 21-26. */

enum BmsTrack
{
	BMS_P1_KEY1 = 0,
	BMS_P1_KEY2,
	BMS_P1_KEY3,
	BMS_P1_KEY4,
	BMS_P1_KEY5,
	BMS_P1_TURN,
	BMS_P1_KEY6,
	BMS_P1_KEY7,
	BMS_P2_KEY1,
	BMS_P2_KEY2,
	BMS_P2_KEY3,
	BMS_P2_KEY4,
	BMS_P2_KEY5,
	BMS_P2_TURN,
	BMS_P2_KEY6,
	BMS_P2_KEY7,
	// max 4 simultaneous auto keysounds
	BMS_AUTO_KEYSOUND_1,
	BMS_AUTO_KEYSOUND_2,
	BMS_AUTO_KEYSOUND_3,
	BMS_AUTO_KEYSOUND_4,
	BMS_AUTO_KEYSOUND_5,
	BMS_AUTO_KEYSOUND_6,
	BMS_AUTO_KEYSOUND_7,
	BMS_AUTO_KEYSOUND_LAST,
	NUM_BMS_TRACKS,
};

const int NUM_NON_AUTO_KEYSOUND_TRACKS = BMS_AUTO_KEYSOUND_1;	
const int NUM_AUTO_KEYSOUND_TRACKS = NUM_BMS_TRACKS - NUM_NON_AUTO_KEYSOUND_TRACKS;	

static bool ConvertRawTrackToTapNote( int iRawTrack, BmsTrack &bmsTrackOut, bool &bIsHoldOut )
{
	if( iRawTrack > 40 )
	{
		bIsHoldOut = true;
		iRawTrack -= 40;
	}
	else
	{
		bIsHoldOut = false;
	}

	switch( iRawTrack )
	{
	case 1:  bmsTrackOut = BMS_AUTO_KEYSOUND_1;	break;
	case 11:	bmsTrackOut = BMS_P1_KEY1;		break;
	case 12:	bmsTrackOut = BMS_P1_KEY2;		break;
	case 13:	bmsTrackOut = BMS_P1_KEY3;		break;
	case 14:	bmsTrackOut = BMS_P1_KEY4;		break;
	case 15:	bmsTrackOut = BMS_P1_KEY5;		break;
	case 16:	bmsTrackOut = BMS_P1_TURN;		break;
	case 18:	bmsTrackOut = BMS_P1_KEY6;		break;
	case 19:	bmsTrackOut = BMS_P1_KEY7;		break;
	case 21:	bmsTrackOut = BMS_P2_KEY1;		break;
	case 22:	bmsTrackOut = BMS_P2_KEY2;		break;
	case 23:	bmsTrackOut = BMS_P2_KEY3;		break;
	case 24:	bmsTrackOut = BMS_P2_KEY4;		break;
	case 25:	bmsTrackOut = BMS_P2_KEY5;		break;
	case 26:	bmsTrackOut = BMS_P2_TURN;		break;
	case 28:	bmsTrackOut = BMS_P2_KEY6;		break;
	case 29:	bmsTrackOut = BMS_P2_KEY7;		break;
	default:	// unknown track
		return false;
	}
	return true;
}

// Find the largest common substring at the start of both strings.
static CString FindLargestInitialSubstring( CString string1, CString string2 )
{
	// First see if the whole first string matches an appropriately-sized
	// substring of the second, then keep chopping off the last character of
	// each until they match.
	unsigned i;
	for( i = 0; i < string1.size() && i < string2.size(); ++i )
		if( string1[i] != string2[i] )
			break;

	return string1.substr(0,i);
}

static StepsType DetermineStepsType( int iPlayer, const NoteData &nd )
{
	ASSERT( NUM_BMS_TRACKS == nd.GetNumTracks() );

	bool bTrackHasNote[NUM_NON_AUTO_KEYSOUND_TRACKS];
	ZERO( bTrackHasNote );

	int iLastRow = nd.GetLastRow();
	for( int t=0; t<NUM_NON_AUTO_KEYSOUND_TRACKS; t++ )
	{
		for( int r=0; r<=iLastRow; r++ )
		{
			if( nd.GetTapNote(t, r).type != TapNote::empty )
			{
				bTrackHasNote[t] = true;
				break;				
			}
		}
	}

	int iNumNonEmptyTracks = 0;
	for( int t=0; t<NUM_NON_AUTO_KEYSOUND_TRACKS; t++ )
		if( bTrackHasNote[t] )
			iNumNonEmptyTracks++;

	switch( iPlayer )
	{
	case 1:		// "1 player"		
		/*	Track counts:
			4 - DDR
			5 - PNM 5-key
			6 - DDR Solo, BM 5-key
			8 - BM 7-key
			9 - PNM 9-key */
		switch( iNumNonEmptyTracks ) 
		{
		case 4:		return STEPS_TYPE_DANCE_SINGLE;
		case 5:		return STEPS_TYPE_PNM_FIVE;
		case 6:
			// FIXME: There's no way to distinguish between these types.
			// They use the same tracks.  Assume it's a BM type since they
			// are more common.
			//return STEPS_TYPE_DANCE_SOLO;
			return STEPS_TYPE_BM_SINGLE5;
		case 8:		return STEPS_TYPE_BM_SINGLE7;
		case 9:		return STEPS_TYPE_PNM_NINE;
		default:	return STEPS_TYPE_INVALID;
		}
	case 2:		// couple/battle
		return STEPS_TYPE_DANCE_COUPLE;
	case 3:		// double
	/*	Track counts:
		8 - DDR Double
		12 - BM Double 5-key
		16 - BM Double 7-key */
		switch( iNumNonEmptyTracks ) 
		{
		case 8:		return STEPS_TYPE_BM_SINGLE7;
		case 12:	return STEPS_TYPE_BM_DOUBLE5;
		case 16:	return STEPS_TYPE_BM_DOUBLE7;
		default:	return STEPS_TYPE_INVALID;
		}
	default:
		LOG->Warn( "Invalid #PLAYER value %d", iPlayer );
		return STEPS_TYPE_INVALID;
	}
}

float BMSLoader::GetBeatsPerMeasure( const MeasureToTimeSig_t &sigs, int iMeasure )
{
	map<int, float>::const_iterator time_sig = sigs.find( iMeasure );

	float fRet = 4.0f;
	if( time_sig != sigs.end() )
	{
		fRet *= time_sig->second;
	}

	time_sig = m_TimeSigAdjustments.find( iMeasure );
	if( time_sig != m_TimeSigAdjustments.end() )
	{
		fRet *= time_sig->second;
	}

	return fRet;
}

int BMSLoader::GetMeasureStartRow( const MeasureToTimeSig_t &sigs, int iMeasureNo )
{
	int iRowNo = 0;
	for( int i = 0; i < iMeasureNo; ++i )
		iRowNo += BeatToNoteRow( GetBeatsPerMeasure(sigs, i) );
	return iRowNo;
}


void BMSLoader::SearchForDifficulty( CString sTag, Steps *pOut )
{
	sTag.MakeLower();

	/* Only match "Light" in parentheses. */
	if( sTag.find( "(light" ) != sTag.npos )
	{
		pOut->SetDifficulty( DIFFICULTY_EASY );
	}
	else if( sTag.find( "another" ) != sTag.npos )
	{
		pOut->SetDifficulty( DIFFICULTY_HARD );
	}
	else if( sTag.find( "(solo)" ) != sTag.npos )
	{
		pOut->SetDescription( "Solo" );
		pOut->SetDifficulty( DIFFICULTY_EDIT );
	}

	LOG->Trace( "Tag \"%s\" is %s", sTag.c_str(), DifficultyToString(pOut->GetDifficulty()).c_str() );
}

bool BMSLoader::LoadFromBMSFile( const CString &sPath, const NameToData_t &mapNameToData, Steps &out )
{
	LOG->Trace( "Steps::LoadFromBMSFile( '%s' )", sPath.c_str() );

	out.m_StepsType = STEPS_TYPE_INVALID;

	// BMS player code.  Fill in below and use to determine StepsType.
	int iPlayer = -1;
	CString sData;
	if( GetTagFromMap( mapNameToData, "#player", sData ) )
		iPlayer = atoi(sData);
	if( GetTagFromMap( mapNameToData, "#playlevel", sData ) )
		out.SetMeter( atoi(sData) );
	if( GetTagFromMap( mapNameToData, "#title", sData ) )
	{
		/* Hack: guess at 6-panel. */

		// extract the Steps description (looks like 'Music <BASIC>')
		size_t iOpenBracket = sData.find_first_of( "<(" );
		size_t iCloseBracket = sData.find_first_of( ">)" );

		if( iOpenBracket != string::npos && iCloseBracket != string::npos && iCloseBracket > iOpenBracket )
			sData = sData.substr( iOpenBracket+1, iCloseBracket-iOpenBracket-1 );

		// if there's a 6 in the description, it's probably part of "6panel" or "6-panel"
		if( sData.find("6") != CString::npos )
			out.m_StepsType = STEPS_TYPE_DANCE_SOLO;
	}

	NoteData ndNotes;
	ndNotes.SetNumTracks( NUM_BMS_TRACKS );

	/* Read time signatures.  Note that these can differ across files in the same
	 * song. */
	MeasureToTimeSig_t mapMeasureToTimeSig;
	ReadTimeSigs( mapNameToData, mapMeasureToTimeSig );

	NameToData_t::const_iterator it;
	for( it = mapNameToData.lower_bound("#00000"); it != mapNameToData.end(); ++it )
	{
		const CString &sName = it->first;
		if( sName.size() != 6 || sName[0] != '#' || !IsAnInt( sName.substr(1,5) ) )
			 continue;

		// this is step or offset data.  Looks like "#00705"
		int iMeasureNo = atoi( sName.substr(1,3).c_str() );
		int iRawTrackNum = atoi( sName.substr(4,2).c_str() );
		int iRowNo = GetMeasureStartRow( mapMeasureToTimeSig, iMeasureNo );
		float fBeatsPerMeasure = GetBeatsPerMeasure( mapMeasureToTimeSig, iMeasureNo );
		const CString &sNoteData = it->second;

		vector<TapNote> vTapNotes;
		for( CString::size_type i=0; i+1<sNoteData.length(); i+=2 )
		{
			CString sNoteId = sNoteData.substr(i,2);
			if( sNoteId != "00" )
			{
				TapNote tn = TAP_ORIGINAL_TAP;
				map<CString,int>::const_iterator it = m_mapWavIdToKeysoundIndex.find(sNoteId);
				if( it != m_mapWavIdToKeysoundIndex.end() )
				{
					tn.bKeysound = true;
					tn.iKeysoundIndex = it->second;
				}
				vTapNotes.push_back( tn );
			}
			else
			{
				vTapNotes.push_back( TAP_EMPTY );
			}
		}

		const unsigned uNumNotesInThisMeasure = vTapNotes.size();
		for( unsigned j=0; j<uNumNotesInThisMeasure; j++ )
		{
			if( vTapNotes[j].type != TapNote::empty )
			{
				float fPercentThroughMeasure = (float)j/(float)uNumNotesInThisMeasure;

				int row = iRowNo + lrintf( fPercentThroughMeasure * fBeatsPerMeasure * ROWS_PER_BEAT );

				// some BMS files seem to have funky alignment, causing us to write gigantic cache files.
				// Try to correct for this by quantizing.
				row = Quantize( row, ROWS_PER_MEASURE/64 );
					
				BmsTrack bmsTrack;
				bool bIsHold;
				if( ConvertRawTrackToTapNote(iRawTrackNum, bmsTrack, bIsHold) )
				{
					TapNote tn = vTapNotes[j];
					if( bmsTrack == BMS_AUTO_KEYSOUND_1 )
					{
						tn.type = TapNote::autoKeysound;

						// shift the auto keysound as far right as possible
						int iLastEmptyTrack = -1;
						if( ndNotes.GetTapLastEmptyTrack(row,iLastEmptyTrack)  &&
							iLastEmptyTrack >= BMS_AUTO_KEYSOUND_1 )
						{
							bmsTrack = (BmsTrack)iLastEmptyTrack;
						}
						else
						{
							// no room for this note.  Drop it.
							continue;
						}
					}
					else if( bIsHold )
					{
						tn.type = TapNote::hold_head;
					}
					else
					{
						tn.type = TapNote::tap;
					}

					ndNotes.SetTapNote( bmsTrack, row, tn );
				}
			}
		}
	}

	// we're done reading in all of the BMS values

	out.m_StepsType = DetermineStepsType( iPlayer, ndNotes );
	if( out.m_StepsType == STEPS_TYPE_INVALID )
	{
		LOG->Warn( "Couldn't determine note type of file '%s'", sPath.c_str() );
		return false;
	}


	// shift all of the autokeysound tracks onto the main tracks
	for( int t=BMS_AUTO_KEYSOUND_1; t<BMS_AUTO_KEYSOUND_1+NUM_AUTO_KEYSOUND_TRACKS; t++ )
	{
		FOREACH_NONEMPTY_ROW_IN_TRACK( ndNotes, t, row )
		{
			TapNote tn = ndNotes.GetTapNote( t, row );
			int iEmptyTrack;
			if( ndNotes.GetTapFirstEmptyTrack(row, iEmptyTrack) )
			{
				ndNotes.SetTapNote( iEmptyTrack, row, tn );
				ndNotes.SetTapNote( t, row, TAP_EMPTY );
			}
			else
			{
				LOG->Warn( "No room to shift." );
			}
		}
	}


	int iNumNewTracks = GameManager::StepsTypeToNumTracks( out.m_StepsType );
	vector<int> iTransformNewToOld;
	iTransformNewToOld.resize( iNumNewTracks, -1 );

	switch( out.m_StepsType )
	{
	case STEPS_TYPE_DANCE_SINGLE:
		iTransformNewToOld[0] = BMS_P1_KEY1;
		iTransformNewToOld[1] = BMS_P1_KEY3;
		iTransformNewToOld[2] = BMS_P1_KEY5;
		iTransformNewToOld[3] = BMS_P1_TURN;
		break;
	case STEPS_TYPE_DANCE_DOUBLE:
	case STEPS_TYPE_DANCE_COUPLE:
		iTransformNewToOld[0] = BMS_P1_KEY1;
		iTransformNewToOld[1] = BMS_P1_KEY3;
		iTransformNewToOld[2] = BMS_P1_KEY5;
		iTransformNewToOld[3] = BMS_P1_TURN;
		iTransformNewToOld[4] = BMS_P2_KEY1;
		iTransformNewToOld[5] = BMS_P2_KEY3;
		iTransformNewToOld[6] = BMS_P2_KEY5;
		iTransformNewToOld[7] = BMS_P2_TURN;
		break;
	case STEPS_TYPE_DANCE_SOLO:
	case STEPS_TYPE_BM_SINGLE5:
		// Hey! Why are these exactly the same? :-)
		iTransformNewToOld[0] = BMS_P1_KEY1;
		iTransformNewToOld[1] = BMS_P1_KEY2;
		iTransformNewToOld[2] = BMS_P1_KEY3;
		iTransformNewToOld[3] = BMS_P1_KEY4;
		iTransformNewToOld[4] = BMS_P1_KEY5;
		iTransformNewToOld[5] = BMS_P1_TURN;
		break;
	case STEPS_TYPE_PNM_FIVE:
		iTransformNewToOld[0] = BMS_P1_KEY3;
		iTransformNewToOld[1] = BMS_P1_KEY4;
		iTransformNewToOld[2] = BMS_P1_KEY5;
		// fix these columns!
		iTransformNewToOld[3] = BMS_P2_KEY2;
		iTransformNewToOld[4] = BMS_P2_KEY3;
		break;
	case STEPS_TYPE_PNM_NINE:
		iTransformNewToOld[0] = BMS_P1_KEY1; // lwhite
		iTransformNewToOld[1] = BMS_P1_KEY2; // lyellow
		iTransformNewToOld[2] = BMS_P1_KEY3; // lgreen
		iTransformNewToOld[3] = BMS_P1_KEY4; // lblue
		iTransformNewToOld[4] = BMS_P1_KEY5; // red
		// fix these columns!
		iTransformNewToOld[5] = BMS_P2_KEY2; // rblue
		iTransformNewToOld[6] = BMS_P2_KEY3; // rgreen
		iTransformNewToOld[7] = BMS_P2_KEY4; // ryellow
		iTransformNewToOld[8] = BMS_P2_KEY5; // rwhite
		break;
	case STEPS_TYPE_BM_DOUBLE5:
		iTransformNewToOld[0] = BMS_P1_KEY1;
		iTransformNewToOld[1] = BMS_P1_KEY2;
		iTransformNewToOld[2] = BMS_P1_KEY3;
		iTransformNewToOld[3] = BMS_P1_KEY4;
		iTransformNewToOld[4] = BMS_P1_KEY5;
		iTransformNewToOld[5] = BMS_P1_TURN;
		iTransformNewToOld[6] = BMS_P2_KEY1;
		iTransformNewToOld[7] = BMS_P2_KEY2;
		iTransformNewToOld[8] = BMS_P2_KEY3;
		iTransformNewToOld[9] = BMS_P2_KEY4;
		iTransformNewToOld[10] = BMS_P2_KEY5;
		iTransformNewToOld[11] = BMS_P2_TURN;
		break;
	case STEPS_TYPE_BM_SINGLE7:
		iTransformNewToOld[0] = BMS_P1_KEY1;
		iTransformNewToOld[1] = BMS_P1_KEY2;
		iTransformNewToOld[2] = BMS_P1_KEY3;
		iTransformNewToOld[3] = BMS_P1_KEY4;
		iTransformNewToOld[4] = BMS_P1_KEY5;
		iTransformNewToOld[5] = BMS_P1_KEY6;
		iTransformNewToOld[6] = BMS_P1_KEY7;
		iTransformNewToOld[7] = BMS_P1_TURN;
		break;
	case STEPS_TYPE_BM_DOUBLE7:
		iTransformNewToOld[0] = BMS_P1_KEY1;
		iTransformNewToOld[1] = BMS_P1_KEY2;
		iTransformNewToOld[2] = BMS_P1_KEY3;
		iTransformNewToOld[3] = BMS_P1_KEY4;
		iTransformNewToOld[4] = BMS_P1_KEY5;
		iTransformNewToOld[5] = BMS_P1_KEY6;
		iTransformNewToOld[6] = BMS_P1_KEY7;
		iTransformNewToOld[7] = BMS_P1_TURN;
		iTransformNewToOld[8] = BMS_P2_KEY1;
		iTransformNewToOld[9] = BMS_P2_KEY2;
		iTransformNewToOld[10] = BMS_P2_KEY3;
		iTransformNewToOld[11] = BMS_P2_KEY4;
		iTransformNewToOld[12] = BMS_P2_KEY5;
		iTransformNewToOld[13] = BMS_P2_KEY6;
		iTransformNewToOld[14] = BMS_P2_KEY7;
		iTransformNewToOld[15] = BMS_P2_TURN;
		break;
	default:
		ASSERT(0);
	}

	NoteData noteData2;
	noteData2.SetNumTracks( iNumNewTracks );
	noteData2.LoadTransformed( ndNotes, iNumNewTracks, &*iTransformNewToOld.begin() );

	out.SetNoteData( noteData2 );

	out.TidyUpData();

	return true;
}

void BMSLoader::GetApplicableFiles( CString sPath, CStringArray &out )
{
	GetDirListing( sPath + CString("*.bms"), out );
	GetDirListing( sPath + CString("*.bme"), out );
}

bool BMSLoader::ReadBMSFile( const CString &sPath, NameToData_t &mapNameToData )
{
	RageFile file;
	if( !file.Open(sPath) )
		RageException::Throw( "Failed to open \"%s\" for reading: %s", sPath.c_str(), file.GetError().c_str() );
	while( !file.AtEOF() )
	{
		CString line;
		if( file.GetLine( line ) == -1 )
		{
			LOG->Warn( "Error reading \"%s\": %s", sPath.c_str(), file.GetError().c_str() );
			return false;
		}

		StripCrnl(line);

		// BMS value names can be separated by a space or a colon.
		size_t iIndexOfSeparator = line.find_first_of( ": " );
		CString value_name = line.substr( 0, iIndexOfSeparator );
		CString value_data;
		if( iIndexOfSeparator != line.npos )
			value_data = line.substr( iIndexOfSeparator+1 );

		value_name.MakeLower();
		mapNameToData.insert( pair<CString,CString>(value_name, value_data) );
	}

	return true;
}

bool BMSLoader::GetTagFromMap( const NameToData_t &mapNameToData, const CString &sName, CString &sOut )
{
	NameToData_t::const_iterator it;
	it = mapNameToData.find( sName );
	if( it == mapNameToData.end() )
		return false;

	sOut = it->second;
	
	return true;
}

/* Finds the longest common match for the given tag in all files.  If the given tag
 * was found in at least one file, returns true; otherwise returns false. */
bool BMSLoader::GetCommonTagFromMapList( const vector<NameToData_t> &aBMSData, const CString &sName, CString &sCommonTag )
{
	bool bFoundOne = false;
	for( unsigned i=0; i < aBMSData.size(); i++ )
	{
		CString sTag;
		if( !GetTagFromMap( aBMSData[i], sName, sTag ) )
			continue;

		if( !bFoundOne )
		{
			bFoundOne = true;
			sCommonTag = sTag;
		}
		else
			sCommonTag = FindLargestInitialSubstring( sCommonTag, sTag );
	}

	return bFoundOne;
}

enum
{
	BMS_TRACK_TIME_SIG = 2,
	BMS_TRACK_BPM = 3,
	BMS_TRACK_BPM_REF = 8,
	BMS_TRACK_STOP = 9
};

void BMSLoader::ReadGlobalTags( const NameToData_t &mapNameToData, Song &out )
{
	CString sData;
	if( GetTagFromMap( mapNameToData, "#title", sData ) )
		GetMainAndSubTitlesFromFullTitle( sData, out.m_sMainTitle, out.m_sSubTitle );

	GetTagFromMap( mapNameToData, "#artist", out.m_sArtist );
	GetTagFromMap( mapNameToData, "#backbmp", out.m_sBackgroundFile );
	GetTagFromMap( mapNameToData, "#wav", out.m_sMusicFile );

	if( GetTagFromMap( mapNameToData, "#bpm", sData ) )
	{
		BPMSegment newSeg( 0, strtof(sData, NULL) );
		out.AddBPMSegment( newSeg );

		LOG->Trace( "Inserting new BPM change at beat %f, BPM %f", NoteRowToBeat(newSeg.m_iStartIndex), newSeg.GetBPM() );
	}

	NameToData_t::const_iterator it;
	for( it = mapNameToData.lower_bound("#wav"); it != mapNameToData.end(); ++it )
	{
		const CString &sName = it->first;

		if( sName.size() != 6 || sName.Left(4) != "#wav" )
			continue;

		// this is keysound file name.  Looks like "#WAV1A"
		CString sData = it->second;
		CString sWavID = sName.Right(2);

		/* Due to bugs in some programs, many BMS files have a "WAV" extension
		 * on files in the BMS for files that actually have some other extension.
		 * Do a search.  Don't do a wildcard search; if sData is "song.wav",
		 * we might also have "song.png", which we shouldn't match. */
		if( !IsAFile(out.GetSongDir()+sData) )
		{
			const char *exts[] = { "ogg", "wav", "mp3", NULL }; // XXX: stop duplicating these everywhere
			for( unsigned i = 0; exts[i] != NULL; ++i )
			{
				CString fn = SetExtension( sData, exts[i] );
				if( IsAFile(out.GetSongDir()+fn) )
				{
					sData = fn;
					break;
				}
			}
		}
		if( !IsAFile(out.GetSongDir()+sData) )
			LOG->Trace( "Song \"%s\" references key \"%s\" that can't be found",
				m_sDir.c_str(), sData.c_str() );

		sWavID.MakeUpper();		// HACK: undo the MakeLower()
		out.m_vsKeysoundFile.push_back( sData );
		m_mapWavIdToKeysoundIndex[ sWavID ] = out.m_vsKeysoundFile.size()-1;
		LOG->Trace( "Inserting keysound index %u '%s'", unsigned(out.m_vsKeysoundFile.size()-1), sWavID.c_str() );
	}

	/* Time signature tags affect all other global timing tags, so read them first. */
	MeasureToTimeSig_t mapMeasureToTimeSig;
	ReadTimeSigs( mapNameToData, mapMeasureToTimeSig );

	for( it = mapNameToData.lower_bound("#00000"); it != mapNameToData.end(); ++it )
	{
		const CString &sName = it->first;
		if( sName.size() != 6 || sName[0] != '#' || !IsAnInt( sName.substr(1,5) ) )
			 continue;
		// this is step or offset data.  Looks like "#00705"
		int iMeasureNo	= atoi( sName.substr(1,3).c_str() );
		int iBMSTrackNo	= atoi( sName.substr(4,2).c_str() );
		int iStepIndex = GetMeasureStartRow( mapMeasureToTimeSig, iMeasureNo );
		float fBeatsPerMeasure = GetBeatsPerMeasure( mapMeasureToTimeSig, iMeasureNo );
		int iRowsPerMeasure = BeatToNoteRow( fBeatsPerMeasure );

		CString sData = it->second;
		int totalPairs = sData.size() / 2;
		for( int i = 0; i < totalPairs; ++i )
		{
			CString sPair = sData.substr(i*2,2);

			int iVal = 0;
			if( sscanf( sPair, "%x", &iVal ) == 0 || iVal == 0 )
				continue;

			int iRow = iStepIndex + (i * iRowsPerMeasure) / totalPairs;
			float fBeat = NoteRowToBeat( iRow );

			switch( iBMSTrackNo )
			{
			case BMS_TRACK_BPM:
				out.SetBPMAtBeat( fBeat, (float) iVal );
				LOG->Trace( "Inserting new BPM change at beat %f, BPM %i", fBeat, iVal );
				break;

			case BMS_TRACK_BPM_REF:
			{
				CString sTagToLookFor = ssprintf( "#bpm%02x", iVal );
				CString sBPM;
				if( GetTagFromMap( mapNameToData, sTagToLookFor, sBPM ) )
				{
					float fBPM = strtof( sBPM, NULL );

					BPMSegment newSeg;
					newSeg.m_iStartIndex = BeatToNoteRow(fBeat);
					newSeg.SetBPM( fBPM );
					out.AddBPMSegment( newSeg );
					LOG->Trace( "Inserting new BPM change at beat %f, BPM %f", fBeat, newSeg.GetBPM() );
				}
				else
					LOG->Warn( "Couldn't find tag '%s' in '%s'.", sTagToLookFor.c_str(), m_sDir.c_str() );
				break;
			}
			case BMS_TRACK_STOP:
			{
				CString sTagToLookFor = ssprintf( "#stop%02x", iVal );
				CString sBeats;
				if( GetTagFromMap( mapNameToData, sTagToLookFor, sBeats ) )
				{
					// find the BPM at the time of this freeze
					float fBPS = out.m_Timing.GetBPMAtBeat(fBeat) / 60.0f;
					float fBeats = strtof(sBeats,NULL) / 48.0f;
					float fFreezeSecs = fBeats / fBPS;

					StopSegment newSeg( BeatToNoteRow(fBeat), fFreezeSecs );
					out.AddStopSegment( newSeg );
					LOG->Trace( "Inserting new Freeze at beat %f, secs %f", fBeat, newSeg.m_fStopSeconds );
				}
				else
					LOG->Warn( "Couldn't find tag '%s' in '%s'.", sTagToLookFor.c_str(), m_sDir.c_str() );
				break;
			}
			}
		}
			
		switch( iBMSTrackNo )
		{
		case BMS_TRACK_BPM_REF:
		{
			// XXX: offset
			int iBPMNo;
			sscanf( sData, "%x", &iBPMNo );	// data is in hexadecimal

			CString sBPM;
			CString sTagToLookFor = ssprintf( "#bpm%02x", iBPMNo );
			if( GetTagFromMap( mapNameToData, sTagToLookFor, sBPM ) )
			{
				float fBPM = strtof( sBPM, NULL );

				BPMSegment newSeg;
				newSeg.m_iStartIndex = iStepIndex;
				newSeg.SetBPM( fBPM );
				out.AddBPMSegment( newSeg );
				LOG->Trace( "Inserting new BPM change at beat %f, BPM %f", NoteRowToBeat(newSeg.m_iStartIndex), newSeg.GetBPM() );
			}
			else
				LOG->Warn( "Couldn't find tag '%s' in '%s'.", sTagToLookFor.c_str(), m_sDir.c_str() );

			break;
		}
		}
	}

	/* Now that we're done reading BPMs, factor out weird time signatures. */
	SetTimeSigAdjustments( mapMeasureToTimeSig, &out );
}

void BMSLoader::ReadTimeSigs( const NameToData_t &mapNameToData, MeasureToTimeSig_t &out )
{
	NameToData_t::const_iterator it;
	for( it = mapNameToData.lower_bound("#00000"); it != mapNameToData.end(); ++it )
	{
		const CString &sName = it->first;
		if( sName.size() != 6 || sName[0] != '#' || !IsAnInt( sName.substr(1,5) ) )
			 continue;

		// this is step or offset data.  Looks like "#00705"
		const CString &sData = it->second;
		int iMeasureNo	= atoi( sName.substr(1,3).c_str() );
		int iBMSTrackNo	= atoi( sName.substr(4,2).c_str() );
		if( iBMSTrackNo == BMS_TRACK_TIME_SIG )
			out[iMeasureNo] = (float) atof(sData);
	}
}

/*
 * Time signatures are often abused to tweak sync.  Real time signatures should
 * cause us to adjust the row offsets so one beat remains one beat.  Fake time signatures,
 * like 1.001 or 0.999, should be removed and converted to BPM changes.  This is much
 * more accurate, and prevents the whole song from being shifted off of the beat, causing
 * BeatToNoteType to be wrong.
 *
 * Evaluate each time signature, and guess which time signatures should be converted
 * to BPM changes.  This isn't perfect, but errors aren't fatal.
 */
void BMSLoader::SetTimeSigAdjustments( const MeasureToTimeSig_t &sigs, Song *pOut )
{
	return;
	m_TimeSigAdjustments.clear();

	MeasureToTimeSig_t::const_iterator it;
	for( it = sigs.begin(); it != sigs.end(); ++it )
	{
		int iMeasure = it->first;
		float fFactor = it->second;

#if 1
		static const float ValidFactors[] =
		{
			0.25f,  /* 1/4 */
			0.5f,   /* 2/4 */
			0.75f,  /* 3/4 */
			0.875f, /* 7/8 */
			1.0f,
			1.5f,   /* 6/4 */
			1.75f   /* 7/4 */
		};

		bool bValidTimeSignature = false;
		for( unsigned i = 0; i < ARRAYLEN(ValidFactors); ++i )
			if( fabsf(fFactor-ValidFactors[i]) < 0.001 )
				bValidTimeSignature = true;

		if( bValidTimeSignature )
			continue;
#else
		/* Alternate approach that I tried first: see if the ratio is sane.  However,
		 * some songs have values like "1.4", which comes out to 7/4 and is not a valid
		 * time signature. */
		/* Convert the factor to a ratio, and reduce it. */
		int iNum = lrintf( fFactor * 1000 ), iDen = 1000;
		int iDiv = gcd( iNum, iDen );
		iNum /= iDiv;
		iDen /= iDiv;

		/* Real time signatures usually come down to 1/2, 3/4, 7/8, etc.  Bogus
		 * signatures that are only there to adjust sync usually look like 99/100. */
		if( iNum <= 8 && iDen <= 8 )
			continue;
#endif

		/* This time signature is bogus.  Convert it to a BPM adjustment for this
		 * measure. */
		LOG->Trace("Converted time signature %f in measure %i to a BPM segment.", fFactor, iMeasure );

		/* Note that this GetMeasureStartRow will automatically include any adjustments
		 * that we've made previously in this loop; as long as we make the timing
		 * adjustment and the BPM adjustment together, everything remains consistent.
		 * Adjust m_TimeSigAdjustments first, or fAdjustmentEndBeat will be wrong. */
		m_TimeSigAdjustments[iMeasure] = 1.0f / fFactor;
		int iAdjustmentStartRow = GetMeasureStartRow( sigs, iMeasure );
		int iAdjustmentEndRow = GetMeasureStartRow( sigs, iMeasure+1 );
		pOut->m_Timing.MultiplyBPMInBeatRange( iAdjustmentStartRow, iAdjustmentEndRow, 1.0f / fFactor );
	}
}

bool BMSLoader::LoadFromDir( CString sDir, Song &out )
{
	LOG->Trace( "Song::LoadFromBMSDir(%s)", sDir.c_str() );

	ASSERT( out.m_vsKeysoundFile.empty() );

	m_sDir = sDir;
	CStringArray arrayBMSFileNames;
	GetApplicableFiles( sDir, arrayBMSFileNames );

	/* We should have at least one; if we had none, we shouldn't have been
	 * called to begin with. */
	ASSERT( arrayBMSFileNames.size() );

	/* Read all BMS files. */
	vector<NameToData_t> aBMSData;
	for( unsigned i=0; i<arrayBMSFileNames.size(); i++ )
	{
		aBMSData.push_back( NameToData_t() );
		ReadBMSFile( out.GetSongDir() + arrayBMSFileNames[i], aBMSData.back() );
	}

	CString commonSubstring;
	GetCommonTagFromMapList( aBMSData, "#title", commonSubstring );

	if( commonSubstring == "" )
	{
		// All bets are off; the titles don't match at all.
		// At this rate we're lucky if we even get the title right.
		LOG->Warn("BMS files in %s have inconsistent titles", sDir.c_str() );
	}

	/* Create a Steps for each. */
	vector<Steps*> apSteps;
	for( unsigned i=0; i<arrayBMSFileNames.size(); i++ )
		apSteps.push_back( new Steps );
		
	// Now, with our fancy little substring, trim the titles and
	// figure out where each goes.
	for( unsigned i=0; i<aBMSData.size(); i++ )
	{
		Steps *pSteps = apSteps[i];
		pSteps->SetDifficulty( DIFFICULTY_MEDIUM );
		CString sTag;
		if( GetTagFromMap( aBMSData[i], "#title", sTag ) && sTag.size() != commonSubstring.size() )
		{
			sTag = sTag.substr( commonSubstring.size(), sTag.size() - commonSubstring.size() );
			sTag.MakeLower();

			// XXX: Someone find me some DDR BMS examples!

			// XXX: We should do this with filenames too, I have plenty of examples.
			//      however, filenames will be trickier, as stuffs at the beginning AND
			//      end change per-file, so we'll need a fancier FindLargestInitialSubstring()

			// XXX: This matches (double), but I haven't seen it used. Again, MORE EXAMPLES NEEDED
			if( sTag.find('l') != sTag.npos )
			{
				unsigned lPos = sTag.find('l');
				if( lPos > 2 && sTag.substr(lPos-2,4) == "solo" )
				{
					// (solo) -- an edit, apparently (Thanks Glenn!)
					pSteps->SetDifficulty( DIFFICULTY_EDIT );
				}
				else
				{
					// Any of [L7] [L14] (LIGHT7) (LIGHT14) (LIGHT) [L] <LIGHT7> <L7>... you get the idea.
					pSteps->SetDifficulty( DIFFICULTY_EASY );
				}
			}
			// [A] <A> (A) [ANOTHER] <ANOTHER> (ANOTHER) (ANOTHER7) Another (DP ANOTHER) (Another) -ANOTHER- [A7] [A14] etc etc etc
			else if( sTag.find('a') != sTag.npos )
				pSteps->SetDifficulty( DIFFICULTY_HARD );
			// XXX: Can also match (double), but should match [B] or [B7]
			else if( sTag.find('b') != sTag.npos )
				pSteps->SetDifficulty( DIFFICULTY_BEGINNER );
			// Other tags I've seen here include (5KEYS) (10KEYS) (7keys) (14keys) (dp) [MIX] [14] (14 Keys Mix)
			// XXX: I'm sure [MIX] means something... anyone know?
		}
	}

	if( commonSubstring == "" )
	{
		// As said before, all bets are off.
		// From here on in, it's nothing but guesswork.

		/* Try to figure out the difficulty of each file. */
		for( unsigned i=0; i<arrayBMSFileNames.size(); i++ )
		{
			// XXX: Is this really effective if Common Substring parsing failed?
			Steps *pSteps = apSteps[i];
			pSteps->SetDifficulty( DIFFICULTY_MEDIUM );
			CString sTag;
			if( GetTagFromMap( aBMSData[i], "#title", sTag ) )
				SearchForDifficulty( sTag, pSteps );
			if( GetTagFromMap( aBMSData[i], "#genre", sTag ) )
				SearchForDifficulty( sTag, pSteps );
		}
	}

	/* Prefer to read global tags from a DIFFICULTY_MEDIUM file.  These tend to
	 * have the least cruft in the #TITLE tag, so it's more likely to get a clean
	 * title. */
	int iMainDataIndex = 0;
	for( unsigned i=1; i<apSteps.size(); i++ )
		if( apSteps[i]->GetDifficulty() == DIFFICULTY_MEDIUM )
			iMainDataIndex = i;

	ReadGlobalTags( aBMSData[iMainDataIndex], out );
	    
	// Override what that global tag said about the title if we have a good substring.
	// Prevents clobbering and catches "D2R (7keys)" / "D2R (Another) (7keys)"
	// Also catches "D2R (7keys)" / "D2R (14keys)"
	if( commonSubstring != "" )
		GetMainAndSubTitlesFromFullTitle( commonSubstring, out.m_sMainTitle, out.m_sSubTitle );

	// Now that we've parsed the keysound data, load the Steps from the rest 
	// of the .bms files.
	for( unsigned i=0; i<arrayBMSFileNames.size(); i++ )
	{
		Steps* pNewNotes = apSteps[i];
		const bool ok = LoadFromBMSFile( out.GetSongDir() + arrayBMSFileNames[i], aBMSData[i], *pNewNotes );
		if( ok )
			out.AddSteps( pNewNotes );
		else
			delete pNewNotes;
	}

	SlideDuplicateDifficulties( out );

	ConvertString( out.m_sMainTitle, "utf-8,japanese" );
	ConvertString( out.m_sArtist, "utf-8,japanese" );


	return true;
}


void BMSLoader::SlideDuplicateDifficulties( Song &p )
{
	/* BMS files have to guess the Difficulty from the meter; this is inaccurate,
	 * and often leads to duplicates.  Slide duplicate difficulties upwards.  We
	 * only do this with BMS files, since a very common bug was having *all*
	 * difficulties slid upwards due to (for example) having two beginner steps.
	 * We do a second pass in Song::TidyUpData to eliminate any remaining duplicates
	 * after this. */
	FOREACH_StepsType( st )
	{
		FOREACH_Difficulty( dc )
		{
			if( dc == DIFFICULTY_EDIT )
				continue;

			vector<Steps*> vSteps;
			p.GetSteps( vSteps, st, dc );

			StepsUtil::SortNotesArrayByDifficulty( vSteps );
			for( unsigned k=1; k<vSteps.size(); k++ )
			{
				Steps* pSteps = vSteps[k];
			
				Difficulty dc2 = min( (Difficulty)(dc+1), DIFFICULTY_CHALLENGE );
				pSteps->SetDifficulty( dc2 );
			}
		}
	}
}

/*
 * (c) 2001-2004 Chris Danford, Glenn Maynard
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
