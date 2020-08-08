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
// sys_win.c -- Win32 system interface code

#include "quakedef.h"
#include "winquake.h"
#include "resource.h"
#include "conproc.h"
#include <limits.h> // Clock LONG_MAX or somethign
#include <errno.h>


// JPG 3.30 - need these for synchronization
#include <fcntl.h>
#include <sys/stat.h>

#define MINIMUM_WIN_MEMORY		0x0c00000 // 12Mb
#define MAXIMUM_WIN_MEMORY		0x2000000 // Baker 3.75 - increase to 32MB minimum

#define CONSOLE_ERROR_TIMEOUT	60.0	// # of seconds to wait on Sys_Error running
										//  dedicated before exiting
#define PAUSE_SLEEP		50				// sleep time on pause or minimization
#define NOT_FOCUS_SLEEP	20				// sleep time when not focus

//qboolean OnChange_sys_highpriority (cvar_t *, char *);
cvar_t	sys_highpriority = {"sys_highpriority", "0", false}; // Baker 3.99r: sometimes this worsens online performance, so not saving this to config

int			starttime;
qboolean	ActiveApp, Minimized;
qboolean	WinNT;

static double		pfreq;

qboolean			isDedicated;


static char			*tracking_tag = "Clams & Mooses";

static HANDLE	tevent;
static HANDLE	hFile;
static HANDLE	heventParent;
static HANDLE	heventChild;


void Sys_InitFloatTime (void);


volatile int					sys_checksum;


/*
================
Sys_PageIn
================
*/
void Sys_PageIn (void *ptr, int size)
{
	byte	*x;
	int		m, n;

// touch all the memory to make sure it's there. The 16-page skip is to
// keep Win 95 from thinking we're trying to page ourselves in (we are
// doing that, of course, but there's no reason we shouldn't)
	x = (byte *)ptr;

	for (n=0 ; n<4 ; n++)
	{
		for (m=0 ; m<(size - 16 * 0x1000) ; m += 4)
		{
			sys_checksum += *(int *)&x[m];
			sys_checksum += *(int *)&x[m + 16 * 0x1000];
		}
	}
}


/*
===============================================================================

SYNCHRONIZATION - JPG 3.30

===============================================================================
*/

int hlock;
_CRTIMP int __cdecl _open(const char *, int, ...);
_CRTIMP int __cdecl _close(int);

/*
================
Sys_GetLock
================
*/
void Sys_GetLock (void)
{
	int i;

	for (i = 0 ; i < 10 ; i++)
	{
		hlock = _open(va("%s/lock.dat",com_gamedir), _O_CREAT | _O_EXCL, _S_IREAD | _S_IWRITE);
		if (hlock != -1)
			return;
		Sleep(1000);
	}

	Sys_Printf("Warning: could not open lock; using crowbar\n");
}

/*
================
Sys_ReleaseLock
================
*/
void Sys_ReleaseLock (void)
{
	if (hlock != -1)
		_close(hlock);
	unlink(va("%s/lock.dat",com_gamedir));
}



int Sys_SetPriority(int priority) 
{
    DWORD p;

	switch (priority) 
	{
		case 0:	p = IDLE_PRIORITY_CLASS; break;
		case 1:	p = NORMAL_PRIORITY_CLASS; break;
		case 2:	p = HIGH_PRIORITY_CLASS; break;
		case 3:	p = REALTIME_PRIORITY_CLASS; break;
		default: return 0;
	}

	return SetPriorityClass(GetCurrentProcess(), p);
}

void OnChange_sys_highpriority (void) /*(cvar_t *var, char *s)*/ 
{
	int ok, q_priority;
	char *desc;
	float priority;

	priority = (int)sys_highpriority.value; //atof(s);
	if (priority == 1) 
	{
		q_priority = 2;
		desc = "high";
	}
	else if (priority == -1) 
	{
		q_priority = 0;
		desc = "low";
	} 
	else 
	{
		q_priority = 1;
		desc = "normal";
	}

	if (!(ok = Sys_SetPriority(q_priority))) 
	{
		Con_Printf("Changing process priority failed\n");
		return; // true;
	}

	Con_Printf("Process priority set to %s\n", (q_priority == 1) ? "normal" : ((q_priority == 2) ? "high" : "low"));
	return; // false;
}


/*
===============================================================================
FILE IO
===============================================================================
*/

#define	MAX_HANDLES		10 //Baker 3.79 - FitzQuake uses 32, but I don't think we should ever have 32 file handles open // johnfitz -- was 10
FILE	*sys_handles[MAX_HANDLES];

int		findhandle (void)
{
	int		i;

	for (i=1 ; i<MAX_HANDLES ; i++)
		if (!sys_handles[i])
			return i;
	Sys_Error ("out of handles");
	return -1;
}

