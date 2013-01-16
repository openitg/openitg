#include "global.h"
#include "NotesLoaderDWI.h"
#include "RageLog.h"
#include "MsdFile.h"
#include "RageUtil_CharConversions.h"
#include "song.h"
#include "Steps.h"

#include <map>
using namespace std;

static std::map<int,int> g_mapDanceNoteToNoteDataColumn;

enum
{
	DANCE_NOTE_NONE = 0,
	DANCE_NOTE_PAD1_LEFT,
	DANCE_NOTE_PAD1_UPLEFT,
	DANCE_NOTE_PAD1_DOWN,
	DANCE_NOTE_PAD1_UP,
	DANCE_NOTE_PAD1_UPRIGHT,
	DANCE_NOTE_PAD1_RIGHT,
	DANCE_NOTE_PAD2_LEFT,
	DANCE_NOTE_PAD2_UPLEFT,
	DANCE_NOTE_PAD2_DOWN,
	DANCE_NOTE_PAD2_UP,
	DANCE_NOTE_PAD2_UPRIGHT,
	DANCE_NOTE_PAD2_RIGHT
};

void DWILoader::DWIcharToNote( char c, GameController i, int &note1Out, int &note2Out )
{
	switch( c )
		{
		case '0':	note1Out = DANCE_NOTE_NONE;			note2Out = DANCE_NOTE_NONE;			break;
		case '1':	note1Out = DANCE_NOTE_PAD1_DOWN;	note2Out = DANCE_NOTE_PAD1_LEFT;	break;
		case '2':	note1Out = DANCE_NOTE_PAD1_DOWN;	note2Out = DANCE_NOTE_NONE;			break;
		case '3':	note1Out = DANCE_NOTE_PAD1_DOWN;	note2Out = DANCE_NOTE_PAD1_RIGHT;	break;
		case '4':	note1Out = DANCE_NOTE_PAD1_LEFT;	note2Out = DANCE_NOTE_NONE;			break;
		case '5':	note1Out = DANCE_NOTE_NONE;			note2Out = DANCE_NOTE_NONE;			break;
		case '6':	note1Out = DANCE_NOTE_PAD1_RIGHT;	note2Out = DANCE_NOTE_NONE;			break;
		case '7':	note1Out = DANCE_NOTE_PAD1_UP;		note2Out = DANCE_NOTE_PAD1_LEFT;	break;
		case '8':	note1Out = DANCE_NOTE_PAD1_UP;		note2Out = DANCE_NOTE_NONE;			break;
		case '9':	note1Out = DANCE_NOTE_PAD1_UP;		note2Out = DANCE_NOTE_PAD1_RIGHT;	break;
		case 'A':	note1Out = DANCE_NOTE_PAD1_UP;		note2Out = DANCE_NOTE_PAD1_DOWN;	break;
		case 'B':	note1Out = DANCE_NOTE_PAD1_LEFT;	note2Out = DANCE_NOTE_PAD1_RIGHT;	break;
		case 'C':	note1Out = DANCE_NOTE_PAD1_UPLEFT;	note2Out = DANCE_NOTE_NONE;			break;
		case 'D':	note1Out = DANCE_NOTE_PAD1_UPRIGHT;	note2Out = DANCE_NOTE_NONE;			break;
		case 'E':	note1Out = DANCE_NOTE_PAD1_LEFT;	note2Out = DANCE_NOTE_PAD1_UPLEFT;	break;
		case 'F':	note1Out = DANCE_NOTE_PAD1_UPLEFT;	note2Out = DANCE_NOTE_PAD1_DOWN;	break;
		case 'G':	note1Out = DANCE_NOTE_PAD1_UPLEFT;	note2Out = DANCE_NOTE_PAD1_UP;		break;
		case 'H':	note1Out = DANCE_NOTE_PAD1_UPLEFT;	note2Out = DANCE_NOTE_PAD1_RIGHT;	break;
		case 'I':	note1Out = DANCE_NOTE_PAD1_LEFT;	note2Out = DANCE_NOTE_PAD1_UPRIGHT;	break;
		case 'J':	note1Out = DANCE_NOTE_PAD1_DOWN;	note2Out = DANCE_NOTE_PAD1_UPRIGHT;	break;
		case 'K':	note1Out = DANCE_NOTE_PAD1_UP;		note2Out = DANCE_NOTE_PAD1_UPRIGHT;	break;
		case 'L':	note1Out = DANCE_NOTE_PAD1_UPRIGHT;	note2Out = DANCE_NOTE_PAD1_RIGHT;	break;
		case 'M':	note1Out = DANCE_NOTE_PAD1_UPLEFT;	note2Out = DANCE_NOTE_PAD1_UPRIGHT;	break;
		default:	
			LOG->Warn( "Encountered invalid DWI note character '%c' in '%s'", c, m_sLoadingFile.c_str() );
			note1Out = DANCE_NOTE_NONE;			note2Out = DANCE_NOTE_NONE;			break;
	}

	switch( i )
	{
	case GAME_CONTROLLER_1:
		break;
	case GAME_CONTROLLER_2:
		if( note1Out != DANCE_NOTE_NONE )
			note1Out += 6;
		if( note2Out != DANCE_NOTE_NONE )
			note2Out += 6;
		break;
	default:
		ASSERT( false );
	}
}

