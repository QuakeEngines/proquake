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
// cl_main.c  -- client main loop

#include "quakedef.h"
#ifdef HTTP_DOWNLOAD
#include "curl.h"
#endif

// we need to declare some mouse variables here, because the menu system
// references them even when on a unix system.

// these two are not intended to be set directly
cvar_t	cl_name = {"_cl_name", "player", true};
cvar_t	cl_color = {"_cl_color", "0", true};

cvar_t	cl_shownet = {"cl_shownet","0"};	// can be 0, 1, or 2
cvar_t	cl_nolerp = {"cl_nolerp","0"};
cvar_t  cl_gameplayhack_monster_lerp = {"cl_gameplayhack_monster_lerp","1"};

cvar_t	lookspring = {"lookspring","0", true};
cvar_t	lookstrafe = {"lookstrafe","0", true};
cvar_t	sensitivity = {"sensitivity","3", true};

cvar_t	m_pitch = {"m_pitch","0.022", true};
cvar_t	m_yaw = {"m_yaw","0.022", true};
cvar_t	m_forward = {"m_forward","1", true};
cvar_t	m_side = {"m_side","0.8", true};

// JPG - added these for %r formatting
cvar_t	pq_needrl = {"pq_needrl", "I need RL", true};
cvar_t	pq_haverl = {"pq_haverl", "I have RL", true};
cvar_t	pq_needrox = {"pq_needrox", "I need rockets", true};

// JPG - added these for %p formatting
cvar_t	pq_quad = {"pq_quad", "quad", true};
cvar_t	pq_pent = {"pq_pent", "pent", true};
cvar_t	pq_ring = {"pq_ring", "eyes", true};

// JPG 3.00 - added these for %w formatting
cvar_t	pq_weapons = {"pq_weapons", "SSG:NG:SNG:GL:RL:LG", true};
cvar_t	pq_noweapons = {"pq_noweapons", "no weapons", true};

// JPG 1.05 - translate +jump to +moveup under water
cvar_t	pq_moveup = {"pq_moveup", "0", true};

// JPG 3.00 - added this by request
cvar_t	pq_smoothcam = {"pq_smoothcam", "1", true};

#ifdef HTTP_DOWNLOAD
cvar_t	cl_web_download		= {"cl_web_download", "1", true};
cvar_t	cl_web_download_url	= {"cl_web_download_url", "http://downloads.quake-1.com/", true};
#endif


cvar_t	cl_bobbing		= {"cl_bobbing", "0"};

client_static_t	cls;
client_state_t	cl;
// FIXME: put these on hunk?
 efrag_t		cl_efrags[MAX_EFRAGS];
entity_t		cl_entities[MAX_EDICTS];
entity_t		cl_static_entities[MAX_STATIC_ENTITIES];
lightstyle_t	cl_lightstyle[MAX_LIGHTSTYLES];
dlight_t		cl_dlights[MAX_DLIGHTS];

int				cl_numvisedicts;
entity_t		*cl_visedicts[MAX_VISEDICTS];


extern cvar_t scr_fov;
static float		savedsensitivity;
static float		savedfov;

/*
=====================
CL_ClearState
=====================
*/
void CL_ClearState (void)
{
	int			i;

	if (!sv.active)
		Host_ClearMemory ();


// wipe the entire cl structure
	memset (&cl, 0, sizeof(cl));

	SZ_Clear (&cls.message);

// clear other arrays
	memset (cl_efrags, 0, sizeof(cl_efrags));
	memset (cl_entities, 0, sizeof(cl_entities));
	memset (cl_dlights, 0, sizeof(cl_dlights));
	memset (cl_lightstyle, 0, sizeof(cl_lightstyle));
	memset (cl_temp_entities, 0, sizeof(cl_temp_entities));
	memset (cl_beams, 0, sizeof(cl_beams));

//
// allocate the efrags and chain together into a free list
//
	cl.free_efrags = cl_efrags;
	for (i=0 ; i<MAX_EFRAGS-1 ; i++)
		cl.free_efrags[i].entnext = &cl.free_efrags[i+1];
	cl.free_efrags[i].entnext = NULL;

}