/*
================
filelength
================
*/
int filelength (FILE *f)
{
	int		pos;
	int		end;




	pos = ftell (f);
	fseek (f, 0, SEEK_END);
	end = ftell (f);
	fseek (f, pos, SEEK_SET);



	return end;
}

int Sys_FileOpenRead (char *path, int *hndl)
{
	FILE	*f;
	int	i, retval;



	i = findhandle ();

	f = fopen(path, "rb");

	if (!f)
	{
		*hndl = -1;
		retval = -1;
	}
	else
	{
		sys_handles[i] = f;
		*hndl = i;
		retval = filelength(f);
	}



	return retval;
}

int Sys_FileOpenWrite (char *path)
{
	FILE	*f;
	int	i;

	i = findhandle ();

	f = fopen(path, "wb");
	if (!f)
		Sys_Error ("Error opening %s: %s", path,strerror(errno));
	sys_handles[i] = f;

	return i;
}

void Sys_FileClose (int handle)
{
	fclose (sys_handles[handle]);
	sys_handles[handle] = NULL;
}

void Sys_FileSeek (int handle, int position)
{
	fseek (sys_handles[handle], position, SEEK_SET);
}

int Sys_FileRead (int handle, void *dest, int count)
{
	int		x;
	x = fread (dest, 1, count, sys_handles[handle]);
	return x;
}

int Sys_FileWrite (int handle, void *data, int count)
{
	int		x;
	x = fwrite (data, 1, count, sys_handles[handle]);
	return x;
}

int	Sys_FileTime (char *path)
{
	FILE	*f;
	int		retval;

	f = fopen(path, "rb");

	if (f)
	{
		fclose(f);
		retval = 1;
	}
	else
	{
		retval = -1;
	}

	return retval;
}

qboolean Explorer_OpenFolder_HighlightFile (char *absolutefilename)
{
	char folder_to_open[MAX_OSPATH];
	char file_highlight[MAX_OSPATH];
	char command_line  [1024];
	int i;

	if (Sys_FileTime(absolutefilename) == -1)
	{
		Con_DPrintf ("File \"%s\" does not exist to show\n", absolutefilename);
		Con_Printf ("File does not exist to show\n");
		return false;
	}

	// Copy it
	strlcpy (file_highlight, absolutefilename, sizeof(file_highlight) );

	// Windows format the slashes
	for (i = 0; file_highlight[i]; i++)
		if (file_highlight[i] == '/')
			file_highlight[i] = '\\';

	// Get the path
	strlcpy (folder_to_open, file_highlight, sizeof(folder_to_open) );
	COM_Reduce_To_Parent_Path (folder_to_open);

	SNPrintf (command_line, sizeof(command_line), "/select,%s", file_highlight);

	// Zero is failure, non-zero is success
	Con_DPrintf ("Folder highlight: explorer.exe with \"%s\"\n", command_line);

	return (ShellExecute(0, "Open", "explorer.exe", command_line, NULL, SW_NORMAL) != 0);

}


qboolean Explorer_OpenFolder (char *fullpath)
{
	char folder_to_open[MAX_OSPATH];
	int i;

	// Copy it
	strlcpy (folder_to_open, fullpath, sizeof(folder_to_open) );

	// Windows format the slashes
	for (i = 0; folder_to_open[i]; i++)
		if (folder_to_open[i] == '/')
			folder_to_open[i] = '\\';

	return (ShellExecute(0, "Open", "explorer.exe", folder_to_open, NULL, SW_NORMAL) != 0);
}

void Sys_OpenFolder_f (void)
{
	if (isDedicated)
		return;

	if (modestate != MODE_WINDOWED)
	{
		Con_Printf ("folder command only works in windowed mode\n");
		return;
	}

	if (cls.recent_file[0])
	{
		char tempname[1024];
		SNPrintf (tempname, sizeof(tempname), "%s/%s", com_gamedir, cls.recent_file);
		
		// See if this file exists ...
		if (Sys_FileTime(tempname) != -1)
		{
			// It does
			if (Explorer_OpenFolder_HighlightFile (tempname))
				Con_Printf ("Explorer opening folder and highlighting ...\n");
			else
				Con_Printf ("Opening folder failed\n");

			return;
		}

		// If the file didn't exist, we open the gamedir folder like normal
	}

	if (Explorer_OpenFolder (com_gamedir))
		Con_Printf  ("Explorer opening folder ...\n");
	else
		Con_Printf ("Opening folder failed\n");

	return;
}


#include <direct.h> // Baker: Removes a warning
void Sys_mkdir (char *path) 
{
	_mkdir (path);
}

