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
// cl_demo.c

#include "quakedef.h"
#include <time.h> // easyrecord stats

#ifdef SUPPORTS_AVI_CAPTURE
#include "movie.h"
#endif

void CL_FinishTimeDemo (void);

typedef struct framepos_s
{
	long				baz;
	struct framepos_s	*next;
} framepos_t;

framepos_t	*dem_framepos = NULL;
qboolean	start_of_demo = false;
qboolean	bumper_on = false;

/*
==============================================================================

DEMO CODE

When a demo is playing back, all NET_SendMessages are skipped, and
NET_GetMessages are read from the demo file.

Whenever cl.time gets past the last received message, another message is
read from the demo file.
==============================================================================
*/

// JPG 1.05 - support for recording demos after connecting to the server
byte	demo_head[3][MAX_MSGLEN];
int		demo_head_size[2];

/*
==============
CL_StopPlayback

Called when a demo file runs out, or the user starts a game
==============
*/
void CL_StopPlayback (void)
{
	if (!cls.demoplayback)
		return;

	fclose (cls.demofile);
	cls.demoplayback = false;
	cls.demofile = NULL;
	cls.state = ca_disconnected;

	Con_DPrintf("Demo playback has ended\n");

	if (cls.timedemo)
		CL_FinishTimeDemo ();

#ifdef SUPPORTS_AVI_CAPTURE
	Movie_StopPlayback ();
#endif


}

/*
====================
CL_WriteDemoMessage

Dumps the current net message, prefixed by the length and view angles
====================
*/
 void CL_WriteDemoMessage (void)
{
	int		len;
	int		i;
	float	f;

	len = LittleLong (net_message.cursize);
	fwrite (&len, 4, 1, cls.demofile);
	for (i=0 ; i<3 ; i++)
	{
		f = LittleFloat (cl.viewangles[i]);
		fwrite (&f, 4, 1, cls.demofile);
	}
	fwrite (net_message.data, net_message.cursize, 1, cls.demofile);
	fflush (cls.demofile);
}

 void PushFrameposEntry (long fbaz)
{
	framepos_t	*newf;

	newf = Q_malloc (sizeof(framepos_t));
	newf->baz = fbaz;

	if (!dem_framepos)
	{
		newf->next = NULL;
		start_of_demo = false;
	}
	else
	{
		newf->next = dem_framepos;
	}
	dem_framepos = newf;
}

static void EraseTopEntry (void)
{
	framepos_t	*top;

	top = dem_framepos;
	dem_framepos = dem_framepos->next;
	free (top);
}

