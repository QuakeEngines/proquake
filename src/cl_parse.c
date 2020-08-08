/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// cl_parse.c  -- parse a message received from the server

#include "quakedef.h"

char *svc_strings[] =
{
	"svc_bad",
	"svc_nop",
	"svc_disconnect",
	"svc_updatestat",
	"svc_version",		// [long] server version
	"svc_setview",		// [short] entity number
	"svc_sound",			// <see code>
	"svc_time",			// [float] server time
	"svc_print",			// [string] null terminated string
	"svc_stufftext",		// [string] stuffed into client's console buffer
						// the string should be \n terminated
	"svc_setangle",		// [vec3] set the view angle to this absolute value

	"svc_serverinfo",		// [long] version
						// [string] signon string
						// [string]..[0]model cache [string]...[0]sounds cache
						// [string]..[0]item cache
	"svc_lightstyle",		// [byte] [string]
	"svc_updatename",		// [byte] [string]
	"svc_updatefrags",	// [byte] [short]
	"svc_clientdata",		// <shortbits + data>
	"svc_stopsound",		// <see code>
	"svc_updatecolors",	// [byte] [byte]
	"svc_particle",		// [vec3] <variable>
	"svc_damage",			// [byte] impact [byte] blood [vec3] from

	"svc_spawnstatic",
	"OBSOLETE svc_spawnbinary",
	"svc_spawnbaseline",

	"svc_temp_entity",		// <variable>
	"svc_setpause",
	"svc_signonnum",
	"svc_centerprint",
	"svc_killedmonster",
	"svc_foundsecret",
	"svc_spawnstaticsound",
	"svc_intermission",
	"svc_finale",			// [string] music [string] text
	"svc_cdtrack",			// [byte] track [byte] looptrack
	"svc_sellscreen",
	"svc_cutscene",
};


/*
===============
CL_EntityNum

This error checks and tracks the total number of entities
===============
*/
entity_t	*CL_EntityNum (int num)
{
	if (num >= cl.num_entities)
	{
		if (num >= MAX_EDICTS)
			Host_Error ("CL_EntityNum: %i is an invalid number",num);

		while (cl.num_entities<=num)
		{
			cl_entities[cl.num_entities].colormap = 0;
			cl.num_entities++;
		}
	}

	return &cl_entities[num];
}

/*
==================
CL_ParseStartSoundPacket
==================
*/
void CL_ParseStartSoundPacket(void)
{
    vec3_t  pos;
    int 	channel, ent;
    int 	sound_num;
    int 	volume;
    int 	field_mask;
    float 	attenuation;
 	int		i;

    field_mask = MSG_ReadByte();

    volume =  (field_mask & SND_VOLUME) ? MSG_ReadByte () : DEFAULT_SOUND_PACKET_VOLUME;
	attenuation = (field_mask & SND_ATTENUATION) ? MSG_ReadByte () / 64.0 :  DEFAULT_SOUND_PACKET_ATTENUATION;

	channel = MSG_ReadShort ();
	sound_num = MSG_ReadByte ();

	ent = channel >> 3;
	channel &= 7;

	if (ent > MAX_EDICTS)
		Host_Error ("CL_ParseStartSoundPacket: ent = %i", ent);

	for (i=0 ; i<3 ; i++)
		pos[i] = MSG_ReadCoord ();

    S_StartSound (ent, channel, cl.sound_precache[sound_num], pos, volume/255.0, attenuation);
}

/*
==================
CL_KeepaliveMessage

When the client is taking a long time to load stuff, send keepalive messages
so the server doesn't disconnect.
==================
*/
void CL_KeepaliveMessage (void)
{
	float	time;
	static float lastmsg;
	int		ret;
	sizebuf_t	old;
	byte		olddata[8192];

	if (sv.active)
		return;		// no need if server is local
	if (cls.demoplayback)
		return;

// read messages from server, should just be nops
	old = net_message;
	memcpy (olddata, net_message.data, net_message.cursize);

	do 
	{
		ret = CL_GetMessage ();
		switch (ret)
		{
		default:
			Host_Error ("CL_KeepaliveMessage: CL_GetMessage failed");

		case 0:
			break;	// nothing waiting

		case 1:
			Host_Error ("CL_KeepaliveMessage: received a message");
			break;

		case 2:
			if (MSG_ReadByte() != svc_nop)
				Host_Error ("CL_KeepaliveMessage: datagram wasn't a nop");
			break;
		}
	} while (ret);

	net_message = old;
	memcpy (net_message.data, olddata, net_message.cursize);

// check time
	time = Sys_FloatTime ();
	if (time - lastmsg < 5)
		return;
	lastmsg = time;

// write out a nop
	Con_Printf ("--> client to server keepalive\n");

	MSG_WriteByte (&cls.message, clc_nop);
	NET_SendMessage (cls.netcon, &cls.message);
	SZ_Clear (&cls.message);
}

#ifdef HTTP_DOWNLOAD
/*
   =====================
   CL_WebDownloadProgress
   Callback function for webdownloads.
   Since Web_Get only returns once it's done, we have to do various things here:
   Update download percent, handle input, redraw UI and send net packets.
   =====================
*/
static int CL_WebDownloadProgress( double percent )
{
	static double time, oldtime, newtime;

	cls.download.percent = percent;
	
	if (sv.active || cls.demoplayback)
		CL_KeepaliveMessage();

	newtime = Sys_FloatTime ();
	time = newtime - oldtime;

	Host_Frame (time);

	oldtime = newtime;

	return cls.download.disconnect; // abort if disconnect received
}
#endif

