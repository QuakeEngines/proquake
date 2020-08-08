/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// Quake is a trademark of Id Software, Inc., (c) 1996 Id Software, Inc. All
// rights reserved.

/*
==============================================================================================================
MH - this file is now nothing more than a Quake-compatible wrapper for my Direct X music playing routines
(qmp3.cpp).

It should be easy to port it to any other engine as a "drop 'n' go" - you'll also need the WM_GRAPHNOTIFY
stuff in gl_vidnt.c and the appropriate headers and libs.
==============================================================================================================
*/

#include "quakedef.h"

#ifdef BUILD_MP3_VERSION
#include <windows.h>
char InitMP3DShow (void);
void PlayMP3DShow (int mp3num);
void KillMP3DShow (void);
void StopMP3DShow (void);
void PawsMP3DShow (int Paused);
void VolmMP3DShow (int Level);
void MesgMP3DShow (int Looping);
void UserMP3DShow (char *mp3name, int verbose);

static void CD_f (void);
static void MP3_f (void);

// it's a dumb warning anyway...
#pragma warning(disable : 4761)


extern	cvar_t	bgmvolume;
cvar_t	cd_enabled = {"cd_enabled","1", true}; // Baker 3.99e: the ability to have the cdplayer shut off from menu

byte	CurrentTrack;

qboolean Looping;
qboolean Paused;
qboolean enabled;
qboolean initialized;

static void CDAudio_Eject (void) {}
static void CDAudio_CloseDoor (void) {}
static int CDAudio_GetAudioDiskInfo (void) {return 0;}

void CDAudio_Stop (void)
{
	if (!sound_started) return;
	if (!enabled) return;

	StopMP3DShow ();
}

void mp3_Stop (void)
{
	if (!sound_started) return;
	// We aren't checking enabled because we aren't checking it to play?

	StopMP3DShow ();
}

void CDAudio_Pause (void)
{
	if (!sound_started) return;
	if (!enabled) return;

	PawsMP3DShow (0);
}

void CDAudio_Resume (void)
{
	if (!sound_started) return;
	if (!enabled) return;

	PawsMP3DShow (1);
}


void CDAudio_Update (void) {}

void CD_Enabled_f(void);
int CDAudio_Init (void)
{
	if (cls.state == ca_dedicated) {
		enabled = false;
		return -1;
	}
		
	Cvar_RegisterVariable (&cd_enabled, CD_Enabled_f); // Baker 3.99e: we want this available even if -nocdaudio is used (for menu)		
		
	if (COM_CheckParm("-nocdaudio"))
	{
		enabled = false;
		return -1;
	}

	if (!InitMP3DShow ()) 
		return -1;

	initialized = true;

	Looping = false;
	Paused = false;

	enabled = true;

	Cmd_AddCommand ("cd", CD_f);
	Cmd_AddCommand ("mp3", MP3_f);

	return 0;
}


void CDAudio_Shutdown (void)
{
	if (!enabled) return;

	KillMP3DShow ();
}


void CDAudio_Play (byte track, qboolean looping)
{
	Looping = false;

	if (!sound_started) return;

	// worldspawn tracks are one ahead of the game
	CurrentTrack = track - 1;

	Con_DPrintf("Track name is %s\n", va ("%02i", CurrentTrack));

	// exit conditions
	if (CurrentTrack > 99) return;
	if (CurrentTrack < 1) return;
	if (!enabled) return;

	// stop any previous instances
	CDAudio_Stop ();

	PlayMP3DShow (CurrentTrack);

	Looping = looping;
}


void CDAudioSetVolume (void)
{
	if (!sound_started) return;

	// set the volume
	VolmMP3DShow ((int) (bgmvolume.value * 10));
}


LONG CDAudio_MessageHandler (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// pass to direct show stuff
	MesgMP3DShow ((int) Looping);

	return 0;
}


