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
// console.c

#ifndef _MSC_VER
#include <unistd.h>
#endif



#include <fcntl.h>
#include <errno.h>
#include "quakedef.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include <time.h> // JPG - needed for console log

int 		con_linewidth;

float		con_cursorspeed = 4;

// JPG - upped CON_TEXTSIZE from 16384 to 65536
#define		CON_TEXTSIZE	65536

qboolean 	con_forcedup;		// because no entities to refresh

int			con_totallines;		// total lines in console scrollback
int			con_backscroll;		// lines up from bottom to display
int			con_current;		// where next message will be printed
int			con_x;				// offset in current line for next print
char		*con_text=0;

cvar_t		con_notifytime = {"con_notifytime","3"};		//seconds
cvar_t		_con_notifylines = {"con_notifylines","4"};
cvar_t		pq_confilter = {"pq_confilter", "0"};	// JPG 1.05 - filter out the "you got" messages
cvar_t		pq_timestamp = {"pq_timestamp", "0"};	// JPG 1.05 - timestamp player binds during a match
cvar_t		pq_removecr = {"pq_removecr", "1"};		// JPG 3.20 - remove \r from console output
cvar_t		con_logcenterprint = {"con_logcenterprint", "1"}; //johnfitz
char		con_lastcenterstring[1024]; //johnfitz
cvar_t		con_nocenterprint	= {"con_nocenterprint", "0"}; // Rook

#define	NUM_CON_TIMES 4
float		con_times[NUM_CON_TIMES];	// realtime time the line was generated
								// for transparent notify lines

int			con_vislines;
int			con_notifylines;		// scan lines to clear for notify lines

qboolean	con_debuglog;

qboolean	cl_inconsole = false;//R00k

extern	char	key_lines[32][MAXCMDLINE];
extern	int		edit_line;
extern	int		key_linepos;

qboolean	con_initialized = false;



extern void M_Menu_Main_f (void);

char logfilename[128];	// JPG - support for different filenames
/*
================
Con_Quakebar -- johnfitz -- returns a bar of the desired length, but never wider than the console

includes a newline, unless len >= con_linewidth.
================
*/
char *Con_Quakebar (int len)
{
	static char bar[42];
	int i;

	len = min(len, sizeof(bar) - 2);
	len = min(len, con_linewidth);

	bar[0] = '\35';
	for (i = 1; i < len - 1; i++)
		bar[i] = '\36';
	bar[len-1] = '\37';

	if (len < con_linewidth)
	{
		bar[len] = '\n';
		bar[len+1] = 0;
	}
	else
		bar[len] = 0;

	return bar;
}

/*
================
Con_ToggleConsole_f
================
*/
void Con_ToggleConsole_f (void)
{
	extern int history_line; //johnfitz

	if (key_dest == key_console)
	{
		if (cls.state == ca_connected)
		{
			key_dest = key_game;
			cl_inconsole = false;//R00k
			key_lines[edit_line][1] = 0;	// clear any typing
			key_linepos = 1;
			history_line = edit_line; //johnfitz -- it should also return you to the bottom of the command history
		}
		else
		{
			M_Menu_Main_f ();
			cl_inconsole = false;
		}
	}
	else
	{
		key_dest = key_console;
		cl_inconsole = true;
		con_backscroll = 0; // JPG - don't want to enter console with backscroll
	}

	SCR_EndLoadingPlaque ();
	memset (con_times, 0, sizeof(con_times));
}

/*
================
Con_Clear_f
================
*/
void Con_Clear_f (void)
{
	if (con_text)
		memset (con_text, ' ', CON_TEXTSIZE);
	con_backscroll = 0; //johnfitz -- if console is empty, being scrolled up is confusing
	history_line = edit_line; //johnfitz -- it should also return you to the bottom of the command history
}

