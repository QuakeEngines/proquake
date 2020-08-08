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
// client.h

typedef struct
{
	vec3_t	viewangles;

// intended velocities
	float	forwardmove;
	float	sidemove;
	float	upmove;
} usercmd_t;

typedef struct
{
	int		length;
	char	map[MAX_STYLESTRING];
} lightstyle_t;

typedef struct
{
	char	name[MAX_SCOREBOARDNAME];
	float	entertime;
	int		frags;
	int		colors;			// two 4 bit fields
	int		ping;			// JPG - added this
	int		addr;			// JPG - added this
	byte	translations[VID_GRADES*256];
} scoreboard_t;

// JPG - added this for teamscore status bar
typedef struct
{
	int colors;
	int frags;
} teamscore_t;

typedef struct
{
	int		destcolor[3];
	int		percent;		// 0-256
} cshift_t;

#define	CSHIFT_CONTENTS	0
#define	CSHIFT_DAMAGE	1
#define	CSHIFT_BONUS	2
#define	CSHIFT_POWERUP	3
#define	NUM_CSHIFTS		4


// client_state_t should hold all pieces of the client state

#define	SIGNONS		4			// signon messages to receive before connected

#define	MAX_DLIGHTS		32
typedef struct
{
	vec3_t	origin;
	float	radius;
	float	die;				// stop lighting after this time
	float	decay;				// drop this each second
	float	minlight;			// don't add when contributing less
	int		key;
} dlight_t;

#define	MAX_BEAMS	24
typedef struct
{
	int		entity;
	struct model_s	*model;
	float	endtime;
	vec3_t	start, end;
} beam_t;


// added by joe
typedef struct framepos_s
{
	long		baz;
	struct framepos_s *next;
} framepos_t;

extern	framepos_t	*dem_framepos;		// by joe

#define	MAX_EFRAGS		640

#define	MAX_MAPSTRING	2048
#define	MAX_DEMOS	32
#define	MAX_DEMONAME	64

typedef enum {
ca_dedicated, 		// a dedicated server with no ability to start a client
ca_disconnected, 	// full screen console with no connection
ca_connected		// valid netcon, talking to a server
} cactive_t;

#ifdef HTTP_DOWNLOAD
typedef struct
{
	qboolean		web;
	char			*name;	
	double			percent;	
	qboolean		disconnect;			// set when user tries to disconnect, to allow cleaning up webdownload	
} download_t;
#endif

// the client_static_t structure is persistant through an arbitrary number
// of server connections
typedef struct
{
	cactive_t	state;

// personalization data sent to server
	char		mapstring[MAX_QPATH];
	char		spawnparms[MAX_MAPSTRING];	// to restart a level

// demo loop control
	int			demonum;		// -1 = don't play demos
	char		demos[MAX_DEMOS][MAX_DEMONAME];		// when not playing

// demo recording info must be here, because record is started before
// entering a map (and clearing client_state_t)
	qboolean	demorecording;
	qboolean	demoplayback;
	qboolean	timedemo;
	int			forcetrack;			// -1 = use normal cd track
	FILE		*demofile;
	int			td_lastframe;		// to meter out one message a frame
	int			td_startframe;		// host_framecount at start
	float		td_starttime;		// realtime at second frame of timedemo


// connection information
	int			signon;			// 0 to SIGNONS
	struct qsocket_s	*netcon;
	sizebuf_t	message;		// writing buffer to send to server
#ifdef HTTP_DOWNLOAD
	download_t	download;
#endif

	qboolean	capturedemo;
	char		demoname[256];		// current demo playing name (length 16 on purpose)
} client_static_t;

extern client_static_t	cls;