static void CD_f (void)
{
	char	*command;
	int		i;
	char	MP3FileName[MAX_OSPATH];
	char	*mp3nametemp;

	if (Cmd_Argc() < 2)
	{
		Con_Printf ("%s play|stop|pause|resume\n", Cmd_Argv (0));
		Con_Printf (" Put MP3s in gamedir/music!!!\n");
		Con_Printf (" spaces are allowed in file names\n");
		return;
	}

	command = Cmd_Argv (1);

	if (_stricmp (command, "play") == 0)
	{
		// build the MP3 file name
		memset (MP3FileName, 0, sizeof (MP3FileName));

		for (i = 2; i < Cmd_Argc (); i++)
		{
			mp3nametemp = Cmd_Argv (i);
			snprintf (MP3FileName, sizeof(MP3FileName), "%s%s ", MP3FileName, mp3nametemp);
		}

		// get rid of the trailing space
		MP3FileName[strlen (MP3FileName) - 1] = 0;

		// stop anything that's currently playing
		if (enabled && sound_started)
		{
			StopMP3DShow ();
		}
		UserMP3DShow (MP3FileName, 1);

		return;
	}

	if (_stricmp (command, "stop") == 0)
	{
		CDAudio_Stop ();
		return;
	}

	if (_stricmp (command, "pause") == 0)
	{
		CDAudio_Pause ();
		return;
	}

	if (_stricmp (command, "resume") == 0)
	{
		CDAudio_Resume ();
		return;
	}
}

static void MP3_f (void)
{
	char	*command;
	int		i;
	char	MP3FileName[MAX_OSPATH];
	char	*mp3nametemp;

	if (Cmd_Argc() < 1)
	{
		Con_Printf ("%s play|stop|pause|resume\n", Cmd_Argv (0));
		Con_Printf (" Put MP3s in gamedir/music!!!\n");
		Con_Printf (" spaces are allowed in file names\n");
		return;
	}

	command = Cmd_Argv (1);
	
	if (_stricmp (command, "stop") == 0)
	{
		CDAudio_Stop ();
		return;
	}

	// Assumed to be a filename at this point


	// build the MP3 file name
	memset (MP3FileName, 0, sizeof (MP3FileName));

	for (i = 1; i < Cmd_Argc (); i++)
	{
		mp3nametemp = Cmd_Argv (i);
		snprintf (MP3FileName, sizeof(MP3FileName), "%s%s ", MP3FileName, mp3nametemp);
	}

	// get rid of the trailing space
	MP3FileName[strlen (MP3FileName) - 1] = 0;

	// stop anything that's currently playing
	if (sound_started)
	{
		StopMP3DShow ();
	}
	UserMP3DShow (MP3FileName, 1);

	return;

}


void CD_Enabled_f(void) {

	// Baker: this is to turn the CD player OFF and ON
	//        we don't want cd_enabled to reflect the current state of things
	//        but rather to turn it off or on.
	//        Yes, we want this to really function like a command
	//        but want it to save session to session so this must be a cvar.
	//        so if the CD can't be on, don't let it be 1, make it be 0.
	//        And remember, we only get here on a change
	//        And remember, cd_enabled defaults to on.

	//        cdaudio is init'd rather early on, before a config is read.
	//        It defaults to on.
	//        So if it is in the config, we will be turn the cd_audio off.
	//        Lets use this function to limit the values to 0 and 1.
	//        If this is changed to off to on, it had to happen in the client.
	//        Either way, if we actually do something, con_print it.
	//        And if we don't, con_print why.

	// Let's value check here to avoid silly things like
	// cd_enabled being changed from 0 to 1

	if (cd_enabled.value !=0 && cd_enabled.value != 1) {
		Cvar_Set("cd_enabled", "1");
		return; // Return because we just did a change, this is going to get triggered again (leave + let the next passthru do it)
	}

	if (cd_enabled.value) {
		// Ok, cd audio is being turned on
		// First, can we turn it on?

		if (!initialized) {
			// Can't turn it on, lets get out but explain why
			Con_Printf("CDAudio cannot be enabled because the device was not initialized\n");
			return;
		}

		if (enabled) {
			// Already on
			return;
		}


		// Ok, we can turn it on.
		enabled = true;
		Con_Printf("MP3 enabled: Music on next map change\n");
		return;

	}


	// Ok, cd audio is being turned off if we got this far.

	if (!initialized) {
		// Can't turn it off.  It wasn't initialized to begin with.
		// But we aren't going to explain this because the main purpose
		// of this is to be able to turn off the CD player automatically via config
		return;
	}

	// Ok now is it already off?
	if (!enabled) {
		// Can't turn it off.  It is off.
		// Can this happen?
		Con_DPrintf("CDAudio is already off\n");
		return;
	}

	// Ok, cd is on so turn it off.

	Con_DPrintf("CDAudio has been turned off.\n");

	if (Looping)
		CDAudio_Stop();
	enabled = false;
	return;

}


