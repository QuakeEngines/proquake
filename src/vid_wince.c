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
// vid_wince.c -- video driver for Windows CE

#ifdef UNDER_CE

#include "quakedef.h"
#include "winquake.h"
#include "d_local.h"

HWND		mainwindow;
static int		lockcount;
int			window_center_x, window_center_y, window_x, window_y, window_width, window_height;
RECT		window_rect;
modestate_t	modestate = MS_UNINIT;
cvar_t		_windowed_mouse = {"_windowed_mouse","0", true};
qboolean	DDActive;

viddef_t	vid;				// global video state

#define	BASEWIDTH	320
#define	BASEHEIGHT	200

byte	vid_buffer[BASEWIDTH*BASEHEIGHT];
short	zbuffer[BASEWIDTH*BASEHEIGHT];
byte	surfcache[256*1024];

unsigned short	d_8to16table[256];
unsigned	d_8to24table[256];

void	VID_SetPalette (unsigned char *palette)
{
}

void	VID_ShiftPalette (unsigned char *palette)
{
}

void	VID_Init (unsigned char *palette)
{
	vid.maxwarpwidth = vid.width = vid.conwidth = BASEWIDTH;
	vid.maxwarpheight = vid.height = vid.conheight = BASEHEIGHT;
	vid.aspect = 1.0;
	vid.numpages = 1;
	vid.colormap = host_colormap;
	vid.fullbright = 256 - LittleLong (*((int *)vid.colormap + 2048));
	vid.buffer = vid.conbuffer = vid_buffer;
	vid.rowbytes = vid.conrowbytes = BASEWIDTH;
	
	d_pzbuffer = zbuffer;
	D_InitCaches (surfcache, sizeof(surfcache));

	DDActive = 1;
}

void	VID_Shutdown (void)
{
}

void	VID_Update (vrect_t *rects)
{
}

void VID_HandlePause (qboolean pause)
{
}

void VID_LockBuffer (void)
{

	lockcount++;

}
		
		
void VID_UnlockBuffer (void)
{

	lockcount--;

	if (lockcount > 0)
		return;

	if (lockcount < 0)
		Sys_Error ("Unbalanced unlock");


}


int VID_ForceUnlockedAndReturnState (void)
{
	int	lk;

	if (!lockcount)
		return 0;

	lk = lockcount;

	return lk;
}


void VID_ForceLockState (int lk)
{

	lockcount = lk;
}

void VID_SetDefaultMode (void)
{
#if 0
	if (vid_initialized)
		VID_SetMode (0, vid_curpal);
#endif
	IN_DeactivateMouse ();
}

/*
================
D_BeginDirectRect
================
*/
void D_BeginDirectRect (int x, int y, byte *pbitmap, int width, int height)
{
}


/*
================
D_EndDirectRect
================
*/
void D_EndDirectRect (int x, int y, int width, int height)
{
}


#endif