void CL_NewMap (void)
{

}

qboolean Download_Attempt (const char* file_to_download)
{	
	extern cvar_t cl_web_download;
	extern cvar_t cl_web_download_url;
	extern int Web_Get( const char *url, const char *referer, const char *name, int resume, int max_downloading_time, int timeout, int ( *_progress )(double) );
	
	char local_tempname[MAX_OSPATH];
	char remote_url[1024];

	if (!cl_web_download.value || cl_web_download_url.string == NULL)
		return false;

	// Local temp file for download; make the path in case it doesn't exist
	SNPrintf (local_tempname, sizeof(local_tempname), "%s/%s.tmp", com_gamedir, file_to_download);
	COM_CreatePath (local_tempname);

	// Remote URL for download
	SNPrintf (remote_url, sizeof(remote_url), "%s%s", cl_web_download_url.string, file_to_download);

	Con_Printf ("Web downloading: %s (%s)\n", file_to_download, cl_web_download_url.string);

	cls.download.web = true;
	cls.download.disconnect = false;
	cls.download.percent = 0.0;

	SCR_EndLoadingPlaque ();

	{
		// let libCURL do it's magic!!
		qboolean success = Web_Get(remote_url, NULL, local_tempname, false, 600, 30, CL_WebDownloadProgress);
		cls.download.web = false;

		if (success)
		{
			char download_finalname[MAX_OSPATH];
			SNPrintf (download_finalname, sizeof(download_finalname), "%s/%s", com_gamedir, file_to_download);

			//Rename the .tmp file to the final precache filename
//			Con_Printf("Web download successful: %s\n", file_to_download);
			rename (local_tempname, download_finalname);

			CL_KeepaliveMessage ();

			if (cls.download.disconnect) // if the user type disconnect in the middle of the download
			{
				cls.download.disconnect = false;
				CL_Disconnect_f();
			}
		
			return true;
		}
		else
		{
			remove (local_tempname);
			Con_Printf( "Web download of %s failed\n", local_tempname );
			CL_KeepaliveMessage ();
			return false;
		}
	}

}

/*
==================
CL_ParseServerInfo
==================
*/
void CL_ParseServerInfo (void)
{
	char	*str;
	int		i;
	int		nummodels, numsounds;
	char	model_precache[MAX_MODELS][MAX_QPATH];
	char	sound_precache[MAX_SOUNDS][MAX_QPATH];
	char	tempname[MAX_QPATH];

	Con_DPrintf ("Serverinfo packet received.\n");
//
// wipe the client_state_t struct
//
	CL_ClearState ();

// parse protocol version number
	i = MSG_ReadLong ();
	if (i != PROTOCOL_VERSION)
	{
		Con_Printf ("Server returned version %i, not %i", i, PROTOCOL_VERSION);
		return;
	}

// parse maxclients
	cl.maxclients = MSG_ReadByte ();
	if (cl.maxclients < 1 || cl.maxclients > MAX_SCOREBOARD)
	{
		Con_Printf("Bad maxclients (%u) from server\n", cl.maxclients);
		return;
	}
	cl.scores = Hunk_AllocName (cl.maxclients*sizeof(*cl.scores), "scores");
	cl.teamscores = Hunk_AllocName (14 * sizeof(*cl.teamscores), "teamscor"); // JPG - for teamscore status bar

// parse gametype
	cl.gametype = MSG_ReadByte ();

// parse signon message
	str = MSG_ReadString ();
	strncpy (cl.levelname, str, sizeof(cl.levelname)-1);

// separate the printfs so the server message can have a color
	Con_Printf("\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");
	Con_Printf ("%c%s\n", 2, str);

// first we go through and touch all of the precache data that still
// happens to be in the cache, so precaching something else doesn't
// needlessly purge it

// precache models

	memset (cl.model_precache, 0, sizeof(cl.model_precache));
	for (nummodels=1 ; ; nummodels++)
	{
		str = MSG_ReadString ();
		if (!str[0])
			break;
		if (nummodels==MAX_MODELS)
		{
			Con_Printf ("Server sent too many model precaches\n");
			return;
		}
		strcpy (model_precache[nummodels], str);
		Mod_TouchModel (str);

	}

// precache sounds
	memset (cl.sound_precache, 0, sizeof(cl.sound_precache));
	for (numsounds=1 ; ; numsounds++)
	{
		str = MSG_ReadString ();
		if (!str[0])
			break;
		if (numsounds==MAX_SOUNDS) 
		{
			Con_Printf ("Server sent too many sound precaches\n");
			return;
		}
		strcpy (sound_precache[numsounds], str);
		S_TouchSound (str);
	}

	COM_StripExtension (COM_SkipPath(model_precache[1]), host_worldname);	

//
// now we try to load everything else until a cache allocation fails
//

	for (i=1 ; i<nummodels ; i++)
	{
		cl.model_precache[i] = Mod_ForName (model_precache[i], false);
		if (cl.model_precache[i] == NULL)
		{
// download start
			qboolean download_try_worked = cls.demoplayback || sv.active ? false : Download_Attempt (model_precache[i] );
				   
			if (download_try_worked)
			{
				i--; // Subtract 1 so we try this model again in next iteration
				continue;  // Bail on loop and resume
			}

			Con_Printf ("Model %s not found\n", model_precache[i]);
			
			return;  //don't disconnect, let them sit in console and ask for help.
		}
		CL_KeepaliveMessage ();
	}

	S_BeginPrecaching ();
	for (i=1 ; i<numsounds ; i++)
	{
		qboolean precached_worked = true;
		cl.sound_precache[i] = S_PrecacheSound (sound_precache[i], &precached_worked);
		if (precached_worked == false)
		{
// download start
			qboolean download_try_worked = cls.demoplayback || sv.active ? false : Download_Attempt (va ("sound/%s", sound_precache[i]) ) ;
				   
			if (download_try_worked)
				S_PrecacheSound_Again (cl.sound_precache[i]);
		
		}
		CL_KeepaliveMessage ();
	}
	S_EndPrecaching ();

// local state
	cl_entities[0].model = cl.worldmodel = cl.model_precache[1];

	
	{
		char mapname[MAX_QPATH];
		char locs_file_name[MAX_QPATH];
		
		COM_StripExtension (model_precache[1], mapname);
		SNPrintf (locs_file_name, sizeof(locs_file_name), "locs/%s.loc", COM_SkipPath(mapname) );
		
		if (LOC_LoadLocations(locs_file_name) == false && cls.demoplayback == false && sv.active == false)
		{
			if (Download_Attempt (locs_file_name) )
				LOC_LoadLocations(locs_file_name);

		}

	}

	
	COM_StripExtension (COM_SkipPath(model_precache[1]), tempname);

	R_NewMap ();

	Hunk_Check ();		// make sure nothing is hurt

}

