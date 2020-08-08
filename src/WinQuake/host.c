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
// host.c -- coordinates spawning and killing of local servers

#include "quakedef.h"

#ifdef _WIN32
#include "movie.h"
#endif

//#include "r_local.h"

/*

A server can always be started, even if the system started out as a client
to a remote system.

A client can NOT be started if the system started as a dedicated server.

Memory is cleared / released when a server or client begins, not when they end.

*/

quakeparms_t host_parms;

qboolean	host_initialized;		// true if into command execution

double		host_frametime;
double		host_time;
double		realtime;				// without any filtering or bounding
double		oldrealtime;			// last frame run
double		last_angle_time;		// JPG - for smooth chasecam
int			host_framecount;

int			host_hunklevel;

int			minimum_memory;

client_t	*host_client;			// current client

jmp_buf 	host_abortserver;

byte		*host_basepal;
byte		*host_colormap;
#ifdef _WIN32
qboolean	nostartdemos = false;
#endif

cvar_t	host_framerate = {"host_framerate","0"};	// set for slow motion
cvar_t	host_speeds = {"host_speeds","0"};			// set for running times
cvar_t	host_timescale = {"host_timescale", "0"}; //johnfitz

cvar_t	sys_ticrate = {"sys_ticrate","0.05", false, true};
cvar_t	serverprofile = {"serverprofile","0"};

cvar_t	fraglimit = {"fraglimit","0",false,true};
cvar_t	timelimit = {"timelimit","0",false,true};
cvar_t	teamplay = {"teamplay","0",false,true};

cvar_t	samelevel = {"samelevel","0"};
cvar_t	noexit = {"noexit","0",false,true};

#ifdef QUAKE2
cvar_t	developer = {"developer","1"};	// should be 0 for release!
#else
cvar_t	developer = {"developer","0"};
#endif

cvar_t	skill = {"skill","1"};						// 0 - 3
cvar_t	deathmatch = {"deathmatch","0"};			// 0, 1, or 2
cvar_t	coop = {"coop","0"};			// 0 or 1

cvar_t	pausable = {"pausable","1"};

cvar_t	temp1 = {"temp1","0"};

void Host_WriteConfig_f (void);

cvar_t	proquake = {"proquake", "L33T"}; // JPG - added this

// JPG - spam protection.  If a client's msg's/second exceeds spam rate
// for an extended period of time, the client is spamming.  Clients are
// allowed a temporary grace of pq_spam_grace messages.  Once used up,
// this grace regenerates while the client shuts up at a rate of one
// message per pq_spam_rate seconds.
cvar_t	pq_spam_rate = {"pq_spam_rate", "0"};  // Baker 3.80x - Set to default of 0; was 1.5 -- bad for coop
cvar_t	pq_spam_grace = {"pq_spam_grace", "999"}; // Baker 3.80x - Set to default of 999; was 10 -- bad for coop

// Baker 3.99g - from Rook ... protect against players connecting and spamming before banfile can kick in
cvar_t	pq_connectmute = {"pq_connectmute", "0", false, true};  // (value in seconds)

// JPG 3.20 - control muting of players that change colour/name
cvar_t	pq_tempmute = {"pq_tempmute", "0"};  // Baker 3.80x - Changed default to 0; was 1 -- interfered with coop

// JPG 3.20 - optionally write player binds to server log
cvar_t	pq_logbinds = {"pq_logbinds", "0"};

// JPG 3.11 - feature request from Slot Zero
cvar_t	pq_showedict = {"pq_showedict", "0"};

// JPG 1.05 - translate dedicated server console output to plain text
cvar_t	pq_dequake = {"pq_dequake", "1"};
cvar_t	pq_maxfps = {"pq_maxfps", "72.0", true};	// Baker 3.80x - save this to config

/*
================
Host_EndGame
================
*/
void Host_EndGame (char *message, ...)
{
	va_list		argptr;
	char		string[1024];

	va_start (argptr,message);
	vsnprintf (string,sizeof(string),message,argptr);
	va_end (argptr);
	Con_DPrintf ("Host_EndGame: %s\n",string);

	if (sv.active)
		Host_ShutdownServer (false);

	if (cls.state == ca_dedicated)
		Sys_Error ("Host_EndGame: %s\n",string);	// dedicated servers exit

if (cls.demonum != -1)
	{
		CL_StopPlayback ();	// JPG 1.05 - patch by CSR to fix crash
		CL_NextDemo ();
	}
	else
		CL_Disconnect ();

	longjmp (host_abortserver, 1);
}