/*
====================
Con_Showtime_f // Baker 3.60 - "time" command to display current date and time in console

time
====================
*/
void Con_Showtime_f (void)
{
	time_t	ltime;
	char	str[80];

	time (&ltime);
	strftime (str, sizeof(str)-1, "%B %d, %Y - %I:%M:%S %p", localtime(&ltime));

	Con_Printf ("%s \n", str);


//	Sys_CopyToClipboard(va("Operating System: %s\r\nThis Thing: %\r\n", cmdline.string));


}


/*
================
Con_Dump_f -- johnfitz -- adapted from quake2 source
================
*/
void Con_Dump_f (void)
{
	int		l, x;
	char	*line;
	FILE	*f;
	char	buffer[1024];
	char	name[MAX_OSPATH];

#if 1
	//johnfitz -- there is a security risk in writing files with an arbitrary filename. so,
	//until stuffcmd is crippled to alleviate this risk, just force the default filename.
	SNPrintf(name, sizeof(name), "%s/condump.txt", com_gamedir);
#else
	if (Cmd_Argc() > 2)
	{
		Con_Printf ("usage: condump <filename>\n");
		return;
	}

	if (Cmd_Argc() > 1)
	{
		if (strstr(Cmd_Argv(1), ".."))
		{
			Con_Printf ("Relative pathnames are not allowed.\n");
			return;
		}
		SNPrintf(name, sizeof(name), "%s/%s", com_gamedir, Cmd_Argv(1));
		COM_DefaultExtension (name, ".txt");
	}
	else
		SNPrintf(name, sizeof(name), "%s/condump.txt", com_gamedir);
#endif

	COM_CreatePath (name);
	f = fopen (name, "w");
	if (!f)
	{
		Con_Printf ("ERROR: couldn't open file.\n", name);
		return;
	}

	// skip initial empty lines
	for (l = con_current - con_totallines + 1 ; l <= con_current ; l++)
	{
		line = con_text + (l%con_totallines)*con_linewidth;
		for (x=0 ; x<con_linewidth ; x++)
			if (line[x] != ' ')
				break;
		if (x != con_linewidth)
			break;
	}

	// write the remaining lines
	buffer[con_linewidth] = 0;
	for ( ; l <= con_current ; l++)
	{
		line = con_text + (l%con_totallines)*con_linewidth;
		strncpy (buffer, line, con_linewidth);
		for (x=con_linewidth-1 ; x>=0 ; x--)
		{
			if (buffer[x] == ' ')
				buffer[x] = 0;
			else
				break;
		}
		for (x=0; buffer[x]; x++)
			buffer[x] &= 0x7f;

		fprintf (f, "%s\n", buffer);
	}

	fclose (f);
	strlcpy (cls.recent_file, name, sizeof(cls.recent_file) );
	Con_Printf ("Dumped console text to %s.\n", name);
}


/*
================
Con_Copy_f -- Baker -- adapted from Con_Dump
================
*/
void Con_Copy_f (void)
{
	char	outstring[CON_TEXTSIZE]="";
	int		l, x;
	char	*line;
	char	buffer[1024];

	// skip initial empty lines
	for (l = con_current - con_totallines + 1 ; l <= con_current ; l++)
	{
		line = con_text + (l%con_totallines)*con_linewidth;
		for (x=0 ; x<con_linewidth ; x++)
			if (line[x] != ' ')
				break;
		if (x != con_linewidth)
			break;
	}

	// write the remaining lines
	buffer[con_linewidth] = 0;
	for ( ; l <= con_current ; l++)
	{
		line = con_text + (l%con_totallines)*con_linewidth;
		strncpy (buffer, line, con_linewidth);
		for (x=con_linewidth-1 ; x>=0 ; x--)
		{
			if (buffer[x] == ' ')
				buffer[x] = 0;
			else
				break;
		}
		for (x=0; buffer[x]; x++)
			buffer[x] &= 0x7f;

		strlcat (outstring, va("%s\r\n", buffer), sizeof(outstring));

	}

	Sys_CopyToClipboard(outstring);
	Con_Printf ("Copied console to clipboard\n");
}


