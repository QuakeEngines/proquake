/*
Ugh, C++

If anybody thinks I'm gonna write a class here though...

This is a drop-in replacement for the existing CD playing stuff.  It's built on DirectShow using the DirectX
8.1 SDK - I don't know if it'll work on older versions of DirectX, but I assume as most people using this
will be gamers anyway, they'll already have upgraded beyond 8.1 a long time ago.  If you want to recompile 
you may need the SDK...

Please don't compile Realm using the Direct X 9 SDK as you'll lose an awful lot of speed if you do (I
gained 50 FPS just by going back to 8.1)

This MP3 interface uses DirectShow for streaming the MP3 from the hard disk.  Better performance may be had
by buffering the entire file in memory and playing from that instead...  I'm kinda new to Direct X in general
so I don't know how to yet :(

I'm not a C++ head by any means, so if anything looks stupid in here, it probably is.

This could probably be very easily modified to enable Quake to stream audio off the web!!!
*/

#include "version.h"

#ifdef BUILD_MP3_VERSION

#include <windows.h>
#include <stdio.h>
#include <dshow.h>

// need these for Quake engine interaction
#define MAX_OSPATH  128
extern "C"
{
	void Con_Printf (char *fmt, ...);
	void Con_DPrintf (char *fmt, ...);
	int	 va_snprintf (char *function, char *buffer, size_t buffersize, const char *format, ...);
	int	 va_vsnprintf (char *function, char *buffer, size_t buffersize, const char *format, va_list args);
	extern char com_gamedir[MAX_OSPATH];
	char *va (char *format, ...);
	void COM_DefaultExtension (char *path, const char *extension, size_t len);
	HWND mainwindow;
};


// this needs to be defined in gl_vidnt.c as well
#define WM_GRAPHNOTIFY  WM_USER + 13
IGraphBuilder *pGraph = NULL;
IMediaControl *pControl = NULL;
IMediaEventEx *pEvent = NULL;
IBasicAudio	  *pAudio = NULL;
IMediaSeeking *pSeek = NULL;

BOOL MP3Enabled = FALSE;
BOOL COMSTUFFOK = FALSE;

void WaitForFilterState (OAFilterState DesiredState)
{
	OAFilterState MP3FS;
	HRESULT hr;

	while (1)
	{
		hr = pControl->GetState(1, &MP3FS);

		if (FAILED (hr)) continue;

		if (MP3FS == DesiredState) return;
	}
}

extern "C" char InitMP3DShow (void)
{
	// COM is beautiful, intuitive and really easy in VB.  This is just clunky and awful.
	HRESULT hr = CoInitialize (NULL);

	if (FAILED (hr))
	{
		Con_Printf ("ERROR - Could not initialize COM library");
		return 0;
	}

	if (FAILED (hr))
	{
		Con_Printf ("ERROR - Could not create the Filter Graph Manager.");

		// kill off COM
		CoUninitialize ();

		return 0;
	}
	COMSTUFFOK = TRUE;
	return 1;
}

extern "C" void KillMP3DShow (void)
{
	if (!COMSTUFFOK) return;

	// stop anything that's playing
	if (MP3Enabled)
	{
		pEvent->SetNotifyWindow ((OAHWND) NULL, 0, 0);
		pControl->Stop ();
		pControl->Release ();
		pEvent->Release ();
		pAudio->Release ();
		pSeek->Release ();
		pGraph->Release ();
	}
	CoUninitialize ();
}

extern "C" void StopMP3DShow (void)
{
	// don't try anything if we couldn't start COM
	if (!COMSTUFFOK) return;

	// don;t try to stop if we're not even playing!!!
	if (!MP3Enabled) return;

	// kill it straight away
	pEvent->SetNotifyWindow ((OAHWND) NULL, 0, 0);
	pControl->Stop ();
	WaitForFilterState (State_Stopped);
	pControl->Release ();
	pEvent->Release ();
	pAudio->Release ();
	pSeek->Release ();
	pGraph->Release ();

	// nothing playing now
	MP3Enabled = FALSE;
}

extern "C" void PawsMP3DShow (int Paused)
{
	// don't try anything if we couldn't start COM
	if (!COMSTUFFOK) return;

	// don;t try to pause if we're not even playing!!!
	if (!MP3Enabled) return;

	// don't wait for the filter states here
	if (Paused)
	{
		pControl->Run ();
	}
	else
	{
		pControl->Pause ();
	}
}

extern "C" void VolmMP3DShow (int Level)
{
	// don't try anything if we couldn't start COM
	if (!COMSTUFFOK) return;

	// don;t try to change volume if we're not even playing!!!
	if (!MP3Enabled) return;

	// put_Volume uses an exponential decibel-based scale going from -10000 (no sound) to 0 (full volume)
	// each 100 represents 1 db.  i could do the maths, but this is faster and more maintainable.
	switch (Level)
	{
	case 0:
		pAudio->put_Volume (-10000);
		break;

	case 1:
		pAudio->put_Volume (-2000);
		break;

	case 2:
		pAudio->put_Volume (-1400);
		break;

	case 3:
		pAudio->put_Volume (-1040);
		break;

	case 4:
		pAudio->put_Volume (-800);
		break;

	case 5:
		// half volume = -6.02 db
		// i got these figures from GoldWave 5's volume changer...
		pAudio->put_Volume (-602);
		break;

	case 6:
		pAudio->put_Volume (-440);
		break;

	case 7:
		pAudio->put_Volume (-310);
		break;

	case 8:
		pAudio->put_Volume (-190);
		break;

	case 9:
		pAudio->put_Volume (-90);
		break;

	case 10:
		pAudio->put_Volume (0);
		break;
	}
}