/*
================
Host_Error

This shuts down both the client and server
================
*/
void Host_Error (char *error, ...)
{
	va_list		argptr;
	char		string[1024];
	static	qboolean inerror = false;

	if (inerror)
		Sys_Error ("Host_Error: recursively entered");
	inerror = true;

	SCR_EndLoadingPlaque ();		// reenable screen updates

	va_start (argptr,error);
	vsnprintf (string,sizeof(string),error,argptr);
	va_end (argptr);
	Con_Printf ("Host_Error: %s\n",string);

	if (sv.active)
		Host_ShutdownServer (false);

	if (cls.state == ca_dedicated)
		Sys_Error ("Host_Error: %s\n",string);	// dedicated servers exit

	CL_Disconnect ();
	cls.demonum = -1;

	inerror = false;

	longjmp (host_abortserver, 1);
}

/*
================
Host_FindMaxClients
================
*/
void	Host_FindMaxClients (void)
{
	int		i;

	svs.maxclients = 1;

	i = COM_CheckParm ("-dedicated");
	if (i)
	{
		cls.state = ca_dedicated;
		if (i != (com_argc - 1))
		{
			svs.maxclients = Q_atoi (com_argv[i+1]);
		}
		else
			svs.maxclients = 8;
	}
	else
		cls.state = ca_disconnected;

	i = COM_CheckParm ("-listen");
	if (i)
	{
		if (cls.state == ca_dedicated)
			Sys_Error ("Only one of -dedicated or -listen can be specified");
		if (i != (com_argc - 1))
			svs.maxclients = Q_atoi (com_argv[i+1]);
		else
			svs.maxclients = 8;
	}
	if (svs.maxclients < 1)
		svs.maxclients = 8;
	else if (svs.maxclients > MAX_SCOREBOARD)
		svs.maxclients = MAX_SCOREBOARD;

	svs.maxclientslimit = svs.maxclients;
	if (svs.maxclientslimit < 4)
		svs.maxclientslimit = 4;
	svs.clients = Hunk_AllocName (svs.maxclientslimit*sizeof(client_t), "clients");

	if (svs.maxclients > 1)
		Cvar_SetValue ("deathmatch", 1.0);
	else
		Cvar_SetValue ("deathmatch", 0.0);
}

char dequake[256];	// JPG 1.05

/*
=======================
Host_InitDeQuake

JPG 1.05 - initialize the dequake array
======================
*/
void Host_InitDeQuake (void)
{
	int i;

	for (i = 1 ; i < 12 ; i++)
		dequake[i] = '#';
	dequake[9] = 9;
	dequake[10] = 10;
	dequake[13] = 13;
	dequake[12] = ' ';
	dequake[1] = dequake[5] = dequake[14] = dequake[15] = dequake[28] = '.';
	dequake[16] = '[';
	dequake[17] = ']';
	for (i = 0 ; i < 10 ; i++)
		dequake[18 + i] = '0' + i;
	dequake[29] = '<';
	dequake[30] = '-';
	dequake[31] = '>';
	for (i = 32 ; i < 128 ; i++)
		dequake[i] = i;
	for (i = 0 ; i < 128 ; i++)
		dequake[i+128] = dequake[i];
	dequake[128] = '(';
	dequake[129] = '=';
	dequake[130] = ')';
	dequake[131] = '*';
	dequake[141] = '>';
}

