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
#include "quakedef.h"

// JPG - added FrikaC's code for pasting from clipboard
// 01-22-2000 FrikaC Begin PASTE
#ifdef _WIN32
#include <windows.h>
#endif
// 01-22-2000 FrikaC End PASTE

/*

key up events are sent even if in console mode

*/


#define		MAXCMDLINE	256
char	key_lines[32][MAXCMDLINE];
int		key_linepos;
int		shift_down=false;
int		ctrl_down=false;
int		alt_down=false;
int		key_lastpress;

int		edit_line=0;
int		history_line=0;

keydest_t		key_dest;

int		key_count;			// incremented every key event

char	*keybindings[256];
qboolean	consolekeys[256];	// if true, can't be rebound while in console
qboolean	menubound[256];	// if true, can't be rebound while in menu
int		keyshift[256];		// key to map to if shift held down in console
int		key_repeats[256];	// if > 1, it is autorepeating
qboolean	keydown[256];

typedef struct
{
	char	*name;
	int		keynum;
} keyname_t;

// Baker 3.60 - following replaced keynames from JoeQuake 0.15

keyname_t keynames[] =
{
	{"TAB", K_TAB},
	{"ENTER", K_ENTER},
	{"ESCAPE", K_ESCAPE},
	{"SPACE", K_SPACE},
	{"BACKSPACE", K_BACKSPACE},

	{"CAPSLOCK", K_CAPSLOCK},
	{"PRINTSCR", K_PRINTSCR},
	{"SCRLCK", K_SCRLCK},
	{"SCROLLOCK", K_SCRLCK},
	{"PAUSE", K_PAUSE},

	{"UPARROW", K_UPARROW},
	{"DOWNARROW", K_DOWNARROW},
	{"LEFTARROW", K_LEFTARROW},
	{"RIGHTARROW", K_RIGHTARROW},

	{"ALT", K_ALT},
	{"LALT", K_LALT},
	{"RALT", K_RALT},
	{"CTRL", K_CTRL},
	{"LCTRL", K_LCTRL},
	{"RCTRL", K_RCTRL},
	{"SHIFT", K_SHIFT},
	{"LSHIFT", K_LSHIFT},
	{"RSHIFT", K_RSHIFT},

	{"WINKEY", K_WIN},
	{"LWINKEY", K_LWIN},
	{"RWINKEY", K_RWIN},
	{"POPUPMENU", K_MENU},

	// keypad keys

	{"NUMLOCK", KP_NUMLOCK},
	{"KP_NUMLCK", KP_NUMLOCK},
	{"KP_NUMLOCK", KP_NUMLOCK},
	{"KP_SLASH", KP_SLASH},
	{"KP_DIVIDE", KP_SLASH},
	{"KP_STAR", KP_STAR},
	{"KP_MULTIPLY", KP_STAR},

	{"KP_MINUS", KP_MINUS},

	{"KP_HOME", KP_HOME},
	{"KP_7", KP_HOME},
	{"KP_UPARROW", KP_UPARROW},
	{"KP_8", KP_UPARROW},
	{"KP_PGUP", KP_PGUP},
	{"KP_9", KP_PGUP},
	{"KP_PLUS", KP_PLUS},

	{"KP_LEFTARROW", KP_LEFTARROW},
	{"KP_4", KP_LEFTARROW},
	{"KP_5", KP_5},
	{"KP_RIGHTARROW", KP_RIGHTARROW},
	{"KP_6", KP_RIGHTARROW},

	{"KP_END", KP_END},
	{"KP_1", KP_END},
	{"KP_DOWNARROW", KP_DOWNARROW},
	{"KP_2", KP_DOWNARROW},
	{"KP_PGDN", KP_PGDN},
	{"KP_3", KP_PGDN},

	{"KP_INS", KP_INS},
	{"KP_0", KP_INS},
	{"KP_DEL", KP_DEL},
	{"KP_ENTER", KP_ENTER},
	
	{"F1", K_F1},
	{"F2", K_F2},
	{"F3", K_F3},
	{"F4", K_F4},
	{"F5", K_F5},
	{"F6", K_F6},
	{"F7", K_F7},
	{"F8", K_F8},
	{"F9", K_F9},
	{"F10", K_F10},
	{"F11", K_F11},
	{"F12", K_F12},

	{"INS", K_INS},
	{"DEL", K_DEL},
	{"PGDN", K_PGDN},
	{"PGUP", K_PGUP},
	{"HOME", K_HOME},
	{"END", K_END},

	{"PAUSE", K_PAUSE},

	{"MWHEELUP", K_MWHEELUP},
	{"MWHEELDOWN", K_MWHEELDOWN},
	{"MOUSE1", K_MOUSE1},
	{"MOUSE2", K_MOUSE2},
	{"MOUSE3", K_MOUSE3},
	{"MOUSE4", K_MOUSE4},
	{"MOUSE5", K_MOUSE5},
	{"MOUSE6", K_MOUSE6},
	{"MOUSE7", K_MOUSE7},
	{"MOUSE8", K_MOUSE8},

	{"JOY1", K_JOY1},
	{"JOY2", K_JOY2},
	{"JOY3", K_JOY3},
	{"JOY4", K_JOY4},

	{"AUX1", K_AUX1},
	{"AUX2", K_AUX2},
	{"AUX3", K_AUX3},
	{"AUX4", K_AUX4},
	{"AUX5", K_AUX5},
	{"AUX6", K_AUX6},
	{"AUX7", K_AUX7},
	{"AUX8", K_AUX8},
	{"AUX9", K_AUX9},
	{"AUX10", K_AUX10},
	{"AUX11", K_AUX11},
	{"AUX12", K_AUX12},
	{"AUX13", K_AUX13},
	{"AUX14", K_AUX14},
	{"AUX15", K_AUX15},
	{"AUX16", K_AUX16},
	{"AUX17", K_AUX17},
	{"AUX18", K_AUX18},
	{"AUX19", K_AUX19},
	{"AUX20", K_AUX20},
	{"AUX21", K_AUX21},
	{"AUX22", K_AUX22},
	{"AUX23", K_AUX23},
	{"AUX24", K_AUX24},
	{"AUX25", K_AUX25},
	{"AUX26", K_AUX26},
	{"AUX27", K_AUX27},
	{"AUX28", K_AUX28},
	{"AUX29", K_AUX29},
	{"AUX30", K_AUX30},
	{"AUX31", K_AUX31},
	{"AUX32", K_AUX32},


	{"SEMICOLON", ';'},	// because a raw semicolon seperates commands
	{"TILDE", '~'},
	{"BACKQUOTE", '`'},
	{"QUOTE", '"'},
	{"APOSTROPHE", '\''},
	{NULL,0}
};