/*
====================
CL_GetMessage

Handles recording and playback of demos, on top of NET_ code
====================
*/
int CL_GetMessage (void)
{
	int		r, i;
	float	f;

	if (cl.paused & 2)
		return 0;

	if	(cls.demoplayback)
	{
		if (start_of_demo && cls.demorewind)
			return 0;

		if (cls.signon < SIGNONS)	// clear stuffs if new demo
			while (dem_framepos)
				EraseTopEntry ();

	// decide if it is time to grab the next message
		if (cls.signon == SIGNONS)	// always grab until fully connected
		{
			if (cls.timedemo)
			{
				if (host_framecount == cls.td_lastframe)
					return 0;		// already read this frame's message
				cls.td_lastframe = host_framecount;
			// if this is the second frame, grab the real td_starttime
			// so the bogus time on the first frame doesn't count
				if (host_framecount == cls.td_startframe + 1)
					cls.td_starttime = realtime;
			}
			// modified by joe to handle rewind playing
			else if (!cls.demorewind && cl.ctime <= cl.mtime[0])
					return 0;		// don't need another message yet
			else if (cls.demorewind && cl.ctime >= cl.mtime[0])
				return 0;

			// joe: fill in the stack of frames' positions
			// enable on intermission or not...?
			// NOTE: it can't handle fixed intermission views!
			if (!cls.demorewind /*&& !cl.intermission*/)
				PushFrameposEntry (ftell(cls.demofile));
		}

	// get the next message
		cls.demo_offset_current = ftell(cls.demofile);
		fread (&net_message.cursize, 4, 1, cls.demofile);
		VectorCopy (cl.mviewangles[0], cl.mviewangles[1]);
		for (i=0 ; i<3 ; i++)
		{
			r = fread (&f, 4, 1, cls.demofile);
			cl.mviewangles[0][i] = LittleFloat (f);
		}

		net_message.cursize = LittleLong (net_message.cursize);
		if (net_message.cursize > MAX_MSGLEN)
			Sys_Error ("Demo message > MAX_MSGLEN");

		r = fread (net_message.data, net_message.cursize, 1, cls.demofile);
		if (r != 1)
		{
			CL_StopPlayback ();
			return 0;
		}

		// joe: get out framestack's top entry
		if (cls.demorewind /*&& !cl.intermission*/)
		{
			if (dem_framepos/* && dem_framepos->baz*/)	// Baker: in theory, if this occurs we ARE at the start of the demo with demo rewind on
			{
			fseek (cls.demofile, dem_framepos->baz, SEEK_SET);
				EraseTopEntry (); // Baker: we might be able to improve this better but not right now.
			}
			if (!dem_framepos)
				bumper_on = start_of_demo = true;
		}

		return 1;
	}

	while (1)
	{
		r = NET_GetMessage (cls.netcon);

		if (r != 1 && r != 2)
			return r;

	// discard nop keepalive message
		if (net_message.cursize == 1 && net_message.data[0] == svc_nop)
			Con_Printf ("<-- server to client keepalive\n");
		else
			break;
	}

	if (cls.demorecording)
		CL_WriteDemoMessage ();

	// JPG 1.05 - support for recording demos after connecting
	if (cls.signon < 2)
	{
		memcpy(demo_head[cls.signon], net_message.data, net_message.cursize);
		demo_head_size[cls.signon] = net_message.cursize;

		if (!cls.signon)
		{
			char *ch;
			int len;

			len = strlen(demo_head[0]);
			ch = strstr(demo_head[0] + len + 1, "ProQuake Server Version");
			if (ch)
				memcpy(ch, va("ProQuake \217Demo\217 Version %4.2f", PROQUAKE_SERIES_VERSION), 28);
			else
			{
				ch = demo_head[0] + demo_head_size[0];
				*ch++ = svc_print;
				ch += 1 + sprintf(ch, "\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n"
								      "\n   \01\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\03");
				*ch++ = svc_print;
				ch += 1 + sprintf(ch, "\02\n   \04ProQuake \217Demo\217 Version %4.2f\06"
								      "\n   \07\10\10\10\10\10\10\10\10\10\10\10\10\10\10\10\10\10\10\10\10\10\10\10\10\10\10\10\10\11"
							          "\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n", PROQUAKE_SERIES_VERSION);
				demo_head_size[0] = ch - (char *) demo_head[0];
			}
		}
	}

	return r;
}


/*
====================
CL_Stop_f

stop recording a demo
====================
*/
void CL_Stop_f (void)
{
	if (cmd_source != src_command)
		return;

	if (!cls.demorecording)
	{
		Con_Printf ("Not recording a demo\n");
		return;
	}

// write a disconnect message to the demo file
	SZ_Clear (&net_message);
	MSG_WriteByte (&net_message, svc_disconnect);
	CL_WriteDemoMessage ();

// finish up
	fclose (cls.demofile);
	strlcpy (cls.recent_file, cls.demoname, sizeof(cls.recent_file) ); // Close demo instance #2 (record end)

	cls.demofile = NULL;
	cls.demorecording = false;
	Con_Printf ("Completed demo\n");
}

void CL_Clear_Demos_Queue (void)
{
	int i;
	for (i = 0;i < MAX_DEMOS; i ++)	// Clear demo loop queue
		cls.demos[i][0] = 0;
	cls.demonum = -1;				// Set next demo to none
}
/*
====================
CL_Record_f

record <demoname> <map> [cd track]
====================
*/