/*
=======================
Host_InitLocal
======================
*/
void Host_InitLocal (void)
{
	Host_InitCommands ();

	Cvar_RegisterVariable (&host_framerate, NULL);
	Cvar_RegisterVariable (&host_speeds, NULL);
	Cvar_RegisterVariable (&host_timescale, NULL); //johnfitz

	Cvar_RegisterVariable (&sys_ticrate, NULL);
	Cvar_RegisterVariable (&serverprofile, NULL);

	Cvar_RegisterVariable (&fraglimit, NULL);
	Cvar_RegisterVariable (&timelimit, NULL);
	Cvar_RegisterVariable (&teamplay, NULL);
	Cvar_RegisterVariable (&samelevel, NULL);
	Cvar_RegisterVariable (&noexit, NULL);
	Cvar_RegisterVariable (&skill, NULL);
	Cvar_RegisterVariable (&developer, NULL);
	Cvar_RegisterVariable (&deathmatch, NULL);
	Cvar_RegisterVariable (&coop, NULL);

	Cvar_RegisterVariable (&pausable, NULL);

	Cvar_RegisterVariable (&temp1, NULL);

	Cmd_AddCommand ("writeconfig", Host_WriteConfig_f);	// by joe

	Cvar_RegisterVariable (&proquake, NULL);		// JPG - added this so QuakeC can find it
	Cvar_RegisterVariable (&pq_spam_rate, NULL);	// JPG - spam protection
	Cvar_RegisterVariable (&pq_spam_grace, NULL);	// JPG - spam protection
	Cvar_RegisterVariable (&pq_connectmute, NULL);	// Baker 3.99g: from Rook, protection against repeatedly connecting + spamming
	Cvar_RegisterVariable (&pq_tempmute, NULL);	// JPG 3.20 - temporary muting
	Cvar_RegisterVariable (&pq_showedict, NULL);	// JPG 3.11 - feature request from Slot Zero
	Cvar_RegisterVariable (&pq_dequake, NULL);	// JPG 1.05 - translate dedicated console output to plain text
	Cvar_RegisterVariable (&pq_maxfps, NULL);		// JPG 1.05
	Cvar_RegisterVariable (&pq_logbinds, NULL);	// JPG 3.20 - log player binds

	Host_FindMaxClients ();

	host_time = 1.0;		// so a think at time 0 won't get called

	last_angle_time = 0.0;  // JPG - smooth chasecam

	Host_InitDeQuake();	// JPG 1.05 - initialize dequake array
}

// Baker 3.60 - Host Write Config - from JoeQuake

/*
===============
Host_WriteConfig
===============
*/

void Host_WriteConfig (char *cfgname)
{
	FILE	*f;
	extern cvar_t cmdline;

	if (!(f = fopen(va("%s/%s", com_gamedir, cfgname), "w")))
	{
		Con_Printf ("Couldn't write %s\n", cfgname);
		return;
	}

#ifdef _WIN32
#ifdef GLQUAKE
	VID_SyncCvars (); //johnfitz -- write actual current mode to config file, in case cvars were messed with
#endif
#endif

	fprintf (f, "// *****************************************\n"); // Baker 3.60
	fprintf (f, "// Generated by ProQuake %4.2f \n", (float)PROQUAKE_VERSION); // Baker 3.60
	fprintf (f, "// *****************************************\n"); // Baker 3.60
#ifdef GLQUAKE
	fprintf (f, "// Video Information:\n\n"); // Baker 3.60
	fprintf (f, "// Vendor  :   %s\n",gl_vendor); // Baker 3.60
	fprintf (f, "// Renderer:   %s\n",gl_renderer); // Baker 3.60
	fprintf (f, "// GL Version: %s\n",gl_version); // Baker 3.60
	fprintf (f, "// *****************************************\n"); // Baker 3.60
#endif
	fprintf (f, "// Your last commmand line: \n//\n"); // Baker 3.60  */
	fprintf (f, "// %s \n", cmdline.string); // Baker 3.60  */
	fprintf (f, "//\n");
	fprintf (f, "//\n");
	// fprintf (f, "// Your Quake folder: %s\n", com_gamedir);  // Later!
	fprintf (f, "// Your maps folder:  %s\\maps\n", com_gamedir);
	fprintf (f, "// Your demos folder:  %s\n", com_gamedir);
	fprintf (f, "// Your screenshots folder:  %s\n", com_gamedir);
	fprintf (f, "//\n");
	//fprintf (f, "// Generated by JoeQuake\n");

	Key_WriteBindings (f);

	Cvar_WriteVariables (f);
	fprintf (f, "\n// *****************************************\n"); // Baker 3.60  */

#ifdef _WIN32
#ifdef GLQUAKE
	//johnfitz -- extra commands to preserve state
	fprintf (f, "vid_restart\n");
#endif
#endif
	fclose (f);
}