/*
==============================================================================

			LINE TYPING INTO THE CONSOLE

==============================================================================


/*
====================
Interactive line editing and console scrollback
====================
*/
void Key_Console (int key, char ascii)
{
	char	*cmd;
	qboolean     passed=false;
	
	// JPG - modified FrikaC's code for pasting from clipboard
#ifdef _WIN32
	HANDLE  th;
	char	*s;
#endif
	// end mod

	//Con_Printf ("Key Console ... k is %d a is %d\n", key, ascii);

	switch (key)
	{
	case KP_STAR:
		key = '*';
		break;
	case KP_SLASH:
		key = '/';
		break;
	case KP_MINUS:
		key = '-';
		break;
	case KP_PLUS:
		key = '+';
		break;
	case KP_HOME:
		key = '7';
		break;
	case KP_UPARROW:
		key = '8';
		break;
	case KP_PGUP:
		key = '9';
		break;
	case KP_LEFTARROW:
		key = '4';
		break;
	case KP_5:
		key = '5';
		break;
	case KP_RIGHTARROW:
		key = '6';
		break;
	case KP_END:
		key = '1';
		break;
	case KP_DOWNARROW:
		key = '2';
		break;
	case KP_PGDN:
		key = '3';
		break;
	case KP_INS:
		key = '0';
		break;
	case KP_DEL:
		key = '.';
		break;
	}

	if (key == K_ENTER || key == KP_ENTER) // Baker 3.60 - keypad enter
	{
		con_backscroll = 0;	// JPG 1.05 - go to end of buffer when user hits enter
		Cbuf_AddText (key_lines[edit_line]+1);	// skip the >
		Cbuf_AddText ("\n");
		Con_Printf ("%s\n",key_lines[edit_line]);
		edit_line = (edit_line + 1) & 31;
		history_line = edit_line;
		key_lines[edit_line][0] = ']';
		key_linepos = 1;
		if (cls.state == ca_disconnected)
			SCR_UpdateScreen ();	// force an update, because the command
									// may take some time
		return;
	}


	// JPG 1.05 - fixed tab completion
	if (key == K_TAB)
	{
		int len, i;
		char *fragment;
		cvar_t *var;
		char *best = "~";
		char *least = "~";

		len = strlen(key_lines[edit_line]);
		for (i = 0 ; i < len - 1 ; i++)
		{
			if (key_lines[edit_line][i] == ' ')
				return;
		}
		fragment = key_lines[edit_line] + 1;

		len--;
		for (var = cvar_vars->next ; var ; var = var->next)
		{
			if (strcmp(var->name, fragment) >= 0 && strcmp(best, var->name) > 0)
				best = var->name;
			if (strcmp(var->name, least) < 0)
				least = var->name;
		}
		cmd = Cmd_CompleteCommand(fragment);
		if (strcmp(cmd, fragment) >= 0 && strcmp(best, cmd) > 0)
			best = cmd;
		if (best[0] == '~')
		{
			cmd = Cmd_CompleteCommand(" ");
			if (strcmp(cmd, least) < 0)
				best = cmd;
			else
				best = least;
		}

		sprintf(key_lines[edit_line], "]%s ", best);
		key_linepos = strlen(key_lines[edit_line]);
		return;
	}
	
	if (key == K_BACKSPACE || key == K_LEFTARROW)
	{
		if (key_linepos > 1)
			key_linepos--;
		return;
	}

	if (key == K_UPARROW)
	{
		do
		{
			history_line = (history_line - 1) & 31;
		} while (history_line != edit_line
				&& !key_lines[history_line][1]);
		if (history_line == edit_line)
			history_line = (edit_line+1)&31;
		Q_strcpy(key_lines[edit_line], key_lines[history_line]);
		key_linepos = Q_strlen(key_lines[edit_line]);
		return;
	}

	if (key == K_DOWNARROW)
	{
		if (history_line == edit_line) return;
		do
		{
			history_line = (history_line + 1) & 31;
		}
		while (history_line != edit_line
			&& !key_lines[history_line][1]);
		if (history_line == edit_line)
		{
			key_lines[edit_line][0] = ']';
			key_linepos = 1;
		}
		else
		{
			Q_strcpy(key_lines[edit_line], key_lines[history_line]);
			key_linepos = Q_strlen(key_lines[edit_line]);
		}
		return;
	}

	if (key == K_PGUP || key==K_MWHEELUP)
	{
		con_backscroll += 2;
		if (con_backscroll > con_totallines - (vid.height>>3) - 1)
			con_backscroll = con_totallines - (vid.height>>3) - 1;
		return;
	}

	if (key == K_PGDN || key==K_MWHEELDOWN)
	{
		con_backscroll -= 2;
		if (con_backscroll < 0)
			con_backscroll = 0;
		return;
	}

	if (key == K_HOME)
	{
		con_backscroll = con_totallines - (vid.height>>3) - 1;
		return;
	}

	if (key == K_END)
	{
		con_backscroll = 0;
		return;
	}

	// JPG - modified FrikaC's code for pasting from clipboard
#ifdef _WIN32
	if ((key=='V' || key=='v') && GetKeyState(VK_CONTROL) < 0) 
	{
		if (OpenClipboard(NULL)) 
		{
			th = GetClipboardData(CF_TEXT);
			if (th) 
			{
				s = GlobalLock(th);
				if (s) 
				{
					while (*s && *s != '\n' && *s != '\r' && *s != '\b' && key_linepos < MAXCMDLINE - 1)
						key_lines[edit_line][key_linepos++] = *s++;
					key_lines[edit_line][key_linepos] = 0;
				}
				GlobalUnlock(th);
			}
			CloseClipboard();
			return;
		}
	}
#endif
	// end mod

	if (ascii>=32 && ascii<=127)
		passed = true;

	// Extra Keys support

	// Is either printable or printable if extended by CTRL or ALT sequence
	if (ctrl_down)
	{
		if (key >= '0' && key <= '9')
		{
			ascii = key - '0' + 0x12;	// yellow number
			passed = true;
		}
		else
		{
			switch (key)
			{
			case '[': ascii = 0x10; passed = true; break;
			case ']': ascii = 0x11; passed = true; break;
			case 'g': ascii = 0x86; passed = true; break;
			case 'r': ascii = 0x87; passed = true; break;
			case 'y': ascii = 0x88; passed = true; break;
			case 'b': ascii = 0x89; passed = true; break;
			case '(': ascii = 0x80; passed = true; break;
			case '=': ascii = 0x81; passed = true; break;
			case ')': ascii = 0x82; passed = true; break;
			case 'a': ascii = 0x83; passed = true; break;
			case '<': ascii = 0x1d; passed = true; break;
			case '-': ascii = 0x1e; passed = true; break;
			case '>': ascii = 0x1f; passed = true; break;
			case ',': ascii = 0x1c; passed = true; break;
			case '.': ascii = 0x9c; passed = true; break;
			case 'B': ascii = 0x8b; passed = true; break;
			case 'C': ascii = 0x8d; passed = true; break;
			}
		}
	}

	if (alt_down && ascii)
	{
		ascii |= 128;		// Baker 3.60 - Bronzing in console by holding ALT from JoeQuake */
		passed = true;
	}

	if (passed == false)
		return;	// non printable */
		
	if (key_linepos < MAXCMDLINE-1)
	{
		key_lines[edit_line][key_linepos] = ascii;
		key_linepos++;
		key_lines[edit_line][key_linepos] = 0;
	}

}