/*
================
Con_ClearNotify
================
*/
void Con_ClearNotify (void)
{
	int		i;

	for (i=0 ; i<NUM_CON_TIMES ; i++)
		con_times[i] = 0;
}


/*
================
Con_MessageMode_f
================
*/
extern qboolean team_message;

void Con_MessageMode_f (void)
{
	key_dest = key_message;
	team_message = false;
}


/*
================
Con_MessageMode2_f
================
*/
void Con_MessageMode2_f (void)
{
	key_dest = key_message;
	team_message = true;
}


/*
================
Con_CheckResize

If the line width has changed, reformat the buffer.
================
*/
void Con_CheckResize (void)
{
	int		i, j, width, oldwidth, oldtotallines, numlines, numchars;
	char	tbuf[CON_TEXTSIZE];

	width = (vid.width >> 3) - 2;

	if (width == con_linewidth)
		return;

	if (width < 1)			// video hasn't been initialized yet
	{
		width = 78;
		con_linewidth = width;
		con_totallines = CON_TEXTSIZE / con_linewidth;
		memset (con_text, ' ', CON_TEXTSIZE);
	}
	else
	{
		oldwidth = con_linewidth;
		con_linewidth = width;
		oldtotallines = con_totallines;
		con_totallines = CON_TEXTSIZE / con_linewidth;
		numlines = oldtotallines;

		if (con_totallines < numlines)
			numlines = con_totallines;

		numchars = oldwidth;

		if (con_linewidth < numchars)
			numchars = con_linewidth;

		memcpy (tbuf, con_text, CON_TEXTSIZE);
		memset (con_text, ' ', CON_TEXTSIZE);

		for (i=0 ; i<numlines ; i++)
		{
			for (j=0 ; j<numchars ; j++)
			{
				con_text[(con_totallines - 1 - i) * con_linewidth + j] = tbuf[((con_current - i + oldtotallines) % oldtotallines) * oldwidth + j];
			}
		}

		Con_ClearNotify ();
	}

	con_backscroll = 0;
	con_current = con_totallines - 1;
}

#include <io.h> //Baker: Removes a warning ... write, open, close