/*
===============
Host_WriteConfiguration

Writes key bindings and archived cvars to config.cfg
===============
*/
void Host_WriteConfiguration (void)
{
// dedicated servers initialize the host but don't parse and set the config.cfg cvars
	if (host_initialized && !isDedicated) // 1999-12-24 logical correction by Maddes
		Host_WriteConfig ("config.cfg");
}

/*
===============
Host_WriteConfig_f

Writes key bindings and ONLY archived cvars to a custom config file
===============
*/
void Host_WriteConfig_f (void)
	{
	char	name[MAX_OSPATH];

	if (Cmd_Argc() != 2)
		{
		Con_Printf ("Usage: writeconfig <filename>\n");
			return;
		}

	Q_strncpyz (name, Cmd_Argv(1), sizeof(name));
	COM_ForceExtension (name, ".cfg");

	Con_Printf ("Writing %s\n", name);

	Host_WriteConfig (name);
}


/*
=================
SV_ClientPrintf

Sends text across to be displayed
FIXME: make this just a stuffed echo?
=================
*/
void SV_ClientPrintf (char *fmt, ...)
{
	va_list		argptr;
	char		string[1024];

	va_start (argptr,fmt);
	vsnprintf (string, sizeof(string),fmt,argptr);
	va_end (argptr);

	MSG_WriteByte (&host_client->message, svc_print);
	MSG_WriteString (&host_client->message, string);
}

/*
=================
SV_BroadcastPrintf

Sends text to all active clients
=================
*/
void SV_BroadcastPrintf (char *fmt, ...)
{
	va_list		argptr;
	char		string[1024];
	int			i;

	va_start (argptr,fmt);
	vsnprintf (string, sizeof(string),fmt,argptr);
	va_end (argptr);

	for (i=0 ; i<svs.maxclients ; i++)
		if (svs.clients[i].active && svs.clients[i].spawned)
		{
			MSG_WriteByte (&svs.clients[i].message, svc_print);
			MSG_WriteString (&svs.clients[i].message, string);
		}
}

/*
=================
Host_ClientCommands

Send text over to the client to be executed
=================
*/
void Host_ClientCommands (char *fmt, ...)
{
	va_list		argptr;
	char		string[1024];

	va_start (argptr,fmt);
	vsnprintf (string, sizeof(string), fmt,argptr);
	va_end (argptr);

	MSG_WriteByte (&host_client->message, svc_stufftext);
	MSG_WriteString (&host_client->message, string);
}

/*
=====================
SV_DropClient

Called when the player is getting totally kicked off the host
if (crash = true), don't bother sending signofs
=====================
*/
void SV_DropClient (qboolean crash)
{
	int		saveSelf;
	int		i;
	client_t *client;

// JPG 3.00 - don't drop a client that's already been dropped!
	if (!host_client->active)
		return;

	if (!crash)
	{
		// send any final messages (don't check for errors)
		if (NET_CanSendMessage (host_client->netconnection))
		{
			MSG_WriteByte (&host_client->message, svc_disconnect);
			NET_SendMessage (host_client->netconnection, &host_client->message);
		}

		if (host_client->edict && host_client->spawned)
		{
		// call the prog function for removing a client
		// this will set the body to a dead frame, among other things
			saveSelf = pr_global_struct->self;
			pr_global_struct->self = EDICT_TO_PROG(host_client->edict);
			PR_ExecuteProgram (pr_global_struct->ClientDisconnect);
			pr_global_struct->self = saveSelf;
		}

		Sys_Printf ("Client %s removed\n",host_client->name);
	}

	// JPG 3.00 - check to see if it's a qsmack client
	if (host_client->netconnection->mod == MOD_QSMACK)
		qsmackActive = false;

// break the net connection
	NET_Close (host_client->netconnection);
	host_client->netconnection = NULL;

// free the client (the body stays around)
	host_client->active = false;
	host_client->name[0] = 0;
	host_client->old_frags = -999999;
	net_activeconnections--;

// send notification to all clients
	for (i=0, client = svs.clients ; i<svs.maxclients ; i++, client++)
	{
		if (!client->active)
			continue;
		MSG_WriteByte (&client->message, svc_updatename);
		MSG_WriteByte (&client->message, host_client - svs.clients);
		MSG_WriteString (&client->message, "");
		MSG_WriteByte (&client->message, svc_updatefrags);
		MSG_WriteByte (&client->message, host_client - svs.clients);
		MSG_WriteShort (&client->message, 0);
		MSG_WriteByte (&client->message, svc_updatecolors);
		MSG_WriteByte (&client->message, host_client - svs.clients);
		MSG_WriteByte (&client->message, 0);
	}
}