//============================================================================

// JPG - added MAX_CHAT_SIZE
#define MAX_CHAT_SIZE 45
char chat_buffer[MAX_CHAT_SIZE];
qboolean team_message = false;

void Key_Message (int key, char ascii)
{
	static int chat_bufferlen = 0;
	// JPG - modified FrikaC's code for pasting from clipboard
#ifdef _WIN32
	HANDLE  th;
	char	*s;
#endif
	// end mod

	if (key == K_ENTER)
	{
		if (team_message)
			Cbuf_AddText ("say_team \"");
		else
			Cbuf_AddText ("say \"");
		Cbuf_AddText(chat_buffer);
		Cbuf_AddText("\"\n");

		key_dest = key_game;
		chat_bufferlen = 0;
		chat_buffer[0] = 0;
		return;
	}

	if (key == K_ESCAPE)
	{
		key_dest = key_game;
		chat_bufferlen = 0;
		chat_buffer[0] = 0;
		return;
	}

	// JPG - modified FrikaC's code for pasting from clipboard
#ifdef _WIN32
	if ((key=='V' || key=='v') && GetKeyState(VK_CONTROL) < 0) 
	{
		if (OpenClipboard(NULL)) 
		{
			th = GetClipboardData(CF_TEXT);
			if (th) 
			{
				s = GlobalLock(th);
				if (s) 
				{
					while (*s && *s != '\n' && *s != '\r' && *s != '\b' && chat_bufferlen < MAX_CHAT_SIZE - (team_message ? 3 : 1))
						chat_buffer[chat_bufferlen++] = *s++;
					chat_buffer[chat_bufferlen] = 0;
				}
				GlobalUnlock(th);
			}
			CloseClipboard();
			return;
		}
	}
#endif
	// end mod


	if (key == K_BACKSPACE)
	{
		if (chat_bufferlen)
		{
			chat_bufferlen--;
			chat_buffer[chat_bufferlen] = 0;
		}
		return;
	}

	if (chat_bufferlen == MAX_CHAT_SIZE - (team_message ? 3 : 1)) // JPG - maximize message length
		return; // all full

	if (ascii < 32 || ascii > 127) // Baker 3.702 I moved this from above
		return;	// non printable

	chat_buffer[chat_bufferlen++] = key;
	chat_buffer[chat_bufferlen] = 0;
}