/*
================
Con_Init
================
*/
void Con_Init (void)
{
#define MAXGAMEDIRLEN	1000
	char	temp[MAXGAMEDIRLEN+1];
	// char	*t2 = "/qconsole.log"; // JPG - don't need this
	char	*ch;	// JPG - added this
	int		fd, n;	// JPG - added these
    time_t  ltime;		// JPG - for console log file

	con_debuglog = COM_CheckParm("-condebug");

	if (con_debuglog)
	{
		// JPG - check for different file name
		if ((con_debuglog < com_argc - 1) && (*com_argv[con_debuglog+1] != '-') && (*com_argv[con_debuglog+1] != '+'))
		{
			if ((ch = strchr(com_argv[con_debuglog+1], '%')) && (ch[1] == 'd'))
			{
				n = 0;
				do
				{
					n = n + 1;
					SNPrintf (logfilename, sizeof(logfilename), com_argv[con_debuglog+1], n);
					strlcat (logfilename, ".log", sizeof(logfilename));
					SNPrintf (temp, sizeof(temp), "%s/%s", com_gamedir, logfilename);
					fd = open(temp, O_CREAT | O_EXCL | O_WRONLY, 0666);
				}
				while (fd == -1);
				close(fd);
			}
			else
				SNPrintf (logfilename, sizeof(logfilename), "%s.log", com_argv[con_debuglog+1]);
		}
		else
			strcpy(logfilename, "qconsole.log");

		// JPG - changed t2 to logfilename
		if (strlen (com_gamedir) < (MAXGAMEDIRLEN - strlen (logfilename)))
		{
			SNPrintf(temp, sizeof(temp), "%s/%s", com_gamedir, logfilename); // JPG - added the '/'
			unlink (temp);
		}

		// JPG - print initial message
		Con_Printf("Log file initialized.\n");
		Con_Printf("%s/%s\n", com_gamedir, logfilename);
		time( &ltime );
		Con_Printf( "%s\n", ctime( &ltime ) );
	}

	con_text = Hunk_AllocName (CON_TEXTSIZE, "context");
	memset (con_text, ' ', CON_TEXTSIZE);
	con_linewidth = -1;
	Con_CheckResize ();

//
// register our commands
//
	Cvar_RegisterVariable (&con_notifytime, NULL);
	Cvar_RegisterVariable (&_con_notifylines, NULL);

	Cvar_RegisterVariable (&con_logcenterprint, NULL);
	Cvar_RegisterVariable (&con_nocenterprint, NULL); // Rook

	Cvar_RegisterVariable (&pq_confilter, NULL);	// JPG 1.05 - make "you got" messages temporary
	Cvar_RegisterVariable (&pq_timestamp, NULL);	// JPG 1.05 - timestamp player binds during a match
	Cvar_RegisterVariable (&pq_removecr, NULL);	// JPG 3.20 - timestamp player binds during a match

	Cmd_AddCommand ("toggleconsole", Con_ToggleConsole_f);
	Cmd_AddCommand ("messagemode", Con_MessageMode_f);
	Cmd_AddCommand ("messagemode2", Con_MessageMode2_f);

	Cmd_AddCommand ("clear", Con_Clear_f);
	Cmd_AddCommand ("condump", Con_Dump_f); //johnfitz
	Cmd_AddCommand ("time", Con_Showtime_f); // Baker 3.60 - "time command

	Cmd_AddCommand ("copy", Con_Copy_f); // Baker 399.m - copy console to clipboard

	con_initialized = true;
	Con_Printf ("Console initialized\n");
}


/*
===============
Con_Linefeed
===============
*/
void Con_Linefeed (void)
{
	con_x = 0;
	con_current++;
	memset (&con_text[(con_current%con_totallines)*con_linewidth], ' ', con_linewidth);

	// JPG - fix backscroll
	if (con_backscroll)
		con_backscroll++;
}

#define DIGIT(x) ((x) >= '0' && (x) <= '9')