/*
===============================================================================
SYSTEM IO
===============================================================================
*/






static qboolean		sc_return_on_enter = false;
void Sys_Error (char *error, ...) 
{
	va_list		argptr;
	char		text[1024];
	char		text2[1024];
	char		*text3 = "Press Enter to exit\n";
	char		*text4 = "***********************************\n";
	char		*text5 = "\n";
	DWORD		dummy;
	double		starttime;
	static int	in_sys_error0 = 0;
	static int	in_sys_error1 = 0;
	static int	in_sys_error2 = 0;
	static int	in_sys_error3 = 0;

	if (!in_sys_error3) 
	{
		in_sys_error3 = 1;
	}

	va_start (argptr, error);
	VSNPrintf (text, sizeof(text), error, argptr);
	va_end (argptr);

	if (isDedicated) 
	{
		va_start (argptr, error);
		VSNPrintf (text, sizeof(text), error, argptr);
		va_end (argptr);

		SNPrintf (text2, sizeof(text2), "ERROR: %s\n", text);
		WriteFile (houtput, text5, strlen (text5), &dummy, NULL);
		WriteFile (houtput, text4, strlen (text4), &dummy, NULL);
		WriteFile (houtput, text2, strlen (text2), &dummy, NULL);
		WriteFile (houtput, text3, strlen (text3), &dummy, NULL);
		WriteFile (houtput, text4, strlen (text4), &dummy, NULL);

		starttime = Sys_FloatTime ();
		sc_return_on_enter = true;	// so Enter will get us out of here

		while (!Sys_ConsoleInput () && ((Sys_FloatTime () - starttime) < CONSOLE_ERROR_TIMEOUT))
		{
		}
	}
	else
	{
	// switch to windowed so the message box is visible, unless we already
	// tried that and failed
		if (!in_sys_error0)
		{
			in_sys_error0 = 1;
			VID_SetDefaultMode ();
			MessageBox(NULL, text, TEXT("Quake Error"), MB_OK | MB_SETFOREGROUND | MB_ICONSTOP);
		}
		else
		{
			MessageBox(NULL, text, TEXT("Double Quake Error"), MB_OK | MB_SETFOREGROUND | MB_ICONSTOP);
		}
	}

	if (!in_sys_error1)
	{
		in_sys_error1 = 1;
		Host_Shutdown ();
	}

// shut down QHOST hooks if necessary
	if (!in_sys_error2) 
	{
		in_sys_error2 = 1;
		DeinitConProc ();
	}

	exit (1);
}

void Sys_Printf (char *fmt, ...) 
{
	va_list		argptr;
	char		text[2048];	// JPG - changed this from 1024 to 2048
	DWORD		dummy;

	if (!isDedicated)
		return;

	va_start (argptr,fmt);
	VSNPrintf (text, sizeof(text), fmt, argptr);
	va_end (argptr);

	// JPG 1.05 - translate to plain text
	if (pq_dequake.value)
	{
		unsigned char *ch;
		for (ch = text ; *ch ; ch++)
			*ch = dequake[*ch];
	}

	WriteFile(houtput, text, strlen (text), &dummy, NULL);

	// JPG 3.00 - rcon (64 doesn't mean anything special, but we need some extra space because NET_MAXMESSAGE == RCON_BUFF_SIZE)
	if (rcon_active  && (rcon_message.cursize < rcon_message.maxsize - strlen(text) - 64))
	{
		rcon_message.cursize--;
		MSG_WriteString(&rcon_message, text);
	}
}

extern char *hunk_base; // JPG - needed for Sys_Quit

void Sys_Quit (void)
{

	Host_Shutdown();

	if (tevent)
		CloseHandle (tevent);

	if (isDedicated)
		FreeConsole ();

	// shut down QHOST hooks if necessary
	DeinitConProc ();

	// JPG - added this to see if it would fix the strange running out of system
	// memory after running quake multiple times
	free(hunk_base);

	exit (0);
}

static	double	pfreq;
static qboolean	hwtimer = false;

void Sys_InitFloatTime (void)
{
	__int64	freq;

	if (!COM_CheckParm("-nohwtimer") && QueryPerformanceFrequency ((LARGE_INTEGER *)&freq) && freq > 0) 
	{
		// hardware timer available
		pfreq = (double)freq;
		hwtimer = true;	
	}
	else
	{
		// make sure the timer is high precision, otherwise NT gets 18ms resolution
		timeBeginPeriod (1);
	}
}