int CDAudio_IsEnabled(void)
{
	// Return -1 for hard NO, return 0 for No, return 1 for enabled

	if (!initialized) // Baker: if it isn't initialized, it can't be enabled
		return -1;

	if (!enabled)
		return 0;

	return 1; // Must be enabled then

}

#else // Classic CD-ROM version


/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// cd_win.c

#include <windows.h>

extern	HWND	mainwindow;
extern	cvar_t	bgmvolume;
cvar_t	cd_enabled = {"cd_enabled","1", true}; // Baker 3.99e: the ability to have the cdplayer shut off from menu

static qboolean cdValid = false;
static qboolean	playing = false;
static qboolean	wasPlaying = false;
static qboolean	initialized = false;
static qboolean	enabled = false;
static qboolean playLooping = false;
static float	cdvolume;
static byte 	remap[100];
static byte		cdrom;
static byte		playTrack;
static byte		maxTrack;

UINT	wDeviceID;


static void CDAudio_Eject(void)
{
	DWORD	dwReturn;

    if (dwReturn = mciSendCommand(wDeviceID, MCI_SET, MCI_SET_DOOR_OPEN, (DWORD)NULL))
		Con_DPrintf("MCI_SET_DOOR_OPEN failed (%i)\n", dwReturn);
}


static void CDAudio_CloseDoor(void)
{
	DWORD	dwReturn;

    if (dwReturn = mciSendCommand(wDeviceID, MCI_SET, MCI_SET_DOOR_CLOSED, (DWORD)NULL))
		Con_DPrintf("MCI_SET_DOOR_CLOSED failed (%i)\n", dwReturn);
}


static int CDAudio_GetAudioDiskInfo(void)
{
	DWORD				dwReturn;
	MCI_STATUS_PARMS	mciStatusParms;


	cdValid = false;

	mciStatusParms.dwItem = MCI_STATUS_READY;
    dwReturn = mciSendCommand(wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_WAIT, (DWORD) (LPVOID) &mciStatusParms);
	if (dwReturn)
	{
		Con_DPrintf("CDAudio: drive ready test - get status failed\n");
		return -1;
	}
	if (!mciStatusParms.dwReturn)
	{
		Con_DPrintf("CDAudio: drive not ready\n");
		return -1;
	}

	mciStatusParms.dwItem = MCI_STATUS_NUMBER_OF_TRACKS;
    dwReturn = mciSendCommand(wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_WAIT, (DWORD) (LPVOID) &mciStatusParms);
	if (dwReturn)
	{
		Con_DPrintf("CDAudio: get tracks - status failed\n");
		return -1;
	}
	if (mciStatusParms.dwReturn < 1)
	{
		Con_DPrintf("CDAudio: no music tracks\n");
		return -1;
	}

	cdValid = true;
	maxTrack = mciStatusParms.dwReturn;

	return 0;
}