/*
================
Con_Print

Handles cursor positioning, line wrapping, etc
All console printing must go through this in order to be logged to disk
If no console is visible, the notify window will pop up.
================
*/
void Con_Print (char *txt)
{
	int		y;
	int		c, l;
	static int	cr;
	int		mask;

	static int fixline = 0;

	//con_backscroll = 0;  // JPG - half of a fix for an annoying problem

	// JPG 1.05 - make the "You got" messages temporary
	if (pq_confilter.value)
		fixline |= !strcmp(txt, "You got armor\n") ||
				   !strcmp(txt, "You receive ") ||
				   !strcmp(txt, "You got the ") ||
				   !strcmp(txt, "no weapon.\n") ||
				   !strcmp(txt, "not enough ammo.\n");

	if (txt[0] == 1)
	{
		mask = 128;		// go to colored text
		S_LocalSound ("misc/talk.wav");
	// play talk wav
		txt++;

		// JPG 1.05 - timestamp player binds during a match (unless the bind already has a time in it)
		if (!cls.demoplayback && (!cls.netcon || cls.netcon->mod != MOD_PROQUAKE || *txt == '(') &&		// JPG 3.30 - fixed old bug hit by some servers
			!con_x && pq_timestamp.value && (cl.minutes || cl.seconds) && cl.seconds < 128)
		{
			int minutes, seconds, match_time, msg_time;
			char buff[16];
			char *ch;

			if (cl.match_pause_time)
				match_time = ceil(60.0 * cl.minutes + cl.seconds - (cl.match_pause_time - cl.last_match_time));
			else
				match_time = ceil(60.0 * cl.minutes + cl.seconds - (cl.time - cl.last_match_time));
			minutes = match_time / 60;
			seconds = match_time - 60 * minutes;

			msg_time = -10;
			ch = txt + 2;
			if (ch[0] && ch[1] && ch[2])
			{
				while (ch[2])
				{
					if (ch[0] == ':' && DIGIT(ch[1]) && DIGIT(ch[2]) && DIGIT(ch[-1]))
					{
						msg_time = 60 * (ch[-1] - '0') + 10 * (ch[1] - '0') + (ch[2] - '0');
						if (DIGIT(ch[-2]))
							msg_time += 600 * (ch[-2] - '0');
						break;
					}
					ch++;
				}
			}
			if (msg_time < match_time - 2 || msg_time > match_time + 2)
			{
				if (pq_timestamp.value == 1)
					SNPrintf (buff, sizeof(buff), "%d%c%02d", minutes, 'X' + 128, seconds);
				else
					SNPrintf (buff, sizeof(buff), "%02d", seconds);

				if (cr)
				{
					con_current--;
					cr = false;
				}
				Con_Linefeed ();
				if (con_current >= 0)
					con_times[con_current % NUM_CON_TIMES] = realtime;

				y = con_current % con_totallines;
				for (ch = buff ; *ch ; ch++)
					con_text[y*con_linewidth+con_x++] = *ch - 30;
				con_text[y*con_linewidth+con_x++] = ' ';
			}
		}
	}
	else if (txt[0] == 2)
	{
		mask = 128;		// go to colored text
		txt++;
	}
	else
		mask = 0;


	while ( (c = *txt) )
	{
	// count word length
		for (l=0 ; l< con_linewidth ; l++)
			if ( txt[l] <= ' ')
				break;

	// word wrap
		if (l != con_linewidth && (con_x + l > con_linewidth) )
			con_x = 0;

		txt++;

		if (cr)
		{
			con_current--;
			cr = false;
		}

		if (!con_x)
		{
			Con_Linefeed ();
		// mark time for transparent overlay
			if (con_current >= 0)
				con_times[con_current % NUM_CON_TIMES] = realtime;
		}

		switch (c)
		{
		case '\n':
			con_x = 0;
			cr = fixline;	// JPG 1.05 - make the "you got" messages temporary
			fixline = 0;	// JPG
			break;

		case '\r':
			if (pq_removecr.value)	// JPG 3.20 - optionally remove '\r'
				c += 128;
			else
			{
				con_x = 0;
				cr = 1;
				break;
			}

		default:	// display character and advance
			y = con_current % con_totallines;
			con_text[y*con_linewidth+con_x] = c | mask;
			con_x++;
			if (con_x >= con_linewidth)
				con_x = 0;
			break;
		}

	}
}


// JPG - increased this from 4096 to 16384 and moved it up here
// See http://www.inside3d.com/qip/q1/bugs.htm, NVidia 5.16 drivers can cause crash
#define	MAXPRINTMSG	16384

/* JPG - took *file out of the argument list and used logfilename instead
================
Con_DebugLog
================
*/
void Con_DebugLog( /* char *file, */ char *fmt, ...)
{
    va_list argptr;
	static char data[MAXPRINTMSG];	// JPG 3.02 - changed from 1024 to MAXPRINTMSG
    int fd;

    va_start(argptr, fmt);
    VSNPrintf(data, sizeof(data), fmt, argptr);
    va_end(argptr);
    fd = open(va("%s/%s", com_gamedir, logfilename), O_WRONLY | O_CREAT | O_APPEND, 0666);
    write(fd, data, strlen(data));
    close(fd);
}


/*
================
Con_Printf

Handles cursor positioning, line wrapping, etc
================
*/
// FIXME: make a buffer size safe vsprintf?
void Con_Printf (char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];
	static qboolean	inupdate;

	va_start (argptr,fmt);
	VSNPrintf (msg,sizeof(msg),fmt,argptr);
	va_end (argptr);

// also echo to debugging console
	Sys_Printf ("%s", msg);	// also echo to debugging console

