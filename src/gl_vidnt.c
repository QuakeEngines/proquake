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
// gl_vidnt.c -- NT GL vid component

#include "quakedef.h"
#include "winquake.h"
#include "resource.h"

#define MODE_WINDOWED			0
#define NO_MODE					(MODE_WINDOWED - 1)
#define MODE_FULLSCREEN_DEFAULT	(MODE_WINDOWED + 1)

vmode_t	modelist[MAX_MODE_LIST];
int		nummodes;

//static vmode_t	badmode;

static DEVMODE	gdevmode;
static qboolean	vid_initialized = false;
static qboolean	windowed, leavecurrentmode;
static qboolean vid_canalttab = false;
static qboolean vid_wassuspended = false;
static int		windowed_mouse;
extern qboolean	flex_mouseactive;  // from in_win.c
static HICON	hIcon;

//int			DIBWidth, DIBHeight;
RECT		WindowRect;
DWORD		WindowStyle, ExWindowStyle;

HWND	mainwindow, dibwindow;

int			vid_modenum = NO_MODE;
int			vid_realmode;
int			vid_default = MODE_WINDOWED;

// SUPPORTS_GLVIDEO_MODESWITCH
qboolean	video_options_disabled = false; // Baker 3.93: D3D version loses surface with this, investigate later

qboolean	vid_fullscreen_only = false; // Baker 3.99h: This is to allow partial video mode switching if -bpp != desktop_bpp, only available if -window isn't used

static int	windowed_default;
unsigned char	vid_curpal[256*3];
qboolean fullsbardraw = false;

// Baker: begin hwgamma support


unsigned short	*currentgammaramp = NULL;
static unsigned short systemgammaramp[3][256];

qboolean	vid_gammaworks = false;
qboolean	vid_hwgamma_enabled = false;
qboolean	customgamma = false;
void VID_SetDeviceGammaRamp (unsigned short *ramps);
void VID_Gamma_Shutdown (void);
void RestoreHWGamma (void);
cvar_t		vid_hwgammacontrol		= {"vid_hwgammacontrol", "2"};//R00k changed to 2 to support windowed modes!



	qboolean using_hwgamma=true; // GLquake

// Baker end hwgamma support

HDC		maindc;

glvert_t glv;

// ProQuake will now permanently using gl_ztrick 0 as default
// First, d3dquake must have this = 0; second, Intel display adapters hate it
cvar_t	gl_ztrick = {"gl_ztrick","0"};

modestate_t	modestate = NO_MODE;

int			window_center_x, window_center_y, window_x, window_y, window_width, window_height;
RECT		window_rect;

HWND WINAPI InitializeWindow (HINSTANCE hInstance, int nCmdShow);

viddef_t	vid;				// global video state


void VID_Menu_Init (void); //johnfitz
void VID_Menu_f (void); //johnfitz
void VID_MenuDraw (void);
void VID_MenuKey (int key);

LONG WINAPI MainWndProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void AppActivate(BOOL fActive, BOOL minimize);

void Key_ClearAllStates (void);
void VID_UpdateWindowStatus (void);
//void GL_Init (void);



typedef void (APIENTRY *lp3DFXFUNC) (int, int, int, int, int, const void*);




qboolean	vid_locked = false; //johnfitz
int			vid_current_bpp;//R00k

extern	cvar_t	cl_confirmquit; // Baker 3.60



cvar_t		vid_fullscreen = {"vid_fullscreen", "1", true};
cvar_t		vid_width = {"vid_width", "640", true};
cvar_t		vid_height = {"vid_height", "480", true};
cvar_t		vid_bpp = {"vid_bpp", "32", true};
cvar_t		vid_refreshrate = {"vid_refreshrate", "60", true};
cvar_t		vid_vsync = {"vid_vsync", "1", true};

static 		qboolean update_vsync = false;
//johnfitz



cvar_t		_windowed_mouse = {"_windowed_mouse","1", true};


typedef BOOL (APIENTRY *SWAPINTERVALFUNCPTR)(int);
SWAPINTERVALFUNCPTR wglSwapIntervalEXT = NULL;


void VID_Vsync_f (void);
void Vid_Win32_InitWindow (HINSTANCE hInstance);
void System_GammaReset (void);
void Vidmodes_Populate (int w, int h, int bpp);

void CheckVsyncControlExtensions (void)
{


	if (COM_CheckParm("-noswapctrl"))
	{
		Con_Warning ("Vertical sync disabled at command line\n");
		return;
	}

#ifdef DX8QUAKE_VSYNC_COMMANDLINE_PARAM
	if (COM_CheckParm("-vsync")) {
		Con_SafePrintf ("\x02Note:");
		Con_Printf (" Vertical sync in fullscreen is on (-vsync param)\n");
		return;
	} else {
		Con_Warning ("Vertical sync in fullscreen is off\n");
		Con_Printf ("Use -vsync in command line to enable vertical sync\n");
		return;
	}
#endif


	if (!CheckExtension("WGL_EXT_swap_control") && !CheckExtension("GL_WIN_swap_hint")) 
	{
		Con_Warning ("Vertical sync not supported (extension not found)\n");
		return;
	}

	if (!(wglSwapIntervalEXT = (void *)wglGetProcAddress("wglSwapIntervalEXT"))) 
	{
		Con_Warning ("vertical sync not supported (wglSwapIntervalEXT failed)\n");
		return;
	}

	Con_Printf("Vsync control extensions found\n");
}


/*
================
VID_UpdateWindowStatus
================
*/
void VID_UpdateWindowStatus (void)
{
#if 1
	GetWindowRect (mainwindow, &window_rect);
#endif
	window_rect.left = window_x;
	window_rect.top = window_y;
	window_rect.right = window_x + window_width;
	window_rect.bottom = window_y + window_height;
	window_center_x = (window_rect.left + window_rect.right) / 2;
	window_center_y = (window_rect.top + window_rect.bottom) / 2;

#if 0
	Con_Printf ("Window info center %i %i (%ix%i to %ix%i)", (int)window_center_x, (int)window_center_y, (int)window_rect.left, (int)window_rect.top, (int)window_rect.right, (int)window_rect.bottom);
#endif

	IN_UpdateClipCursor ();
}


/*
================
CenterWindow
================
*/
void CenterWindow(HWND hWndCenter, int width, int height, BOOL lefttopjustify)
{
	// get the size of the desktop working area and the window
	RECT workarea, rect;
	int out_left, out_top;

	// Baker: MSDN says SPI_GETWORKAREA is for the primary monitor so we are ok!
	if (!SystemParametersInfo (SPI_GETWORKAREA, 0, &workarea, 0) || !GetWindowRect (hWndCenter, &rect))
	{
		Sys_Error ("CenterWindow: SystemParametersInfo failed\n");
		return;
	}

	// center it properly in the working area (don't assume that top and left are 0!!!)

	out_left = workarea.left + ((workarea.right - workarea.left) - (rect.right - rect.left)) / 2;
	out_top  = workarea.top + ((workarea.bottom - workarea.top) - (rect.bottom - rect.top)) / 2;

	if (out_left < 0) out_left = 0;
	if (out_top < 0) out_top = 0;

	SetWindowPos
	(
		hWndCenter,
		NULL,
		out_left,
		out_top,
		0,
		0,
		SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW | SWP_DRAWFRAME
	);

}

void VID_Consize_f();

/*
================
VID_SetWindowedMode
================
*/
qboolean VID_SetWindowedMode (int modenum)
{
	HDC				hdc;
	int				lastmodestate, width, height;
	RECT			rect;

	lastmodestate = modestate;

	WindowRect.top = WindowRect.left = 0;

	WindowRect.right = modelist[modenum].width;
	WindowRect.bottom = modelist[modenum].height;

//	DIBWidth = modelist[modenum].width;
//	DIBHeight = modelist[modenum].height;

	if (modelist[modenum].width == GetSystemMetrics(SM_CXSCREEN) && modelist[modenum].height == GetSystemMetrics(SM_CYSCREEN))
		WindowStyle = WS_POPUP; // Window covers entire screen; no caption, borders etc
	else
		WindowStyle = WS_OVERLAPPED | WS_BORDER | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

	ExWindowStyle = 0;

	rect = WindowRect;
	AdjustWindowRectEx(&rect, WindowStyle, FALSE, 0);
	width = rect.right - rect.left;
	height = rect.bottom - rect.top;

	// Create the DIB window
	dibwindow = CreateWindowEx (
		 ExWindowStyle,
		 TEXT(ENGINE_NAME), // "WinQuake",
		 va("%s %s %s",ENGINE_NAME, RENDERER_NAME, ENGINE_VERSION), // D3D diff 3 of 14
		 WindowStyle,
		 rect.left, rect.top,
		 width,
		 height,
		 NULL,
		 NULL,
		 global_hInstance,
		 NULL);

	if (!dibwindow)
		Sys_Error ("Couldn't create DIB window");

	// Center and show the DIB window
	CenterWindow(dibwindow, WindowRect.right - WindowRect.left, WindowRect.bottom - WindowRect.top, false);

	ShowWindow (dibwindow, SW_SHOWDEFAULT);
	UpdateWindow (dibwindow);

	modestate = MODE_WINDOWED;

// because we have set the background brush for the window to NULL
// (to avoid flickering when re-sizing the window on the desktop),
// we clear the window to black when created, otherwise it will be
// empty while Quake starts up.
	hdc = GetDC(dibwindow);
	PatBlt(hdc,0,0,WindowRect.right,WindowRect.bottom,BLACKNESS);
	ReleaseDC(dibwindow, hdc);

	//johnfitz -- stuff
	vid.width = modelist[modenum].width;
	vid.height = modelist[modenum].height;
	//johnfitz

	VID_Consize_f();

	vid.numpages = 2;

	mainwindow = dibwindow;

//	SendMessage (mainwindow, WM_SETICON, (WPARAM)TRUE, (LPARAM)hIcon);
//	SendMessage (mainwindow, WM_SETICON, (WPARAM)FALSE, (LPARAM)hIcon);

	return true;
}