double Sys_FloatTime (void) 
{
	__int64		pcount;
	static	__int64	startcount;
	static	DWORD	starttime;
	static qboolean	first = true;
	DWORD	now;

	if (hwtimer) 
	{
		QueryPerformanceCounter ((LARGE_INTEGER *)&pcount);
		if (first) 
		{
			first = false;
			startcount = pcount;
			return 0.0;
		}
		// TODO: check for wrapping
		return (pcount - startcount) / pfreq;
	}

	now = timeGetTime ();

	if (first) 
	{
		first = false;
		starttime = now;
		return 0.0;
	}

	if (now < starttime) // wrapped?
		return (now / 1000.0) + (LONG_MAX - starttime / 1000.0);

	if (now - starttime == 0)
		return 0.0;

	return (now - starttime) / 1000.0;
}

HANDLE				hinput, houtput;
char *Sys_ConsoleInput (void) 
{
	static char	text[256];
	static int		len;
	INPUT_RECORD	recs[1024];
	int		ch;
	DWORD	numread, numevents, dummy;

	if (!isDedicated)
		return NULL;

	for ( ;; )
	{
		if (!GetNumberOfConsoleInputEvents (hinput, &numevents))
			Sys_Error ("Error getting # of console events");

		if (numevents <= 0)
			break;

		if (!ReadConsoleInput(hinput, recs, 1, &numread))
			Sys_Error ("Error reading console input");

		if (numread != 1)
			Sys_Error ("Couldn't read console input");

		if (recs[0].EventType == KEY_EVENT) 
		{
			if (!recs[0].Event.KeyEvent.bKeyDown) 
			{
				ch = recs[0].Event.KeyEvent.uChar.AsciiChar;

				switch (ch) 
				{
					case '\r':
						WriteFile(houtput, "\r\n", 2, &dummy, NULL);

						if (len) 
						{
							text[len] = 0;
							len = 0;
							return text;
						}
						else if (sc_return_on_enter)
						{
						// special case to allow exiting from the error handler on Enter
							text[0] = '\r';
							len = 0;
							return text;
						}

						break;

					case '\b':
						WriteFile(houtput, "\b \b", 3, &dummy, NULL);
						if (len)
						{
							len--;
						}
						break;

					default:
						if (ch >= ' ') 
						{
							WriteFile(houtput, &ch, 1, &dummy, NULL);
							text[len] = ch;
							len = (len + 1) & 0xff;
						}
						break;
				}
			}
		}
	}

	return NULL;
}

void Sys_Sleep (void)
{
	Sleep (1);
}

void Sys_SendKeyEvents (void) 
{
    MSG        msg;

	while (PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE)) 
	{
	// we always update if there are any event, even if we're paused
		scr_skipupdate = 0;

		if (!GetMessage (&msg, NULL, 0, 0))
			Sys_Quit ();

      	TranslateMessage (&msg);
      	DispatchMessage (&msg);
	}
}



/*
==============================================================================
 WINDOWS CRAP
==============================================================================
*/


void SleepUntilInput (int time)
{

	MsgWaitForMultipleObjects(1, &tevent, FALSE, time, QS_ALLINPUT);
}



/********************************* CLIPBOARD *********************************/

#define	SYS_CLIPBOARD_SIZE		256

char *Sys_GetClipboardData (void) 
{
	HANDLE		th;
	char		*clipText, *s, *t;
	static	char	clipboard[SYS_CLIPBOARD_SIZE];

	if (!OpenClipboard(NULL))
		return NULL;

	if (!(th = GetClipboardData(CF_TEXT))) 
	{
		CloseClipboard ();
		return NULL;
	}

	if (!(clipText = GlobalLock(th))) 
	{
		CloseClipboard ();
		return NULL;
	}

	s = clipText;
	t = clipboard;
	while (*s && t - clipboard < SYS_CLIPBOARD_SIZE - 1 && *s != '\n' && *s != '\r' && *s != '\b')
		*t++ = *s++;
	*t = 0;

	GlobalUnlock (th);
	CloseClipboard ();

	return clipboard;
}

// copies given text to clipboard
void Sys_CopyToClipboard(char *text) 
{
	char *clipText;
	HGLOBAL hglbCopy;

	if (!OpenClipboard(NULL))
		return;

	if (!EmptyClipboard()) 
	{
		CloseClipboard();
		return;
	}

	if (!(hglbCopy = GlobalAlloc(GMEM_DDESHARE, strlen(text) + 1))) 
	{
		CloseClipboard();
		return;
	}

	if (!(clipText = GlobalLock(hglbCopy))) 
	{
		CloseClipboard();
		return;
	}

	strcpy((char *) clipText, text);
	GlobalUnlock(hglbCopy);
	SetClipboardData(CF_TEXT, hglbCopy);

	CloseClipboard();
}