#ifdef SUPPORTS_TRANSFORM_INTERPOLATION
extern cvar_t r_interpolate_transform;
void CL_EntityInterpolateOrigins (entity_t *ent)
{
	qboolean no_interpolate=false;

	if (r_interpolate_transform.value)
	{
		float timepassed = cl.time - ent->translate_start_time;
		float blend = 0;
		vec3_t delta = {0, 0, 0};

		if (ent->translate_start_time == 0 || timepassed > 1)
		{
			ent->translate_start_time = cl.time;

			VectorCopy (ent->origin, ent->lastorigin);
			VectorCopy (ent->origin, ent->currorigin);
		}

		if (!VectorCompare (ent->origin, ent->currorigin))
		{
			ent->translate_start_time = cl.time;

			VectorCopy (ent->currorigin, ent->lastorigin);
			VectorCopy (ent->origin,  ent->currorigin);

			blend = 0;
		}
		else
		{
			blend = timepassed / 0.1;

			if (cl.paused || blend > 1) blend = 1;
		}

		VectorSubtract (ent->currorigin, ent->lastorigin, delta);

		// use cubic interpolation
		{
			float lastlerp = 2 * (blend * blend * blend) - 3 * (blend * blend) + 1;
			float currlerp = 3 * (blend * blend) - 2 * (blend * blend * blend);

			ent->origin[0] = ent->lastorigin[0] * lastlerp + ent->currorigin[0] * currlerp;
			ent->origin[1] = ent->lastorigin[1] * lastlerp + ent->currorigin[1] * currlerp;
			ent->origin[2] = ent->lastorigin[2] * lastlerp + ent->currorigin[2] * currlerp;
		}




	}
}


void CL_EntityInterpolateAngles (entity_t *ent)
{
	if (r_interpolate_transform.value)
	{
		float timepassed = cl.time - ent->rotate_start_time;
		float blend = 0;
		vec3_t delta = {0, 0, 0};

		if (ent->rotate_start_time == 0 || timepassed > 1)
		{
			ent->rotate_start_time = cl.time;

			VectorCopy (ent->angles, ent->lastangles);
			VectorCopy (ent->angles, ent->currangles);
		}

		if (!VectorCompare (ent->angles, ent->currangles))
		{
			ent->rotate_start_time = cl.time;

			VectorCopy (ent->currangles, ent->lastangles);
			VectorCopy (ent->angles,  ent->currangles);

			blend = 0;
		}
		else
		{
			blend = timepassed / 0.1;

			if (cl.paused || blend > 1) blend = 1;
		}

		VectorSubtract (ent->currangles, ent->lastangles, delta);

		// always interpolate along the shortest path
		if (delta[0] > 180) delta[0] -= 360; else if (delta[0] < -180) delta[0] += 360;
		if (delta[1] > 180) delta[1] -= 360; else if (delta[1] < -180) delta[1] += 360;
		if (delta[2] > 180) delta[2] -= 360; else if (delta[2] < -180) delta[2] += 360;

		// get currangles on the shortest path
		VectorAdd (ent->lastangles, delta, delta);

		// use cubic interpolation
		{
			float lastlerp = 2 * (blend * blend * blend) - 3 * (blend * blend) + 1;
			float currlerp = 3 * (blend * blend) - 2 * (blend * blend * blend);

			ent->angles[0] = ent->lastangles[0] * lastlerp + delta[0] * currlerp;
			ent->angles[1] = ent->lastangles[1] * lastlerp + delta[1] * currlerp;
			ent->angles[2] = ent->lastangles[2] * lastlerp + delta[2] * currlerp;
		}
	}
}

void CL_ClearInterpolation (entity_t *ent)
{

#ifdef SUPPORTS_HARDWARE_ANIM_INTERPOLATION
	ent->frame_interval = -1;
#endif

	ent->frame_start_time = 0;
	ent->lastpose = ent->currpose;

	ent->translate_start_time = 0;
	ent->lastorigin[0] = ent->lastorigin[1] = ent->lastorigin[2] = 0;
	ent->currorigin[0] = ent->currorigin[1] = ent->currorigin[2] = 0;

	ent->rotate_start_time = 0;
	ent->lastangles[0] = ent->lastangles[1] = ent->lastangles[2] = 0;
	ent->currangles[0] = ent->currangles[1] = ent->currangles[2] = 0;
}
#endif