/*
================
VID_SetFullDIBMode
================
*/
qboolean VID_SetFullDIBMode (int modenum)
{
	HDC				hdc;
	int				lastmodestate, width, height;
	RECT			rect;

	if (!leavecurrentmode)
	{
		gdevmode.dmFields = DM_BITSPERPEL |
							DM_PELSWIDTH |
							DM_PELSHEIGHT |
							DM_DISPLAYFREQUENCY; //johnfitz -- refreshrate
		gdevmode.dmBitsPerPel = modelist[modenum].bpp;
		gdevmode.dmPelsWidth = modelist[modenum].width; // << modelist[modenum].halfscreen;
		gdevmode.dmPelsHeight = modelist[modenum].height;
		gdevmode.dmDisplayFrequency = modelist[modenum].refreshrate; //johnfitz -- refreshrate
		gdevmode.dmSize = sizeof (gdevmode);

		if (ChangeDisplaySettings (&gdevmode, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
			Sys_Error ("Couldn't set fullscreen DIB mode");
	}

	lastmodestate = modestate;
	modestate = MODE_FULLSCREEN;

	WindowRect.top = WindowRect.left = 0;

	WindowRect.right = modelist[modenum].width;
	WindowRect.bottom = modelist[modenum].height;

//	DIBWidth = modelist[modenum].width;
//	DIBHeight = modelist[modenum].height;

	WindowStyle = WS_POPUP;
	ExWindowStyle = 0;

	rect = WindowRect;
	AdjustWindowRectEx(&rect, WindowStyle, FALSE, 0);

	width = rect.right - rect.left;
	height = rect.bottom - rect.top;

	// Create the DIB window
	dibwindow = CreateWindowEx (
		 ExWindowStyle,
		 TEXT(ENGINE_NAME),	// "WinQuake" in the past, now "ProQuake"
		 va("%s %s %s",ENGINE_NAME, RENDERER_NAME, ENGINE_VERSION), // D3D diff 4 of 14
		 WindowStyle,
		 rect.left, rect.top,
		 width,
		 height,
		 NULL,
		 NULL,
		 global_hInstance,
		 NULL);

	if (!dibwindow)
		Sys_Error ("Couldn't create DIB window");

	ShowWindow (dibwindow, SW_SHOWDEFAULT);
	UpdateWindow (dibwindow);

	// Because we have set the background brush for the window to NULL (to avoid flickering when re-sizing the window on the desktop),
	// we clear the window to black when created, otherwise it will be  empty while Quake starts up.
	hdc = GetDC(dibwindow);
	PatBlt(hdc,0,0,WindowRect.right,WindowRect.bottom,BLACKNESS);
	ReleaseDC(dibwindow, hdc);

	vid.width = modelist[modenum].width;
	vid.height = modelist[modenum].height;

	VID_Consize_f();

	vid.numpages = 2;

// needed because we're not getting WM_MOVE messages fullscreen on NT
	window_x = window_y = 0;
	mainwindow = dibwindow;

//	SendMessage (mainwindow, WM_SETICON, (WPARAM)TRUE, (LPARAM)hIcon);
//	SendMessage (mainwindow, WM_SETICON, (WPARAM)FALSE, (LPARAM)hIcon);

	return true;
}

/*
================
VID_SetMode
================
*/
extern void IN_StartupMouse (void);
int VID_SetMode (int modenum, unsigned char *palette)
{
	int				original_mode, temp;
	qboolean		stat;
    MSG				msg;

	if ((windowed && modenum) || (!windowed && modenum < 1) || (!windowed && modenum >= nummodes))
		Sys_Error ("Bad video mode");

// so Con_Printfs don't mess us up by forcing vid and snd updates
	temp = scr_disabled_for_loading;
	scr_disabled_for_loading = true;

	S_BlockSound ();
	S_ClearBuffer ();
//	CDAudio_Pause ();

	if (vid_modenum == NO_MODE)
		original_mode = windowed_default;
	else
		original_mode = vid_modenum;

	// Set either the fullscreen or windowed mode
	if (modelist[modenum].type == MODE_WINDOWED)
	{
		if (_windowed_mouse.value && key_dest == key_game)
		{
			stat = VID_SetWindowedMode(modenum);
			IN_Mouse_Acquire ();
		}
		else
		{
			IN_Mouse_Unacquire ();			
			stat = VID_SetWindowedMode(modenum);
		}
	}
	else if (modelist[modenum].type == MODE_FULLSCREEN)
	{
		stat = VID_SetFullDIBMode(modenum);
		IN_Mouse_Acquire ();
	}
	else
	{
		Sys_Error ("VID_SetMode: Bad mode type in modelist");
	}

	window_width = modelist[modenum].width;
	window_height = modelist[modenum].height;
	VID_UpdateWindowStatus ();

	S_UnblockSound ();
//	CDAudio_Resume ();
	scr_disabled_for_loading = temp;

	if (!stat)
	{
		Sys_Error ("Couldn't set video mode");
	}

	// now we try to make sure we get the focus on the mode switch, because sometimes in some systems we don't.
// sometimes in some systems we don't.  We grab the foreground, then
// finish setting up, pump all our messages, and sleep for a little while
// to let messages finish bouncing around the system, then we put
// ourselves at the top of the z order, then grab the foreground again,
// Who knows if it helps, but it probably doesn't hurt

	SetForegroundWindow (mainwindow);
	VID_SetPaletteOld (palette);
	vid_modenum = modenum;

	while (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
	{
      	TranslateMessage (&msg);
      	DispatchMessage (&msg);
	}

	Sleep (100);

	SetWindowPos (mainwindow, HWND_TOP, 0, 0, 0, 0, SWP_DRAWFRAME | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOCOPYBITS);

	SetForegroundWindow (mainwindow);

// fix the leftover Alt from any Alt-Tab or the like that switched us away
	Key_ClearAllStates ();

	VID_SetPaletteOld (palette);

	vid.recalc_refdef = 1;

	//R00k mouse died on mode change
	IN_StartupMouse ();

	return true;
}

/*
===============
VID_Vsync_f -- johnfitz
===============
*/
void VID_Vsync_f (void)
{
	update_vsync = true;
	//return false;
}

/*
===================
VID_Restart -- johnfitz -- change video modes on the fly
===================
*/
void VID_SyncCvars (void);

#ifdef DX8QUAKE
void D3D_WrapResetMode (int newmodenum, qboolean newmode_is_windowed) 
{
	// Baker: wrap the reset mode with all the stuff we need
	int temp;

	Key_ClearAllStates ();
	temp = scr_disabled_for_loading;
	scr_disabled_for_loading = true;

	S_BlockSound ();
	S_ClearBuffer ();
//	CDAudio_Pause ();

//	ShowWindow (dibwindow, SW_SHOWDEFAULT);
//	UpdateWindow (dibwindow);

//	modestate = MODE_WINDOWED;
//	DIBWidth =  modelist[newmodenum].width;
//	DIBHeight = modelist[newmodenum].height;

	// Baker: since we aren't actually do a mode change, but a resize
	//        let's do this here to be safe
//	IN_DeactivateMouse ();
//	IN_ShowMouse ();

	// Set either the fullscreen or windowed mode

#if 0
	Key_ClearAllStates ();
	IN_Mouse_Unacquire ();

	if (modelist[newmodenum].type == MODE_WINDOWED)
	{
		vid_modenum = newmodenum;
		modestate = MODE_WINDOWED;
		D3D_ResetMode (modelist[newmodenum].width, modelist[newmodenum].height, modelist[newmodenum].bpp, newmode_is_windowed);
	}
	else
	{
		D3D_ResetMode (modelist[newmodenum].width, modelist[newmodenum].height, modelist[newmodenum].bpp, newmode_is_windowed);
		modestate = MODE_FULLSCREEN;
	}
#else
	if (modelist[newmodenum].type == MODE_WINDOWED) {
		if (_windowed_mouse.value && key_dest == key_game) {
			D3D_ResetMode (modelist[newmodenum].width, modelist[newmodenum].height, modelist[newmodenum].bpp, newmode_is_windowed);
			vid_modenum = newmodenum;
			modestate = MODE_WINDOWED;
//			IN_ActivateMouse ();
//			IN_HideMouse ();
			IN_Mouse_Acquire ();
		}
		else 
		{
//			IN_DeactivateMouse ();
//			IN_ShowMouse ();
			IN_Mouse_Unacquire ();
			D3D_ResetMode (modelist[newmodenum].width, modelist[newmodenum].height, modelist[newmodenum].bpp, newmode_is_windowed);
			modestate = MODE_WINDOWED;
		}
	} else if (modelist[newmodenum].type == MODE_FULLSCREEN) {
		// and reset the mode
		D3D_ResetMode (modelist[newmodenum].width, modelist[newmodenum].height, modelist[newmodenum].bpp, newmode_is_windowed);
		modestate = MODE_FULLSCREEN;
//		IN_ActivateMouse ();
//		IN_HideMouse ();
		IN_Mouse_Acquire ();
	} else {
		Sys_Error ("VID_SetMode: Bad mode type in modelist");
	}
#endif

	// now fill in all the ugly globals that Quake stores the same data in over and over again
	// (this will be different for different engines)


	vid.width = window_width = WindowRect.right = modelist[newmodenum].width;
	vid.height = window_height = WindowRect.bottom = modelist[newmodenum].height;
	VID_Consize_f();



	// these also needed
	VID_UpdateWindowStatus ();
#if 0
	{
		MSG				msg;
		while (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
		{
      		TranslateMessage (&msg);
      		DispatchMessage (&msg);
		}

		Sleep (100);
	}

	VID_UpdateWindowStatus ();
#endif
	VID_SetPaletteOld (host_basepal);
	Key_ClearAllStates ();
	S_UnblockSound ();
//	CDAudio_Resume ();

	scr_disabled_for_loading = temp;
	vid.recalc_refdef = 1;
	IN_StartupMouse();
#if 0
	if (modestate = MODE_FULLSCREEN)
	{
		// Deal with an oddity issue.
		window_rect.left = window_rect.top = window_x = window_y = 0;
		window_rect.right = window_width;
		window_rect.bottom = window_height;
		window_center_x = (window_rect.left + window_rect.right) / 2;
		window_center_y = (window_rect.top + window_rect.bottom) / 2;
	}
	IN_ClearStates ();
#endif 
}
#endif

qboolean vid_force_restart = false;

void VID_Restart_f (void);

void VID_ForceRestart_f (void)
{
	vid_force_restart = true;
	Con_Printf("Forced video restart ...\n");
	VID_Restart_f ();
	vid_force_restart = false;
}


BOOL bSetupPixelFormat(HDC hDC);
void VID_Gamma_Restore (void);

void VID_Restart_f (void)
{
	HDC			hdc;
	HGLRC		hrc;
	int			i;
	qboolean	mode_changed = false;
	vmode_t		oldmode;

	if (vid_locked)
		return;

//
// check cvars against current mode
//
	if (vid_fullscreen.value || vid_fullscreen_only)
	{
		if (modelist[vid_default].type == MODE_WINDOWED)
			mode_changed = true;
		else if (modelist[vid_default].refreshrate != (int)vid_refreshrate.value)
			mode_changed = true;
	}
	else
		if (modelist[vid_default].type != MODE_WINDOWED)
			mode_changed = true;

	if (modelist[vid_default].width != (int)vid_width.value ||
		modelist[vid_default].height != (int)vid_height.value)
		mode_changed = true;

	if (mode_changed || vid_force_restart)
	{
//
// decide which mode to set
//
		oldmode = modelist[vid_default];

		if (vid_fullscreen.value || vid_fullscreen_only)
		{
			for (i=1; i<nummodes; i++)
			{
				if (modelist[i].width == (int)vid_width.value &&
					modelist[i].height == (int)vid_height.value &&
					modelist[i].bpp == (int)vid_bpp.value &&
					modelist[i].refreshrate == (int)vid_refreshrate.value)
				{
					break;
				}
			}

			if (i == nummodes)
			{
				Con_Printf ("%dx%dx%d %dHz is not a valid fullscreen mode\n",
							(int)vid_width.value,
							(int)vid_height.value,
							(int)vid_bpp.value,
							(int)vid_refreshrate.value);
				return;
			}

			windowed = false;
			vid_default = i;
		}
		else //not fullscreen
		{
			hdc = GetDC (NULL);
			if (GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE)
			{
				Con_Printf ("Can't run windowed on non-RGB desktop\n");
				ReleaseDC (NULL, hdc);
				return;
			}
			ReleaseDC (NULL, hdc);

			if (vid_width.value < 320)
			{
				Con_Printf ("Window width can't be less than 320\n");
				return;
			}

			if (vid_height.value < 200)
			{
				Con_Printf ("Window height can't be less than 200\n");
				return;
			}

			modelist[0].width = (int)vid_width.value;
			modelist[0].height = (int)vid_height.value;
			SNPrintf (modelist[0].modedesc, sizeof(modelist[0].modedesc), "%dx%dx%d %dHz",
					 modelist[0].width,
					 modelist[0].height,
					 modelist[0].bpp,
					 modelist[0].refreshrate);

			windowed = true;
			vid_default = 0;
		}
//
// destroy current window
//

		// Baker: restore gamma after Window is destroyed
		// to avoid Windows desktop looking distorted due
		// during switch

		if (using_hwgamma && vid_hwgamma_enabled)
			VID_Gamma_Restore ();


#ifdef DX8QUAKE

		// instead of destroying the window, context, etc we just need to resize the window and reset the device for D3D
		// we need this too
		vid_canalttab = false;

		D3D_WrapResetMode (vid_default, windowed);

#else
		hrc = wglGetCurrentContext();
		hdc = wglGetCurrentDC();
		wglMakeCurrent(NULL, NULL);

		vid_canalttab = false;

		if (hdc && dibwindow)
			ReleaseDC (dibwindow, hdc);
		if (modestate == MODE_FULLSCREEN)
			ChangeDisplaySettings (NULL, 0);
		if (maindc && dibwindow)
			ReleaseDC (dibwindow, maindc);
		maindc = NULL;
		if (dibwindow)
			DestroyWindow (dibwindow);
//
// set new mode
//

		VID_SetMode (vid_default, host_basepal);

		maindc = GetDC(mainwindow);
	bSetupPixelFormat(maindc);


		// if bpp changes, recreate render context and reload textures
		if (modelist[vid_default].bpp != oldmode.bpp)
		{
			wglDeleteContext (hrc);
			hrc = wglCreateContext (maindc);
			if (!wglMakeCurrent (maindc, hrc))
				Sys_Error ("VID_Restart: wglMakeCurrent failed");
			//Bakertest: TexMgr_ReloadImages ();
			GL_SetupState ();
		}
		else
			if (!wglMakeCurrent (maindc, hrc))
#if 1
			{
				char szBuf[80];
				LPVOID lpMsgBuf;
				DWORD dw = GetLastError();
				FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL );
				SNPrintf(szBuf, sizeof(szBuf), "VID_Restart: wglMakeCurrent failed with error %d: %s", dw, lpMsgBuf);
 				Sys_Error (szBuf);
			}
#else
				Sys_Error ("VID_Restart: wglMakeCurrent failed");
#endif

#endif // !DX8QUAKE
		vid_canalttab = true;

		// Baker: Now that we have created the new window, restore it

		if (using_hwgamma && vid_hwgamma_enabled)
			VID_SetDeviceGammaRamp (currentgammaramp);


		//swapcontrol settings were lost when previous window was destroyed
		VID_Vsync_f ();


		//conwidth and conheight need to be recalculated

		//vid.conwidth = (vid_conwidth.value > 0) ? (int)vid_conwidth.value : vid.width;
		//vid.conwidth = CLAMP (320, vid.conwidth, vid.width);
		//vid.conwidth &= 0xFFFFFFF8;
		//vid.conheight = vid.conwidth * vid.height / vid.width;


//		VID_Consize_f();

	}

//
// keep cvars in line with actual mode
//

	VID_SyncCvars ();
	VID_Consize_f ();

}

void VID_Fullscreen(void) 
{
	// Only do if video mode switching is enabled?  Yes

	if (!host_initialized)
		return;

	if (modestate == MODE_FULLSCREEN  || vid_fullscreen_only) 
	{
		Con_DPrintf("VID_Fullscreen: Already fullscreen\n");
		return;
	}

	if (vid_locked) 
	{
		Con_Printf("VID_Fullscreen: Video mode switching is locked\n");
		return;
	}

	Cvar_SetValue("vid_fullscreen", 1);
	VID_Restart_f();
}

void VID_Windowed(void) 
{
	// Only do if video mode switching is enabled?  Yes

	if (!host_initialized)
		return;

	if (modestate == MODE_WINDOWED) {
		Con_DPrintf("VID_Windowed: Already windowed\n");
		return;
	}

	if (vid_locked) {
		Con_Printf("VID_Windowed: Video mode switching is locked\n");
		return;
	}

	//MessageBox(NULL,"Stage1","Stage1",MB_OK);
	Cvar_SetValue("vid_fullscreen", 0);
	VID_Restart_f();
	//MessageBox(NULL,"Stage2","Stage2",MB_OK);
}

qboolean VID_CanSwitchedToWindowed(void) {

	if (!host_initialized)
		return 0; // can't

	if (modestate == MODE_WINDOWED)
		return 0; // can't; already are

	if (vid_locked)
		return 0; // can't switch modes

	if (vid_fullscreen_only)
		return 0; // can't switch to windowed mode

	return 1; // can and we aren't in it already
}

qboolean VID_WindowedSwapAvailable(void) 
{
	if (!host_initialized)
		return 0; // can't

	if (vid_locked)
		return 0; // can't switch modes

	if (vid_fullscreen_only)
		return 0; // can't switch to/from windowed mode

	return 1; //switchable
}

qboolean VID_isFullscreen(void) 
{
	if (modestate == MODE_WINDOWED)
		return 0;

	return 1; // modestate == MODE_FULLSCREEN

}

/*
================
VID_Test -- johnfitz -- like vid_restart, but asks for confirmation after switching modes
================
*/
void VID_Test_f (void)
{
	vmode_t oldmode;
	qboolean	mode_changed = false;

	if (vid_locked)
		return;
//
// check cvars against current mode
//
	if (vid_fullscreen.value || vid_fullscreen_only)
	{
		if (modelist[vid_default].type == MODE_WINDOWED)
			mode_changed = true;
/*		else if (modelist[vid_default].bpp != (int)vid_bpp.value)
			mode_changed = true; */
		else if (modelist[vid_default].refreshrate != (int)vid_refreshrate.value)
			mode_changed = true;
	}
	else
		if (modelist[vid_default].type != MODE_WINDOWED)
			mode_changed = true;

	if (modelist[vid_default].width != (int)vid_width.value ||
		modelist[vid_default].height != (int)vid_height.value)
		mode_changed = true;

	if (!mode_changed)
		return;
//
// now try the switch
//
	oldmode = modelist[vid_default];

	VID_Restart_f ();

	//pop up confirmation dialoge
	if (!SCR_ModalMessage("Would you like to keep this\nvideo mode? (y/n)\n", 5.0f))
	{
		//revert cvars and mode
		Cvar_Set ("vid_width", va("%i", oldmode.width));
		Cvar_Set ("vid_height", va("%i", oldmode.height));
//		Cvar_Set ("vid_bpp", va("%i", oldmode.bpp));
		Cvar_Set ("vid_refreshrate", va("%i", oldmode.refreshrate));
		Cvar_Set ("vid_fullscreen", ((vid_fullscreen_only) ? "1" : (oldmode.type == MODE_WINDOWED) ? "0" : "1"));
		VID_Restart_f ();
	}
}

/*
================
VID_Unlock -- johnfitz
================
*/
void VID_Unlock_f (void)
{
	vid_locked = false;

	//sync up cvars in case they were changed during the lock
	Cvar_Set ("vid_width", va("%i", modelist[vid_default].width));
	Cvar_Set ("vid_height", va("%i", modelist[vid_default].height));
	Cvar_Set ("vid_refreshrate", va("%i", modelist[vid_default].refreshrate));
	Cvar_Set ("vid_fullscreen", (vid_fullscreen_only) ? "1" : ((windowed) ? "0" : "1"));
}








/*
=================
GL_BeginRendering -- sets values of glx, gly, glwidth, glheight
=================
*/
void GL_BeginRendering (int *x, int *y, int *width, int *height) {
	*x = *y = 0;
	*width = WindowRect.right - WindowRect.left;
	*height = WindowRect.bottom - WindowRect.top;
}


/*
=================
GL_EndRendering
=================
*/


void GL_EndRendering (void)
{
	// Baker: hw gamma support

	if (using_hwgamma) 
	{
		static qboolean	old_hwgamma_enabled;

		vid_hwgamma_enabled = vid_hwgammacontrol.value && vid_gammaworks && ActiveApp && !Minimized;
		vid_hwgamma_enabled = vid_hwgamma_enabled && (modestate == MODE_FULLSCREEN || vid_hwgammacontrol.value == 2);
		if (vid_hwgamma_enabled != old_hwgamma_enabled) {
			old_hwgamma_enabled = vid_hwgamma_enabled;
			if (vid_hwgamma_enabled && currentgammaramp)
				VID_SetDeviceGammaRamp (currentgammaramp);
			else
				RestoreHWGamma ();
		}
	}
	// Baker end hwgamma support


	if (!scr_skipupdate)
	{

		if (wglSwapIntervalEXT && update_vsync && vid_vsync.string[0])
			wglSwapIntervalEXT (vid_vsync.value ? 1 : 0);
		update_vsync = false;
#if defined(DX8QUAKE)
	FakeSwapBuffers();

#else
		SwapBuffers (maindc);
#endif
	}

// handle the mouse state when windowed if that's changed
	if (modestate == MODE_WINDOWED)
	{
		if (!_windowed_mouse.value) 
		{
			if (windowed_mouse)	
			{
				IN_Mouse_Unacquire ();
				windowed_mouse = false;
			}
		}
		else 
		{
			windowed_mouse = true;
			if (key_dest == key_game && !flex_mouseactive && ActiveApp)
			{
				IN_Mouse_Acquire ();
			} 
			else if (flex_mouseactive && key_dest != key_game) 
			{
				IN_Mouse_Unacquire ();
			}
		}
	}
#ifdef RELEASE_MOUSE_FULLSCREEN // Baker release mouse even when fullscreen
	else
	{
			windowed_mouse = true;
			if (key_dest == key_game && !flex_mouseactive && ActiveApp)
			{
				IN_Mouse_Acquire ();
			}
			else if (flex_mouseactive && key_dest != key_game)
			{
#define MOUSE_RELEASE_GAME_SAFE  (cls.state != ca_connected || sv.active==true || key_dest == key_menu || cls.demoplayback)				
				if (MOUSE_RELEASE_GAME_SAFE)
			{
					IN_Mouse_Unacquire ();
				}
			}
	}
#endif


	if (fullsbardraw)
		Sbar_Changed();
}

void VID_SetDefaultMode (void)
{
	IN_Mouse_Unacquire ();
}


void	VID_Shutdown (void)
{
   	HGLRC hRC;
   	HDC	  hDC;

	if (vid_initialized)
	{

		if (using_hwgamma)
			RestoreHWGamma ();

		vid_canalttab = false;
		hRC = wglGetCurrentContext();
    	hDC = wglGetCurrentDC();

    	wglMakeCurrent(NULL, NULL);

    	if (hRC)
    	    wglDeleteContext(hRC);

		// Baker hwgamma support

		if (using_hwgamma)
			VID_Gamma_Shutdown (); //johnfitz
		// Baker end hwgamma support


		if (hDC && dibwindow)
			ReleaseDC(dibwindow, hDC);

		if (modestate == MODE_FULLSCREEN)
			ChangeDisplaySettings (NULL, 0);

		if (maindc && dibwindow)
			ReleaseDC (dibwindow, maindc);

		AppActivate(false, false);
	}
}



// Baker begin hwgamma support
void VID_ShiftPalette (unsigned char *palette) {}

// Note: ramps must point to a static array
void VID_SetDeviceGammaRamp (unsigned short *ramps)
{
	if (vid_gammaworks)
	{
		currentgammaramp = ramps;
		if (vid_hwgamma_enabled)
		{
			SetDeviceGammaRamp (maindc, ramps);
			customgamma = true;
		}
	}
}

void InitHWGamma (void)
{


	if (!using_hwgamma)
	{
//		Con_Printf ("Note: Hardware gamma unavailable due to -gamma parameter\n"); // No warning, unnecessary
		return;
	}

	if (COM_CheckParm("-nohwgamma"))
	{
		Con_Warning ("Hardware gamma disabled at command line\n");
		return;
	}

	if (!GetDeviceGammaRamp (maindc, systemgammaramp))
	{
		Con_Warning ("Hardware gamma not available (GetDeviceGammaRamp failed)\n");
		return;
	}

	Con_Success ("Hardware gamma enabled\n");
	vid_gammaworks = true;

}

void RestoreHWGamma (void) 
{
	if (vid_gammaworks && customgamma) 
	{
		customgamma = false;
		SetDeviceGammaRamp (maindc, systemgammaramp);
	}
}

/*
================
VID_Gamma_Restore -- restore system gamma
================
*/
void VID_Gamma_Restore (void)
{
	if (maindc)
	{
		if (vid_gammaworks)
			if (SetDeviceGammaRamp(maindc, systemgammaramp) == false)
				Con_Printf ("VID_Gamma_Restore: failed on SetDeviceGammaRamp\n");
	}
}

/*
================
VID_Gamma_Shutdown -- called on exit
================
*/
void VID_Gamma_Shutdown (void)
{
	VID_Gamma_Restore ();
}




// Baker end hwgamma support

BOOL bSetupPixelFormat(HDC hDC) 
{
    static PIXELFORMATDESCRIPTOR pfd = {
	sizeof(PIXELFORMATDESCRIPTOR),	// size of this pfd
	1,				// version number
	PFD_DRAW_TO_WINDOW |	// support window
	PFD_SUPPORT_OPENGL |	// support OpenGL
	PFD_DOUBLEBUFFER,		// double buffered
	PFD_TYPE_RGBA,			// RGBA type
	24,				// 24-bit color depth
	0, 0, 0, 0, 0, 0,		// color bits ignored
	0,				// no alpha buffer
	0,				// shift bit ignored
	0,				// no accumulation buffer
	0, 0, 0, 0, 			// accum bits ignored
	32,				// 32-bit z-buffer
	0,				// no stencil buffer
	0,				// no auxiliary buffer
	PFD_MAIN_PLANE,			// main layer
	0,				// reserved
	0, 0, 0				// layer masks ignored
    };
    int pixelformat;

    if ((pixelformat = ChoosePixelFormat(hDC, &pfd)) == 0 ) 
	{
        MessageBox(NULL, "ChoosePixelFormat failed", "Error", MB_OK);
        return FALSE;
    }

    if (SetPixelFormat(hDC, pixelformat, &pfd) == FALSE) 
	{
        MessageBox(NULL, "SetPixelFormat failed", "Error", MB_OK);
        return FALSE;
    }

    return TRUE;
}

/*
===================================================================

MAIN WINDOW

===================================================================
*/



void AppActivate(BOOL fActive, BOOL minimize)
{
/****************************************************************************
*
* Function:     AppActivate
* Parameters:   fActive - True if app is activating
*
* Description:  If the application is activating, then swap the system
*               into SYSPAL_NOSTATIC mode so that our palettes will display
*               correctly.
*
****************************************************************************/
	static BOOL	sound_active;

	ActiveApp = fActive;
	Minimized = minimize;

// enable/disable sound on focus gain/loss
	if (!ActiveApp && sound_active)
	{
		S_BlockSound ();
		S_ClearBuffer ();

		sound_active = false;
	}
	else if (ActiveApp && !sound_active)
	{
		S_UnblockSound ();
		sound_active = true;
	}

	if (fActive)
	{
		if (modestate == MODE_FULLSCREEN)
		{

			IN_Mouse_Acquire ();
			if (vid_canalttab && vid_wassuspended)
			{
				vid_wassuspended = false;
				ChangeDisplaySettings (&gdevmode, CDS_FULLSCREEN);
				ShowWindow(mainwindow, SW_SHOWNORMAL);

				// Fix for alt-tab bug in NVidia drivers
                MoveWindow (mainwindow, 0, 0, gdevmode.dmPelsWidth, gdevmode.dmPelsHeight, false);

				// scr_fullupdate = 0;
				Sbar_Changed ();
			}

		}
		else if (modestate == MODE_WINDOWED && Minimized)
		{
			ShowWindow (mainwindow, SW_RESTORE);
		}
		else if ((modestate == MODE_WINDOWED) && _windowed_mouse.value && key_dest == key_game)
		{
			IN_Mouse_Acquire ();
		}
		IN_Keyboard_Acquire ();

		if (using_hwgamma && vid_hwgamma_enabled)
			if (vid_canalttab && /* !Minimized &&*/ currentgammaramp)
				VID_SetDeviceGammaRamp (currentgammaramp);

	}

	if (!fActive)
	{

		if (using_hwgamma && vid_hwgamma_enabled)
			RestoreHWGamma ();

		if (modestate == MODE_FULLSCREEN)
		{



			IN_Mouse_Unacquire ();
			if (vid_canalttab) 
			{
				ChangeDisplaySettings (NULL, 0);
				vid_wassuspended = true;
			}
		}
		else if ((modestate == MODE_WINDOWED) && _windowed_mouse.value)
		{
			IN_Mouse_Unacquire ();
		}
		IN_Keyboard_Unacquire ();
	}

//	if (fActive)
//		Sys_SetWindowCaption (va ("Active + %s", Minimized ? "Minimized" : "Not Mini") );
//	else
//		Sys_SetWindowCaption (va ("Deactivated + %s", Minimized ? "Minimized" : "Not Mini") );
}

int IN_MapKey (int key);
extern int 	key_special_dest;
int 		extmousex, extmousey; // Baker: for tracking Windowed mouse coordinates
/* main window procedure */
LONG WINAPI MainWndProc (HWND    hWnd, UINT    uMsg, WPARAM  wParam, LPARAM  lParam) {
    LONG    lRet = 1;
	int		fActive, fMinimized, temp;
	extern unsigned int uiWheelMessage;
	char	state[256];
	char	asciichar[4];
	int		vkey;
	int		charlength;
	qboolean down = false;


    switch (uMsg) 
	{
		// JACK: This is the mouse wheel with the Intellimouse
		// Its delta is either positive or neg, and we generate the proper Event.
		case WM_MOUSEWHEEL:
			if ((short) HIWORD(wParam) > 0)
			{
				Key_Event(K_MWHEELUP, 0, true);
				Key_Event(K_MWHEELUP, 0, false);
			}
			else
			{
				Key_Event(K_MWHEELDOWN, 0, true);
				Key_Event(K_MWHEELDOWN, 0, false);
			}
			break;

		case WM_KILLFOCUS:
			if (modestate == MODE_FULLSCREEN)
				ShowWindow(mainwindow, SW_SHOWMINNOACTIVE);
			break;

		case WM_CREATE:
			break;


		case WM_SIZE:
#if 0
            break;

#endif
		case WM_MOVE:
#if 0
			window_x = (int) LOWORD(lParam);
			window_y = (int) HIWORD(lParam);
#else
			VID_UpdateWindowStatus ();
#endif
			break;

		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			down=true;

		case WM_KEYUP:
		case WM_SYSKEYUP:
			// Baker 3.703 - old way
			// Key_Event (IN_MapKey(lParam), false);
			vkey = IN_MapKey(lParam);
				GetKeyboardState (state);
				// alt/ctrl/shift tend to produce funky ToAscii values,
				// and if it's not a single character we don't know care about it
				charlength = ToAscii (wParam, lParam >> 16, state, (unsigned short *)asciichar, 0);
				if (vkey == K_ALT || vkey == K_CTRL || vkey == K_SHIFT || charlength == 0)
					asciichar[0] = 0;
				else if( charlength == 2 ) {
					asciichar[0] = asciichar[1];
				}

				Key_Event (vkey, asciichar[0], down);
			break;

		case WM_SYSCHAR:
		// keep Alt-Space from happening
			break;

	// this is complicated because Win32 seems to pack multiple mouse events into
	// one update sometimes, so we always check all states and look for events
		case WM_LBUTTONUP:
			// Mouse isn't active + special destination
			// means Quake doesn't control mouse
			if (key_special_dest && !flex_mouseactive)
			{
				extmousex = Q_rint((float)LOWORD(lParam)*((float)vid.width/(float)glwidth)); //Con_Printf("Mouse click x/y %d/%d\n", extmousex, extmousey);
				extmousey = Q_rint((float)HIWORD(lParam)*((float)vid.height/(float)glheight));
				Key_Event (K_MOUSECLICK_BUTTON1, 0, false);
				break;
			}
		case WM_RBUTTONUP:
			// Mouse isn't active + special destination
			// means Quake doesn't control mouse
			if (key_special_dest && !flex_mouseactive)
			{
				Key_Event (K_MOUSECLICK_BUTTON2, 0, false);
				break;
			}

// Since we are not trapping button downs for special destination
// like namemaker or customize controls, we need the down event
// captures to be below the above code so it doesn't filter into it
// The code below is safe due to the "& MK_xBUTTON" checks
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_MOUSEMOVE:
			temp = 0;

			if (wParam & MK_LBUTTON)
				temp |= 1;

			if (wParam & MK_RBUTTON)
				temp |= 2;

			if (wParam & MK_MBUTTON)
			{
				if (key_special_dest && !flex_mouseactive)
				{
					Key_Event (K_MOUSECLICK_BUTTON3, 0, false);
					break; // Get out
				}
				temp |= 4;
			}

			if (wParam & MK_XBUTTON1)
			{
				if (key_special_dest && !flex_mouseactive)
				{
					Key_Event (K_MOUSECLICK_BUTTON4, 0, false);
					break; // Get out
				}
				temp |= 8;
			}

			if (wParam & MK_XBUTTON2)
			{
				if (key_special_dest && !flex_mouseactive)
				{
					Key_Event (K_MOUSECLICK_BUTTON5, 0, false);
					break; // Get out
				}
				temp |= 16;
			}

			IN_MouseEvent (temp);

			break;



   	    case WM_CLOSE:

			if (!cl_confirmquit.value || MessageBox(mainwindow, "Are you sure you want to quit?", "Confirm Exit", MB_YESNO | MB_SETFOREGROUND | MB_ICONQUESTION) == IDYES)
				Sys_Quit ();

	        break;

		case WM_ACTIVATE:
			fActive = LOWORD(wParam);
			fMinimized = (BOOL) HIWORD(wParam);
			AppActivate(!(fActive == WA_INACTIVE), fMinimized);

		// fix the leftover Alt from any Alt-Tab or the like that switched us away
			Key_ClearAllStates ();

			break;

   	    case WM_DESTROY:
			if (dibwindow)
				DestroyWindow (dibwindow);

            PostQuitMessage (0);
        break;

/*
		case WM_GRAPHNOTIFY:
#else
		case MM_MCINOTIFY:
#endif
            lRet = CDAudio_MessageHandler (hWnd, uMsg, wParam, lParam);
			break;
			*/

    	default:
            // pass all unhandled messages to DefWindowProc
            lRet = DefWindowProc (hWnd, uMsg, wParam, lParam);
        break;
    }

    // return 1 if handled message, 0 if not
    return lRet;
}





//==========================================================================
//
//  INIT
//
//==========================================================================



/*
===================
VID_Init
===================
*/



void Vid_UpdateDesktopProperties (int* width, int* height, int* bpp, int* freq, int* usablew, int* usableh)
{
	DEVMODE	devmode; // From MH
	RECT 	workarea;

	if (!EnumDisplaySettings (NULL, ENUM_CURRENT_SETTINGS, &devmode))
	{
		Sys_Error ("VID_UpdateDesktopProperties: EnumDisplaySettings failed\n");
		return;
	}

   // Baker: MSDN says SPI_GETWORKAREA is for the primary monitor so we are ok!
   if (!SystemParametersInfo (SPI_GETWORKAREA, 0, &workarea, 0))
   {
	   Sys_Error ("VID_UpdateDesktopProperties: SystemParametersInfo failed\n");
	   return;
   }

	// To help know what fullscreen mode is best.
	*width = devmode.dmPelsWidth;
	*height	= devmode.dmPelsHeight;
	*bpp = devmode.dmBitsPerPel;
	*freq =	devmode.dmDisplayFrequency;

	// To help us know what windowed mode is best.
	// These areas do not include the taskbar
   	*usablew	= workarea.right - workarea.left;
   	*usableh	= workarea.bottom - workarea.top;

}

extern	cvar_t vid_consize;
qboolean Parse_Int_From_String (int *retval, char *string_to_search, char *string_to_find)
{
	int beginning_of_line, value_spot, value_spot_end;
	int spot, end_of_line, i;
	char *cursor;

	cursor = strstr(string_to_search, string_to_find);

#ifdef TOUCHUP_VIDEO_READ_CVARS_EARLY // Baker change
	while (1)
	{
#endif // Baker change +
		if (cursor == NULL)
			return false; // Didn't find it.

		spot = cursor - string_to_search; // Offset into string.

		// Find beginning of line.  Go from location backwards until find newline or hit beginning of buffer

		for (i = spot - 1, beginning_of_line = -1 ; i >= 0 && beginning_of_line == -1; i-- )
			if (string_to_search[i] == '\n')
				beginning_of_line = i + 1;
			else if (i == 0)
				beginning_of_line = 0;

		if (beginning_of_line == -1)
			return false; // Didn't find beginning of line?  Errr.  This shouldn't happen

#ifdef TOUCHUP_VIDEO_READ_CVARS_EARLY // Baker change
		if (beginning_of_line != spot)
		{
			// Be skeptical of matches that are not beginning of the line
			// These might be aliases or something and the engine doesn't write the config
			// in this manner.  So advance the cursor past the search result and look again.
			cursor = strstr(cursor + strlen(string_to_find), string_to_find);
			continue; // Try again
		}

		break;
	}
#endif // Baker change +

	// Find end of line. Go from location ahead of string and stop at newline or it automatically stops at EOF

	for (i = spot + strlen(string_to_find), end_of_line = -1; string_to_find[i] && end_of_line == -1; i++ )
		if (string_to_search[i] == '\r' || string_to_search[i] == '\n')
			end_of_line = i - 1;

	if (end_of_line == -1) // Hit null terminator
		end_of_line = strlen(string_to_search) - 1;

	// We have beginning of line and end of line.  Go from spot + strlen forward and skip spaces and quotes.
	for (i = spot + strlen(string_to_find), value_spot = -1, value_spot_end = -1; i <= end_of_line && (value_spot == -1 || value_spot_end == -1); i++)
		if (string_to_search[i] == ' ' || string_to_search[i] == '\"')
		{
			// If we already found the start, we are looking for the end and just found it
			// Otherwise we are just skipping these characters because we ignore them
			if (value_spot != -1)
				value_spot_end = i - 1;

		}
		else if (value_spot == -1)
			value_spot = i; // We didn't have the start but now we found it

	// Ok check what we found

	if (value_spot == -1)
		return false; // No value

	if (value_spot_end == -1)
		value_spot_end = end_of_line;

	do
	{
		// Parse it and set return value
		char temp = string_to_search[value_spot_end + 1];

		string_to_search[value_spot_end + 1] = 0;
		*retval = atoi (&string_to_search[value_spot]);
		string_to_search[value_spot_end + 1] = temp;

	} while (0);
	return true;

}

qboolean Vid_Read_Early_Cvars (void)
{
	cvar_t *video_cvars[] = {&vid_fullscreen, &vid_width, &vid_height, &vid_bpp, &vid_refreshrate, NULL};
	char config_buffer[8192];
	FILE *f;
	int bytes_size = COM_FOpenFile ("config.cfg", &f), bytes_read, i;
	qboolean found_any_vid_cvars = false;

	// Read the file into the buffer.  Read the first 8000 bytes (if longer, tough cookies)
	// Because it is pretty likely that size of file will get a "SZ_GetSpace: overflow without allowoverflow set"
	// During command execution

	if (bytes_size == -1) // File not found
		return false;

	if (bytes_size > 8000)
		bytes_size = 8000;

	bytes_read = fread (config_buffer, 1, bytes_size, f);
	config_buffer [bytes_read + 1] = 0; // Just in case

	fclose (f);

	for (i = 0; video_cvars[i]; i++)
	{
		int value;
		qboolean found = Parse_Int_From_String (&value, config_buffer, video_cvars[i]->name);

		if (found == false)
			continue;

		Cvar_SetValue (video_cvars[i]->name, (float)value);
#if 0 // Baker: I used this to debug since Con_Printf can't work this early
		MessageBox(NULL, va("Cvar found %s = %i",  video_cvars[i]->name, value), "Vid_Read_Early_Cvars", MB_OK);
#endif
		found_any_vid_cvars = true;
	}


	return found_any_vid_cvars;
}

void VID_Init (unsigned char *palette)
{
	HGLRC	baseRC; //johnfitz -- moved here from global scope, since it was only used in this
	DEVMODE	devmode;
	extern	cvar_t gl_clear;
	extern	void GL_PrintExtensions_f (void);

	memset(&devmode, 0, sizeof(devmode));

	{
		Cvar_RegisterVariable (&vid_fullscreen, NULL); //johnfitz
		Cvar_RegisterVariable (&vid_width, NULL); //johnfitz
		Cvar_RegisterVariable (&vid_height, NULL); //johnfitz
		Cvar_RegisterVariable (&vid_bpp, NULL); //johnfitz
		Cvar_RegisterVariable (&vid_consize, VID_Consize_f); //Baker 3.97: this supercedes vid_conwidth/vid_conheight cvars
		Cvar_RegisterVariable (&vid_refreshrate, NULL); //johnfitz
		Cvar_RegisterVariable (&vid_vsync, VID_Vsync_f);


		Cvar_RegisterVariable (&_windowed_mouse, NULL);

		Cvar_RegisterVariable (&gl_clear, NULL); // Baker: cvar needs registered here so we can set it
		Cvar_RegisterVariable (&gl_ztrick, NULL);

		// Baker hwgamma support
		Cvar_RegisterVariable (&vid_hwgammacontrol, NULL);
		// Baker end hwgamma support

		Cmd_AddCommand ("vid_unlock", VID_Unlock_f); //johnfitz
		Cmd_AddCommand ("vid_restart", VID_Restart_f); //johnfitz
		Cmd_AddCommand ("vid_force_restart", VID_ForceRestart_f); //Baker: restart regardless of whether mode is same
		Cmd_AddCommand ("vid_test", VID_Test_f); //johnfitz

		Cmd_AddCommand ("gl_info", GL_PrintExtensions_f);
	//	Cmd_AddCommand ("vid_describecurrentmode", VID_DescribeCurrentMode_f);
	//	Cmd_AddCommand ("vid_describemodes", VID_DescribeModes_f);
	}

	System_GammaReset (); // Baker reset the gamma

	Vid_UpdateDesktopProperties (&vid.desktop_width, &vid.desktop_height, &vid.desktop_bpp, &vid.desktop_dispfreq, &vid.desktop_areawidth, &vid.desktop_areaheight);
	Vid_Win32_InitWindow (global_hInstance);

	
	if (COM_CheckParm("-gamma")) // Baker hwgamma support
		using_hwgamma = false;
	
	// Command line param overrides
	if (COM_CheckParm("-current"))
	{
		Cvar_SetValue (vid_fullscreen.name, 1);
		Cvar_SetValue (vid_width.name, vid.desktop_width);
		Cvar_SetValue (vid_height.name, vid.desktop_height);
		Cvar_SetValue (vid_bpp.name, vid.desktop_bpp);
		Cvar_SetValue (vid_refreshrate.name, vid.desktop_dispfreq);
	}
	else if (COM_CheckParm("-window") || COM_CheckParm("-width") || COM_CheckParm("-height") || COM_CheckParm("-bpp") )
	{
		int pnum, val;
	
		if (COM_CheckParm("-window"))
		{
			Cvar_SetValue (vid_fullscreen.name, 0);
		}

		if ( (pnum = COM_CheckParm("-width")) && pnum + 1 < com_argc && (val = atoi(com_argv[pnum + 1])) )
			Cvar_SetValue (vid_width.name, val);

		if ( (pnum = COM_CheckParm("-height")) && pnum + 1 < com_argc && (val = atoi(com_argv[pnum + 1])) )
			Cvar_SetValue (vid_height.name, val);

		if ( (pnum = COM_CheckParm("-bpp")) && pnum + 1 < com_argc && (val = atoi(com_argv[pnum + 1])) )
			Cvar_SetValue (vid_bpp.name, val);
	}
	else
		Vid_Read_Early_Cvars (); // TODO: Specifying command line params doesn't read the cvars.


	Vidmodes_Populate ((int)vid_width.value, (int)vid_height.value, (int)vid_bpp.value);

	windowed = !(vid_default = vid_fullscreen.value ? 1 : 0);

	Check_GammaOld (palette);
	VID_SetPaletteOld (palette);	// builds d_8to24table
	
	VID_SetMode (vid_default, palette);
//	Vid_SetMode_On_Cvars ();

	vid_initialized = true;

    maindc = GetDC(mainwindow);
	bSetupPixelFormat(maindc);

	InitHWGamma ();
	// baker end hwgamma support
    baseRC = wglCreateContext( maindc );
	if (!baseRC)
		Sys_Error ("Could not initialize GL (wglCreateContext failed).\n\nMake sure you are in 65535 color mode, and try running -window.");
    if (!wglMakeCurrent( maindc, baseRC ))
		Sys_Error ("VID_Init: wglMakeCurrent failed");

	GL_Init ();
	CheckVsyncControlExtensions ();


	vid_realmode = vid_modenum;

	vid_menucmdfn = VID_Menu_f; //johnfitz
	vid_menudrawfn = VID_MenuDraw;
	vid_menukeyfn = VID_MenuKey;

//	strcpy (badmode.modedesc, "Bad mode");
	vid_canalttab = true;

	if (COM_CheckParm("-fullsbar"))
		fullsbardraw = true;

	VID_Menu_Init(); //johnfitz

	//johnfitz -- command line vid settings should override config file settings.
	//so we have to lock the vid mode from now until after all config files are read.
	if (COM_CheckParm("-width") || COM_CheckParm("-height") /*|| COM_CheckParm("-bpp")*/ || COM_CheckParm("-window"))
	{
		// Baker 3.99h: I'd like to allow video mode changing with -bpp != desktop
//		vid_locked = true;
	}
	else
	{
		// Baker: 3.99h if vid is not -window and bpp != desktop_bpp then vid_windowtypelocked = true;
		if (vid_bpp.value != vid.desktop_bpp)
			vid_fullscreen_only = true;
	}

	//johnfitz
}

/*
================
VID_SyncCvars -- johnfitz -- set vid cvars to match current video mode
================
*/
extern qboolean vid_consize_ignore_callback;
void VID_SyncCvars (void)
{
	Cvar_Set ("vid_width", va("%i", modelist[vid_default].width));
	Cvar_Set ("vid_height", va("%i", modelist[vid_default].height));
	Cvar_Set ("vid_refreshrate", va("%i", modelist[vid_default].refreshrate));
	Cvar_Set ("vid_fullscreen", (vid_fullscreen_only) ? "1" : ((windowed) ? "0" : "1"));
}

// Baker 3.97: new scheme supercedes these

/*
==================
VID_Consize_f -- Baker -- called when vid_consize changes
==================
*/
extern qpic_t *conback;
//qboolean vid_smoothfont = false;
extern qboolean smoothfont_init;

void VID_Consize_f(void)
{

	float startwidth;
	float startheight;
	float desiredwidth;
	int contype = vid_consize.value;
	int exception = 0;

	startwidth = vid.width = modelist[vid_default].width;
	startheight = vid.height = modelist[vid_default].height;

//	Con_Printf("Entering ...\n");
//	Con_Printf("vid.width is %d and vid.height is %d\n", vid.width, vid.height);
//	Con_Printf("vid.conwidth is %d and vid.conheight is %d\n", vid.conwidth, vid.conheight);

	// Baker 3.97
	// We need to appropriately set vid.width, vid.height, vid.smoothfont (?)

//	vid_smoothfont = false; // Assume it is unnecessary

	if (contype == -1)
	{
		// Automatic consize to avoid microscopic text
		if (vid.width>=1024)
			contype = 1;
		else
			contype = 0;
	}

	switch (contype)
	{

		case 0: // consize is width

			desiredwidth = vid.width;
			break;

		case 1: // consize is 50% width (if possible)

			// if resolution is < 640, must use the resolution itself.
			if (vid.width < 640) {
				exception = 1; // Notify later about console resolution unavailable
				desiredwidth = vid.width;
				break;
			}

			desiredwidth = (int)(vid.width/2);
			break;

		case 3:
			desiredwidth = 320;
			break;

		default:
			// If vid.width is under 640, must use 320?
			if (vid.width < 640) {
				exception = 2; // Notify later about console resolution unavailable
				desiredwidth = vid.width;
				break;
			}
			desiredwidth = 640;
			break;
	}

	vid.conwidth = CLAMP (320, desiredwidth, vid.width);
	vid.conwidth &= 0xFFFFFFF8;                      // But this must be a multiple of 8
	vid.conheight = vid.conwidth * vid.height / vid.width;  // Now set height using proper aspect ratio
	vid.conheight &= 0xFFFFFFF8;					  // This too must be a multiple of 8

	conback->width = vid.width = vid.conwidth; // = vid.width;
	conback->height = vid.height = vid.conheight; // = vid.height;

	//  Determine if smooth font is needed

	if ((int)(startwidth / vid.conwidth) == ((startwidth + 0.0f) / (vid.conwidth + 0.0f)) /*&& (int)(startheight / vid.conheight) == ((startheight + 0.0f) / (vid.conheight + 0.0f))*/)
	{
		SmoothFontSet (false);
	}
	 else
	{
		SmoothFontSet (true);
	}

	// Print messages AFTER console resizing to ensure they print right
	if (exception)
	{
		if (exception == 1)
			Con_Printf ("VID_Consize_f: 50%% console size unavailable, using 100%% for this resolution.\n");
		else
			Con_Printf ("VID_Consize_f: 640 console size unavailable, using 100%% for this resolution.\n");
	}

	vid.recalc_refdef = 1;

}


//==========================================================================
//
//  NEW VIDEO MENU -- johnfitz
//
//==========================================================================

extern void M_Menu_Options_f (void);
extern void M_Print (int cx, int cy, char *str);
extern void M_PrintWhite (int cx, int cy, char *str);
extern void M_DrawCharacter (int cx, int line, int num);
extern void M_DrawTransPic (int x, int y, qpic_t *pic);
extern void M_DrawPic (int x, int y, qpic_t *pic);
extern void M_DrawCheckbox (int x, int y, int on);

extern qboolean	m_entersound;


#define VIDEO_OPTIONS_ITEMS 6
int		video_cursor_table[] = {48, 56, 64, 72, 88, 96};
int		video_options_cursor = 0;

typedef struct {int width,height;} vid_menu_mode;

//TODO: replace these fixed-length arrays with hunk_allocated buffers

vid_menu_mode vid_menu_modes[MAX_MODE_LIST];
int vid_menu_nummodes=0;

//int vid_menu_bpps[4];
//int vid_menu_numbpps=0;

int vid_menu_rates[20];
int vid_menu_numrates=0;

/*
================
VID_Menu_Init
================
*/
void VID_Menu_Init (void)
{
	int i,j,h,w;

	for (i=1;i<nummodes;i++) //start i at mode 1 because 0 is windowed mode
	{
		w = modelist[i].width;
		h = modelist[i].height;

		for (j=0;j<vid_menu_nummodes;j++)
{
			if (vid_menu_modes[j].width == w &&
				vid_menu_modes[j].height == h)
				break;
		}

		if (j==vid_menu_nummodes)
		{
			vid_menu_modes[j].width = w;
			vid_menu_modes[j].height = h;
			vid_menu_nummodes++;
		}
	}
}


/*
================
VID_Menu_RebuildRateList

regenerates rate list based on current vid_width, vid_height and vid_bpp
================
*/
void VID_Menu_RebuildRateList (void)
{
	int i,j,r;

	vid_menu_numrates=0;

	for (i=1;i<nummodes;i++) //start i at mode 1 because 0 is windowed mode
	{
		//rate list is limited to rates available with current width/height/bpp
		if (modelist[i].width != vid_width.value ||
			modelist[i].height != vid_height.value /*||
			modelist[i].bpp != vid_bpp.value*/)
			continue;

		r = modelist[i].refreshrate;

		for (j=0;j<vid_menu_numrates;j++)
		{
			if (vid_menu_rates[j] == r)
				break;
		}

		if (j==vid_menu_numrates)
		{
			vid_menu_rates[j] = r;
			vid_menu_numrates++;
		}
	}

	//if vid_refreshrate is not in the new list, change vid_refreshrate
	for (i=0;i<vid_menu_numrates;i++)
		if (vid_menu_rates[i] == (int)(vid_refreshrate.value))
			break;

	if (i==vid_menu_numrates)
		Cvar_Set ("vid_refreshrate",va("%i",vid_menu_rates[0]));
	}

/*
================
VID_Menu_ChooseNextMode

chooses next resolution in order, then updates vid_width and
vid_height cvars, then updates bpp and refreshrate lists
================
*/
void VID_Menu_ChooseNextMode (int dir)
{
	int i;

	for (i=0;i<vid_menu_nummodes;i++)
	{
		if (vid_menu_modes[i].width == vid_width.value &&
			vid_menu_modes[i].height == vid_height.value)
			break;
	}

	if (i==vid_menu_nummodes) //can't find it in list, so it must be a custom windowed res
	{
		i = 0;
	}
			else
	{
		i+=dir;
		if (i>=vid_menu_nummodes)
			i = 0;
		else if (i<0)
			i = vid_menu_nummodes-1;
	}

	Cvar_Set ("vid_width",va("%i",vid_menu_modes[i].width));
	Cvar_Set ("vid_height",va("%i",vid_menu_modes[i].height));
	VID_Menu_RebuildRateList ();
}

/*
================
VID_Menu_ChooseNextRate

chooses next refresh rate in order, then updates vid_refreshrate cvar
================
*/
void VID_Menu_ChooseNextRate (int dir)
{
	int i;

	for (i=0;i<vid_menu_numrates;i++)
	{
		if (vid_menu_rates[i] == vid_refreshrate.value)
			break;
	}

	if (i==vid_menu_numrates) //can't find it in list
	{
		i = 0;
	}
	else
	{
		i+=dir;
		if (i>=vid_menu_numrates)
			i = 0;
		else if (i<0)
			i = vid_menu_numrates-1;
}

	Cvar_Set ("vid_refreshrate",va("%i",vid_menu_rates[i]));
}

/*
================
VID_MenuKey
================
*/
void VID_MenuKey (int key)
{
	switch (key)
	{
	case K_ESCAPE:
		VID_SyncCvars (); //sync cvars before leaving menu. FIXME: there are other ways to leave menu
		S_LocalSound ("misc/menu1.wav");
		M_Menu_Options_f ();
		break;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		video_options_cursor--;
		if (video_options_cursor < 0)
			video_options_cursor = VIDEO_OPTIONS_ITEMS-1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		video_options_cursor++;
		if (video_options_cursor >= VIDEO_OPTIONS_ITEMS)
			video_options_cursor = 0;
		break;

	case K_LEFTARROW:
		S_LocalSound ("misc/menu3.wav");
		switch (video_options_cursor)
		{
		case 0:
			VID_Menu_ChooseNextMode (-1);
			break;
		case 1:
			//VID_Menu_ChooseNextBpp (-1);
			break;
		case 2:
			VID_Menu_ChooseNextRate (-1);
			break;
		case 3:
			Cbuf_AddText ("toggle vid_fullscreen\n");
			break;
		case 4:
		case 5:
	default:
		break;
	}
		break;

	case K_RIGHTARROW:
		S_LocalSound ("misc/menu3.wav");
		switch (video_options_cursor)
		{
		case 0:
			VID_Menu_ChooseNextMode (1);
			break;
		case 1:
			//VID_Menu_ChooseNextBpp (1);
			break;
		case 2:
			VID_Menu_ChooseNextRate (1);
			break;
		case 3:
			if (vid_bpp.value == vid.desktop_bpp)
				Cbuf_AddText ("toggle vid_fullscreen\n");
			break;
		case 4:
		case 5:
		default:
			break;
		}
		break;

	case K_ENTER:
		m_entersound = true;
		switch (video_options_cursor)
		{
		case 0:
			VID_Menu_ChooseNextMode (1);
			break;
		case 1:
			SCR_ModalMessage("Colordepth (bits per pixel) must be set\nusing -bpp 16 or -bpp 32 from the\ncommand line.\n\nPress Y or N to continue.",0.0f);
			break;
		case 2:
			VID_Menu_ChooseNextRate (1);
			break;
		case 3:
			if (vid_bpp.value == vid.desktop_bpp)
				Cbuf_AddText ("toggle vid_fullscreen\n");
			else
				SCR_ModalMessage("Changing between fullscreen and\nwindowed mode is not available\nbecause your color depth does\nnot match the desktop.\n\nRemove -bpp from your command line\nto have this available.\n\nPress Y or N to continue.",0.0f);

			break;

		case 4:
			Cbuf_AddText ("vid_test\n");
			break;
		case 5:
			Cbuf_AddText ("vid_restart\n");
			break;
		default:
			break;
		}
		break;

	default:
		break;
	}
}

/*
================
VID_MenuDraw
================
*/
void VID_MenuDraw (void)
{
	int i = 0;
	qpic_t *p;
	char *title;
	int  aspectratio1;

	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp"));

	//p = Draw_CachePic ("gfx/vidmodes.lmp");
	p = Draw_CachePic ("gfx/p_option.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);

	// title
	title = "Video Options";
	M_PrintWhite ((320-8*strlen(title))/2, 32, title);

	// options
	M_Print (16, video_cursor_table[i], "        Video mode");
	M_Print (184, video_cursor_table[i], va("%ix%i", (int)vid_width.value, (int)vid_height.value));

	// Baker: show aspect ratio
	aspectratio1 = (int)((vid_width.value/vid_height.value) * 100.0f);

	if (aspectratio1 == 133) // 1.33333
		M_PrintWhite (264, video_cursor_table[i], "4:3");
	else if (aspectratio1 == 125) // 1.25
		M_PrintWhite (264, video_cursor_table[i], "5:4");
	else if (aspectratio1 == 160) // 1.6
		M_PrintWhite (264, video_cursor_table[i], "8:5");
	else if (aspectratio1 == 166) // 1.6666
		M_PrintWhite (264, video_cursor_table[i], "5:3");
	else if (aspectratio1 == 155) // 1.5555
		M_PrintWhite (264, video_cursor_table[i], "14:9");
	else if (aspectratio1 == 177) // 1.7777
		M_PrintWhite (264, video_cursor_table[i], "16:9");
	// Baker ... else just don't print one

	i++;

	M_Print (16, video_cursor_table[i], "       Color depth");
	M_Print (184, video_cursor_table[i], va("%i [locked]", (int)vid_bpp.value));
	i++;

	M_Print (16, video_cursor_table[i], "      Refresh rate");
	M_Print (184, video_cursor_table[i], va("%i Hz", (int)vid_refreshrate.value));
	i++;

	M_Print (16, video_cursor_table[i], "        Fullscreen");

	if (vid_bpp.value == vid.desktop_bpp)
		M_DrawCheckbox (184, video_cursor_table[i], (int)vid_fullscreen.value);
	else
		M_Print (184, video_cursor_table[i], va("%s [locked]", (int)vid_fullscreen.value ? "on" : "off"));

	i++;

	M_Print (16, video_cursor_table[i], "      Test changes");
	i++;

	M_Print (16, video_cursor_table[i], "     Apply changes");

	// cursor
	M_DrawCharacter (168, video_cursor_table[video_options_cursor], 12+((int)(realtime*4)&1));

	// notes          "345678901234567890123456789012345678"
//	M_Print (16, 172, "Windowed modes always use the desk- ");
//	M_Print (16, 180, "top color depth, and can never be   ");
//	M_Print (16, 188, "larger than the desktop resolution. ");
}

/*
================
VID_Menu_f
================
*/
void VID_Menu_f (void)
{
	key_dest = key_menu;
	m_state = m_video;
	m_entersound = true;

	//set all the cvars to match the current mode when entering the menu
	VID_SyncCvars ();

	//set up bpp and rate lists based on current cvars
	VID_Menu_RebuildRateList ();
}