// log all messages to file
	if (con_debuglog)
		Con_DebugLog( /* va("%s/qconsole.log",com_gamedir), */ "%s", msg);  // JPG - got rid of filename

	if (!con_initialized)
		return;

	if (cls.state == ca_dedicated)
		return;		// no graphics mode

// write it to the scrollable buffer
	Con_Print (msg);

// update the screen if the console is displayed
	if (cls.signon != SIGNONS && !scr_disabled_for_loading )
	{
	// protect against infinite loop if something in SCR_UpdateScreen calls
	// Con_Printd
		if (!inupdate)
		{
			inupdate = true;
			SCR_UpdateScreen ();
			inupdate = false;
		}
	}
}

/*
================
Con_Warning -- johnfitz -- prints a warning to the console
================
*/
void Con_Warning (char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];

	va_start (argptr,fmt);
	vsprintf (msg,fmt,argptr);
	va_end (argptr);

	Con_SafePrintf ("\x02Warning: ");
	Con_Printf ("%s", msg);
}

/*
================
Con_DPrintf

A Con_Printf that only shows up if the "developer" cvar is set
================
*/
void Con_DPrintf (char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];

	if (!developer.value)
		return;			// don't confuse non-developers with techie stuff...

	va_start (argptr,fmt);
	VSNPrintf (msg,sizeof(msg),fmt,argptr);
	va_end (argptr);

	Con_SafePrintf ("%s", msg);
}

/*
================
Con_Success
================
*/
void Con_Success (char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];

	va_start (argptr,fmt);
	vsprintf (msg,fmt,argptr);
	va_end (argptr);

	//Con_SafePrintf ("\x02Success: ");
	Con_Printf ("%s", msg);
}


/*
==================
Con_SafePrintf

Okay to call even when the screen can't be updated
==================
*/
void Con_SafePrintf (char *fmt, ...)
{
	va_list		argptr;
	char		msg[1024];
	int			temp;

	va_start (argptr,fmt);
	VSNPrintf (msg,sizeof(msg),fmt,argptr);
	va_end (argptr);

	temp = scr_disabled_for_loading;
	scr_disabled_for_loading = true;
	Con_Printf ("%s", msg);
	scr_disabled_for_loading = temp;
}

/*
================
Con_CenterPrintf -- johnfitz -- pad each line with spaces to make it appear centered
================
*/
void Con_CenterPrintf (int linewidth, char *fmt, ...)
{
	va_list		argptr;
	char	msg[MAXPRINTMSG]; //the original message
	char	line[MAXPRINTMSG]; //one line from the message
	char	spaces[21]; //buffer for spaces
	char	*src, *dst;
	int		len, s;

	va_start (argptr,fmt);
	VSNPrintf (msg,sizeof(msg),fmt,argptr);
	va_end (argptr);

	linewidth = min (linewidth, con_linewidth);
	for (src = msg; *src; )
	{
		dst = line;
		while (*src && *src != '\n')
			*dst++ = *src++;
		*dst = 0;
		if (*src == '\n')
			src++;

		len = strlen(line);
		if (len < linewidth)
		{
			s = (linewidth-len)/2;
			memset (spaces, ' ', s);
			spaces[s] = 0;
			Con_Printf ("%s%s\n", spaces, line);
		}
		else
			Con_Printf ("%s\n", line);
	}
}

/*
==================
Con_LogCenterPrint -- johnfitz -- echo centerprint message to the console
==================
*/
void Con_LogCenterPrint (char *str)
{
	if (!strcmp(str, con_lastcenterstring))
		return; //ignore duplicates

	if (cl.gametype == GAME_DEATHMATCH && con_logcenterprint.value != 2)
		return; //don't log in deathmatch

	strcpy(con_lastcenterstring, str);

	if (con_logcenterprint.value)
	{
		Con_Printf (Con_Quakebar(40));
		Con_CenterPrintf (40, "%s\n", str);
		Con_Printf (Con_Quakebar(40));
		Con_ClearNotify ();
	}
}