/*
==================
CL_ParseUpdate

Parse an entity update message from the server
If an entities model or origin changes from frame to frame, it must be
relinked.  Other attributes can change without relinking.
==================
*/
int	bitcounts[16];

static void CL_ParseUpdate (int bits)
{
	int		i, num;
	model_t		*model;
	qboolean	forcelink;
	entity_t	*ent;
//#ifdef GL_QUAKE_SKIN_METHOD
	int			skin;
//#endif

	if (cls.signon == SIGNONS - 1)
	{	// first update is the final signon stage
		cls.signon = SIGNONS;
		CL_SignonReply ();
	}

	if (bits & U_MOREBITS)
		bits |= (MSG_ReadByte() << 8);

	num = (bits & U_LONGENTITY) ? MSG_ReadShort () : MSG_ReadByte ();

	ent = CL_EntityNum (num);

for (i=0 ; i<16 ; i++)
if (bits&(1<<i))
	bitcounts[i]++;

	forcelink = (ent->msgtime != cl.mtime[1]) ? true : false;

	ent->msgtime = cl.mtime[0];

	if (bits & U_MODEL)
	{
		ent->modelindex = MSG_ReadByte ();
		if (ent->modelindex >= MAX_MODELS)
			Host_Error ("CL_ParseUpdate: bad modelindex");
	}
	else
	{
		ent->modelindex = ent->baseline.modelindex;
	}

	model = cl.model_precache[ent->modelindex];
	if (model != ent->model)
	{
		ent->model = model;
	// automatic animation (torches, etc) can be either all together or randomized
		if (model)
				ent->syncbase = (model->synctype == ST_RAND) ? (float)(rand()&0x7fff) / 0x7fff : 0.0;
		else
			forcelink = true;	// hack to make null model players work

#ifdef SUPPORTS_TRANSFORM_INTERPOLATION
		// if the model has changed we must also reset the interpolation data
		// lastpose and currpose are critical as they might be pointing to invalid frames in the new model!!!
		CL_ClearInterpolation (ent);

		// reset frame and skin too...!
		if (!(bits & U_FRAME)) ent->frame = 0;
		if (!(bits & U_SKIN)) ent->skinnum = 0;
#endif

//#ifdef GL_QUAKE_SKIN_METHOD
		if (num > 0 && num <= cl.maxclients)
			R_TranslatePlayerSkin (num - 1);
//#endif
	}

	ent->frame = (bits & U_FRAME) ? MSG_ReadByte() : ent->baseline.frame;

	i = (bits & U_COLORMAP) ? MSG_ReadByte() : ent->baseline.colormap;
	if (!i)
		ent->colormap = 0;
	else
	{
//		Con_Printf("ent: num %i colormap %i frame %i\n", num, i, ent->frame);
		if (i > cl.maxclients)
			Sys_Error ("i >= cl.maxclients");
		ent->colormap = i;
	}


	skin = (bits & U_SKIN) ? MSG_ReadByte() : ent->baseline.skin;
	if (skin != ent->skinnum)
	{
		ent->skinnum = skin;
		if (num > 0 && num <= cl.maxclients)
			R_TranslatePlayerSkin (num - 1);
	}

	ent->effects = (bits & U_EFFECTS) ? MSG_ReadByte() : ent->baseline.effects;

// shift the known values for interpolation
	VectorCopy (ent->msg_origins[0], ent->msg_origins[1]);
	VectorCopy (ent->msg_angles[0], ent->msg_angles[1]);

	ent->msg_origins[0][0] = (bits & U_ORIGIN1) ? MSG_ReadCoord() : ent->baseline.origin[0];
	ent->msg_angles[0][0] = (bits & U_ANGLE1) ? MSG_ReadAngle() : ent->baseline.angles[0];

	ent->msg_origins[0][1] = (bits & U_ORIGIN2) ? MSG_ReadCoord() : ent->baseline.origin[1];
	ent->msg_angles[0][1] = (bits & U_ANGLE2) ? MSG_ReadAngle() : ent->baseline.angles[1];

	ent->msg_origins[0][2] = (bits & U_ORIGIN3) ? MSG_ReadCoord() : ent->baseline.origin[2];
	ent->msg_angles[0][2] = (bits & U_ANGLE3) ? MSG_ReadAngle() : ent->baseline.angles[2];


	{
		extern cvar_t cl_gameplayhack_monster_lerp;

		if ( bits & U_NOLERP )
			if (!cl_gameplayhack_monster_lerp.value)
				ent->forcelink = true;
			else
				ent->forcelink = (sv.active == true);
	}

	if ( forcelink )
	{	// didn't have an update last message
		VectorCopy (ent->msg_origins[0], ent->msg_origins[1]);
		VectorCopy (ent->msg_origins[0], ent->origin);
		VectorCopy (ent->msg_angles[0], ent->msg_angles[1]);
		VectorCopy (ent->msg_angles[0], ent->angles);
		ent->forcelink = true;
	}
}

/*
==================
CL_ParseBaseline
==================
*/
void CL_ParseBaseline (entity_t *ent)
{
	int			i;

	ent->baseline.modelindex = MSG_ReadByte ();
	ent->baseline.frame = MSG_ReadByte ();
	ent->baseline.colormap = MSG_ReadByte();
	ent->baseline.skin = MSG_ReadByte();
	for (i=0 ; i<3 ; i++)
	{
		ent->baseline.origin[i] = MSG_ReadCoord ();
		ent->baseline.angles[i] = MSG_ReadAngle ();
	}
}