void CDAudio_Play(byte track, qboolean looping)
{
	DWORD				dwReturn;
    MCI_PLAY_PARMS		mciPlayParms;
	MCI_STATUS_PARMS	mciStatusParms;

	if (!enabled)
		return;

	if (!cdValid)
	{
		CDAudio_GetAudioDiskInfo();
		if (!cdValid)
			return;
	}

	track = remap[track];

	if (track < 1 || track > maxTrack)
	{
		Con_DPrintf("CDAudio: Bad track number %u.\n", track);
		return;
	}

	// don't try to play a non-audio track
	mciStatusParms.dwItem = MCI_CDA_STATUS_TYPE_TRACK;
	mciStatusParms.dwTrack = track;
    dwReturn = mciSendCommand(wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK | MCI_WAIT, (DWORD) (LPVOID) &mciStatusParms);
	if (dwReturn)
	{
		Con_DPrintf("MCI_STATUS failed (%i)\n", dwReturn);
		return;
	}
	if (mciStatusParms.dwReturn != MCI_CDA_TRACK_AUDIO)
	{
		Con_Printf("CDAudio: track %i is not audio\n", track);
		return;
	}

	// get the length of the track to be played
	mciStatusParms.dwItem = MCI_STATUS_LENGTH;
	mciStatusParms.dwTrack = track;
    dwReturn = mciSendCommand(wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK | MCI_WAIT, (DWORD) (LPVOID) &mciStatusParms);
	if (dwReturn)
	{
		Con_DPrintf("MCI_STATUS failed (%i)\n", dwReturn);
		return;
	}

	if (playing)
	{
		if (playTrack == track)
			return;
		CDAudio_Stop();
	}

    mciPlayParms.dwFrom = MCI_MAKE_TMSF(track, 0, 0, 0);
	mciPlayParms.dwTo = (mciStatusParms.dwReturn << 8) | track;
    mciPlayParms.dwCallback = (DWORD)mainwindow;
    dwReturn = mciSendCommand(wDeviceID, MCI_PLAY, MCI_NOTIFY | MCI_FROM | MCI_TO, (DWORD)(LPVOID) &mciPlayParms);
	if (dwReturn)
	{
		Con_DPrintf("CDAudio: MCI_PLAY failed (%i)\n", dwReturn);
		return;
	}

	playLooping = looping;
	playTrack = track;
	playing = true;

	if (cdvolume == 0.0)
		CDAudio_Pause ();
}


void CDAudio_Stop(void)
{
	DWORD	dwReturn;

	if (!enabled)
		return;

	if (!playing)
		return;

    if (dwReturn = mciSendCommand(wDeviceID, MCI_STOP, 0, (DWORD)NULL))
		Con_DPrintf("MCI_STOP failed (%i)", dwReturn);

	wasPlaying = false;
	playing = false;
}


void CDAudio_Pause(void)
{
	DWORD				dwReturn;
	MCI_GENERIC_PARMS	mciGenericParms;

	if (!enabled)
		return;

	if (!playing)
		return;

	mciGenericParms.dwCallback = (DWORD)mainwindow;
    if (dwReturn = mciSendCommand(wDeviceID, MCI_PAUSE, 0, (DWORD)(LPVOID) &mciGenericParms))
		Con_DPrintf("MCI_PAUSE failed (%i)", dwReturn);

	wasPlaying = playing;
	playing = false;
}


void CDAudio_Resume(void)
{
	DWORD			dwReturn;
    MCI_PLAY_PARMS	mciPlayParms;

	if (!enabled)
		return;

	if (!cdValid)
		return;

	if (!wasPlaying)
		return;

    mciPlayParms.dwFrom = MCI_MAKE_TMSF(playTrack, 0, 0, 0);
    mciPlayParms.dwTo = MCI_MAKE_TMSF(playTrack + 1, 0, 0, 0);
    mciPlayParms.dwCallback = (DWORD)mainwindow;
    dwReturn = mciSendCommand(wDeviceID, MCI_PLAY, MCI_TO | MCI_NOTIFY, (DWORD)(LPVOID) &mciPlayParms);
	if (dwReturn)
	{
		Con_DPrintf("CDAudio: MCI_PLAY failed (%i)\n", dwReturn);
		return;
	}
	playing = true;
}