/*
=====================
CL_Disconnect

Sends a disconnect message to the server
This is also called on Host_Error, so it shouldn't cause any errors
=====================
*/
void CL_Disconnect (void)
{
// stop sounds (especially looping!)
	S_StopAllSounds (true);


//	// This makes sure ambient sounds remain silent
//	cl.worldmodel = NULL;

#ifdef HTTP_DOWNLOAD
	// We have to shut down webdownloading first
	if( cls.download.web )
	{
		cls.download.disconnect = true;
		return;
	}

#endif

// if running a local server, shut it down
	if (cls.demoplayback)
		CL_StopPlayback ();
	else if (cls.state == ca_connected)
	{
		if (cls.demorecording)
			CL_Stop_f ();

		Con_DPrintf ("Sending clc_disconnect\n");
		SZ_Clear (&cls.message);
		MSG_WriteByte (&cls.message, clc_disconnect);
		NET_SendUnreliableMessage (cls.netcon, &cls.message);
		SZ_Clear (&cls.message);
		NET_Close (cls.netcon);

		cls.state = ca_disconnected;
		if (sv.active)
			Host_ShutdownServer(false);
	}

	cls.demoplayback = cls.timedemo = false;
	cls.signon = 0;
	cl.intermission = 0; // Baker: So critical.  SCR_UpdateScreen uses this.
//	SCR_EndLoadingPlaque (); // Baker: NOOOOOO.  This shows between start demos.  We need this.

}

void CL_Disconnect_f (void)
{
	CL_Clear_Demos_Queue (); // disconnect is a very intentional action so clear out startdemos

#ifdef HTTP_DOWNLOAD
	// We have to shut down webdownloading first
	if (cls.download.web)
	{
		cls.download.disconnect = true;
		return;
	}

#endif
	CL_Disconnect ();
	if (sv.active)
		Host_ShutdownServer (false);
}

/*
=====================
CL_EstablishConnection

Host should be either "local" or a net address to be passed on
=====================
*/
void CL_EstablishConnection (char *host)
{
	if (cls.state == ca_dedicated)
		return;

	if (cls.demoplayback)
		return;

	CL_Disconnect ();

	cls.netcon = NET_Connect (host);
	if (!cls.netcon) // Baker 3.60 - Rook's Qrack port 26000 notification on failure
	{
		Con_Printf ("\nsyntax: connect server:port (port is optional)\n");//r00k added
		if (net_hostport != 26000)
			Con_Printf ("\nTry using port 26000\n");//r00k added
		Host_Error ("connect failed");
	}

	Con_DPrintf ("CL_EstablishConnection: connected to %s\n", host);

		// JPG - proquake message
	if (cls.netcon->mod == MOD_PROQUAKE)
	{
		if (pq_cheatfree)
			Con_Printf("%c%cConnected to Cheat-Free server%c\n", 1, 29, 31);
		else
			Con_Printf("%c%cConnected to ProQuake server%c\n", 1, 29, 31);
	}
	cls.demonum = -1;			// not in the demo loop now
	cls.state = ca_connected;
	cls.signon = 0;				// need all the signon messages before playing

	MSG_WriteByte(&cls.message, clc_nop);	// JPG 3.40 - fix for NAT
}