/*
==================
CL_ParseClientdata

Server information pertaining to this client only
==================
*/
static void CL_ParseClientdata (int bits)
{
	int		i, j;

	cl.viewheight = (bits & SU_VIEWHEIGHT) ? MSG_ReadChar() : DEFAULT_VIEWHEIGHT;
	cl.idealpitch = (bits & SU_IDEALPITCH) ? MSG_ReadChar () :0;

	VectorCopy (cl.mvelocity[0], cl.mvelocity[1]);
	for (i=0 ; i<3 ; i++)
	{
		cl.punchangle[i] = (bits & (SU_PUNCH1<<i) ) ? MSG_ReadChar() : 0;
		cl.mvelocity[0][i] = (bits & (SU_VELOCITY1<<i) ) ?  MSG_ReadChar()*16 : 0;
	}

// [always sent]	if (bits & SU_ITEMS)
		i = MSG_ReadLong ();
	if (cl.items != i)
	{	// set flash times
		Sbar_Changed ();
		for (j=0 ; j<32 ; j++)
			if ( (i & (1<<j)) && !(cl.items & (1<<j)))
				cl.item_gettime[j] = cl.time;
		cl.items = i;
	}

	cl.onground = (bits & SU_ONGROUND) != 0;
	cl.inwater = (bits & SU_INWATER) != 0;

	cl.stats[STAT_WEAPONFRAME] = (bits & SU_WEAPONFRAME) ?MSG_ReadByte (): 0;

	i = (bits & SU_ARMOR) ? MSG_ReadByte () : 0;
	if (cl.stats[STAT_ARMOR] != i)
	{
		cl.stats[STAT_ARMOR] = i;
		Sbar_Changed ();
	}

	i = (bits & SU_WEAPON) ? MSG_ReadByte (): 0;
	if (cl.stats[STAT_WEAPON] != i)
	{
		cl.stats[STAT_WEAPON] = i;
		Sbar_Changed ();
	}

	i = MSG_ReadShort ();
	if (cl.stats[STAT_HEALTH] != i)
	{
		if (i <= 0)
			memcpy(cl.death_location, cl_entities[cl.viewentity].origin, sizeof(vec3_t));
		cl.stats[STAT_HEALTH] = i;
		Sbar_Changed ();
	}

	i = MSG_ReadByte ();
	if (cl.stats[STAT_AMMO] != i)
	{
		cl.stats[STAT_AMMO] = i;
		Sbar_Changed ();
	}

	for (i=0 ; i<4 ; i++)
	{
		j = MSG_ReadByte ();
		if (cl.stats[STAT_SHELLS+i] != j)
		{
			cl.stats[STAT_SHELLS+i] = j;
			Sbar_Changed ();
		}
	}

	i = MSG_ReadByte ();

	if (standard_quake)
	{
		if (cl.stats[STAT_ACTIVEWEAPON] != i)
		{
			cl.stats[STAT_ACTIVEWEAPON] = i;
			Sbar_Changed ();
		}
	}
	else
	{
		if (cl.stats[STAT_ACTIVEWEAPON] != (1<<i))
		{
			cl.stats[STAT_ACTIVEWEAPON] = (1<<i);
			Sbar_Changed ();
		}
	}
}

/*
=====================
CL_NewTranslation
=====================
*/
 void CL_NewTranslation (int slot)
{
// CL_Parse calls this


	R_TranslatePlayerSkin (slot);

}

/*
=====================
CL_ParseStatic
=====================
*/
void CL_ParseStatic (void)
{
	entity_t *ent;

	if (cl.num_statics >= MAX_STATIC_ENTITIES)
		Host_Error ("Too many static entities");
	ent = &cl_static_entities[cl.num_statics];
	cl.num_statics++;
	CL_ParseBaseline (ent);

// copy it to the current state
	ent->model = cl.model_precache[ent->baseline.modelindex];
	ent->frame = ent->baseline.frame;
	ent->colormap = 0;
	ent->skinnum = ent->baseline.skin;
	ent->effects = ent->baseline.effects;

	VectorCopy (ent->baseline.origin, ent->origin);
	VectorCopy (ent->baseline.angles, ent->angles);
	R_AddEfrags (ent);
}

/*
===================
CL_ParseStaticSound
===================
*/
 void CL_ParseStaticSound (void)
{
	int			i, sound_num, vol, atten;
	vec3_t		org;

	for (i=0 ; i<3 ; i++)
		org[i] = MSG_ReadCoord ();
	sound_num = MSG_ReadByte ();
	vol = MSG_ReadByte ();
	atten = MSG_ReadByte ();

	S_StaticSound (cl.sound_precache[sound_num], org, vol, atten);
}

// JPG - added this
static int MSG_ReadBytePQ (void)
{
	return MSG_ReadByte() * 16 + MSG_ReadByte() - 272;
}

// JPG - added this
static int MSG_ReadShortPQ (void)
{
	return MSG_ReadBytePQ() * 256 + MSG_ReadBytePQ();
}