void Sys_SetWindowCaption (char *newcaption)
{
	if (!mainwindow)
		return;

	if (!newcaption)
		SetWindowText (mainwindow, va("%s %s %s",ENGINE_NAME, RENDERER_NAME, ENGINE_VERSION));
	else
		SetWindowText (mainwindow, newcaption);
}


/*
==================
WinMain
==================
*/
HINSTANCE	global_hInstance;
int			global_nCmdShow;
char		*argv[MAX_NUM_ARGVS];
static char	*empty_string = "";
HWND		hwnd_dialog;

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) 
{
	quakeparms_t	parms;
	double			time, oldtime, newtime;
	MEMORYSTATUS	lpBuffer;
	static	char	cwd[1024];
	int				t, i;
//	RECT			rect;
	char			*ch;	// JPG 3.00 - for eliminating quotes from exe name
	char			*e;
	FILE			*fpak0;
	char			fpaktest[1024], exeline[MAX_OSPATH];
    /* previous instances do not exist in Win32 */
    if (hPrevInstance)
        return 0;

#ifdef SUPPORTS_SERVER_BROWSER // Baker change +
	// Check lpCmdline to see if we are to run as server browser 
	// download process utility.  If so, we download server browser data
	// write it to file and get out.
	if (strstr (lpCmdLine, "-server_update") )
	{
		int			html_read_numbytes;
		const char*	html_read;
		int			exitcode = 0;
		Server_Browser_Util_Get_Page_Alloc (&html_read, SERVER_BROWSER_URL, &html_read_numbytes);

		i = GetModuleFileName(NULL, com_basedir, sizeof(com_basedir) - 1);
		if(!i)
			Sys_Error("FS_InitFilesystemEx: GetModuleFileName failed");

		com_basedir[i] = 0; // ensure null terminator
		for (e = com_basedir + strlen(com_basedir) - 1; e >= com_basedir; e--)
			if (*e == '/' || *e == '\\')
			{
				*e = 0;
				break;
			}

		exitcode = Server_Browser_Util_Write_Servers (html_read, html_read_numbytes, SERVER_LIST_NAME);
		exit (exitcode);
	}
#endif // Baker change + SUPPORTS_SERVER_BROWSER


	global_hInstance = hInstance;
	global_nCmdShow = nCmdShow;

	lpBuffer.dwLength = sizeof(MEMORYSTATUS);
	GlobalMemoryStatus (&lpBuffer);

	if (!GetCurrentDirectory (sizeof(cwd), cwd))
		Sys_Error ("Couldn't determine current directory");

	if (cwd[strlen(cwd)-1] == '/')
		cwd[strlen(cwd)-1] = 0;

	// Baker 3.76 - playing demos via file association

	SNPrintf (fpaktest, sizeof(fpaktest), "%s/id1/pak0.pak", cwd); // Baker 3.76 - Sure this isn't gfx.wad, but let's be realistic here

	if(!(i = GetModuleFileName(NULL, com_basedir, sizeof(com_basedir)-1)))
		Sys_Error("FS_InitFilesystemEx: GetModuleFileName failed");

	com_basedir[i] = 0; // ensure null terminator

	SNPrintf(exeline, sizeof(exeline), "%s \"%%1\"", com_basedir);
#if 0
	if (COM_CheckParm ("-noassocdem") == 0) 
	{
		void CreateSetKeyExtension(void);
		void CreateSetKeyDescription(void);
		void CreateSetKeyCommandLine(const char*  exeline);


		// Build registry entries
		CreateSetKeyExtension();
		CreateSetKeyDescription();
		CreateSetKeyCommandLine(exeline);
		Con_Printf("Registry Init\n");
		// End build entries
	}
#endif
	// Strip to the bare path; needed for demos started outside Quake folder
	for (e = com_basedir+strlen(com_basedir)-1; e >= com_basedir; e--)
			if (*e == '/' || *e == '\\')
			{
				*e = 0;
				break;
			}

	SNPrintf (cwd, sizeof(cwd), "%s", com_basedir);

	if (fpak0 = fopen(fpaktest, "rb"))  
	{
		fclose (fpak0); // Pak0 found so close it; we have a valid directory
	} 
	else 
	{
		// Failed to find pak0.pak, use the dir the exe is in
		SNPrintf (cwd, sizeof(cwd), "%s", com_basedir);
	}
	// End Baker 3.76

	parms.basedir = cwd;
	parms.cachedir = NULL;

	parms.argc = 1;
	argv[0] = GetCommandLine();	// JPG 3.00 - was empty_string
	lpCmdLine[-1] = 0;			// JPG 3.00 - isolate the exe name, eliminate quotes
	if (argv[0][0] == '\"')
		argv[0]++;
	if (ch = strchr(argv[0], '\"'))
		*ch = 0;


	while (*lpCmdLine && (parms.argc < MAX_NUM_ARGVS)) 
	{
		while (*lpCmdLine && ((*lpCmdLine <= 32) || (*lpCmdLine > 126)))
			lpCmdLine++;

		if (*lpCmdLine) 
		{
			if (*lpCmdLine == '\"')
			{
				lpCmdLine++;

				argv[parms.argc] = lpCmdLine;
				parms.argc++;

				while (*lpCmdLine && *lpCmdLine != '\"') // this include chars less that 32 and greate than 126... is that evil?
					lpCmdLine++;
			}
			else
			{
				argv[parms.argc] = lpCmdLine;
				parms.argc++;

				while (*lpCmdLine && ((*lpCmdLine > 32) && (*lpCmdLine <= 126)))
					lpCmdLine++;
			}

			if (*lpCmdLine) 
			{
				*lpCmdLine = 0;
				lpCmdLine++;
			}

		}
	}

	parms.argv = argv;

	COM_InitArgv (parms.argc, parms.argv);

	parms.argc = com_argc;
	parms.argv = com_argv;

	isDedicated = (COM_CheckParm ("-dedicated"));


// take the greater of all the available memory or half the total memory,
// but at least 8 Mb and no more than 16 Mb, unless they explicitly
// request otherwise
	parms.memsize = lpBuffer.dwAvailPhys;

	if (parms.memsize < MINIMUM_WIN_MEMORY)
		parms.memsize = MINIMUM_WIN_MEMORY;

	if (parms.memsize < (lpBuffer.dwTotalPhys >> 1))
		parms.memsize = lpBuffer.dwTotalPhys >> 1;

	if (parms.memsize > MAXIMUM_WIN_MEMORY)
		parms.memsize = MAXIMUM_WIN_MEMORY;

	if ((t = COM_CheckParm("-heapsize")) != 0 && t + 1 < com_argc)
		parms.memsize = atoi (com_argv[t+1]) * 1024;

	if ((t = COM_CheckParm("-mem")) != 0 && t + 1 < com_argc)
		parms.memsize = atoi (com_argv[t+1]) * 1024 * 1024;

	parms.membase = Q_malloc (parms.memsize);

	// Baker 3.99n: JoeQuake doesn't do this next one
	Sys_PageIn (parms.membase, parms.memsize);

	tevent = CreateEvent(NULL, FALSE, FALSE, NULL);

	if (!tevent)
		Sys_Error ("Couldn't create event");

	if (isDedicated)
	{
		if (!AllocConsole ())
		{
			Sys_Error ("Couldn't create dedicated server console");
		}

		hinput = GetStdHandle (STD_INPUT_HANDLE);
		houtput = GetStdHandle (STD_OUTPUT_HANDLE);

	// give QHOST a chance to hook into the console
		if ((t = COM_CheckParm ("-HFILE")) > 0)
		{
			if (t < com_argc)
				hFile = (HANDLE)atoi (com_argv[t+1]);
		}

		if ((t = COM_CheckParm ("-HPARENT")) > 0)
		{
			if (t < com_argc)
				heventParent = (HANDLE)atoi (com_argv[t+1]);
		}

		if ((t = COM_CheckParm ("-HCHILD")) > 0)
		{
			if (t < com_argc)
				heventChild = (HANDLE)atoi (com_argv[t+1]);
		}

		InitConProc (hFile, heventParent, heventChild);
	}

	Sys_Init ();

// because sound is off until we become active
	S_BlockSound ();

	Sys_Printf ("Host_Init\n");
	Host_Init (&parms);

	oldtime = Sys_FloatTime ();

    /* main window message loop */
	while (1) 
	{
		if (isDedicated) 
		{
			newtime = Sys_FloatTime ();
			time = newtime - oldtime;

			while (time < sys_ticrate.value )
			{
				Sys_Sleep();
				newtime = Sys_FloatTime ();
				time = newtime - oldtime;
			}
		}
		else
		{
		// yield the CPU for a little while when paused, minimized, or not the focus
			if (cl.paused && (!ActiveApp || Minimized))
			{
				SleepUntilInput (PAUSE_SLEEP);
				scr_skipupdate = 1;		// no point in bothering to draw
			} 
			else if (!ActiveApp) 
			{
				SleepUntilInput (NOT_FOCUS_SLEEP);
			}

			newtime = Sys_FloatTime ();
			time = newtime - oldtime;
		}

		Host_Frame (time);
		oldtime = newtime;

	}

    // return success of application
    return TRUE;
}

