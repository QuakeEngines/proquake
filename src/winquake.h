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
// winquake.h: Win32-specific Quake header file


#include <windows.h>


#if defined(_MSC_VER) && _MSC_VER <=1200 // MSVC6 ONLY -- Do not do for CodeBlocks/MinGW/GCC
#define WM_MOUSEWHEEL                   0x020A
#define MK_XBUTTON1 0x0020
#define MK_XBUTTON2 0x0040
#define LLKHF_UP			(KF_UP >> 8)
#define KF_UP				0x8000
#define WM_GRAPHNOTIFY  WM_USER + 13



typedef ULONG ULONG_PTR; // Baker 3.99r: this is not appropriate for 64 bit, only 32 but I'm not building 64 bit version
typedef struct 
{
    DWORD   vkCode;
    DWORD   scanCode;
    DWORD   flags;
    DWORD   time;
    ULONG_PTR dwExtraInfo;
} *PKBDLLHOOKSTRUCT;
#endif



#ifndef SERVERONLY
// Sound
#include <dsound.h>

extern LPDIRECTSOUND pDS;
extern LPDIRECTSOUNDBUFFER pDSBuf;

extern DWORD gSndBufSize;
void S_BlockSound (void);
void S_UnblockSound (void);

// CD
//LONG CDAudio_MessageHandler (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Handles
extern	HINSTANCE	global_hInstance;
extern	int			global_nCmdShow;

// Window states


extern modestate_t	modestate;

extern HWND			mainwindow;
extern qboolean		ActiveApp, Minimized;
void VID_SetDefaultMode (void);


// Input
extern cvar_t _windowed_mouse;
void IN_Mouse_Acquire (void);
void IN_Mouse_Unacquire (void);
void IN_MouseEvent (int mstate);
extern int		window_center_x, window_center_y;
extern RECT		window_rect;

extern qboolean	mouseinitialized;
extern HWND		hwnd_dialog;
void IN_UpdateClipCursor (void);
void CenterWindow(HWND hWndCenter, int width, int height, BOOL lefttopjustify);

#endif



extern qboolean	WinNT;



extern qboolean	winsock_lib_initialized;


extern HANDLE	hinput, houtput;



int (PASCAL FAR *pWSAStartup)(WORD wVersionRequired, LPWSADATA lpWSAData);
int (PASCAL FAR *pWSACleanup)(void);
int (PASCAL FAR *pWSAGetLastError)(void);
SOCKET (PASCAL FAR *psocket)(int af, int type, int protocol);
int (PASCAL FAR *pioctlsocket)(SOCKET s, long cmd, u_long FAR *argp);
int (PASCAL FAR *psetsockopt)(SOCKET s, int level, int optname, const char FAR * optval, int optlen);
int (PASCAL FAR *precvfrom)(SOCKET s, char FAR * buf, int len, int flags, struct sockaddr FAR *from, int FAR * fromlen);
int (PASCAL FAR *psendto)(SOCKET s, const char FAR * buf, int len, int flags, const struct sockaddr FAR *to, int tolen);
int (PASCAL FAR *pclosesocket)(SOCKET s);
int (PASCAL FAR *pgethostname)(char FAR * name, int namelen);
struct hostent FAR * (PASCAL FAR *pgethostbyname)(const char FAR * name);
struct hostent FAR * (PASCAL FAR *pgethostbyaddr)(const char FAR * addr, int len, int type);
int (PASCAL FAR *pgetsockname)(SOCKET s, struct sockaddr FAR *name, int FAR * namelen);