unsigned source_data[1056] = {
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

byte *COM_LoadFile (char *path, int usehunk);	// JPG 3.00 - model checking
unsigned source_key1 = 0x36117cbd;
unsigned source_key2 = 0x2e26857c;

/*
=====================
CL_SignonReply

An svc_signonnum has been received, perform a client side setup
=====================
*/
void CL_SignonReply (void)
{
	char 	str[8192];
	int i;	// JPG 3.00

	Con_DPrintf ("CL_SignonReply: %i\n", cls.signon);

	switch (cls.signon)
	{
	case 1:
		MSG_WriteByte (&cls.message, clc_stringcmd);
		MSG_WriteString (&cls.message, "prespawn");

		// JPG 3.50
		if (cls.netcon && !cls.netcon->encrypt)
			cls.netcon->encrypt = 3;
		break;

	case 2:
		MSG_WriteByte (&cls.message, clc_stringcmd);
		MSG_WriteString (&cls.message, va("name \"%s\"\n", cl_name.string));

		MSG_WriteByte (&cls.message, clc_stringcmd);
		MSG_WriteString (&cls.message, va("color %i %i\n", ((int)cl_color.value)>>4, ((int)cl_color.value)&15));

		MSG_WriteByte (&cls.message, clc_stringcmd);
		SNPrintf (str, sizeof(str), "spawn %s", cls.spawnparms);
		MSG_WriteString (&cls.message, str);

		// JPG 3.20 - model and .exe checking
		if (pq_cheatfree)
		{
			FILE *f;
			unsigned crc;
			char path[64];
#ifdef SUPPORTS_CHEATFREE_MODE

			strcpy (path, argv[0]);
#endif // ^^ MACOSX can't support this code but Windows/Linux do
#ifdef _WIN32
			if (!strstr(path, ".exe") && !strstr(path, ".EXE"))
				strlcat (path, ".exe", sizeof(path));
#endif // ^^ This is Windows operating system specific; Linux does not need
			f = fopen(path, "rb");
			if (!f)
				Host_Error("Could not open %s", path);
			fclose(f);
			crc = Security_CRC_File(path);
			MSG_WriteLong(&cls.message, crc);
			MSG_WriteLong(&cls.message, source_key1);

			if (!cl.model_precache[1])
				MSG_WriteLong(&cls.message, 0);
			for (i = 1 ; cl.model_precache[i] ; i++)
			{
				if (cl.model_precache[i]->name[0] != '*')
				{
					byte *data;
					int len;

					data = COM_LoadFile(cl.model_precache[i]->name, 2);			// 2 = temp alloc on hunk
					if (data)
					{
						len = (*(int *)(data - 12)) - 16;							// header before data contains size
						MSG_WriteLong(&cls.message, Security_CRC(data, len));
					}
					else
						Host_Error("Could not load %s", cl.model_precache[i]->name);
				}
			}
		}

		break;

	case 3:
		MSG_WriteByte (&cls.message, clc_stringcmd);
		MSG_WriteString (&cls.message, "begin");
		Cache_Report ();		// print remaining memory

		// JPG 3.50
		if (cls.netcon)
			cls.netcon->encrypt = 1;
		break;

	case 4:
		SCR_EndLoadingPlaque ();		// allow normal screen updates
		break;
	}
}

/*
=====================
CL_NextDemo

Called to play the next demo in the demo loop
=====================
*/
void CL_NextDemo (void)
{
	char	str[1024];

	if (cls.demonum == -1)
		return;		// don't play demos

//	SCR_BeginLoadingPlaque (); // Baker: Moved below

	if (!cls.demos[cls.demonum][0] || cls.demonum == MAX_DEMOS)
	{
		cls.demonum = 0;
		if (!cls.demos[cls.demonum][0])
		{
#ifdef SUPPORTS_DEMO_AUTOPLAY
			if (nostartdemos)
				nostartdemos = false; // Baker 3.76 -- part of hack to avoid start demos with dem autoplay
			else
#endif  // ^^ Uses Windows specific functionality
			Con_DPrintf ("No demos listed with startdemos\n");

			
			cls.demonum = -1;
			CL_Disconnect();	// JPG 1.05 - patch by CSR to fix crash
			return;
		}
	}

	SCR_BeginLoadingPlaque (); // Baker: Moved to AFTER we know demo will play
	SNPrintf (str, sizeof(str), "nextstartdemo %s\n", cls.demos[cls.demonum]);
	Cbuf_InsertText (str);
	cls.demonum++;
}

/*
==============
CL_PrintEntities_f
==============
*/
void CL_PrintEntities_f (void)
{
	entity_t	*ent;
	int			i;

	for (i=0,ent=cl_entities ; i<cl.num_entities ; i++,ent++)
	{
		Con_Printf ("%3i:",i);
		if (!ent->model)
		{
			Con_Printf ("EMPTY\n");
			continue;
		}
		Con_Printf ("%s:%2i  (%5.1f,%5.1f,%5.1f) [%5.1f %5.1f %5.1f]\n"
		, ent->model->name,ent->frame, ent->origin[0], ent->origin[1], ent->origin[2], ent->angles[0], ent->angles[1], ent->angles[2]);
	}
}



/*
===============
CL_AllocDlight
===============
*/
dlight_t *CL_AllocDlight (int key)
{
	int		i;
	dlight_t	*dl;

// first look for an exact key match
	if (key)
	{
		dl = cl_dlights;
		for (i=0 ; i<MAX_DLIGHTS ; i++, dl++)
		{
			if (dl->key == key)
			{
				memset (dl, 0, sizeof(*dl));
				dl->key = key;
				return dl;
			}
		}
	}

// then look for anything else
	dl = cl_dlights;
	for (i=0 ; i<MAX_DLIGHTS ; i++, dl++)
	{
		if (dl->die < cl.time)
		{
			memset (dl, 0, sizeof(*dl));
			dl->key = key;
			return dl;
		}
	}

	dl = &cl_dlights[0];
	memset (dl, 0, sizeof(*dl));
	dl->key = key;
	return dl;
}


/*
===============
CL_DecayLights
===============
*/
void CL_DecayLights (void)
{
	int			i;
	dlight_t	*dl;
	float		time;

	time = fabs (cl.time - cl.oldtime); // Baker: To make sure it stays forward oriented time

	dl = cl_dlights;
	for (i=0 ; i<MAX_DLIGHTS ; i++, dl++)
	{
		if (dl->die < cl.time || !dl->radius)
			continue;

		dl->radius -= time*dl->decay;
		if (dl->radius < 0)
			dl->radius = 0;
	}
}

/*
===============
CL_LerpPoint

Determines the fraction between the last two messages that the objects
should be put at.
===============
*/
float CL_LerpPoint (void)
{
	extern qboolean bumper_on;
	float	f, frac;

	f = cl.mtime[0] - cl.mtime[1];

	if (!f || cl_nolerp.value || cls.timedemo || sv.active)
	{
		// Baker 3.75 demo rewind
		cl.time = cl.ctime = cl.mtime[0];
		return 1;
	}

	if (f > 0.1) // dropped packet, or start of demo
	{	
		cl.mtime[1] = cl.mtime[0] - 0.1;
		f = 0.1f;
	}
	frac = (cl.ctime - cl.mtime[1]) / f;

	if (frac < 0)
	{
		if (frac < -0.01)
			if (bumper_on)
			{
				cl.ctime = cl.mtime[1];
			}
			else cl.time = cl.ctime = cl.mtime[1];
		frac = 0;
	}
	else if (frac > 1)
	{
		if (frac > 1.01)
			if (bumper_on)
				cl.ctime = cl.mtime[0];
			else cl.time = cl.ctime = cl.mtime[0]; // Here is where we get foobar'd
		frac = 1;
	}

	return frac;
}

extern cvar_t pq_timer; // JPG - need this for CL_RelinkEntities

/*
===============
CL_RelinkEntities
===============
*/
void CL_RelinkEntities (void)
{
	entity_t	*ent;
	int			i, j;
	float		frac, f, d;
	vec3_t		delta;
	float		bobjrotate;
	vec3_t		oldorg;
	dlight_t	*dl;
	void CL_ClearInterpolation (entity_t *ent);
	void CL_EntityInterpolateOrigins (entity_t *ent);
	void CL_EntityInterpolateAngles (entity_t *ent);



// determine partial update time
	frac = CL_LerpPoint ();

// JPG - check to see if we need to update the status bar
	if (pq_timer.value && ((int) cl.time != (int) cl.oldtime))
		Sbar_Changed();

	cl_numvisedicts = 0;

//
// interpolate player info
//
	for (i=0 ; i<3 ; i++)
		cl.velocity[i] = cl.mvelocity[1][i] + frac * (cl.mvelocity[0][i] - cl.mvelocity[1][i]);
	//PROQUAKE ADDITION --START
	if (cls.demoplayback || (cl.last_angle_time > host_time && !(in_attack.state & 3)) && pq_smoothcam.value) // JPG - check for last_angle_time for smooth chasecam!
	{
	// interpolate the angles
		for (j=0 ; j<3 ; j++)
		{
			d = cl.mviewangles[0][j] - cl.mviewangles[1][j];
			if (d > 180)
				d -= 360;
			else if (d < -180)
				d += 360;

			// JPG - I can't set cl.viewangles anymore since that messes up the demorecording.  So instead,
			// I'll set lerpangles (new variable), and view.c will use that instead.
			cl.lerpangles[j] = cl.mviewangles[1][j] + frac*d;
			if (cls.demoplayback)
				cl.viewangles[j] = cl.mviewangles[1][j] + frac*d;
		}
	}
	else VectorCopy(cl.viewangles, cl.lerpangles);
	//PROQUAKE ADDITION --END

	bobjrotate = anglemod(100 * cl.ctime);

// start on the entity after the world
	for (i=1,ent=cl_entities+1 ; i<cl.num_entities ; i++,ent++)
	{
		if (!ent->model)
		{	// empty slot
			if (ent->forcelink)
				R_RemoveEfrags (ent);	// just became empty
			continue;
		}

// if the object wasn't included in the last packet, remove it
		if (ent->msgtime != cl.mtime[0])
		{
#ifdef SUPPORTS_TRANSFORM_INTERPOLATION
			CL_ClearInterpolation (ent);
#endif
			ent->model = NULL;
			continue;
		}

		VectorCopy (ent->origin, oldorg);

		if (ent->forcelink)
		{	// the entity was not updated in the last message 
			// so move to the final spot
			VectorCopy (ent->msg_origins[0], ent->origin);
			VectorCopy (ent->msg_angles[0], ent->angles);
		}
		else
		{	// if the delta is large, assume a teleport and don't lerp
			f = frac;
			for (j=0 ; j<3 ; j++)
			{
				delta[j] = ent->msg_origins[0][j] - ent->msg_origins[1][j];
				if (delta[j] > 100 || delta[j] < -100)
				{
					f = 1;		// assume a teleportation, not a motion
				}
			}

#ifdef SUPPORTS_TRANSFORM_INTERPOLATION
			if (f >= 1) 
				CL_ClearInterpolation (ent);
#endif

		// interpolate the origin and angles
			for (j=0 ; j<3 ; j++)
			{
				ent->origin[j] = ent->msg_origins[1][j] + f*delta[j];

				d = ent->msg_angles[0][j] - ent->msg_angles[1][j];
				if (d > 180)
					d -= 360;
				else if (d < -180)
					d += 360;
				ent->angles[j] = ent->msg_angles[1][j] + f*d;
			}

		}



#ifdef SUPPORTS_TRANSFORM_INTERPOLATION
		CL_EntityInterpolateOrigins (ent);
		CL_EntityInterpolateAngles (ent);
#endif

// rotate binary objects locally
		if (ent->model->flags & EF_ROTATE)
		{
			ent->angles[1] = bobjrotate;
			if (cl_bobbing.value)
				ent->origin[2] += sin(bobjrotate / 90 * M_PI) * 5 + 5;
		}
		if (ent->effects & EF_BRIGHTFIELD)
			R_EntityParticles (ent);

		if (ent->effects & EF_MUZZLEFLASH)
		{
			vec3_t		fv, rv, uv;

			dl = CL_AllocDlight (i);
			VectorCopy (ent->origin,  dl->origin);
			dl->origin[2] += 16;
			AngleVectors (ent->angles, fv, rv, uv);

			VectorMA (dl->origin, 18, fv, dl->origin);
			dl->radius = 200 + (rand()&31);
			dl->minlight = 32;
			dl->die = cl.time + 0.1;
		}
		if (ent->effects & EF_BRIGHTLIGHT)
		{
			dl = CL_AllocDlight (i);
			VectorCopy (ent->origin,  dl->origin);
			dl->origin[2] += 16;
			dl->radius = 400 + (rand()&31);
			dl->die = cl.time + 0.001;
		}
		if (ent->effects & EF_DIMLIGHT)
		{
			dl = CL_AllocDlight (i);
			VectorCopy (ent->origin,  dl->origin);
			dl->radius = 200 + (rand()&31);
			dl->die = cl.time + 0.001;
		}

		if (ent->model->flags & EF_GIB)
			R_RocketTrail (oldorg, ent->origin, 2);
		else if (ent->model->flags & EF_ZOMGIB)
			R_RocketTrail (oldorg, ent->origin, 4);
		else if (ent->model->flags & EF_TRACER)
			R_RocketTrail (oldorg, ent->origin, 3);
		else if (ent->model->flags & EF_TRACER2)
			R_RocketTrail (oldorg, ent->origin, 5);
		else if (ent->model->flags & EF_ROCKET)
		{
			R_RocketTrail (oldorg, ent->origin, 0);
			dl = CL_AllocDlight (i);
			VectorCopy (ent->origin, dl->origin);
			dl->radius = 200;
			dl->die = cl.time + 0.01;
		}
		else if (ent->model->flags & EF_GRENADE)
			R_RocketTrail (oldorg, ent->origin, 1);
		else if (ent->model->flags & EF_TRACER3)
			R_RocketTrail (oldorg, ent->origin, 6);

		ent->forcelink = false;

		if (i == cl.viewentity && (!chase_active.value || pq_cheatfree))	// JPG 3.20 - added pq_cheatfree
			continue;

		if (cl_numvisedicts < MAX_VISEDICTS)
		{
			cl_visedicts[cl_numvisedicts] = ent;
			cl_numvisedicts++;
		}
	}

}

/*
===============
CL_ReadFromServer

Read all incoming data from the server
===============
*/
int CL_ReadFromServer (void)
{
	int		ret;



	// Baker 3.75 - demo rewind
	cl.oldtime = cl.ctime;
	cl.time += host_frametime;
	if (!cls.demorewind || !cls.demoplayback)	// by joe
		cl.ctime += host_frametime;
	else
		cl.ctime -= host_frametime;
	// Baker 3.75 - end demo fast rewind

	do 
	{
		ret = CL_GetMessage ();
		if (ret == -1)
			Host_Error ("CL_ReadFromServer: lost server connection");
		if (!ret)
			break;

		cl.last_received_message = realtime;
                CL_ParseServerMessage ();
	} while (ret && cls.state == ca_connected);

	if (cl_shownet.value)
		Con_Printf ("\n");

	CL_RelinkEntities ();
	CL_UpdateTEnts ();
//
// Demo progress
//
	if (cls.demoplayback && cls.capturedemo /*cls.demonum == -1 && !cls.timedemo && !cls.titledemo*/)
	{
		static float olddrealtime; // Yay.  Another timer.  Sheesh.
		float timeslice = realtime - olddrealtime;
		olddrealtime = realtime;

		if (cl.paused & 2)
			timeslice = 0;

		// If we have no start cltime, fill it in now
		if (!cls.demo_cltime_start)
		{
			cls.demo_cltime_start = cl.time;
			cls.demo_cltime_elapsed = 0;
		}
		else cls.demo_cltime_elapsed += host_frametime;

		// If we have no start hosttime, fill it in now
		if (!cls.demo_hosttime_start)
		{
			cls.demo_hosttime_start = realtime;
			cls.demo_hosttime_elapsed = 0;	
		}
		else cls.demo_hosttime_elapsed += timeslice; // Advance time only if we are not paused

		while (1)
		{
			// This is the "percentage" (0 to 1) of the demoplay that has been completed.
			float completed_amount = (cls.demo_offset_current - cls.demo_offset_start)/(float)cls.demo_file_length;
			float remaining_time = 0;
			int minutes, seconds;
			char tempstring[256];
			extern int vid_default;
			extern char movie_codec[12];

			if (vid_default != 0)
				break; // Don't bother, we are in fullscreen mode.

			if (timeslice = 0)
				break; // Don't bother updating the caption if we are paused

			if (cls.demo_hosttime_elapsed)
				remaining_time = (cls.demo_hosttime_elapsed / completed_amount) - cls.demo_hosttime_elapsed;

			minutes = COM_Minutes((int)remaining_time);
			seconds = COM_Seconds((int)remaining_time);

			sprintf (tempstring, "Demo: %s (%3.1f%% elapsed: %4.1f secs) - Estimated Remaining %d:%02d (Capturing: %s)", cls.demoname, completed_amount * 100, cls.demo_hosttime_elapsed, (int)minutes, (int)seconds, movie_codec);

			Sys_SetWindowCaption (tempstring);
			break;
		}

	}
//
// bring the links up to date
//
	return 0;
}

/*
=================
CL_SendCmd
=================
*/
void CL_SendCmd (void)
{
	usercmd_t		cmd;

	if (cls.state != ca_connected)
		return;

	if (cls.signon == SIGNONS)
	{
	// get basic movement from keyboard
		CL_BaseMove (&cmd);

	// allow mice or other external controllers to add to the move
		IN_Move (&cmd);

	// send the unreliable message
		CL_SendMove (&cmd);

	}

	if (cls.demoplayback)
	{
		SZ_Clear (&cls.message);
		return;
	}

// send the reliable message
	if (!cls.message.cursize)
		return;		// no message at all

	if (!NET_CanSendMessage (cls.netcon))
	{
		Con_DPrintf ("CL_WriteToServer: can't send\n");
		return;
	}

	if (NET_SendMessage (cls.netcon, &cls.message) == -1)
		Host_Error ("CL_WriteToServer: lost server connection");

	SZ_Clear (&cls.message);
}

// Baker 3.85:  This should really be located elsewhere, but duplicating it in both gl_screen.c and screen.c is silly.
//              Quakeworld has the equivalent in cl_cmd.c

extern cvar_t default_fov;

void CL_Fov_f (void) 
{
	if (scr_fov.value == 90.0 && default_fov.value) 
	{
		if (default_fov.value == 90)
			return; // Baker 3.99k: Don't do a message saying default FOV has been set to 90 if it is 90!

		Cvar_SetValue ("fov", default_fov.value);
		Con_Printf("fov set to default_fov %s\n", default_fov.string);
	}
}

void CL_Default_fov_f (void) 
{

	if (default_fov.value == 0)
		return; // Baker: this is totally permissible and happens with Reset to defaults.

	if (default_fov.value < 10.0 || default_fov.value > 140.0) {
		Cvar_SetValue ("default_fov", 0.0f);
		Con_Printf("Default fov %s is out-of-range; set to 0\n", default_fov.string);
	}

}

// End Baker



/*
================
CL_SaveFOV

Saves the FOV
================
*/
static void CL_SaveFOV_f (void) 
{
	savedfov = scr_fov.value;
}

/*
================
CL_RestoreFOV

Restores FOV to saved level
================
*/
static void CL_RestoreFOV_f (void) 
{
	if (!savedfov) 
	{
		Con_Printf("RestoreFOV: No saved FOV to restore\n");
		return;
	}

	Cvar_SetValue("fov", savedfov);
}

/*
================
CL_SaveSensivity

Saves the Sensitivity
================
*/
static void CL_SaveSensitivity_f (void) 
{
	savedsensitivity = sensitivity.value;
}

/*
================
CL_RestoreSensitivity

Restores Sensitivity to saved level
================
*/
static void CL_RestoreSensitivity_f (void) 
{
	if (!savedsensitivity) 
	{
		Con_Printf("RestoreSensitivity: No saved SENSITIVITY to restore\n");
		return;
	}

	Cvar_SetValue("sensitivity", savedsensitivity);
}

/*
=============
CL_Tracepos_f -- johnfitz

display impact point of trace along VPN
=============
*/
extern void TraceLine (vec3_t start, vec3_t end, vec3_t impact);
static void CL_Tracepos_f (void)
{
	vec3_t	v, w;

	VectorScale(vpn, 8192.0, v);
	TraceLine(r_refdef.vieworg, v, w);

	if (VectorLength(w) == 0)
		Con_Printf ("Tracepos: trace didn't hit anything\n");
	else
		Con_Printf ("Tracepos: (%i %i %i)\n", (int)w[0], (int)w[1], (int)w[2]);
}

/*
=============
CL_Viewpos_f -- johnfitz

display client's position and angles
=============
*/
void CL_Viewpos_f (void)
{
#if 0
	//camera position
	Con_Printf ("Viewpos: (%i %i %i) %i %i %i\n",
		(int)r_refdef.vieworg[0],
		(int)r_refdef.vieworg[1],
		(int)r_refdef.vieworg[2],
		(int)r_refdef.viewangles[PITCH],
		(int)r_refdef.viewangles[YAW],
		(int)r_refdef.viewangles[ROLL]);
#else
	//player position
	Con_Printf ("You are at xyz = %i %i %i   angles: %i %i %i\n",
		(int)cl_entities[cl.viewentity].origin[0],
		(int)cl_entities[cl.viewentity].origin[1],
		(int)cl_entities[cl.viewentity].origin[2],
		(int)cl.viewangles[PITCH],
		(int)cl.viewangles[YAW],
		(int)cl.viewangles[ROLL]);
#endif
}



/*
=================
CL_Init
=================
*/
void CL_Init (void)
{
	SZ_Alloc (&cls.message, 1024);

	CL_InitInput ();
	CL_InitTEnts ();

// register our commands
	Cvar_RegisterVariable (&cl_name, NULL);
	Cvar_RegisterVariable (&cl_color, NULL);
	Cvar_RegisterVariable (&cl_upspeed, NULL);
	Cvar_RegisterVariable (&cl_forwardspeed, NULL);
	Cvar_RegisterVariable (&cl_backspeed, NULL);
	Cvar_RegisterVariable (&cl_sidespeed, NULL);
	Cvar_RegisterVariable (&cl_movespeedkey, NULL);
	Cvar_RegisterVariable (&cl_yawspeed, NULL);
	Cvar_RegisterVariable (&cl_pitchspeed, NULL);
	Cvar_RegisterVariable (&cl_anglespeedkey, NULL);
	Cvar_RegisterVariable (&cl_shownet, NULL);
	Cvar_RegisterVariable (&cl_nolerp, NULL);
	Cvar_RegisterVariable (&lookspring, NULL);
	Cvar_RegisterVariable (&lookstrafe, NULL);
	Cvar_RegisterVariable (&sensitivity, NULL);
	Cvar_RegisterVariable (&freelook, NULL);   // Baker 3.60 - Freelook cvar support

	Cvar_RegisterVariable (&m_pitch, NULL);
	Cvar_RegisterVariable (&m_yaw, NULL);
	Cvar_RegisterVariable (&m_forward, NULL);
	Cvar_RegisterVariable (&m_side, NULL);

	Cvar_RegisterVariable (&cl_gameplayhack_monster_lerp, NULL); // Hacks!
	Cvar_RegisterVariable (&cl_bobbing, NULL);

	Cmd_AddCommand ("entities", CL_PrintEntities_f);
	Cmd_AddCommand ("disconnect", CL_Disconnect_f);
	Cmd_AddCommand ("record", CL_Record_f);
	Cmd_AddCommand ("stop", CL_Stop_f);
	Cmd_AddCommand ("playdemo", CL_PlayDemo_f);

	Cmd_AddCommand ("nextstartdemo", CL_PlayDemo_NextStartDemo_f);
	Cmd_AddCommand ("savefov", CL_SaveFOV_f);
	Cmd_AddCommand ("savesensitivity", CL_SaveSensitivity_f);
	Cmd_AddCommand ("restorefov", CL_RestoreFOV_f);
	Cmd_AddCommand ("restoresensitivity", CL_RestoreSensitivity_f);

	Cmd_AddCommand ("timedemo", CL_TimeDemo_f);

	Cmd_AddCommand ("tracepos", CL_Tracepos_f); //johnfitz
	Cmd_AddCommand ("viewpos", CL_Viewpos_f);

// JPG - added these for %r formatting
	Cvar_RegisterVariable (&pq_needrl, NULL);
	Cvar_RegisterVariable (&pq_haverl, NULL);
	Cvar_RegisterVariable (&pq_needrox, NULL);

	// JPG - added these for %p formatting
	Cvar_RegisterVariable (&pq_quad, NULL);
	Cvar_RegisterVariable (&pq_pent, NULL);
	Cvar_RegisterVariable (&pq_ring, NULL);

	// JPG 3.00 - %w formatting
	Cvar_RegisterVariable (&pq_weapons, NULL);
	Cvar_RegisterVariable (&pq_noweapons, NULL);

	// JPG 1.05 - added this for +jump -> +moveup translation
	Cvar_RegisterVariable (&pq_moveup, NULL);

	// JPG 3.02 - added this by request
	Cvar_RegisterVariable (&pq_smoothcam, NULL);

#ifdef HTTP_DOWNLOAD
	Cvar_RegisterVariable (&cl_web_download, NULL);
	Cvar_RegisterVariable (&cl_web_download_url, NULL);
#endif
}