/* JPG - added this function for ProQuake messages
=======================
CL_ParseProQuakeMessage
=======================
*/
 void CL_ParseProQuakeMessage (void)
{
	int cmd, i;
	int team, frags, shirt, ping;

	MSG_ReadByte();
	cmd = MSG_ReadByte();

	switch (cmd)
	{
	case pqc_new_team:
		Sbar_Changed ();
		team = MSG_ReadByte() - 16;
		if (team < 0 || team > 13)
			Host_Error ("CL_ParseProQuakeMessage: pqc_new_team invalid team");
		shirt = MSG_ReadByte() - 16;
		cl.teamgame = true;
		// cl.teamscores[team].frags = 0;	// JPG 3.20 - removed this
		cl.teamscores[team].colors = 16 * shirt + team;
		//Con_Printf("pqc_new_team %d %d\n", team, shirt);
		break;

	case pqc_erase_team:
		Sbar_Changed ();
		team = MSG_ReadByte() - 16;
		if (team < 0 || team > 13)
			Host_Error ("CL_ParseProQuakeMessage: pqc_erase_team invalid team");
		cl.teamscores[team].colors = 0;
		cl.teamscores[team].frags = 0;		// JPG 3.20 - added this
		//Con_Printf("pqc_erase_team %d\n", team);
		break;

	case pqc_team_frags:
		Sbar_Changed ();
		team = MSG_ReadByte() - 16;
		if (team < 0 || team > 13)
			Host_Error ("CL_ParseProQuakeMessage: pqc_team_frags invalid team");
		frags = MSG_ReadShortPQ();
		if (frags & 32768)
			frags = frags - 65536;
		cl.teamscores[team].frags = frags;
		//Con_Printf("pqc_team_frags %d %d\n", team, frags);
		break;

	case pqc_match_time:
		Sbar_Changed ();
		cl.minutes = MSG_ReadBytePQ();
		cl.seconds = MSG_ReadBytePQ();
		cl.last_match_time = cl.time;
		//Con_Printf("pqc_match_time %d %d\n", cl.minutes, cl.seconds);
		break;

	case pqc_match_reset:
		Sbar_Changed ();
		for (i = 0 ; i < 14 ; i++)
		{
			cl.teamscores[i].colors = 0;
			cl.teamscores[i].frags = 0;		// JPG 3.20 - added this
		}
		//Con_Printf("pqc_match_reset\n");
		break;

	case pqc_ping_times:
		while (ping = MSG_ReadShortPQ())
		{
			if ((ping / 4096) >= cl.maxclients)
				Host_Error ("CL_ParseProQuakeMessage: pqc_ping_times > MAX_SCOREBOARD");
			cl.scores[ping / 4096].ping = ping & 4095;
		}
		cl.last_ping_time = cl.time;
		/*
		Con_Printf("pqc_ping_times ");
		for (i = 0 ; i < cl.maxclients ; i++)
			Con_Printf("%4d ", cl.scores[i].ping);
		Con_Printf("\n");
		*/
		break;
	}
}

//  Q_VERSION



 void Q_Version(char *s)
{
	static float q_version_reply_time = -20.0; // Baker: so it can be instantly used
	char *t;
	int l = 0, n = 0;

	// Baker: do not allow spamming of it, 20 second interval max
	if (realtime - q_version_reply_time < 20)
		return;

	t = s;
	t += 1;  // Baker: lazy, to avoid name "q_version" triggering this; later do it "right"
	l = strlen(t);

	while (n < l)
	{
		if (!strncmp(t, ": q_version", 9))
		{
				Cbuf_AddText (va("say %s version %s\n", ENGINE_NAME, VersionString()));
				Cbuf_Execute ();
				q_version_reply_time = realtime;
				break; // Baker: only do once per string
		}
		n += 1;t += 1;
	}
}
extern cvar_t pq_scoreboard_pings; // JPG - need this for CL_ParseProQuakeString

