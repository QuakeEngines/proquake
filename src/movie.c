/*
Copyright (C) 2002 Quake done Quick

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
// movie.c -- video capturing

#include "quakedef.h"
#include "movie.h"
#include "movie_avi.h"

extern	float	scr_con_current;
extern qboolean	scr_drawloading;
extern	short	*snd_out;
extern	int	snd_linear_count, soundtime;

// Variables for buffering audio
short	capture_audio_samples[44100];	// big enough buffer for 1fps at 44100Hz
int	captured_audio_samples;

static	int	out_size, ssize, outbuf_size;
static	byte	*outbuf, *picture_buf;
static	FILE	*moviefile;

static	float	hack_ctr;


cvar_t	capture_codec	= {"cl_capturevideo_codec", "auto"};
cvar_t	capture_fps	= {"cl_capturevideo_fps", "30.0"};
cvar_t	capture_console	= {"cl_capturevideo_console", "1"};
cvar_t	capture_hack	= {"cl_capturevideo_hack", "0"};
cvar_t	capture_mp3	= {"cl_capturevideo_mp3", "0"};
cvar_t	capture_mp3_kbps = {"cl_capturevideo_mp3_kbps", "128"};

static qboolean movie_is_capturing = false;
qboolean	avi_loaded, acm_loaded;

qboolean Movie_IsActive (void)
{
	// don't output whilst console is down or 'loading' is displayed
	if ((!capture_console.value && scr_con_current > 0) || scr_drawloading)
		return false;

	// Never capture the console if capturedemo is running
	if (cls.capturedemo && scr_con_current > 0)
		return false;

	// otherwise output if a file is open to write to
	return movie_is_capturing;
}


char	movie_capturing_name[MAX_QPATH];
char	movie_capturing_fullpath[MAX_OSPATH]; // fullpath
char	movie_codec[12];
void Movie_Start_Capturing (char *moviename)
{

	hack_ctr = capture_hack.value;

	strlcpy (movie_capturing_name, moviename, sizeof(movie_capturing_name) );
	COM_ForceExtension (movie_capturing_name, ".avi");
	sprintf (movie_capturing_fullpath, "%s/%s", com_gamedir, movie_capturing_name);

	if (!(moviefile = fopen(movie_capturing_fullpath, "wb")))
	{
		COM_CreatePath (movie_capturing_fullpath);
		if (!(moviefile = fopen(movie_capturing_fullpath, "wb")))
		{
			Con_Printf ("ERROR: Couldn't open %s\n", movie_capturing_name);
			return;
		}
	}

	if (strcasecmp("auto", capture_codec.string) == 0)
	{
		// Automatic
		char *codec_order[] = {"vp80", "xvid", "divx", "none"};
		int	count = sizeof(codec_order) / sizeof(codec_order[0]);
		int result, i;

		for (i = 0; i < count ; i ++)
		{
			result = Capture_Open (movie_capturing_fullpath, codec_order[i], true);
			if (result == true)
			{
				strcpy (movie_codec, codec_order[i]);
				break;
			}
		}
		if (result != true)
		{
			movie_is_capturing = false;
			Con_Printf ("ERROR: Couldn't create video stream\n");
		}
		else
			movie_is_capturing = true;
	}
	else
	{
		movie_is_capturing = (Capture_Open (movie_capturing_fullpath, capture_codec.string, false) > 0);	
		if (movie_is_capturing)
			strcpy (movie_codec, capture_codec.string);
	}
}



void Movie_Stop (void)
{
	movie_is_capturing = false;
	Capture_Close ();
	fclose (moviefile);
	strlcpy (cls.recent_file, movie_capturing_name, sizeof(cls.recent_file) );

	if (cls.demo_hosttime_elapsed /*cls.capturedemo*/) // Because cls.capturedemo already was cleared :(
		Con_Printf ("Video completed: %s in %d:%02d (codec: %s)\n", movie_capturing_name, COM_Minutes((int)cls.demo_hosttime_elapsed), COM_Seconds((int)cls.demo_hosttime_elapsed), movie_codec);
	else
		Con_Printf ("Video completed: %s (codec: %s)\n", movie_capturing_name, movie_codec);
}

void Movie_Stop_Capturing (void)
{
	if (movie_is_capturing == 0)
	{
		Con_Printf ("Not capturing\n");
		return;
	}

	if (cls.capturedemo)
		cls.capturedemo = false;

	Movie_Stop ();

}

void Movie_StopPlayback (void);

void Movie_CaptureDemo_f (void)
{
	if (Cmd_Argc() != 2)
	{
		Con_Printf ("Usage: capturedemo <demoname>\n\nNote: stopdemo will stop video capture\nUse cl_capturevideo_* cvars for codec, fps, etc.\n");
		return;
	}

	if (movie_is_capturing)
	{
		Con_Printf ("Can't capture demo, video is capturing\n");
		return;
	}

	// Baker: This is a performance benchmark.  No reason to have console up.
	if (key_dest != key_game)
		key_dest = key_game;

	CL_Clear_Demos_Queue (); // timedemo is a very intentional action

	CL_PlayDemo_f ();
	if (!cls.demoplayback)
		return;

	Movie_Start_Capturing (Cmd_Argv(1));
	cls.capturedemo = true;

	if (!movie_is_capturing)
	{
		Movie_StopPlayback ();

		// Baker: If capturedemo fails, just stop the demo playback.
		// Don't confuse the user
		Host_Stopdemo_f ();

 		// If +capturedemo in command line, we exit after demo capture 
		// completed (even if failed .. and this is failure location here)
		if (cls.capturedemo_and_exit)
			Host_Quit ();
	}

}