//============================================================================


/*
===================
Returns a key number to be used to index keybindings[] by looking at
the given string.  Single ascii characters return themselves, while
the K_* names are matched up.
===================
*/
int
Key_StringToKeynum (char *str)
{
	keyname_t	*kn;
	
	if (!str || !str[0])
		return -1;
	if (!str[1])
		return tolower(str[0]);

	for (kn = keynames; kn->name; kn++) {
		if (!Q_strcasecmp(str,kn->name))
			return kn->keynum;
	}
	return -1;
}

/*
===================
Returns a string (either a single ascii char, or a K_* name) for the
given keynum.
FIXME: handle quote special (general escape sequence?)
===================
*/
char *Key_KeynumToString (int keynum)
{
	keyname_t	*kn;	
	static	char	tinystr[2];
	
	if (keynum == -1)
		return "<KEY NOT FOUND>";
	if (keynum > 32 && keynum < 127)
	{	// printable ascii
		tinystr[0] = keynum;
		tinystr[1] = 0;
		return tinystr;
	}
	
	for (kn=keynames ; kn->name ; kn++)
		if (keynum == kn->keynum)
			return kn->name;

	return "<UNKNOWN KEYNUM>";
}


/*
===================
Key_SetBinding
===================
*/
void Key_SetBinding (int keynum, char *binding)
{
	char	*new;
	int		l;
			
	if (keynum == -1)
		return;

// free old bindings
	if (keybindings[keynum])
	{
		Z_Free (keybindings[keynum]);
		keybindings[keynum] = NULL;
	}
			
// allocate memory for new binding
	l = Q_strlen (binding);	
	new = Z_Malloc (l+1);
	Q_strcpy (new, binding);
	new[l] = 0;
	keybindings[keynum] = new;	
}