static void CD_f (void)
{
	char	*command;
	int	ret, n;

	if (Cmd_Argc() < 2)
		return;

	command = Cmd_Argv (1);

	if (!Q_strcasecmp(command, "on"))

	{
		enabled = true;
		return;
	}

	if (!Q_strcasecmp(command, "off"))

	{
		if (playing)
			CDAudio_Stop();
		enabled = false;
		return;
	}

	if (!Q_strcasecmp(command, "reset"))

	{
		enabled = true;
		if (playing)
			CDAudio_Stop();
		for (n = 0; n < 100; n++)
			remap[n] = n;
		CDAudio_GetAudioDiskInfo();

		return;
	}

	if (!Q_strcasecmp(command, "remap"))

	{
		ret = Cmd_Argc() - 2;
		if (ret <= 0)
		{
			for (n = 1; n < 100; n++)
				if (remap[n] != n)
					Con_Printf("  %u -> %u\n", n, remap[n]);
			return;
		}
		for (n = 1; n <= ret; n++)
			remap[n] = Q_atoi(Cmd_Argv (n+1));

		return;
	}

	if (!Q_strcasecmp(command, "close"))

	{
		CDAudio_CloseDoor();
		return;
	}

	if (!cdValid)
	{
		CDAudio_GetAudioDiskInfo();
		if (!cdValid)
		{
			Con_Printf("No CD in player.\n");
			return;
		}
	}

	if (!Q_strcasecmp(command, "play"))

	{
		CDAudio_Play((byte)Q_atoi(Cmd_Argv (2)), false);
		return;
	}

	if (!Q_strcasecmp(command, "loop"))

	{
		CDAudio_Play((byte)Q_atoi(Cmd_Argv (2)), true);
		return;
	}

	if (!Q_strcasecmp(command, "stop"))

	{
		CDAudio_Stop();
		return;
	}

	if (!Q_strcasecmp(command, "pause"))

	{
		CDAudio_Pause();
		return;
	}

	if (!Q_strcasecmp(command, "resume"))

	{
		CDAudio_Resume();
		return;
	}

	if (!Q_strcasecmp(command, "eject"))

	{
		if (playing)
			CDAudio_Stop();
		CDAudio_Eject();
		cdValid = false;

		return;
	}

	if (!Q_strcasecmp(command, "info"))

	{
		Con_Printf("%u tracks\n", maxTrack);
		if (playing)
			Con_Printf("Currently %s track %u\n", playLooping ? "looping" : "playing", playTrack);
		else if (wasPlaying)
			Con_Printf("Paused %s track %u\n", playLooping ? "looping" : "playing", playTrack);
		Con_Printf("Volume is %f\n", cdvolume);

		return;
	}
}


LONG CDAudio_MessageHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (lParam != wDeviceID)
		return 1;

	switch (wParam)
	{
		case MCI_NOTIFY_SUCCESSFUL:
			if (playing)
			{
				playing = false;
				if (playLooping)
					CDAudio_Play(playTrack, true);
			}
			break;

		case MCI_NOTIFY_ABORTED:
		case MCI_NOTIFY_SUPERSEDED:
			break;

		case MCI_NOTIFY_FAILURE:
			Con_DPrintf("MCI_NOTIFY_FAILURE\n");
			CDAudio_Stop ();
			cdValid = false;
			break;

		default:
			Con_DPrintf("Unexpected MM_MCINOTIFY type (%i)\n", wParam);
			return 1;
	}

	return 0;
}


void CDAudio_Update(void)
{
	if (!enabled)
		return;

	if (bgmvolume.value != cdvolume)
	{
		if (cdvolume)
		{
			Cvar_SetValue ("bgmvolume", 0.0);
			cdvolume = bgmvolume.value;
			CDAudio_Pause ();
		}
		else
		{
			Cvar_SetValue ("bgmvolume", 1.0);
			cdvolume = bgmvolume.value;
			CDAudio_Resume ();
		}
	}
}