/*
==================
Host_ShutdownServer

This only happens at the end of a game, not between levels
==================
*/
void Host_ShutdownServer(qboolean crash)
{
	int		i, count;
	sizebuf_t		buf;
	unsigned char	message[4];
	double			start;

	if (!sv.active)
		return;

	sv.active = false;

// stop all client sounds immediately
	if (cls.state == ca_connected)
		CL_Disconnect ();

// flush any pending messages - like the score!!!
	start = Sys_DoubleTime();
	do
	{
		count = 0;
		for (i=0, host_client = svs.clients ; i<svs.maxclients ; i++, host_client++)
		{
			if (host_client->active && host_client->message.cursize)
			{
				if (NET_CanSendMessage (host_client->netconnection))
				{
					NET_SendMessage(host_client->netconnection, &host_client->message);
					SZ_Clear (&host_client->message);
				}
				else
				{
					NET_GetMessage(host_client->netconnection);
					count++;
				}
			}
		}
		if ((Sys_DoubleTime() - start) > 3.0)
			break;
	}
	while (count);

// make sure all the clients know we're disconnecting
	buf.data = message;
	buf.maxsize = 4;
	buf.cursize = 0;
	MSG_WriteByte(&buf, svc_disconnect);
	count = NET_SendToAll(&buf, 5);
	if (count)
		Con_Printf("Host_ShutdownServer: NET_SendToAll failed for %u clients\n", count);

	for (i=0, host_client = svs.clients ; i<svs.maxclients ; i++, host_client++)
		if (host_client->active)
			SV_DropClient(crash);

//
// clear structures
//
	memset (&sv, 0, sizeof(sv));
	memset (svs.clients, 0, svs.maxclientslimit*sizeof(client_t));
}


/*
================
Host_ClearMemory

This clears all the memory used by both the client and server, but does
not reinitialize anything.
================
*/
void Host_ClearMemory (void)
{
	Con_DPrintf ("Clearing memory\n");
	D_FlushCaches ();
	Mod_ClearAll ();
	if (host_hunklevel)
		Hunk_FreeToLowMark (host_hunklevel);

	cls.signon = 0;
	memset (&sv, 0, sizeof(sv));
	memset (&cl, 0, sizeof(cl));
}


//==============================================================================
//
// Host Frame
//
//==============================================================================

/*
===================
Host_FilterTime

Returns false if the time is too short to run a frame
===================
*/
qboolean Host_FilterTime (float time)
{
	double	fps;

	realtime += time;

	fps = max(10, pq_maxfps.value);

	if (!cls.capturedemo && !cls.timedemo && realtime - oldrealtime < 1.0 / fps)
		return false;		// framerate is too high

#ifdef _WIN32
	if (Movie_IsActive())
		host_frametime = Movie_FrameTime ();
	else
#endif

	host_frametime = realtime - oldrealtime;
	if (cls.demoplayback)
		host_frametime *= bound(0, cl_demospeed.value, 20);
	oldrealtime = realtime;

	//johnfitz -- host_timescale is more intuitive than host_framerate
	if (host_timescale.value > 0)
		host_frametime *= host_timescale.value;
	//johnfitz
	else if (host_framerate.value > 0)
		host_frametime = host_framerate.value;
	else // don't allow really long or short frames
		host_frametime = bound(0.001, host_frametime, 0.1);

	return true;
}

/*
===================
Host_GetConsoleCommands

Add them exactly as if they had been typed at the console
===================
*/
void Host_GetConsoleCommands (void)
{
	char	*cmd;

	while (1)
	{
		cmd = Sys_ConsoleInput ();
		if (!cmd)
			break;
		Cbuf_AddText (cmd);
	}
}