/*
===================
Key_Unbind_f
===================
*/
void Key_Unbind_f (void)
{
	int		b;

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("unbind <key> : remove commands from a key\n");
		return;
	}
	
	b = Key_StringToKeynum (Cmd_Argv(1));
	if (b==-1)
	{
		Con_Printf ("\"%s\" isn't a valid key\n", Cmd_Argv(1));
		return;
	}

	Key_SetBinding (b, "");
}

void Key_Unbindall_f (void)
{
	int		i;
	
	for (i=0 ; i<256 ; i++)
		if (keybindings[i])
			Key_SetBinding (i, "");
}

/*
============
Key_Bindlist_f -- johnfitz
============
*/
void Key_Bindlist_f (void)
{
	int		i, count;

	count = 0;
	for (i=0 ; i<256 ; i++)
		if (keybindings[i])
			if (*keybindings[i])
			{
				Con_SafePrintf ("   %s \"%s\"\n", Key_KeynumToString(i), keybindings[i]);
				count++;
			}
	Con_SafePrintf ("%i bindings\n", count);
}

/*
===================
Key_Bind_f
===================
*/
void Key_Bind_f (void)
{
	int			i, c, b;
	char		cmd[1024];
	
	c = Cmd_Argc();

	if (c != 2 && c != 3)
	{
		Con_Printf ("bind <key> [command] : attach a command to a key\n");
		return;
	}
	b = Key_StringToKeynum (Cmd_Argv(1));
	if (b==-1)
	{
		Con_Printf ("\"%s\" isn't a valid key\n", Cmd_Argv(1));
		return;
	}

	if (c == 2)
	{
		if (keybindings[b])
			Con_Printf ("\"%s\" = \"%s\"\n", Cmd_Argv(1), keybindings[b] );
		else
			Con_Printf ("\"%s\" is not bound\n", Cmd_Argv(1) );
		return;
	}
	
// copy the rest of the command line
	cmd[0] = 0;		// start out with a null string
	for (i=2 ; i< c ; i++)
	{
		if (i > 2)
			strcat (cmd, " ");
		strcat (cmd, Cmd_Argv(i));
	}

	Key_SetBinding (b, cmd);
}

/*
============
Key_WriteBindings

Writes lines containing "bind key value"
============
*/
void Key_WriteBindings (FILE *f)
{
	int		i;

	fprintf (f, "\n// Key bindings\n\n");
	for (i=0 ; i<256 ; i++)
		if (keybindings[i])
			if (*keybindings[i])
				fprintf (f, "bind \"%s\" \"%s\"\n", Key_KeynumToString(i), keybindings[i]);
}