void Movie_Capture_Toggle_f (void)
{
	if (Cmd_Argc() != 2 || strcasecmp(Cmd_Argv(1), "toggle") != 0)
	{
		Con_Printf ("usage: %s <toggle>\n\nset cl_capturevideo_codec and fps first\n", Cmd_Argv (0));
		Con_Printf (movie_is_capturing ? "status: movie capturing\n" : "status: not capturing\n");
		return;
	}

	if (cls.capturedemo)
	{
		Con_Printf ("Can't capturevideo toggle, capturedemo running\n");
		return;
	}

	if (movie_is_capturing)
	{
		Movie_Stop_Capturing ();
	}
	else
	{
//		byte	*buffer;
		char	aviname[MAX_QPATH];
		char	checkname[MAX_OSPATH];
		char	barename[MAX_QPATH] = "video";
		int		i;

		if (cl.worldmodel)
			COM_StripExtension (cl.worldmodel->name + 5, barename);

	// find a file name to save it to
		for (i=0; i<10000; i++)
		{
			sprintf (aviname, "%s%04i.avi", barename, i);
			sprintf (checkname, "%s/%s", com_gamedir, aviname);
			if (Sys_FileTime(checkname) == -1)
				break;	// file doesn't exist
		}
		if (i == 10000)
		{
			Con_Printf ("Movie_Capture_Toggle_f: Couldn't find an unused filename\n");
			return;
 		}

		Movie_Start_Capturing (aviname);
	}

}

void CaptureCodec_Validate (void)
{
	if (!capture_codec.string[0]) // Empty string ... assume user means none
	{
		Cvar_Set (capture_codec.name, "none");
		Con_Printf ("%s set to \"%s\"\n", capture_codec.name, capture_codec.string);
	}

	if (capture_codec.string[0] == '0') // Begins with 0 ... set to auto
	{
		Cvar_Set (capture_codec.name, "auto");
		Con_Printf ("%s set to \"%s\"\n", capture_codec.name, capture_codec.string);
	}
}

void Movie_Init (void)
{
	AVI_LoadLibrary ();
	if (!avi_loaded)
		return;

	captured_audio_samples = 0;

	Cmd_AddCommand ("capturevideo", Movie_Capture_Toggle_f);
	Cmd_AddCommand ("capturedemostop", Movie_Stop_Capturing);
	Cmd_AddCommand ("capturedemo", Movie_CaptureDemo_f);

	Cvar_RegisterVariable (&capture_codec, CaptureCodec_Validate);
	Cvar_RegisterVariable (&capture_fps, NULL);

	Cvar_RegisterVariable (&capture_console, NULL);
	Cvar_RegisterVariable (&capture_hack, NULL);


	ACM_LoadLibrary ();
	if (!acm_loaded)
		return;

	Cvar_RegisterVariable (&capture_mp3, NULL);
	Cvar_RegisterVariable (&capture_mp3_kbps, NULL);
}

void Movie_StopPlayback (void)
{
	if (!cls.capturedemo)
		return;

	cls.capturedemo = false;
	Movie_Stop ();

	// If +capturedemo in command line, we exit after demo capture 
	// completed (even if failed .. and this is failure location here)

	if (cls.capturedemo_and_exit)
		Host_Quit ();
}

double Movie_FrameTime (void)
{
	double	time;

	if (capture_fps.value > 0)
		time = !capture_hack.value ? 1.0 / capture_fps.value : 1.0 / (capture_fps.value * (capture_hack.value + 1.0));
	else
		time = 1.0 / 30.0;
	return CLAMP (1.0 / 1000, time, 1.0);
}

void Movie_UpdateScreen (void)
{
	int	i, size = glwidth * glheight * 3;
	byte	*buffer, temp;

	if (!Movie_IsActive())
		return;

	if (capture_hack.value)
	{
		if (hack_ctr != capture_hack.value)
		{
			if (!hack_ctr)
				hack_ctr = capture_hack.value;
			else
				hack_ctr--;
			return;
		}
		hack_ctr--;
	}


	buffer = Q_malloc (size);
	glReadPixels (glx, gly, glwidth, glheight, GL_RGB, GL_UNSIGNED_BYTE, buffer);
//	ApplyGamma (buffer, size);  Baker: a thought

	for (i = 0 ; i < size ; i += 3)
	{
		temp = buffer[i];
		buffer[i] = buffer[i+2];
		buffer[i+2] = temp;
	}

	Capture_WriteVideo (buffer);

	free (buffer);
}

void Movie_TransferStereo16 (void)
{
	if (!Movie_IsActive())
		return;

	// Copy last audio chunk written into our temporary buffer
	memcpy (capture_audio_samples + (captured_audio_samples << 1), snd_out, snd_linear_count * shm->channels);
	captured_audio_samples += (snd_linear_count >> 1);

	if (captured_audio_samples >= Q_rint (host_frametime * shm->speed))
	{
		// We have enough audio samples to match one frame of video
		Capture_WriteAudio (captured_audio_samples, (byte *)capture_audio_samples);
		captured_audio_samples = 0;
	}
}

qboolean Movie_GetSoundtime (void)
{
	if (!Movie_IsActive())
		return false;

	soundtime += Q_rint (host_frametime * shm->speed * (Movie_FrameTime() / host_frametime));

	return true;
}