void DWILoader::DWIcharToNoteCol( char c, GameController i, int &col1Out, int &col2Out )
{
	int note1, note2;
	DWIcharToNote( c, i, note1, note2 );

	if( note1 != DANCE_NOTE_NONE )
		col1Out = g_mapDanceNoteToNoteDataColumn[note1];
	else
		col1Out = -1;

	if( note2 != DANCE_NOTE_NONE )
		col2Out = g_mapDanceNoteToNoteDataColumn[note2];
	else
		col2Out = -1;
}

/* Ack.  DWI used to use <...> to indicate 1/192nd notes; at some
 * point, <...> was changed to indicate jumps, and `' was used for
 * 1/192nds.  So, we have to do a check to figure out what it really
 * means.  If it contains 0s, it's most likely 192nds; otherwise,
 * it's most likely a jump.  Search for a 0 before the next >: */
bool DWILoader::Is192( const CString &sStepData, int pos )
{
	while( pos < (int) sStepData.size() )
	{
		if( sStepData[pos] == '>' )
			return false;
		if( sStepData[pos] == '0' )
			return true;
		++pos;
	}
	
	return false;
}

bool DWILoader::LoadFromDWITokens( 
	CString sMode, 
	CString sDescription,
	CString sNumFeet,
	CString sStepData1, 
	CString sStepData2,
	Steps &out)
{
	CHECKPOINT_M( "DWILoader::LoadFromDWITokens()" );

	out.m_StepsType = STEPS_TYPE_INVALID;

	if(		 sMode == "SINGLE" )	out.m_StepsType = STEPS_TYPE_DANCE_SINGLE;
	else if( sMode == "DOUBLE" )	out.m_StepsType = STEPS_TYPE_DANCE_DOUBLE;
	else if( sMode == "COUPLE" )	out.m_StepsType = STEPS_TYPE_DANCE_COUPLE;
	else if( sMode == "SOLO" )		out.m_StepsType = STEPS_TYPE_DANCE_SOLO;
	else	
	{
		ASSERT(0);	// Unrecognized DWI notes format
		out.m_StepsType = STEPS_TYPE_DANCE_SINGLE;
	}


	g_mapDanceNoteToNoteDataColumn.clear();
	switch( out.m_StepsType )
	{
	case STEPS_TYPE_DANCE_SINGLE:
		g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_LEFT] = 0;
		g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_DOWN] = 1;
		g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_UP] = 2;
		g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_RIGHT] = 3;
		break;
	case STEPS_TYPE_DANCE_DOUBLE:
	case STEPS_TYPE_DANCE_COUPLE:
		g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_LEFT] = 0;
		g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_DOWN] = 1;
		g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_UP] = 2;
		g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_RIGHT] = 3;
		g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD2_LEFT] = 4;
		g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD2_DOWN] = 5;
		g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD2_UP] = 6;
		g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD2_RIGHT] = 7;
		break;
	case STEPS_TYPE_DANCE_SOLO:
		g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_LEFT] = 0;
		g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_UPLEFT] = 1;
		g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_DOWN] = 2;
		g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_UP] = 3;
		g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_UPRIGHT] = 4;
		g_mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_RIGHT] = 5;
		break;
	default:
		ASSERT(0);
	}

	int iNumFeet = atoi(sNumFeet);
	// out.SetDescription(sDescription); // Don't put garbage in the description.
	out.SetMeter(iNumFeet);
	out.SetDifficulty( StringToDifficulty(sDescription) );

	NoteData newNoteData;
	newNoteData.SetNumTracks( g_mapDanceNoteToNoteDataColumn.size() );

	for( int pad=0; pad<2; pad++ )		// foreach pad
	{
		CString sStepData;
		switch( pad )
		{
		case 0:
			sStepData = sStepData1;
			break;
		case 1:
			if( sStepData2 == "" )	// no data
				continue;	// skip
			sStepData = sStepData2;
			break;
		default:
			ASSERT( false );
		}

		sStepData.Replace("\n", "");
		sStepData.Replace("\r", "");
		sStepData.Replace("\t", "");
		sStepData.Replace(" ", "");

		double fCurrentBeat = 0;
		double fCurrentIncrementer = 1.0/8 * BEATS_PER_MEASURE;

		for( CString::size_type i=0; i<sStepData.length(); )
		{
			char c = sStepData[i++];
			switch( c )
			{
			// begins a series
			case '(':
				fCurrentIncrementer = 1.0/16 * BEATS_PER_MEASURE;
				break;
			case '[':
				fCurrentIncrementer = 1.0/24 * BEATS_PER_MEASURE;
				break;
			case '{':
				fCurrentIncrementer = 1.0/64 * BEATS_PER_MEASURE;
				break;
			case '`':
				fCurrentIncrementer = 1.0/192 * BEATS_PER_MEASURE;
				break;

			// ends a series
			case ')':
			case ']':
			case '}':
			case '\'':
			case '>':
				fCurrentIncrementer = 1.0/8 * BEATS_PER_MEASURE;
				break;

			default:	// this is a note character
			{
				if( c == '!' )
				{
					LOG->Warn( "Unexpected character in '%s': '!'", m_sLoadingFile.c_str() );
					continue;
				}

				bool jump = false;
				if( c == '<' )
				{
					/* Arr.  Is this a jump or a 1/192 marker? */
					if( Is192( sStepData, i ) )
					{
						fCurrentIncrementer = 1.0/192 * BEATS_PER_MEASURE;
						break;
					}
					
					/* It's a jump.  We need to keep reading notes until we hit a >. */
					jump = true;
					i++;
				}
				
				const int iIndex = BeatToNoteRow( (float)fCurrentBeat );
				i--;
				do {
					c = sStepData[i++];

					if( jump && c == '>' )
						break;

					int iCol1, iCol2;
					DWIcharToNoteCol( c, (GameController)pad, iCol1, iCol2 );

					if( iCol1 != -1 )
						newNoteData.SetTapNote(iCol1, iIndex, TAP_ORIGINAL_TAP);
					if( iCol2 != -1 )
						newNoteData.SetTapNote(iCol2, iIndex, TAP_ORIGINAL_TAP);

					if( sStepData[i] == '!' )
					{
						i++;
						const char holdChar = sStepData[i++];
						
						DWIcharToNoteCol( holdChar, (GameController)pad, iCol1, iCol2 );

						if( iCol1 != -1 )
							newNoteData.SetTapNote(iCol1, iIndex, TAP_ORIGINAL_HOLD_HEAD);
						if( iCol2 != -1 )
							newNoteData.SetTapNote(iCol2, iIndex, TAP_ORIGINAL_HOLD_HEAD);
					}
				}
				while( jump );
				fCurrentBeat += fCurrentIncrementer;
			}
			break;
		}
		}
	}

	/* Fill in iDuration. */
	for( int t=0; t<newNoteData.GetNumTracks(); ++t )
	{
		FOREACH_NONEMPTY_ROW_IN_TRACK( newNoteData, t, iHeadRow )
		{
			TapNote tn = newNoteData.GetTapNote( t, iHeadRow  );
			if( tn.type != TapNote::hold_head )
				continue;

			int iTailRow = iHeadRow;
			bool bFound = false;
			while( !bFound && newNoteData.GetNextTapNoteRowForTrack(t, iTailRow) )
			{
				const TapNote &TailTap = newNoteData.GetTapNote( t, iTailRow );
				if( TailTap.type == TapNote::empty )
					continue;

				newNoteData.SetTapNote( t, iTailRow, TAP_EMPTY );
				tn.iDuration = iTailRow - iHeadRow;
				newNoteData.SetTapNote( t, iHeadRow, tn );
				bFound = true;
			}

			if( !bFound )
			{
				/* The hold was never closed.  */
				LOG->Warn( "File \"%s\":\"%s\" failed to close a hold note on track %i", 
					m_sLoadingFile.c_str(),
					sDescription.c_str(),
					t );

				newNoteData.SetTapNote( t, iHeadRow, TAP_EMPTY );
			}
		}
	}

	ASSERT( newNoteData.GetNumTracks() > 0 );

	out.SetNoteData( newNoteData );

	out.TidyUpData();

	return true;
}