/*
==================
Host_ServerFrame
==================
*/
#ifdef FPS_20

void _Host_ServerFrame (void)
{
// run the world state
	pr_global_struct->frametime = host_frametime;

// read client messages
	SV_RunClients ();

// move things around and think
// always pause in single player if in console or menus
	if (!sv.paused && (svs.maxclients > 1 || key_dest == key_game) )
		SV_Physics ();
}

void Host_ServerFrame (void)
{
	float	save_host_frametime;
	float	temp_host_frametime;

// run the world state
	pr_global_struct->frametime = host_frametime;

// set the time and clear the general datagram
	SV_ClearDatagram ();

// check for new clients
	SV_CheckForNewClients ();

	temp_host_frametime = save_host_frametime = host_frametime;
	while(temp_host_frametime > (1.0/72.0))
	{
		if (temp_host_frametime > 0.05)
			host_frametime = 0.05;
		else
			host_frametime = temp_host_frametime;
		temp_host_frametime -= host_frametime;
		_Host_ServerFrame ();
	}
	host_frametime = save_host_frametime;

// send all messages to the clients
	SV_SendClientMessages ();
}

#else

void Host_ServerFrame (void)
{
// JPG 3.00 - stuff the port number into the server console once every minute
	static double port_time = 0;

	if (port_time > sv.time + 1 || port_time < sv.time - 60)
	{
		port_time = sv.time;
		Cmd_ExecuteString(va("port %d\n", net_hostport), src_command);
	}

// run the world state
	pr_global_struct->frametime = host_frametime;

// set the time and clear the general datagram
	SV_ClearDatagram ();

// check for new clients
	SV_CheckForNewClients ();

// read client messages
	SV_RunClients ();

// move things around and think
// always pause in single player if in console or menus
	if (!sv.paused && (svs.maxclients > 1 || key_dest == key_game) )
		SV_Physics ();

// send all messages to the clients
	SV_SendClientMessages ();
}

#endif

/*
==================
Host_Frame

Runs all active servers
==================
*/
void _Host_Frame (double time)
{
	static double		time1 = 0, time2 = 0, time3 = 0;
	int			pass1, pass2, pass3;

	if (setjmp (host_abortserver) )
		return;			// something bad happened, or the server disconnected

// keep the random time dependent
	rand ();

// decide the simulation time
	if (!Host_FilterTime (time))
	{
		// JPG - if we're not doing a frame, still check for lagged moves to send
		if (!sv.active && (cl.movemessages > 2))
			CL_SendLagMove();
		return;			// don't run too fast, or packets will flood out
	}

// get new key events
#if !defined (__APPLE__) && !defined (MACOSX)
	Sys_SendKeyEvents ();
#endif /* !__APPLE__ && !MACOSX */

// allow mice or other external controllers to add commands
	IN_Commands ();

// process console commands
	Cbuf_Execute ();

	NET_Poll();

// if running the server locally, make intentions now
	if (sv.active)
		CL_SendCmd ();

//-------------------
//
// server operations
//
//-------------------

// check for commands typed to the host
	Host_GetConsoleCommands ();

	if (sv.active)
		Host_ServerFrame ();

//-------------------
//
// client operations
//
//-------------------

// if running the server remotely, send intentions now after
// the incoming messages have been read
	if (!sv.active)
		CL_SendCmd ();

	host_time += host_frametime;

// fetch results from server
	if (cls.state == ca_connected)
                CL_ReadFromServer ();

	if (host_speeds.value)
		time1 = Sys_DoubleTime ();

// update video
	SCR_UpdateScreen ();

	if (host_speeds.value)
		time2 = Sys_DoubleTime ();

	if (cls.signon == SIGNONS)
	{
		// update audio
		S_Update (r_origin, vpn, vright, vup);
		CL_DecayLights ();
	}
	else
	{
		S_Update (vec3_origin, vec3_origin, vec3_origin, vec3_origin);
	}

	CDAudio_Update();

	if (host_speeds.value)
	{
		pass1 = (time1 - time3)*1000;
		time3 = Sys_DoubleTime ();
		pass2 = (time2 - time1)*1000;
		pass3 = (time3 - time2)*1000;
		Con_Printf ("%3i tot %3i server %3i gfx %3i snd\n", pass1+pass2+pass3, pass1, pass2, pass3);
	}

	if (!cls.demoplayback && cl_demorewind.value)
	{
		Cvar_Set ("cl_demorewind", "0");
		Con_Printf ("Demorewind is only enabled during playback\n");
	}

	host_framecount++;
}

