/*
Copyright (C) 1999-2000, contributors of the QuakeForge project

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
// cl_server_browser.h

#define	MAX_SERVER_LIST	256
#define SERVER_LIST_NAME va("%s/%s", com_basedir, "liveservers.txt") // "liveservers.txt"
#define SERVER_BROWSER_URL "http://quakeone.com/servers/"

typedef struct 
{
	char	*server;
	char	*description;
} server_entry_t;

extern	server_entry_t	server_browser_list[MAX_SERVER_LIST];

//void ServerBrowser_Init (void);
//void ServerBrowser_Shutdown (void);
void ServerBrowser_Set (int i,char *addr,char *desc);
void ServerBrowser_Reset_NoFree (int i);
void ServerBrowser_Reset (int i);
void ServerBrowser_Switch (int a,int b);
int ServerBrowser_Length (void);
void ServerBrowser_Load (void);
//void ServerBrowser_Save (FILE *f);

// Called in main when running as server browser fetch util
char* Server_Browser_Util_Get_Page_Alloc (const char** bufferin, const char* my_site, int* bytes);
int   Server_Browser_Util_Write_Servers (const char* buffer_in, int bufsize, const char* filename);

typedef enum {
	http_initiated,			// Initiated, no disconnect attempt made
	http_disconnecting,		// Disconnection attempt made
	http_fakeprogress,		// Fake progress bar --- MEAT --- continues until timeout or completion
	http_reconnecting,		// Completed + successful (THE END)
	http_notreconnecting,	// Completed + successful (ALTERNATE END)
	http_error,				// Any failure except timeout 
	http_timeout,			// Timeout failure ... firewall advisory?
	http_success
} http_state;

// Called in menu.c
void HTTP_ServerBrowser_Begin_Query (void);
void HTTP_ServerBrowser_Check_Query_Completion (void);