void CL_Record_f (void)
{
	int		c;
	char	name[MAX_OSPATH];
	int		track;

	if (cmd_source != src_command)
		return;

	if (cls.demoplayback) 
	{
		Con_Printf ("Can't record during demo playback\n");
		return;
	}

	c = Cmd_Argc();
	// Baker: demo parameters = 2 thru 4
	// Not supporting autoname yet (==1)
	if (c != 2 && c != 3 && c != 4)
	{
		Con_Printf ("record <demoname> [<map> [cd track]]\n");
		return;
	}

	if (strstr(Cmd_Argv(1), ".."))
	{
		Con_Printf ("Relative pathnames are not allowed.\n");
		return;
	}

	// JPG 3.00 - consecutive demo bug
	if (cls.demorecording)
		CL_Stop_f();

	// JPG 1.05 - replaced it with this
	if (c == 2 && cls.state == ca_connected && cls.signon < 2)
	{
		Con_Printf("Can't record - try again when connected\n");
		return;
	}

	// write the forced cd track number, or -1
	if (c == 4)
	{
		track = atoi(Cmd_Argv(3));
		Con_Printf ("Forcing CD track to %i\n", cls.forcetrack);
	}
	else
		track = -1;

	SNPrintf(name, sizeof(name), "%s/%s", com_gamedir, Cmd_Argv(1));
	CL_Clear_Demos_Queue (); // timedemo is a very intentional action

//
// start the map up
//
	if (c > 2)
	{
		Cmd_ExecuteString ( va("map %s", Cmd_Argv(2)), src_command);
		// If couldn't find the map, don't start recording
		if (cls.state != ca_connected)
			return;
	}

//
// open the demo file
//
	COM_ForceExtension (name, ".dem");

	Con_Printf ("recording to %s.\n", name);
	cls.demofile = fopen (name, "wb");
	if (!cls.demofile)
	{
		Con_Printf ("ERROR: couldn't open demo for writing.\n");
		return;
	}

	// Officially recording ... copy the name for reference
	SNPrintf(cls.demoname, sizeof(cls.demoname), "%s", name);

	cls.forcetrack = track;
	fprintf (cls.demofile, "%i\n", cls.forcetrack);

	cls.demorecording = true;

	// Initialize the demo file if we're already connected
	if (c == 2 && cls.state == ca_connected)
	{
		byte *data = net_message.data;
		int cursize = net_message.cursize;
		int i;

		for (i = 0 ; i < 2 ; i++)
		{
			net_message.data = demo_head[i];
			net_message.cursize = demo_head_size[i];
			CL_WriteDemoMessage();
		}

		net_message.data = demo_head[2];
		SZ_Clear (&net_message);

		// current names, colors, and frag counts
		for (i=0 ; i < cl.maxclients ; i++)
		{
			MSG_WriteByte (&net_message, svc_updatename);
			MSG_WriteByte (&net_message, i);
			MSG_WriteString (&net_message, cl.scores[i].name);
			MSG_WriteByte (&net_message, svc_updatefrags);
			MSG_WriteByte (&net_message, i);
			MSG_WriteShort (&net_message, cl.scores[i].frags);
			MSG_WriteByte (&net_message, svc_updatecolors);
			MSG_WriteByte (&net_message, i);
			MSG_WriteByte (&net_message, cl.scores[i].colors);
		}

		// send all current light styles
		for (i = 0 ; i < MAX_LIGHTSTYLES ; i++)
		{
			MSG_WriteByte (&net_message, svc_lightstyle);
			MSG_WriteByte (&net_message, i);
			MSG_WriteString (&net_message, cl_lightstyle[i].map);
		}

		// Write out single player stats too
		MSG_WriteByte (&net_message, svc_updatestat);
		MSG_WriteByte (&net_message, STAT_TOTALSECRETS);
		MSG_WriteLong (&net_message, pr_global_struct->total_secrets);

		MSG_WriteByte (&net_message, svc_updatestat);
		MSG_WriteByte (&net_message, STAT_TOTALMONSTERS);
		MSG_WriteLong (&net_message, pr_global_struct->total_monsters);

		MSG_WriteByte (&net_message, svc_updatestat);
		MSG_WriteByte (&net_message, STAT_SECRETS);
		MSG_WriteLong (&net_message, pr_global_struct->found_secrets);

		MSG_WriteByte (&net_message, svc_updatestat);
		MSG_WriteByte (&net_message, STAT_MONSTERS);
		MSG_WriteLong (&net_message, pr_global_struct->killed_monsters);

		// view entity
		MSG_WriteByte (&net_message, svc_setview);
		MSG_WriteShort (&net_message, cl.viewentity);

		// signon
		MSG_WriteByte (&net_message, svc_signonnum);
		MSG_WriteByte (&net_message, 3);

		CL_WriteDemoMessage();

		// restore net_message
		net_message.data = data;
		net_message.cursize = cursize;
	}
}

 void StartPlayingOpenedDemo (void)
{
	int		c;
	qboolean	neg = false;

	cls.demoplayback = true;
	cls.state = ca_connected;
	cls.forcetrack = 0;

	while ((c = getc(cls.demofile)) != '\n')
	{
		if (c == '-')
			neg = true;
		else
			cls.forcetrack = cls.forcetrack * 10 + (c - '0');
	}

	if (neg)
		cls.forcetrack = -cls.forcetrack;
}