/*
===================
Key_Init
===================
*/
void Key_Init (void)
{
	int		i;

	for (i=0 ; i<32 ; i++)
	{
		key_lines[i][0] = ']';
		key_lines[i][1] = 0;
	}
	key_linepos = 1;
	
//
// init ascii characters in console mode
//
	for (i=32 ; i<128 ; i++)
		consolekeys[i] = true;
	consolekeys[K_ENTER] = true;
	consolekeys[K_TAB] = true;
	consolekeys[K_LEFTARROW] = true;
	consolekeys[K_RIGHTARROW] = true;
	consolekeys[K_UPARROW] = true;
	consolekeys[K_DOWNARROW] = true;
	consolekeys[K_BACKSPACE] = true;
	consolekeys[K_INS] = true;
	consolekeys[K_DEL] = true;
	consolekeys[K_HOME] = true;
	consolekeys[K_END] = true;
	consolekeys[K_PGUP] = true;
	consolekeys[K_PGDN] = true;
	consolekeys[K_ALT] = true;
	// consolekeys[K_LALT] = true;
	// consolekeys[K_RALT] = true;
	consolekeys[K_CTRL] = true;
	// consolekeys[K_LCTRL] = true;
	// consolekeys[K_RCTRL] = true;
	consolekeys[K_SHIFT] = true;
	// consolekeys[K_LSHIFT] = true;
	// consolekeys[K_RSHIFT] = true;
	consolekeys[K_MWHEELUP] = true;
	consolekeys[K_MWHEELDOWN] = true;
	consolekeys['`'] = false;
	consolekeys['~'] = false;

	consolekeys[KP_SLASH]	 = true;
	consolekeys[KP_STAR] = true;
	consolekeys[KP_MINUS] = true;
	consolekeys[KP_HOME] = true;
	consolekeys[KP_UPARROW] = true;
	consolekeys[KP_PGUP] = true;
	consolekeys[KP_PLUS] = true;
	consolekeys[KP_LEFTARROW] = true;
	consolekeys[KP_5] = true;
	consolekeys[KP_RIGHTARROW] = true;
	consolekeys[KP_END] = true;
	consolekeys[KP_DOWNARROW] = true;
	consolekeys[KP_PGDN]	 = true;
	consolekeys[KP_ENTER] = true;
	consolekeys[KP_INS] = true;
	consolekeys[KP_DEL] = true;


	for (i=0 ; i<256 ; i++)
		keyshift[i] = i;
	for (i='a' ; i<='z' ; i++)
		keyshift[i] = i - 'a' + 'A';
	keyshift['1'] = '!';
	keyshift['2'] = '@';
	keyshift['3'] = '#';
	keyshift['4'] = '$';
	keyshift['5'] = '%';
	keyshift['6'] = '^';
	keyshift['7'] = '&';
	keyshift['8'] = '*';
	keyshift['9'] = '(';
	keyshift['0'] = ')';
	keyshift['-'] = '_';
	keyshift['='] = '+';
	keyshift[','] = '<';
	keyshift['.'] = '>';
	keyshift['/'] = '?';
	keyshift[';'] = ':';
	keyshift['\''] = '"';
	keyshift['['] = '{';
	keyshift[']'] = '}';
	keyshift['`'] = '~';
	keyshift['\\'] = '|';

	menubound[K_ESCAPE] = true;
	for (i=0 ; i<12 ; i++)
		menubound[K_F1+i] = true;

//
// register our functions
//
	Cmd_AddCommand ("bindlist",Key_Bindlist_f); //johnfitz
	Cmd_AddCommand ("bind",Key_Bind_f);
	Cmd_AddCommand ("unbind",Key_Unbind_f);
	Cmd_AddCommand ("unbindall",Key_Unbindall_f);


}