// the client_state_t structure is wiped completely at every
// server signon
typedef struct
{
	int			movemessages;	// since connecting to this server
								// throw out the first couple, so the player
								// doesn't accidentally do something the
								// first frame
	usercmd_t	cmd;			// last command sent to the server

// information for local display
	int			stats[MAX_CL_STATS];	// health, etc
	int			items;			// inventory bit flags
	float	item_gettime[32];	// cl.time of aquiring item, for blinking
	float		faceanimtime;	// use anim frame if cl.time < this

	cshift_t	cshifts[NUM_CSHIFTS];	// color shifts for damage, powerups
	cshift_t	prev_cshifts[NUM_CSHIFTS];	// and content types

// the client maintains its own idea of view angles, which are
// sent to the server each frame.  The server sets punchangle when
// the view is temporarliy offset, and an angle reset commands at the start
// of each level and after teleporting.
	vec3_t		mviewangles[2];	// during demo playback viewangles is lerped
								// between these
	vec3_t		viewangles;

	vec3_t		mvelocity[2];	// update by server, used for lean+bob
								// (0 is newest)
	vec3_t		velocity;		// lerped between mvelocity[0] and [1]

	vec3_t		punchangle;		// temporary offset

// pitch drifting vars
	float		idealpitch;
	float		pitchvel;
	qboolean	nodrift;
	float		driftmove;
	double		laststop;

	float		viewheight;
	float		crouch;			// local amount for smoothing stepups

	qboolean	paused;			// send over by server
	qboolean	onground;
	qboolean	inwater;

	int			intermission;	// don't change view angle, full screen, etc
	int			completed_time;	// latched at intermission start

	double		mtime[2];		// the timestamp of last two messages
	double		time;			// clients view of time, should be between
								// servertime and oldservertime to generate
								// a lerp point for other data
	double		oldtime;		// previous cl.time, time-oldtime is used
								// to decay light values and smooth step ups
	double		ctime;			// joe: copy of cl.time, to avoid incidents caused by rewind


	float		last_received_message;	// (realtime) for net trouble icon

// information that is static for the entire time connected to a server
	struct model_s		*model_precache[MAX_MODELS];
	struct sfx_s		*sound_precache[MAX_SOUNDS];

	char		levelname[40];	// for display on solo scoreboard
	int			viewentity;		// cl_entities[cl.viewentity] = player
	int			maxclients;
	int			gametype;

// refresh related state
	struct model_s	*worldmodel;	// cl_entities[0].model
	struct efrag_s	*free_efrags;
	int			num_entities;	// held in cl_entities array
	int			num_statics;	// held in cl_staticentities array
	entity_t	viewent;			// the gun model

	int			cdtrack, looptrack;	// cd audio

// frag scoreboard
	scoreboard_t	*scores;			// [cl.maxclients]
	teamscore_t		*teamscores;		// [13] - JPG for teamscores in status bar
	qboolean		teamgame;			// JPG = true for match, false for individual
	int				minutes;			// JPG - for match time in status bar
	int				seconds;			// JPG - for match time in status bar
	double			last_match_time;	// JPG - last time match time was obtained
	double			last_ping_time;		// JPG - last time pings were obtained
	qboolean		console_ping;		// JPG 1.05 - true if the ping came from the console
	double			last_status_time;	// JPG 1.05 - last time status was obtained
	qboolean		console_status;		// JPG 1.05 - true if the status came from the console
	double			match_pause_time;	// JPG - time that match was paused (or 0)
	vec3_t			lerpangles;			// JPG - angles now used by view.c so that smooth chasecam doesn't fuck up demos
	vec3_t			death_location;		// JPG 3.20 - used for %d formatting
} client_state_t;

extern	client_state_t	cl;

// cvars
extern	cvar_t	cl_name;
extern	cvar_t	cl_color;

extern	cvar_t	cl_upspeed;
extern	cvar_t	cl_forwardspeed;
extern	cvar_t	cl_backspeed;
extern	cvar_t	cl_sidespeed;

extern	cvar_t	cl_movespeedkey;

extern	cvar_t	cl_yawspeed;
extern	cvar_t	cl_pitchspeed;

extern	cvar_t	cl_anglespeedkey;

extern	cvar_t	cl_shownet;
extern	cvar_t	cl_nolerp;

extern	cvar_t	cl_pitchdriftspeed;
extern	cvar_t	lookspring;
extern	cvar_t	lookstrafe;
extern	cvar_t	sensitivity;

extern	cvar_t	m_pitch;
extern	cvar_t	m_yaw;
extern	cvar_t	m_forward;
extern	cvar_t	m_side;

extern	cvar_t	cl_sbar;
extern	cvar_t	cl_demorewind;
extern	cvar_t	cl_demospeed;