/* JPG - on a svc_print, check to see if the string contains useful information
======================
CL_ParseProQuakeString
======================
*/
 void CL_ParseProQuakeString (char *string)
{
	static int checkping = -1;
	int ping, i;
	char *s, *s2, *s3;
	static int checkip = -1;	// player whose IP address we're expecting

	// JPG 1.05 - for ip logging
	static int remove_status = 0;
	static int begin_status = 0;
	static int playercount = 0;

	// JPG 3.02 - made this more robust.. try to eliminate screwups due to "unconnected" and '\n'
	s = string;
	if (!strcmp(string, "Client ping times:\n") && pq_scoreboard_pings.value)
	{
		cl.last_ping_time = cl.time;
		checkping = 0;
		if (!cl.console_ping)
			*string = 0;
	}
	else if (checkping >= 0)
	{
		while (*s == ' ')
			s++;
		ping = 0;
		if (*s >= '0' && *s <= '9')
		{
			while (*s >= '0' && *s <= '9')
				ping = 10 * ping + *s++ - '0';
			if ((*s++ == ' ') && *s && (s2 = strchr(s, '\n')))
			{
				s3 = cl.scores[checkping].name;
				while ((s3 = strchr(s3, '\n')) && s2)
				{
					s3++;
					s2 = strchr(s2+1, '\n');
				}
				if (s2)
				{
					*s2 = 0;
					if (!strncmp(cl.scores[checkping].name, s, 15))
					{
						cl.scores[checkping].ping = ping > 9999 ? 9999 : ping;
						for (checkping++ ; !*cl.scores[checkping].name && checkping < cl.maxclients ; checkping++);
					}
					*s2 = '\n';
				}
				if (!cl.console_ping)
					*string = 0;
				if (checkping == cl.maxclients)
					checkping = -1;
			}
			else
				checkping = -1;
		}
		else
			checkping = -1;
		cl.console_ping = cl.console_ping && (checkping >= 0);	// JPG 1.05 cl.sbar_ping -> cl.console_ping
	}

	// check for match time
	if (!strncmp(string, "Match ends in ", 14))
	{
		s = string + 14;
		if ((*s != 'T') && strchr(s, 'm'))
		{
			sscanf(s, "%d", &cl.minutes);
			cl.seconds = 0;
			cl.last_match_time = cl.time;
		}
	}
	else if (!strcmp(string, "Match paused\n"))
		cl.match_pause_time = cl.time;
	else if (!strcmp(string, "Match unpaused\n"))
	{
		cl.last_match_time += (cl.time - cl.match_pause_time);
		cl.match_pause_time = 0;
	}
	else if (!strcmp(string, "The match is over\n") || !strncmp(string, "Match begins in", 15))
		cl.minutes = 255;
	else if (checkping < 0)
	{
		s = string;
		i = 0;
		while (*s >= '0' && *s <= '9')
			i = 10 * i + *s++ - '0';
		if (!strcmp(s, " minutes remaining\n"))
		{
			cl.minutes = i;
			cl.seconds = 0;
			cl.last_match_time = cl.time;
		}
	}

	// JPG 1.05 check for IP information
	if (iplog_size)
	{
		if (!strncmp(string, "host:    ", 9))
		{
			begin_status = 1;
			if (!cl.console_status)
				remove_status = 1;
		}
		if (begin_status && !strncmp(string, "players: ", 9))
		{
			begin_status = 0;
			remove_status = 0;
			if (sscanf(string + 9, "%d", &playercount))
			{
				if (!cl.console_status)
					*string = 0;
			}
			else
				playercount = 0;
		}
		else if (playercount && string[0] == '#')
		{
			if (!sscanf(string, "#%d", &checkip) || --checkip < 0 || checkip >= cl.maxclients)
				checkip = -1;
			if (!cl.console_status)
				*string = 0;
			remove_status = 0;
		}
		else if (checkip != -1)
		{
			int a, b, c;
			if (sscanf(string, "   %d.%d.%d", &a, &b, &c) == 3)
			{
				cl.scores[checkip].addr = (a << 16) | (b << 8) | c;
				IPLog_Add(cl.scores[checkip].addr, cl.scores[checkip].name);
			}
			checkip = -1;
			if (!cl.console_status)
				*string = 0;
			remove_status = 0;

			if (!--playercount)
				cl.console_status = 0;
		}
		else
		{
			playercount = 0;
			if (remove_status)
				*string = 0;
		}
	}
	Q_Version(string);//R00k: look for "q_version" requests
}

#define SHOWNET(x) if(cl_shownet.value==2)Con_Printf ("%3i:%s\n", msg_readcount-1, x);

