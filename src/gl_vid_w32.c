


#include "quakedef.h"
#include "winquake.h"
#include "resource.h"


LONG WINAPI MainWndProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


void Vid_Win32_InitWindow (HINSTANCE hInstance)
{
	HICON	hIcon = LoadIcon (global_hInstance, MAKEINTRESOURCE (IDI_ICON2));
	WNDCLASS		wc;

	/* Register the frame class */
    wc.style         = 0;
    wc.lpfnWndProc   = (WNDPROC)MainWndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = hIcon;
    wc.hCursor       = LoadCursor (NULL,IDC_ARROW);
	wc.hbrBackground = NULL;
    wc.lpszMenuName  = 0;
    wc.lpszClassName = ENGINE_NAME; //"WinQuake";

    if (!RegisterClass (&wc) )
		Sys_Error ("Couldn't register window class");



}




#define VIDEO_MODES_MAXWIDTH		10000
#define VIDEO_MODES_MAXHEIGHT		10000


int Vidmodes_Add (int modetype, int w, int h, int bpp, int dispfreq)
{
	// Make sure doesn't exist already
	int i;

	for (i = 0; i < nummodes; i++)
		if (modelist[i].width == w && modelist[i].height == h && modelist[i].bpp)
			if (modelist[i].type == modetype && modelist[nummodes].refreshrate == dispfreq)
				return 0;  // Mode already exists
	
	// Mode is new
	modelist[nummodes].type = modetype; // MODE_WINDOWED, etc.
	modelist[nummodes].width = w;
	modelist[nummodes].height =	h;
	modelist[nummodes].bpp = bpp;
	modelist[nummodes].refreshrate = dispfreq;

	// Fill in description
	SNPrintf (modelist[nummodes].modedesc, sizeof(modelist[nummodes].modedesc), "%dx%dx%d %dHz", 
			 modelist[nummodes].width,
			 modelist[nummodes].height,
			 modelist[nummodes].bpp,
			 modelist[nummodes].refreshrate);

	nummodes++;
	return 1;
}

int Vidmodes_AddAll (qboolean doLow)
{
	int this_hardware_modenum = 0, added = 0;
	BOOL	stat;
	do
	{
		DEVMODE	devm;
		if ( (stat = EnumDisplaySettings (NULL, this_hardware_modenum, &devm)) )
		{
			qboolean modeAcceptable = devm.dmBitsPerPel >= 15 && devm.dmPelsWidth <= VIDEO_MODES_MAXWIDTH && devm.dmPelsHeight <=  VIDEO_MODES_MAXHEIGHT && nummodes < MAX_MODE_LIST;
			qboolean isLow = devm.dmPelsWidth < 640 || devm.dmPelsHeight < 480;
			devm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL | DM_DISPLAYFREQUENCY;

			if ( modeAcceptable && isLow == doLow && ChangeDisplaySettings (&devm, CDS_TEST | CDS_FULLSCREEN) == DISP_CHANGE_SUCCESSFUL)
				added += Vidmodes_Add (MODE_FULLSCREEN, devm.dmPelsWidth, devm.dmPelsHeight, devm.dmBitsPerPel, devm.dmDisplayFrequency);
		}
		this_hardware_modenum ++;
	} while (stat);
	return added;
}

void Vidmodes_Populate (int w, int h, int bpp)
{
	int added = 0;

	Vidmodes_Add (MODE_WINDOWED, w, h, bpp /*vid.desktop_bpp*/, vid.desktop_dispfreq);
	Vidmodes_Add (MODE_FULLSCREEN, w, h, bpp /*vid.desktop_bpp*/, vid.desktop_dispfreq);

	added += Vidmodes_AddAll (false);
	added += Vidmodes_AddAll (true);

	if (added == 0)
		Con_SafePrintf ("No fullscreen DIB modes found\n");

}


void System_GammaReset (void)
{
	HDC hdc = GetDC (NULL);
	WORD gammaramps[3][256];
	int i;

	for (i = 0;i < 256;i++)
		gammaramps[0][i] = gammaramps[1][i] = gammaramps[2][i] = (i * 65535) / 255;

	if ( SetDeviceGammaRamp(hdc, &gammaramps[0][0]) == 0)
		Con_SafePrintf ("Failed to reset gamma\n");

	ReleaseDC (NULL, hdc);
}