void Host_Frame (double time)
{
	double	time1, time2;
	static double	timetotal;
	static int		timecount;
	int		i, c, m;

	if (!serverprofile.value)
	{
		_Host_Frame (time);
		return;
	}

	time1 = Sys_DoubleTime ();
	_Host_Frame (time);
	time2 = Sys_DoubleTime ();

	timetotal += time2 - time1;
	timecount++;

	if (timecount < 1000)
		return;

	m = timetotal*1000/timecount;
	timecount = timetotal = 0;
	c = 0;
	for (i=0 ; i<svs.maxclients ; i++)
		if (svs.clients[i].active)
			c++;

	Con_Printf ("serverprofile: %2i clients %2i msec\n",  c,  m);
}

//============================================================================


extern int vcrFile;
#define	VCR_SIGNATURE	0x56435231
// "VCR1"

void Host_InitVCR (quakeparms_t *parms)
{
	int		i, len, n;
	char	*p;

	if (COM_CheckParm("-playback"))
	{
		if (com_argc != 2)
			Sys_Error("No other parameters allowed with -playback\n");

		Sys_FileOpenRead("quake.vcr", &vcrFile);
		if (vcrFile == -1)
			Sys_Error("playback file not found\n");

		Sys_FileRead (vcrFile, &i, sizeof(int));
		if (i != VCR_SIGNATURE)
			Sys_Error("Invalid signature in vcr file\n");

		Sys_FileRead (vcrFile, &com_argc, sizeof(int));
		com_argv = Q_malloc(com_argc * sizeof(char *));
		com_argv[0] = parms->argv[0];
		for (i = 0; i < com_argc; i++)
		{
			Sys_FileRead (vcrFile, &len, sizeof(int));
			p = Q_malloc(len);
			Sys_FileRead (vcrFile, p, len);
			com_argv[i+1] = p;
		}
		com_argc++; /* add one for arg[0] */
		parms->argc = com_argc;
		parms->argv = com_argv;
	}

	if ( (n = COM_CheckParm("-record")) != 0)
	{
		vcrFile = Sys_FileOpenWrite("quake.vcr");

		i = VCR_SIGNATURE;
		Sys_FileWrite(vcrFile, &i, sizeof(int));
		i = com_argc - 1;
		Sys_FileWrite(vcrFile, &i, sizeof(int));
		for (i = 1; i < com_argc; i++)
		{
			if (i == n)
			{
				len = 10;
				Sys_FileWrite(vcrFile, &len, sizeof(int));
				Sys_FileWrite(vcrFile, "-playback", len);
				continue;
			}
			len = strlen(com_argv[i]) + 1;
			Sys_FileWrite(vcrFile, &len, sizeof(int));
			Sys_FileWrite(vcrFile, com_argv[i], len);
		}
	}

}