/*
==============================================================================

DRAWING

==============================================================================
*/

/*
================
Con_DrawInput

The input line scrolls horizontally if typing goes beyond the right edge
================
*/
void Con_DrawInput (void)
{
	int		y;
	int		i;
	char	*text;

	if (key_dest != key_console && !con_forcedup)
		return;		// don't draw anything

	text = key_lines[edit_line];

// add the cursor frame
	text[key_linepos] = 10+((int)(realtime*con_cursorspeed)&1);

// fill out remainder with spaces
	for (i=key_linepos+1 ; i< con_linewidth ; i++)
		text[i] = ' ';

//	prestep if horizontally scrolling
	if (key_linepos >= con_linewidth)
		text += 1 + key_linepos - con_linewidth;

// draw it
	y = con_vislines-16;

	for (i=0 ; i<con_linewidth ; i++)
		Draw_Character ( (i+1)<<3, con_vislines - 16, text[i]);

// remove cursor
	key_lines[edit_line][key_linepos] = 0;
}


/*
================
Con_DrawNotify

Draws the last few lines of output transparently over the game top
================
*/
void Con_DrawNotify (void)
{
	int		x, v;
	char	*text;
	int		i;
	float	time;
	extern char chat_buffer[];
	int		maxlines = CLAMP (0, _con_notifylines.value, NUM_CON_TIMES);

	v = 0;
	for (i = con_current - maxlines + 1 ; i <= con_current ; i++)
	{
		if (i < 0)
			continue;
		time = con_times[i % NUM_CON_TIMES];
		if (time == 0)
			continue;
		time = realtime - time;
		if (time > con_notifytime.value)
			continue;
		text = con_text + (i % con_totallines)*con_linewidth;

		clearnotify = 0;

		for (x = 0 ; x < con_linewidth ; x++)
			Draw_Character ( (x+1)<<3, v, text[x]);

		v += 8;
	}


	if (key_dest == key_message)
	{
		clearnotify = 0;

		// JPG - was x = 0 etc.. recoded with x = 5, i = 0
		i = 0;

		// JPG - added support for team messages
		if (team_message)
		{
			Draw_String (8, v, "(say team):");
			x = 12; // Baker 3.90: 7 increased to 12 for "say_team"
		}
		else
		{
			Draw_String (8, v, "say:");
			x = 5;
		}

		while(chat_buffer[i])
		{
			Draw_Character ( x<<3, v, chat_buffer[i]);
			x++;

			// JPG - added this for longer says
			i++;
			if (x > con_linewidth)
			{
				x = team_message ? 12 : 5; // Baker 3.90: 7 increased to 12 "(say)" ---> "(say_team)"
				v += 8;
			}
		}
		Draw_Character ( x<<3, v, 10+((int)(realtime*con_cursorspeed)&1));
		v += 8;
	}

	if (v > con_notifylines)
		con_notifylines = v;
}

/*
================
Con_DrawConsole

Draws the console with the solid background
The typing input line at the bottom should only be drawn if typing is allowed
================
*/
void Con_DrawConsole (int lines, qboolean drawinput)
{
	int				i, j, x, y, rows;
	char			*text;

	if (lines <= 0)
		return;

// draw the background
	Draw_ConsoleBackground (lines);

// draw the text
	con_vislines = lines;

	rows = (lines-16)>>3;		// rows of text to draw
	y = lines - 16 - (rows<<3);	// may start slightly negative

	for (i= con_current - rows + 1 ; i<=con_current ; i++, y+=8 )
	{
		j = i - con_backscroll;
		if (j<0)
			j = 0;
		text = con_text + (j % con_totallines)*con_linewidth;

		for (x=0 ; x<con_linewidth ; x++)
			Draw_Character ( (x+1)<<3, y, text[x]);
	}

// draw the input prompt, user text, and cursor if desired
	if (drawinput)
		Con_DrawInput ();
}