#ifdef SUPPORTS_SERVER_BROWSER // Baker change +
/*
==================
Sys_Process_Run

Launches process with cmdline passed
==================
*/

HANDLE Sys_Process_Run (char *cmdline, const char *description) 
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	memset (&si, 0, sizeof(si));
	si.cb = sizeof(si);
	si.wShowWindow = SW_SHOWMINNOACTIVE;
	si.dwFlags = STARTF_USESHOWWINDOW;

	Con_DPrintf("Sys_Process_Run: command line: \"%s\"\n", cmdline);
	if (!CreateProcess(NULL, cmdline, NULL, NULL, FALSE, GetPriorityClass(GetCurrentProcess()), NULL, com_basedir, &si, &pi)) 
	{
		Con_Printf ("Couldn't execute %s process\n", description);
		return NULL;
	}

	// Return the handle
	return pi.hProcess;
}

int Sys_Process_IsStillRunning (HANDLE processhandle) 
{
	DWORD	ExitCode;

	if (!processhandle) 
	{
		Con_Printf ("WARNING: NULL process handle\n");
		return -1; // Error
	}

	if (!GetExitCodeProcess(processhandle, &ExitCode)) 
	{
		Con_Printf ("WARNING: GetExitCodeProcess failed\n");
		return -1; // Error
	}

	if (ExitCode == STILL_ACTIVE)
		return 1; // Still running

	return 0; // No longer running; completed
}
#endif // Baker change + SUPPORTS_SERVER_BROWSER