/*
====================
CL_PlayDemo_f

playdemo [demoname]
====================
*/
// Baker: So we know this is a real start demo
qboolean play_as_start_demo = false;
void CL_PlayDemo_NextStartDemo_f (void)
{
	play_as_start_demo = true;
	CL_PlayDemo_f (); // Inherits the cmd_argc and cmd_argv
	play_as_start_demo = false;
}

void CL_PlayDemo_f (void)
{
	char	name[256];

	if (cmd_source != src_command)
		return;

	if (!play_as_start_demo)
		CL_Clear_Demos_Queue ();
	if (Cmd_Argc() != 2)
	{
		Con_Printf ("playdemo <demoname> : plays a demo\n");
		return;
	}


//
// disconnect from server
//
	CL_Disconnect ();

	// Revert
	cls.demorewind = false;
	cls.demospeed = 0; // 0 = Don't use
	bumper_on = false;

//
// open the demo file
//
	strlcpy (name, Cmd_Argv(1), sizeof(name));
	COM_DefaultExtension (name, ".dem");

	COM_FOpenFile (name, &cls.demofile);
	if (!cls.demofile) 
	{
#ifdef SUPPORTS_DEMO_AUTOPLAY
		// Baker 3.76 - Check outside the demos folder!
		cls.demofile = fopen(name, "rb"); // Baker 3.76 - check DarkPlaces file system

		if (!cls.demofile)  // Baker 3.76 - still failed
#endif
		{
			Con_Printf ("ERROR: couldn't open %s\n", name);
			cls.demonum = -1;		// stop demo loop
			return;
		}
	}

	SNPrintf(cls.demoname, sizeof(cls.demoname), "%s", name);
	cls.demo_offset_start = ftell (cls.demofile);	// qfs_lastload.offset instead?
	cls.demo_file_length = com_filesize;
	cls.demo_hosttime_start	= cls.demo_hosttime_elapsed = 0; // Fill this in ... host_time;
	cls.demo_cltime_start = cls.demo_cltime_elapsed = 0; // Fill this in

	Con_Printf ("Playing demo from %s\n", COM_SkipPath(name));

	StartPlayingOpenedDemo ();
}

/*
====================
CL_FinishTimeDemo
====================
*/
 void CL_FinishTimeDemo (void)
{
	int		frames;
	float	time;

	cls.timedemo = false;

// the first frame didn't count
	frames = (host_framecount - cls.td_startframe) - 1;
	time = realtime - cls.td_starttime;
	if (!time)
		time = 1;
	Con_Printf ("%i frames %5.1f seconds %5.1f fps\n", frames, time, frames/time);
}

/*
====================
CL_TimeDemo_f

timedemo [demoname]
====================
*/
void CL_TimeDemo_f (void)
{
	if (cmd_source != src_command)
		return;

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("timedemo <demoname> : gets demo speeds\n");
		return;
	}

	// Baker: Close the console for timedemo
	// Baker: This is a performance benchmark.  No reason to have console up.
	if (key_dest != key_game)
		key_dest = key_game;
	CL_Clear_Demos_Queue (); // timedemo is a very intentional action

	CL_PlayDemo_f ();

	// don't trigger timedemo mode if playdemo fails
	if (!cls.demofile) return;

// cls.td_starttime will be grabbed at the second frame of the demo, so
// all the loading time doesn't get counted

	cls.timedemo = true;
	cls.td_startframe = host_framecount;
	cls.td_lastframe = -1;		// get a new message this frame
}