/*
====================
Host_Init
====================
*/
void Host_Init (quakeparms_t *parms)
{
#ifdef _WIN32
#ifdef GLQUAKE
	FILE *fp = fopen("opengl32.dll","r");
	if (fp) {
		// exists
		fclose(fp);
		Sys_Error ("OpenGL32.dll found in Quake folder.  You must delete this file from your Quake folder to run this engine.");
	}
#endif
#endif

	if (standard_quake)
		minimum_memory = MINIMUM_MEMORY;
	else
		minimum_memory = MINIMUM_MEMORY_LEVELPAK;

	if (COM_CheckParm ("-minmemory"))
		parms->memsize = minimum_memory;

	host_parms = *parms;

	if (parms->memsize < minimum_memory)
		Sys_Error ("Only %4.1f megs of memory available, can't execute game", parms->memsize / (float)0x100000);

	com_argc = parms->argc;
	com_argv = parms->argv;

		// JPG 3.00 - moved this here
#ifdef _WIN32
	srand(time(NULL) ^ _getpid());
#else
	srand(time(NULL) ^ getpid());
#endif

	Memory_Init (parms->membase, parms->memsize);
	Cbuf_Init ();
	Cmd_Init ();
	Cvar_Init (); //johnfitz
	V_Init ();
	Chase_Init ();
	Host_InitVCR (parms);
	COM_Init (parms->basedir);
	Host_InitLocal ();

	W_LoadWadFile ("gfx.wad");
	Key_Init ();
	Con_Init ();
	M_Init ();
	PR_Init ();
	Mod_Init ();
	Security_Init ();	// JPG 3.20 - cheat free
	NET_Init ();
	SV_Init ();
	IPLog_Init ();	// JPG 1.05 - ip address logging

	Con_Printf ("Exe: "__TIME__" "__DATE__"\n");
	Con_Printf ("%4.1f megabyte heap\n",parms->memsize/ (1024*1024.0));

	R_InitTextures ();		// needed even for dedicated servers

	if (cls.state != ca_dedicated)
	{
		host_basepal = (byte *)COM_LoadHunkFile ("gfx/palette.lmp");
		if (!host_basepal)
			Sys_Error ("Couldn't load gfx/palette.lmp");
		host_colormap = (byte *)COM_LoadHunkFile ("gfx/colormap.lmp");
		if (!host_colormap)
			Sys_Error ("Couldn't load gfx/colormap.lmp");

#ifndef _WIN32 // on non win32, mouse comes before video for security reasons
		IN_Init ();
#endif
		VID_Init (host_basepal);

		Draw_Init ();
		SCR_Init ();
		R_Init ();
#ifndef	_WIN32
	// on Win32, sound initialization has to come before video initialization, so we
	// can put up a popup if the sound hardware is in use
		S_Init ();
#else

#ifdef	GLQUAKE
	// FIXME: doesn't use the new one-window approach yet
		S_Init ();
#endif

#endif	// _WIN32
		CDAudio_Init ();
		Sbar_Init ();
		CL_Init ();
#ifdef _WIN32 // on non win32, mouse comes before video for security reasons
		IN_Init ();
#endif

#ifdef _WIN32
		// Baker: 3.99m to get sys info
		// must be AFTER video init stuff
		Sys_InfoInit();  // We don't care about dedicated servers for this

		// Baker 3.76 - Autoplay demo

		if (com_argc >= 2)
		{
			char *infile = com_argv[1];

			if (infile[0] && infile[0] != '-' && infile[0] != '+') {
				char tmp[1024] = {0}, *ext = COM_FileExtension(infile);

				if (!Q_strncasecmp(ext, "dem", sizeof("dem")))
					Q_snprintfz(tmp, sizeof(tmp), "playdemo \"%s\"\n", infile);

				if (tmp[0])
				{
					nostartdemos = true;
					Cbuf_AddText(tmp);
				}
			}
		}
#endif


	}

	Cbuf_InsertText ("exec quake.rc\n");

	// Baker 3.80x: this is a hack
	Cbuf_AddText ("savefov\n");
	Cbuf_AddText ("savesensitivity\n");

	Hunk_AllocName (0, "-HOST_HUNKLEVEL-");
	host_hunklevel = Hunk_LowMark ();

	host_initialized = true;
	Con_Printf ("Host Initialized\n");
	Sys_Printf ("========Quake Initialized=========\n");
}


/*
===============
Host_Shutdown

FIXME: this is a callback from Sys_Quit and Sys_Error.  It would be better
to run quit through here before the final handoff to the sys code.
===============
*/
void Host_Shutdown(void)
{
	static qboolean isdown = false;

	if (isdown)
	{
		printf ("recursive shutdown\n");
		return;
	}
	isdown = true;

// keep Con_Printf from trying to update the screen
	scr_disabled_for_loading = true;

	Host_WriteConfiguration ();
	IPLog_WriteLog ();	// JPG 1.05 - ip loggging

	CDAudio_Shutdown ();
	NET_Shutdown ();
	S_Shutdown();
	IN_Shutdown ();

	if (cls.state != ca_dedicated)
	{
		VID_Shutdown();
	}
}