/*
===================
MesgMP3DShow

MP3 Message handler.  The only one we're interested in is a stop message.
Everything else is handled within the engine code.
===================
*/
extern "C" void MesgMP3DShow (int Looping)
{
	// don't try anything if we couldn't start COM
	if (!COMSTUFFOK) return;

	// don't try anything if we're not even playing!!!
	if (!MP3Enabled) return;

	LONG		evCode;
	LONG_PTR	evParam1, evParam2;
    HRESULT		hr = S_OK;

    // Process all queued events
    while (SUCCEEDED (pEvent->GetEvent (&evCode, &evParam1, &evParam2, 0)))
    {
        // Free memory associated with callback, since we're not using it
        hr = pEvent->FreeEventParams (evCode, evParam1, evParam2);

        // If this is the end of the clip, reset to beginning
        if (evCode == EC_COMPLETE && Looping)
        {
            LONGLONG pos = 0;

            // Reset to first frame of movie
            hr = pSeek->SetPositions (&pos, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
        }
		else if (evCode == EC_COMPLETE)
		{
			// have to explicitly stop it when it completes otherwise the interfaces will remain open
			// when the next MP3 is played, and both will play simultaneously...!
			StopMP3DShow ();
		}
    }
    return;
}

/*
==================
TouchMP3

quickly confirm that a file exists without having to route it through labyrinthine COM interfaces
this isn't limited to MP3's only, by the way... specify an extension in your "cd play" or "mp3 play"
command and it'll play the file if you have a codec that works with direct show
==================
*/
char *TouchMP3 (char *MP3Name, int verbose)
{
	static char MP3File[1024];

	// slap on a ".mp3" extension if it doesn't have one
	COM_DefaultExtension (MP3Name, ".mp3", sizeof(MP3Name));

	// try the current game directory first
	va_snprintf ("TouchMP3", MP3File, sizeof(MP3File), "%s/music/%s", com_gamedir, MP3Name);

	FILE *f = fopen (MP3File, "rb");

	if (!f)
	{
		// no music in the current game directory so lets try ID1
		va_snprintf ("TouchMP3", MP3File, sizeof(MP3File), "id1/music/%s", MP3Name);

		f = fopen (MP3File, "rb");

		// no music in ID1 either
		if (!f)
		{
			if (verbose > 0) 
			{
				Con_Printf ("couldn't find %s!!!\n", MP3Name);
			}
			return NULL;
		}
	}
	fclose (f);

	if (verbose) 
	{
		Con_DPrintf ("playing %s\n", MP3Name);
	}
	return MP3File;
}

/*
==================
StringToLPCWSTR

fucking stupid MS data types
==================
*/
WCHAR *StringToLPCWSTR (char *instr)
{
	static WCHAR outstr[1024];

	if (!instr)
	{
		return '\0';
	}

	for (int i = 0; ; i++)
	{
		outstr[i] = instr[i];

		if (instr[i] == '\0') break;
	}
	return outstr;
}

/*
=====================
SetupMP3DShow

Initialize COM interfaces and begin playing the MP3
=====================
*/
void SetupMP3DShow (WCHAR *MP3File)
{
	if (!MP3File) return;
	if (!COMSTUFFOK) return;

	// Create the filter graph manager and query for interfaces.
	CoCreateInstance (CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void **) &pGraph);
	pGraph->QueryInterface (IID_IMediaControl, (void **) &pControl);
	pGraph->QueryInterface (IID_IMediaEventEx, (void **) &pEvent);
	pGraph->QueryInterface (IID_IBasicAudio, (void **) &pAudio);
	pGraph->QueryInterface (IID_IMediaSeeking, (void **) &pSeek);
	pGraph->RenderFile (MP3File, NULL);

	// send events through the standard window event handler
	pEvent->SetNotifyWindow ((OAHWND) mainwindow, WM_GRAPHNOTIFY, 0);

	// Run the graph.
	pControl->Run ();

	// tell us globally that we can play OK
	MP3Enabled = TRUE;

	// wait until it reports playing
	WaitForFilterState (State_Running);

	// examples in the SDK will wait for the event to complete here, but this is totally inappropriate
	// for a game engine.  
}

extern "C" void UserMP3DShow (char *mp3name, int verbose)
{
	MP3Enabled = FALSE;

	WCHAR *MP3File = StringToLPCWSTR (TouchMP3 (mp3name, verbose));

	SetupMP3DShow (MP3File);
}

extern "C" void PlayMP3DShow (int mp3num)
{
	UserMP3DShow (va ("track%02i", mp3num), -1); // Baker: using -1 to denote we SAY what we are playing, but say nothing if no mp3 track is found
}



#endif