/*
===================
Key_Event

Called by the system between frames for both key up and key down events
Should NOT be called during an interrupt!
===================
*/
extern qboolean	cl_inconsole; // Baker 3.76 - from Qrack
void Key_Event (int key, char ascii, qboolean down)
{
	char	*kb;
	char	cmd[1024];

	keydown[key] = down;

	if (!down)
		key_repeats[key] = 0;

	key_lastpress = key;
	key_count++;
	if (key_count <= 0)
	{
		return;		// just catching keys for Con_NotifyBox
	}

// update auto-repeat status
	if (down)
	{
		key_repeats[key]++;

		// JPG 1.05 - added K_PGUP, K_PGDN, K_TAB and check to make sure that key_dest isn't key_game
		// JPG 3.02 - added con_forcedup check
		if ((key_repeats[key] > 1) && ((key != K_BACKSPACE && key != K_PAUSE && key != K_PGUP && key != K_PGDN && key != K_TAB) || 
			((key_dest == key_game) && !con_forcedup)))
		{
			return;	// ignore most autorepeats
		}
			
		if (key >= K_MOUSE5 && key<= K_MOUSE8 && !keybindings[key])
			Con_Printf ("%s is unbound, hit F4 to set.\n", Key_KeynumToString (key) );
	}

	if (key == K_SHIFT)
	{
		shift_down = down;
	}

	if (key == K_ALT)
	{
		alt_down = down;
	}

	if (key == K_CTRL)
	{
		ctrl_down = down;
	}


//
// handle escape specialy, so the user can never unbind it
//
	if (key == K_ESCAPE)
	{
		if (!down)
			return;
		switch (key_dest)
		{
		case key_message:
			Key_Message (key, ascii);
			break;
		case key_menu:
			M_Keydown (key, ascii, down);
			break;
		case key_game:
		case key_console:
			M_ToggleMenu_f ();
			break;
		default:
			Sys_Error ("Bad key_dest");
		}
		return;
	}

//
// key up events only generate commands if the game key binding is
// a button command (leading + sign).  These will occur even in console mode,
// to keep the character from continuing an action started before a console
// switch.  Button commands include the kenum as a parameter, so multiple
// downs can be matched with ups
//
	if (!down)
	{
		kb = keybindings[key];  // Baker 3.703 is this right
		if (kb && kb[0] == '+')
		{
			sprintf (cmd, "-%s %i\n", kb+1, key);
			Cbuf_AddText (cmd);
		}
		if (keyshift[key] != key)
		{
			kb = keybindings[keyshift[key]];
			if (kb && kb[0] == '+')
			{
				sprintf (cmd, "-%s %i\n", kb+1, key);
				Cbuf_AddText (cmd);
			}
		}
		return;
	}

//
// during demo playback, most keys bring up the main menu
//
	if (cls.demoplayback && down && consolekeys[key] && key_dest == key_game && key != K_TAB && key != K_PGUP && key != K_PGDN && key != 0x3d && key != 0x2d) // Baker 3.60 -- permit TAB to display scorebar during demo playback / Baker 3.67: add +/- keys 0x22d = -  0x3d = +
	{
		M_ToggleMenu_f ();
		return;
	}

//
// if not a consolekey, send to the interpreter no matter what mode is
//
	if ( (key_dest == key_menu && menubound[key])
	|| (key_dest == key_console && !consolekeys[key])
	|| (key_dest == key_game && ( !con_forcedup || !consolekeys[key] ) ) )
	{
		kb = keybindings[key];
		if (kb)
		{
			if (kb[0] == '+')
			{	// button commands add keynum as a parm
				sprintf (cmd, "%s %i\n", kb, key);
				Cbuf_AddText (cmd);
			}
			else
			{
				Cbuf_AddText (kb);
				Cbuf_AddText ("\n");
			}
		}
		return;
	}

	if (!down)
		return;		// other systems only care about key down events

	if (shift_down)
	{
		key = keyshift[key];
	}

	switch (key_dest)
	{
	case key_message:
		Key_Message (key, ascii);
		break;
	case key_menu:
		M_Keydown (key, ascii, down);
		break;

	case key_game:
	case key_console:
		Key_Console (key, ascii);
		break;
	default:
		Sys_Error ("Bad key_dest");
	}
}

/*
================
Key_ClearAllStates - Baker 3.71 - this should be here
================
*/
void Key_ClearAllStates (void)
{
	int		i;

// send an up event for each key, to make sure the server clears them all
	/*for (i=0 ; i<256 ; i++)
	{
		Key_Event (i, 0, false);
	} //  Baker 3.71 -- DP doesn~t do this  */

	Key_ClearStates ();
 	//IN_ClearStates ();  //Baker 3.71 - DP doesn~t do this
}

/*
===================
Key_ClearStates
===================
*/
void Key_ClearStates (void)
{
	int		i;

	for (i=0 ; i<256 ; i++)
	{
		keydown[i] = false;
		key_repeats[i] = 0;
	}
}