#define	MAX_TEMP_ENTITIES	64			// lightning bolts, etc
#define	MAX_STATIC_ENTITIES	128			// torches, etc

// FIXME, allocate dynamically
extern	efrag_t			cl_efrags[MAX_EFRAGS];
extern	entity_t		cl_entities[MAX_EDICTS];
extern	entity_t		cl_static_entities[MAX_STATIC_ENTITIES];
extern	lightstyle_t	cl_lightstyle[MAX_LIGHTSTYLES];
extern	dlight_t		cl_dlights[MAX_DLIGHTS];
extern	entity_t		cl_temp_entities[MAX_TEMP_ENTITIES];
extern	beam_t			cl_beams[MAX_BEAMS];

//=============================================================================

// cl_main.c
dlight_t *CL_AllocDlight (int key);
void	CL_DecayLights (void);

void CL_Init (void);

void CL_EstablishConnection (char *host);
void CL_Signon1 (void);
void CL_Signon2 (void);
void CL_Signon3 (void);
void CL_Signon4 (void);

void CL_Disconnect (void);
void CL_Disconnect_f (void);
void CL_NextDemo (void);

#define			MAX_VISEDICTS	256
extern	int				cl_numvisedicts;
extern	entity_t		*cl_visedicts[MAX_VISEDICTS];

#ifdef SUPPORTS_AUTOID
// model indexes
typedef	enum modelindex_s {
	mi_player, mi_q3torso, mi_q3head, mi_eyes, mi_rocket, mi_grenade,
	mi_flame0, mi_flame0_md3, mi_flame1, mi_flame2, mi_explo1, mi_explo2, mi_bubble,
	mi_fish, mi_dog, mi_soldier, mi_enforcer, mi_knight, mi_hknight,
	mi_scrag, mi_ogre, mi_fiend, mi_vore, mi_shambler,
	mi_h_dog, mi_h_soldier, mi_h_enforcer, mi_h_knight, mi_h_hknight, mi_h_scrag,
	mi_h_ogre, mi_h_fiend, mi_h_vore, mi_h_shambler, mi_h_zombie, mi_h_player,
	mi_gib1, mi_gib2, mi_gib3, NUM_MODELINDEX
} modelindex_t;

extern	modelindex_t cl_modelindex[NUM_MODELINDEX];
extern	char		*cl_modelnames[NUM_MODELINDEX];
#endif

// cl_input.c
typedef struct
{
	int		down[2];		// key nums holding it down
	int		state;			// low bit is down state
} kbutton_t;

extern	kbutton_t	in_mlook, in_klook;
extern 	kbutton_t 	in_strafe;
extern 	kbutton_t 	in_speed;
extern	kbutton_t	in_attack; // JPG - added this for completeness


void CL_InitInput (void);
void CL_SendCmd (void);
void CL_SendMove (usercmd_t *cmd);
void CL_SendLagMove (void); // JPG - synthetic lag

void CL_ClearState (void);

int  CL_ReadFromServer (void);
void CL_WriteToServer (usercmd_t *cmd);
void CL_BaseMove (usercmd_t *cmd);

float CL_KeyState (kbutton_t *key);
char *Key_KeynumToString (int keynum);

// cl_demo.c
void CL_StopPlayback (void);
int CL_GetMessage (void);
void CL_Stop_f (void);
void CL_Record_f (void);
void CL_PlayDemo_f (void);
void CL_TimeDemo_f (void);

// cl_parse.c
void CL_ParseServerMessage (void);
void CL_NewTranslation (int slot);
#ifdef SUPPORTS_AUTOID
void CL_InitModelnames (void);
#endif

// view.c
void V_StartPitchDrift (void);
void V_StopPitchDrift (void);

void V_RenderView (void);

#ifdef SUPPORTS_ENHANCED_GAMMA
void V_UpdatePaletteNew (void);
#endif // ^^ Only applies to vid_wgl at moment.  Linux and MACOSX should be able to support in future

void V_UpdatePaletteOld (void);
void V_Register (void);
void V_ParseDamage (void);
void V_SetContentsColor (int contents);

// cl_tent.c
void CL_InitTEnts (void);
void CL_ParseTEnt (void);
void CL_UpdateTEnts (void);
void CL_SignonReply (void);