/* This value can be in either "HH:MM:SS.sssss", "MM:SS.sssss", "SSS.sssss"
 * or milliseconds. */
float DWILoader::ParseBrokenDWITimestamp(const CString &arg1, const CString &arg2, const CString &arg3)
{
	if(arg1.empty()) return 0;

	/* 1+ args */
	if(arg2.empty())
	{
		/* If the value contains a period, treat it as seconds; otherwise ms. */
		if(arg1.find_first_of(".") != arg1.npos)
			return strtof( arg1, NULL );
		else
			return strtof( arg1, NULL ) / 1000.f;
	}

	/* 2+ args */
	if(arg3.empty())
		return HHMMSSToSeconds( arg1+":"+arg2 );

	/* 3+ args */
	return HHMMSSToSeconds( arg1+":"+arg2+":"+arg3 );
}

bool DWILoader::LoadFromDWIFile( CString sPath, Song &out )
{
	LOG->Trace( "Song::LoadFromDWIFile(%s)", sPath.c_str() );
	m_sLoadingFile = sPath;

	MsdFile msd;
	if( !msd.ReadFile( sPath ) )
		RageException::Throw( "Error opening file \"%s\": %s", sPath.c_str(), msd.GetError().c_str() );

	for( unsigned i=0; i<msd.GetNumValues(); i++ )
	{
		int iNumParams = msd.GetNumParams(i);
		const MsdFile::value_t &sParams = msd.GetValue(i);
		CString sValueName = sParams[0];

		if(iNumParams < 1)
		{
			LOG->Warn("Got \"%s\" tag with no parameters in '%s'", sValueName.c_str(), m_sLoadingFile.c_str() );
			continue;
		}

		// handle the data
		if( 0==stricmp(sValueName,"FILE") )
			out.m_sMusicFile = sParams[1];

		else if( 0==stricmp(sValueName,"TITLE") )
		{
			GetMainAndSubTitlesFromFullTitle( sParams[1], out.m_sMainTitle, out.m_sSubTitle );

			/* As far as I know, there's no spec on the encoding of this text. (I didn't
			 * look very hard, though.)  I've seen at least one file in ISO-8859-1. */
			ConvertString( out.m_sMainTitle, "utf-8,english" );
			ConvertString( out.m_sSubTitle, "utf-8,english" );
		}

		else if( 0==stricmp(sValueName,"ARTIST") )
		{
			out.m_sArtist = sParams[1];
			ConvertString( out.m_sArtist, "utf-8,english" );
		}

		else if( 0==stricmp(sValueName,"CDTITLE") )
			out.m_sCDTitleFile = sParams[1];

		else if( 0==stricmp(sValueName,"BPM") )
			out.AddBPMSegment( BPMSegment(0, strtof(sParams[1], NULL)) );

		else if( 0==stricmp(sValueName,"DISPLAYBPM") )
		{
			// #DISPLAYBPM:[xxx..xxx]|[xxx]|[*]; 
		    int iMin, iMax;
			/* We can't parse this as a float with sscanf, since '.' is a valid
			 * character in a float.  (We could do it with a regex, but it's not
			 * worth bothering with since we don't display fractional BPM anyway.) */
		    if( sscanf( sParams[1], "%i..%i", &iMin, &iMax ) == 2 )
			{
				out.m_DisplayBPMType = Song::DISPLAY_SPECIFIED;
				out.m_fSpecifiedBPMMin = (float) iMin;
				out.m_fSpecifiedBPMMax = (float) iMax;
			}
			else if( sscanf( sParams[1], "%i", &iMin ) == 1 )
			{
				out.m_DisplayBPMType = Song::DISPLAY_SPECIFIED;
				out.m_fSpecifiedBPMMin = out.m_fSpecifiedBPMMax = (float) iMin;
			}
			else
			{
				out.m_DisplayBPMType = Song::DISPLAY_RANDOM;
			}
		}

		else if( 0==stricmp(sValueName,"GAP") )
			// the units of GAP is 1/1000 second
			out.m_Timing.m_fBeat0OffsetInSeconds = -atoi( sParams[1] ) / 1000.0f;

		else if( 0==stricmp(sValueName,"SAMPLESTART") )
			out.m_fMusicSampleStartSeconds = ParseBrokenDWITimestamp(sParams[1], sParams[2], sParams[3]);

		else if( 0==stricmp(sValueName,"SAMPLELENGTH") )
			out.m_fMusicSampleLengthSeconds = ParseBrokenDWITimestamp(sParams[1], sParams[2], sParams[3]);

		else if( 0==stricmp(sValueName,"FREEZE") )
		{
			CStringArray arrayFreezeExpressions;
			split( sParams[1], ",", arrayFreezeExpressions );

			for( unsigned f=0; f<arrayFreezeExpressions.size(); f++ )
			{
				CStringArray arrayFreezeValues;
				split( arrayFreezeExpressions[f], "=", arrayFreezeValues );
				if( arrayFreezeValues.size() != 2 )
				{
					LOG->Warn( "Invalid FREEZE in '%s': '%s'", m_sLoadingFile.c_str(), arrayFreezeExpressions[f].c_str() );
					continue;
				}
				int iFreezeRow = BeatToNoteRow( strtof( arrayFreezeValues[0],NULL ) / 4.0f );
				float fFreezeSeconds = strtof( arrayFreezeValues[1], NULL ) / 1000.0f;
				
				out.AddStopSegment( StopSegment(iFreezeRow, fFreezeSeconds) );
//				LOG->Trace( "Adding a freeze segment: beat: %f, seconds = %f", fFreezeBeat, fFreezeSeconds );
			}
		}

		else if( 0==stricmp(sValueName,"CHANGEBPM")  || 0==stricmp(sValueName,"BPMCHANGE") )
		{
			CStringArray arrayBPMChangeExpressions;
			split( sParams[1], ",", arrayBPMChangeExpressions );

			for( unsigned b=0; b<arrayBPMChangeExpressions.size(); b++ )
			{
				CStringArray arrayBPMChangeValues;
				split( arrayBPMChangeExpressions[b], "=", arrayBPMChangeValues );
				if( arrayBPMChangeValues.size() != 2 )
				{
					LOG->Warn( "Invalid CHANGEBPM in '%s': '%s'", m_sLoadingFile.c_str(), arrayBPMChangeExpressions[b].c_str() );
					continue;
				}
				
				BPMSegment bs;
				bs.m_iStartIndex = BeatToNoteRow( strtof( arrayBPMChangeValues[0],NULL ) / 4.0f );
				bs.SetBPM( strtof( arrayBPMChangeValues[1], NULL ) );
				out.AddBPMSegment( bs );
			}
		}

		else if( 0==stricmp(sValueName,"SINGLE")  || 
			     0==stricmp(sValueName,"DOUBLE")  ||
				 0==stricmp(sValueName,"COUPLE")  || 
				 0==stricmp(sValueName,"SOLO"))
		{
			Steps* pNewNotes = new Steps;
			LoadFromDWITokens( 
				sParams[0], 
				sParams[1], 
				sParams[2], 
				sParams[3], 
				(iNumParams==5) ? sParams[4] : CString(""),
				*pNewNotes
				);
			if(pNewNotes->m_StepsType != STEPS_TYPE_INVALID)
				out.AddSteps( pNewNotes );
			else
				delete pNewNotes;
		}
		else if( 0==stricmp(sValueName,"DISPLAYTITLE") ||
			0==stricmp(sValueName,"DISPLAYARTIST") )
		{
			/* We don't want to support these tags.  However, we don't want
			 * to pick up images used here as song images (eg. banners). */
			CString param = sParams[1];
			/* "{foo} ... {foo2}" */
			size_t pos = 0;
			while( pos < CString::npos )
			{

				size_t startpos = param.find('{', pos);
				if( startpos == CString::npos )
					break;
				size_t endpos = param.find('}', startpos);
				if( endpos == CString::npos )
					break;

				CString sub = param.substr( startpos+1, endpos-startpos-1 );

				pos = endpos + 1;

				BlacklistedImages.insert( sub.c_str() );
			}
		}
		else
		{
			// do nothing.  We don't care about this value name
		}
	}

	return true;
}

void DWILoader::GetApplicableFiles( CString sPath, CStringArray &out )
{
	GetDirListing( sPath + CString("*.dwi"), out );
}

bool DWILoader::LoadFromDir( CString sPath, Song &out )
{
	CStringArray aFileNames;
	GetApplicableFiles( sPath, aFileNames );

	if( aFileNames.size() > 1 )
		RageException::Throw( "There is more than one DWI file in '%s'.  There should be only one!", sPath.c_str() );

	/* We should have exactly one; if we had none, we shouldn't have been
	 * called to begin with. */
	ASSERT( aFileNames.size() == 1 );

	return LoadFromDWIFile( sPath + aFileNames[0], out );
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