void Sys_OpenQuakeFolder_f(void)
{
	HINSTANCE			ret;

	ret = ShellExecute(0, "Open", com_basedir, NULL, NULL, SW_NORMAL);

	if (ret==0)
		Con_Printf("Opening Quake folder failed\n");
	else
		Con_Printf("Quake folder opened in Explorer\n");

}

void Sys_HomePage_f(void)
{
	HINSTANCE			ret;
/*
	qboolean			switch_to_windowed = false;

	if ((switch_to_windowed = VID_CanSwitchedToWindowed()))
		VID_Windowed();
*/
//	char	outstring[CON_TEXTSIZE]="";

//	SNPrintf(outstring, size(outstring), "%s", ENGINE_HOMEPAGE_URL);

	ret = ShellExecute(0, NULL, ENGINE_HOMEPAGE_URL, NULL, NULL, SW_NORMAL);

	if (ret==0)
		Con_Printf("Opening home page failed\n");
	else
		Con_Printf("%s home page opened in default browser\n", ENGINE_NAME);

}

char q_system_string[1024] = "";

void Sys_InfoPrint_f(void) 
{
	Con_Printf ("%s\n", q_system_string);
}

void Sys_Sleep_f (void) 
{
	if (Cmd_Argc() == 1) 
	{
		Con_Printf ("Usage: %s <milliseconds> : let system sleep and yield cpu\n", Cmd_Argv(0));
		return;
	}

	Con_Printf ("Sleeping %i milliseconds ...\n", atoi(Cmd_Argv(1)));
	Sleep (atoi(Cmd_Argv(1)));
}


char * SYSINFO_GetString(void)
{
	return q_system_string;
}
int     SYSINFO_memory = 0;
int     SYSINFO_MHz = 0;
char *  SYSINFO_processor_description = NULL;
char *  SYSINFO_3D_description        = NULL;

qboolean	WinNT, Win2K, WinXP, Win2K3, WinVISTA;
char WinVers[30];