void CD_Enabled_f(void) {

	// Baker: this is to turn the CD player OFF and ON
	//        we don't want cd_enabled to reflect the current state of things
	//        but rather to turn it off or on.
	//        Yes, we want this to really function like a command
	//        but want it to save session to session so this must be a cvar.
	//        so if the CD can't be on, don't let it be 1, make it be 0.
	//        And remember, we only get here on a change
	//        And remember, cd_enabled defaults to on.

	//        cdaudio is init'd rather early on, before a config is read.
	//        It defaults to on.
	//        So if it is in the config, we will be turn the cd_audio off.
	//        Lets use this function to limit the values to 0 and 1.
	//        If this is changed to off to on, it had to happen in the client.
	//        Either way, if we actually do something, con_print it.
	//        And if we don't, con_print why.

	// Let's value check here to avoid silly things like
	// cd_enabled being changed from 0 to 1

	if (cd_enabled.value !=0 && cd_enabled.value != 1) {
		Cvar_Set("cd_enabled", "1");
		return; // Return because we just did a change, this is going to get triggered again (leave + let the next passthru do it)
	}

	if (cd_enabled.value) {
		// Ok, cd audio is being turned on
		// First, can we turn it on?

		if (!initialized) {
			// Can't turn it on, lets get out but explain why
			Con_Printf("CDAudio cannot be enabled because the device was not initialized\n");
			return;
		}

		if (enabled) {
			// Already on
			return;
		}


		// Ok, we can turn it on.
		enabled = true;
		Con_Printf("CDAudio enabled: Music on next map change\n");
		return;

	}



	// Ok, cd audio is being turned off if we got this far.

	if (!initialized) {
		// Can't turn it off.  It wasn't initialized to begin with.
		// But we aren't going to explain this because the main purpose
		// of this is to be able to turn off the CD player automatically via config
		return;
	}

	// Ok now is it already off?
	if (!enabled) {
		// Can't turn it off.  It is off.
		// Can this happen?
		Con_DPrintf("CDAudio is already off\n");
		return;
	}

	// Ok, cd is on so turn it off.

	Con_DPrintf("CDAudio has been turned off.\n");

	if (playing)
		CDAudio_Stop();
	enabled = false;
	return;

}

int CDAudio_Init(void)
{
	DWORD	dwReturn;
	MCI_OPEN_PARMS	mciOpenParms;
    MCI_SET_PARMS	mciSetParms;
	int				n;

	if (cls.state == ca_dedicated)
		return -1;

	Cvar_RegisterVariable (&cd_enabled, CD_Enabled_f); // Baker 3.99e: we want this available even if -nocdaudio is used (for menu)

	if (COM_CheckParm("-nocdaudio"))
		return -1;

	mciOpenParms.lpstrDeviceType = TEXT("cdaudio");
	if (dwReturn = mciSendCommand(0, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_SHAREABLE, (DWORD) (LPVOID) &mciOpenParms))
	{
		Con_Printf("CDAudio_Init: MCI_OPEN failed (%i)\n", dwReturn);
		return -1;
	}
	wDeviceID = mciOpenParms.wDeviceID;

    // Set the time format to track/minute/second/frame (TMSF).
    mciSetParms.dwTimeFormat = MCI_FORMAT_TMSF;
    if (dwReturn = mciSendCommand(wDeviceID, MCI_SET, MCI_SET_TIME_FORMAT, (DWORD)(LPVOID) &mciSetParms))
    {
		Con_Printf("MCI_SET_TIME_FORMAT failed (%i)\n", dwReturn);
        mciSendCommand(wDeviceID, MCI_CLOSE, 0, (DWORD)NULL);

		return -1;
    }

	for (n = 0; n < 100; n++)
		remap[n] = n;
	initialized = true;
	enabled = true;

	if (CDAudio_GetAudioDiskInfo())
	{
		Con_Printf("CDAudio_Init: No CD in player.\n");
		cdValid = false;
	}

	Cmd_AddCommand ("cd", CD_f);

	Con_Printf("CD Audio Initialized\n");

	return 0;
}


void CDAudio_Shutdown(void)
{
	if (!initialized)
		return;

	CDAudio_Stop();
	if (mciSendCommand(wDeviceID, MCI_CLOSE, MCI_WAIT, (DWORD)NULL))
		Con_DPrintf("CDAudio_Shutdown: MCI_CLOSE failed\n");
}

int CDAudio_IsEnabled(void)
{
	// Return -1 for hard NO, return 0 for No, return 1 for enabled

	if (!initialized) // Baker: if it isn't initialized, it can't be enabled
		return -1;

	if (!enabled)
		return 0;

	return 1; // Must be enabled then

}




#endif
