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
// vid.h -- video driver defs

#define MAX_MODE_LIST	600

typedef struct vrect_s
{
	int			x,y,width,height;
	struct vrect_s	*pnext;
} vrect_t;

typedef enum {NO_MODE = -1, MODE_WINDOWED = 0, MODE_FULLSCREEN = 1} modestate_t;


typedef struct 
{
	int			modenum;
	modestate_t	type;
	int			width;
	int			height;
	int			bpp;
	int			refreshrate; //johnfitz
	char		modedesc[17];
} vmode_t;

extern vmode_t	modelist[MAX_MODE_LIST];
extern int		nummodes;


typedef struct
{
	modestate_t		dispmode;
	unsigned		width;
	unsigned		height;
	int				bpp;
	int				dispfreq;

	float			aspect;			// width / height -- < 0 is taller than wide
	int				numpages;
	int				recalc_refdef;		// if true, recalc vid-based stuff
	unsigned		conwidth;
	unsigned		conheight;

	int				desktop_width;
	int				desktop_height;
	int				desktop_bpp;
	int				desktop_dispfreq;

   	int				desktop_areawidth;
   	int				desktop_areaheight;

} viddef_t;

extern	viddef_t	vid;				// global video state
extern	unsigned	d_8to24table[256];
extern void (*vid_menudrawfn)(void);
extern void (*vid_menukeyfn)(int key);
extern void (*vid_menucmdfn)(void); //johnfitz
void VID_SyncCvars (void);


void	VID_SetPaletteOld (unsigned char *palette);
// called at startup and after any gamma correction

void	VID_ShiftPaletteOld (unsigned char *palette);
// called for bonus and pain flashes, and for underwater color changes

void	VID_Init (unsigned char *palette);
// Called at startup to set up translation tables, takes 256 8 bit RGB values
// the palette data will go away after the call, so it must be copied off if
// the video driver will need it again

void	VID_Shutdown (void);
// Called at shutdown

void	VID_Update (vrect_t *rects);
// flushes the given rectangles from the view buffer to the screen

int VID_SetMode (int modenum, unsigned char *palette);
// sets the mode; only used by the Quake engine for resetting to mode 0 (the
// base mode) on memory allocation failures



// by joe - gamma stuff
void VID_SetDeviceGammaRamp (unsigned short *ramps);
extern	qboolean vid_hwgamma_enabled;


#ifdef RELEASE_MOUSE_FULLSCREEN
// We will release the mouse if fullscreen under several circumstances
// but specifically NOT if connected to a server that isn't us
// In multiplayer you wouldn't want to release the mouse by going to console
// But we'll say it's ok if you went to the menu

//|| cls.demoplayback || key_dest == key_menu || sv.active)
#endif

qboolean VID_WindowedSwapAvailable(void);
qboolean VID_isFullscreen(void);
void VID_Windowed(void);
void VID_Fullscreen(void);

#ifdef MACOSX
extern qboolean qMinimized;
#endif