void Sys_InfoInit(void)
{
	MEMORYSTATUS    memstat;
	LONG            ret;
	HKEY            hKey;
	OSVERSIONINFO	vinfo;
	vinfo.dwOSVersionInfoSize = sizeof(vinfo);

	if (!GetVersionEx(&vinfo))
		Sys_Error ("Couldn't get OS info");

	if ((vinfo.dwMajorVersion < 4) || (vinfo.dwPlatformId == VER_PLATFORM_WIN32s))
		Sys_Error ("Qrack requires at least Win95 or greater.");

	WinNT = (vinfo.dwPlatformId == VER_PLATFORM_WIN32_NT) ? true : false;

	if ((Win2K = WinNT && (vinfo.dwMajorVersion == 5) && (vinfo.dwMinorVersion == 0)))
		SNPrintf(WinVers, sizeof(WinVers),"Windows 2000");
	else if ((WinXP = WinNT && (vinfo.dwMajorVersion == 5) && (vinfo.dwMinorVersion == 1)))
		SNPrintf(WinVers, sizeof(WinVers),"Windows XP");
	else if ((Win2K3 = WinNT && (vinfo.dwMajorVersion == 5) && (vinfo.dwMinorVersion == 2)))
		SNPrintf(WinVers, sizeof(WinVers),"Windows 2003");
	else if ((WinVISTA = WinNT && (vinfo.dwMajorVersion == 6) && (vinfo.dwMinorVersion == 0)))
		SNPrintf(WinVers, sizeof(WinVers),"Windows Vista");

	else if (WinNT && vinfo.dwMajorVersion == 6 && vinfo.dwMinorVersion == 1)
		SNPrintf(WinVers, sizeof(WinVers),"Windows 7");
	else if (WinNT && vinfo.dwMajorVersion >= 6 )
		SNPrintf(WinVers, sizeof(WinVers),"Windows 7 or later");
	else
		SNPrintf(WinVers, sizeof(WinVers),"Windows 95/98/ME");

	Con_Printf("Operating System: %s\n", WinVers);

	GlobalMemoryStatus(&memstat);
	SYSINFO_memory = memstat.dwTotalPhys;

	ret = RegOpenKey(
	          HKEY_LOCAL_MACHINE,
	          "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
	          &hKey);

	if (ret == ERROR_SUCCESS) 
	{
		DWORD type;
		byte  data[1024];
		DWORD datasize;

		datasize = 1024;
		ret = RegQueryValueEx(
		          hKey,
		          "~MHz",
		          NULL,
		          &type,
		          data,
		          &datasize);

		if (ret == ERROR_SUCCESS  &&  datasize > 0  &&  type == REG_DWORD)
			SYSINFO_MHz = *((DWORD *)data);

		datasize = 1024;
		ret = RegQueryValueEx (
		          hKey,
		          "ProcessorNameString",
		          NULL,
		          &type,
		          data,
		          &datasize);

		if (ret == ERROR_SUCCESS  &&  datasize > 0  &&  type == REG_SZ)
			SYSINFO_processor_description = Q_strdup((char *) data);

		RegCloseKey(hKey);
	}


	{
		extern const char *gl_renderer;

		if (gl_renderer  &&  gl_renderer[0])
			SYSINFO_3D_description = Q_strdup(gl_renderer);
	}


	SNPrintf (q_system_string, sizeof(q_system_string), "%dMB", (int)(SYSINFO_memory / 1024. / 1024. + .5));

	if (SYSINFO_processor_description) 
	{
		char	myprocessor[256];
		SNPrintf(myprocessor, 256, (const char*)strltrim(SYSINFO_processor_description));
		strlcat (q_system_string, ", ", sizeof(q_system_string));
		strlcat (q_system_string, myprocessor, sizeof(q_system_string));
	}
	
	if (SYSINFO_MHz) 
	{
		strlcat (q_system_string, va(" %dMHz", SYSINFO_MHz), sizeof(q_system_string));
	}
	if (SYSINFO_3D_description) 
	{
		strlcat (q_system_string, ", ", sizeof(q_system_string));
		strlcat (q_system_string, SYSINFO_3D_description, sizeof(q_system_string));
	}

	//Con_Printf("sys: %s\n", q_system_string);
	Cmd_AddCommand ("homepage", Sys_HomePage_f);
	Cmd_AddCommand ("openquakefolder", Sys_OpenQuakeFolder_f);
	Cmd_AddCommand ("sysinfo", Sys_InfoPrint_f);
	Cmd_AddCommand ("sleep", Sys_Sleep_f);

	Cvar_RegisterVariable(&sys_highpriority, OnChange_sys_highpriority);
}

void Sys_Init (void) 
{
	OSVERSIONINFO	vinfo;

	Sys_InitFloatTime ();

	vinfo.dwOSVersionInfoSize = sizeof(vinfo);

	if (!GetVersionEx (&vinfo))
		Sys_Error ("Couldn't get OS info");

	if ((vinfo.dwMajorVersion < 4) || (vinfo.dwPlatformId == VER_PLATFORM_WIN32s))
		Sys_Error ("Quake requires at least Win95 or NT 4.0");

	WinNT = (vinfo.dwPlatformId == VER_PLATFORM_WIN32_NT) ? true:false;
}

int Sys_GetHardDriveSerial (const char* mydir)
{
	DWORD	dwSerial, dwMFL, dwSysFlags;
	UCHAR	szFileSys[255], szVolNameBuff[255];
	BOOL	bSuccess;
	char drivebuf[30] = {0};
	int i;
	
	// Copy until hits end of path
	for (i = 0; i < (sizeof(drivebuf) - 2) && mydir[i]; i++)
	{
		drivebuf[i] = mydir[i];
		if (mydir[i] == '\\')
			break;
	}
	
	if (bSuccess = GetVolumeInformation((LPTSTR)drivebuf,(LPTSTR)szVolNameBuff, 255, &dwSerial, &dwMFL, &dwSysFlags, (LPTSTR)szFileSys, 255) )
	{
		return (int)dwSerial;
	}
	else return 0;
}