/*
=====================
CL_ParseServerMessage
=====================
*/
void Con_LogCenterPrint (char *str);
void CL_ParseServerMessage (void)
{
	int			cmd;
	int			i;
	char		*str;
	extern	cvar_t con_nocenterprint;

//
// if recording demos, copy the message out
//
	if (cl_shownet.value == 1)
		Con_Printf ("%i ",net_message.cursize);
	else if (cl_shownet.value == 2)
		Con_Printf ("------------------\n");

	cl.onground = false;	// unless the server says otherwise
//
// parse the message
//
	MSG_BeginReading ();

	while (1)
	{
		if (msg_badread)
			Host_Error ("CL_ParseServerMessage: Bad server message");

		cmd = MSG_ReadByte ();

		if (cmd == -1)
		{
			SHOWNET("END OF MESSAGE");
			return;		// end of message
		}

	// if the high bit of the command byte is set, it is a fast update
		if (cmd & 128)
		{
			SHOWNET("fast update");
			CL_ParseUpdate (cmd&127);
			continue;
		}

		SHOWNET(svc_strings[cmd]);

	// other commands
		switch (cmd)
		{
		default:
			Host_Error ("CL_ParseServerMessage: Illegible server message\n");
			break;

		case svc_nop:
//			Con_Printf ("svc_nop\n");
			break;

		case svc_time:
			cl.mtime[1] = cl.mtime[0];
			cl.mtime[0] = MSG_ReadFloat ();
			break;

		case svc_clientdata:
			i = MSG_ReadShort ();
			CL_ParseClientdata (i);
			break;

		case svc_version:
			i = MSG_ReadLong ();
			if (i != PROTOCOL_VERSION)
				Host_Error ("CL_ParseServerMessage: Server is protocol %i instead of %i\n", i, PROTOCOL_VERSION);
			break;

		case svc_disconnect:
			Host_EndGame ("Server disconnected\n");

		case svc_print:
			// JPG - check to see if the message contains useful information
			str = MSG_ReadString();
			CL_ParseProQuakeString(str);
			Con_Printf ("%s", str);
			break;

		case svc_centerprint:
			str = MSG_ReadString ();
			if (!con_nocenterprint.value)
			{
				SCR_CenterPrint (str);
			}
			Con_LogCenterPrint (str);//johnfitz -- log centerprints to console
			break;

		case svc_stufftext:
			// JPG - check for ProQuake message
			if (MSG_PeekByte() == MOD_PROQUAKE)
				CL_ParseProQuakeMessage();
			// Still want to add text, even on ProQuake messages.  This guarantees compatibility;
			// unrecognized messages will essentially be ignored but there will be no parse errors
			Cbuf_AddText (MSG_ReadString ());
			break;

		case svc_damage:
			V_ParseDamage ();
			break;

		case svc_serverinfo:
			CL_ParseServerInfo ();
			vid.recalc_refdef = true;	// leave intermission full screen
			break;

		case svc_setangle: // JPG - added mviewangles for smooth chasecam, set last_angle_time
			for (i=0 ; i<3 ; i++)
				cl.viewangles[i] = MSG_ReadAngle ();

			if (!cls.demoplayback)
			{
				VectorCopy (cl.mviewangles[0], cl.mviewangles[1]);

				// JPG - hack with last_angle_time to autodetect continuous svc_setangles
				if (cl.last_angle_time > host_time - 0.3)
					cl.last_angle_time = host_time + 0.3;
				else if (cl.last_angle_time > host_time - 0.6)
					cl.last_angle_time = host_time;
				else
					cl.last_angle_time = host_time - 0.3;

				for (i=0 ; i<3 ; i++)
					cl.mviewangles[0][i] = cl.viewangles[i];
			}
			break;

		case svc_setview:
			cl.viewentity = MSG_ReadShort ();
			break;

		case svc_lightstyle:
			i = MSG_ReadByte ();
			if (i >= MAX_LIGHTSTYLES)
				Sys_Error ("svc_lightstyle > MAX_LIGHTSTYLES");
			strcpy (cl_lightstyle[i].map,  MSG_ReadString());
			cl_lightstyle[i].length = strlen(cl_lightstyle[i].map);
			break;

		case svc_sound:
			CL_ParseStartSoundPacket();
			break;

		case svc_stopsound:
			i = MSG_ReadShort();
			S_StopSound(i>>3, i&7);
			break;

		case svc_updatename:
			Sbar_Changed ();
			i = MSG_ReadByte ();
			if (i >= cl.maxclients)
				Host_Error ("CL_ParseServerMessage: svc_updatename > MAX_SCOREBOARD");
			strcpy (cl.scores[i].name, MSG_ReadString ());
			break;

		case svc_updatefrags:
			Sbar_Changed ();
			i = MSG_ReadByte ();
			if (i >= cl.maxclients)
				Host_Error ("CL_ParseServerMessage: svc_updatefrags > MAX_SCOREBOARD");
			cl.scores[i].frags = MSG_ReadShort ();
			break;

		case svc_updatecolors:
			Sbar_Changed ();
			i = MSG_ReadByte ();
			if (i >= cl.maxclients)
				Host_Error ("CL_ParseServerMessage: svc_updatecolors > MAX_SCOREBOARD");
			cl.scores[i].colors = MSG_ReadByte ();
			CL_NewTranslation (i);
			break;

		case svc_particle:
			R_ParseParticleEffect ();
			break;

		case svc_spawnbaseline:
			i = MSG_ReadShort ();
			// must use CL_EntityNum() to force cl.num_entities up
			CL_ParseBaseline (CL_EntityNum(i));
			break;

		case svc_spawnstatic:
			CL_ParseStatic ();
			break;

		case svc_temp_entity:
			CL_ParseTEnt ();
			break;

		case svc_setpause:

			cl.paused = MSG_ReadByte ();

			break;

		case svc_signonnum:
			i = MSG_ReadByte ();
			if (i <= cls.signon)
				Host_Error ("Received signon %i when at %i", i, cls.signon);
			cls.signon = i;
			CL_SignonReply ();
			break;

		case svc_killedmonster:
			if (cls.demoplayback && cls.demorewind)
				cl.stats[STAT_MONSTERS]--;
			else
			cl.stats[STAT_MONSTERS]++;

			break;

		case svc_foundsecret:
			if (cls.demoplayback && cls.demorewind)
				cl.stats[STAT_SECRETS]--;
			else
			cl.stats[STAT_SECRETS]++;
			break;

		case svc_updatestat:
			i = MSG_ReadByte ();
			if (i < 0 || i >= MAX_CL_STATS)
				Sys_Error ("svc_updatestat: %i is invalid", i);
			cl.stats[i] = MSG_ReadLong ();
			break;

		case svc_spawnstaticsound:
			CL_ParseStaticSound ();
			break;

		case svc_cdtrack:
			cl.cdtrack = MSG_ReadByte ();
			cl.looptrack = MSG_ReadByte ();
/*
			if ( (cls.demoplayback || cls.demorecording) && (cls.forcetrack != -1) )
				CDAudio_Play ((byte)cls.forcetrack, true);
			else
				CDAudio_Play ((byte)cl.cdtrack, true);*/
			break;

		case svc_intermission:
			if (cls.demoplayback && cls.demorewind)
				cl.intermission = 0;
			else cl.intermission = 1;
			// intermission bugfix -- by joe 
//			cl.completed_time = cl.time;
			cl.completed_time = cl.mtime[0];
			vid.recalc_refdef = true;	// go to full screen
			break;

		case svc_finale:
			if (cls.demoplayback && cls.demorewind)
				cl.intermission = 0;
			else cl.intermission = 2;
			cl.completed_time = cl.time;
			vid.recalc_refdef = true;	// go to full screen
			//johnfitz -- log centerprints to console
			str = MSG_ReadString ();
			SCR_CenterPrint (str);
			Con_LogCenterPrint (str);
			//johnfitz
			break;

		case svc_cutscene:
			if (cls.demoplayback && cls.demorewind)
				cl.intermission = 0;
			else cl.intermission = 3;
			cl.completed_time = cl.time;
			vid.recalc_refdef = true;	// go to full screen
			//johnfitz -- log centerprints to console
			str = MSG_ReadString ();
			SCR_CenterPrint (str);
			Con_LogCenterPrint (str);
			//johnfitz
			break;

		case svc_sellscreen:
			Cmd_ExecuteString ("help", src_command);
			break;

		}
	}
}
