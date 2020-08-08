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
// server_browser_list.c -- serverlist addressbook

#include "quakedef.h"

server_entry_t	server_browser_list[MAX_SERVER_LIST];

/*
void ServerBrowser_Init (void)
{
	memset (&server_browser_list, 0, sizeof(server_browser_list));
}

// Baker: no need for this since we aren't saving.  And this doesn't Z_Free
void ServerBrowser_Shutdown (void)
{
	remove (SERVER_LIST_NAME);
}
*/

void ServerBrowser_Set (int i, char *addr, char *desc)
{
	if (i >= MAX_SERVER_LIST || i < 0)
		Sys_Error ("ServerBrowser_Set: Bad index %d", i);

	if (server_browser_list[i].server)
		Z_Free (server_browser_list[i].server);
	if (server_browser_list[i].description)
		Z_Free (server_browser_list[i].description);

	server_browser_list[i].server = CopyString (addr);
	server_browser_list[i].description = CopyString (desc);
}

void ServerBrowser_Reset_All(void)
{
	int i;

	for (i=MAX_SERVER_LIST-1; i>=0; i--)
		ServerBrowser_Reset (i);

}


void ServerBrowser_Reset_NoFree (int i)
{
	if (i >= MAX_SERVER_LIST || i < 0)
		Sys_Error ("ServerBrowser_Reset_NoFree: Bad index %d", i);

	server_browser_list[i].description = server_browser_list[i].server = NULL;
}

void ServerBrowser_Reset (int i)
{
	if (i >= MAX_SERVER_LIST || i < 0)
		Sys_Error ("ServerBrowser_Reset: Bad index %d", i);

	if (server_browser_list[i].server)
	{
		Z_Free (server_browser_list[i].server);
		server_browser_list[i].server = NULL;
	}

	if (server_browser_list[i].description)
	{
		Z_Free (server_browser_list[i].description);
		server_browser_list[i].description = NULL;
	}
}

void ServerBrowser_Switch (int a, int b)
{
	server_entry_t	temp;

	if (a >= MAX_SERVER_LIST || a < 0)
		Sys_Error ("ServerBrowser_Switch: Bad index %d", a);
	if (b >= MAX_SERVER_LIST || b < 0)
		Sys_Error ("ServerBrowser_Switch: Bad index %d", b);

	memcpy (&temp, &server_browser_list[a], sizeof(temp));
	memcpy (&server_browser_list[a], &server_browser_list[b], sizeof(temp));
	memcpy (&server_browser_list[b], &temp, sizeof(temp));
}

int ServerBrowser_Length (void)
{
	int	count;

	for (count = 0 ; count < MAX_SERVER_LIST && server_browser_list[count].server ; count++)
		;

	return count;
}

void ServerBrowser_Load (void)
{
	int	c, len, argc, count;
	char	line[128], *desc, *addr;
	FILE	*f = fopen(SERVER_LIST_NAME, "rt");

	if (!f)
		return;

	count = len = 0;
	while ((c = getc(f)))
	{
		if (c == '\n' || c == '\r' || c == EOF)
		{
			if (c == '\r' && (c = getc(f)) != '\n' && c != EOF)
				ungetc (c, f);

			line[len] = 0;
			len = 0;
			Cmd_TokenizeString (line);

			if ((argc = Cmd_Argc()) >= 1)
			{
				addr = Cmd_Argv(0);
				desc = (argc >= 2) ? Cmd_Args() : "Unknown";
				ServerBrowser_Set (count, addr, desc);
				if (++count == MAX_SERVER_LIST)
					break;
			}
			if (c == EOF)
				break;	//just in case an EOF follows a '\r'
		}
		else
		{
			if (len + 1 < sizeof(line))
				line[len++] = c;
		}
	}

	fclose (f);
}

/*
void ServerBrowser_Save (FILE *f)
{
	int	i;

	for (i=0 ; i<MAX_SERVER_LIST ; i++)
	{
		if (!server_browser_list[i].server)
			break;

		fprintf (f, "%s\t%s\n", server_browser_list[i].server, server_browser_list[i].description);
	}
}
*/

#include <windows.h>

static	HANDLE	hServerBrowserProcess = NULL;

HANDLE Sys_Process_Run (char *cmdline, const char *description);
int Sys_Process_IsStillRunning (HANDLE processhandle);

/*
==================
HTTP_ServerBrowser_Begin_Query

Initiate a server browser HTTP query
==================
*/

void HTTP_ServerBrowser_Begin_Query (void)
{
	char cmdline[512]; // Store the command line

	remove (SERVER_LIST_NAME);

	if (hServerBrowserProcess)
	{
		Con_Printf ("Warning: Serverget process already running\n");
		return;
	}

	Con_Printf("%s\nRetrieving server info via HTTP\n%s", Con_Quakebar(36), Con_Quakebar(36));

	// Build our command line, which is the ProQuake command line + " -server_update"
	SNPrintf  (cmdline, sizeof(cmdline), "%s -server_update", argv[0]);

	// Initiate the process; store the handle
	hServerBrowserProcess = Sys_Process_Run (cmdline, "server browser"); // Param 2 describes process purpose
	if (!hServerBrowserProcess)
	{
		Con_Printf("ERROR: Unable to start server browser process\n");  // Error: couldn't start!
		return;
	}

	// Set client server download information
	cls.serverbrowser = true;
	cls.serverbrowser_starttime = Sys_FloatTime();
	cls.serverbrowser_nextchecktime = cls.serverbrowser_starttime + 1.0f; // Check again in 1 second
	cls.serverbrowser_lastsuccesstime = 0; // No valid success time
	cls.serverbrowser_state = http_initiated;  // value = 0

	// Wipe the server browser info
	ServerBrowser_Reset_All();
}


void HTTP_ServerBrowser_Check_Query_Completion (void)
{
	int			ExitResult;
	qboolean	success = false;

	// Limit checking to once every 0.5 to 3 seconds or so
	if (Sys_FloatTime() <= cls.serverbrowser_nextchecktime)
		return;

	// Reset next check time because we should set this below
	cls.serverbrowser_nextchecktime = 0;

	// If we are here, we are checking the state and setting nextchecktime too
	// These states are where we WERE last check; now set to what we want them to be

	switch (cls.serverbrowser_state)
	{
	case http_initiated:
	case http_disconnecting: // Not relevant to server list process

		cls.serverbrowser_state = http_fakeprogress;
		cls.serverbrowser_nextchecktime = Sys_FloatTime() + 0.3f;
		break;

	case http_fakeprogress:
	default:
		// We'll check for completion every 1.0 seconds

		cls.serverbrowser_nextchecktime = Sys_FloatTime() + 0.3f;
		break;

	case http_reconnecting:		// Not applicable here
	case http_notreconnecting:	// Not applicable here
	case http_error:
	case http_timeout:
	case http_success:
		// These are the 5 "finished" messages
		// And we just wanted to give them 3 seconds on-screen so they could be read
		// They have now been displayed: clear the message
		Con_DPrintf("Serverlist download cleared\n");

		cls.serverbrowser = false;
		cls.serverbrowser_nextchecktime = 0; // We won't be checking again
		return; // Nothing else to do
	}

	// If we are here, we are always here to check for completion
	// Regardless of state we are here to check on progress

	ExitResult = Sys_Process_IsStillRunning (hServerBrowserProcess);

	if (ExitResult == 1)
	{
		// ***************************************
		// STILL RUNNING
		// ***************************************
		cls.serverbrowser_nextchecktime = Sys_FloatTime() + 0.5f;
		Sys_Sleep (); // Sleep some to speed it up (Does it help?  Not sure.  But we can't finish until it's done)
		return;
	}

	// ****************************************
	// NOT STILL RUNNING - IT'S DONE (OR ERROR)
	// ****************************************

	hServerBrowserProcess = NULL; // <--------- No matter what, it's done

	if (ExitResult == -1)
	{
		cls.serverbrowser_state = http_error;							// Some sort of error happened
		cls.serverbrowser_nextchecktime = Sys_FloatTime() + 3.0f;		// Display it for 3 seconds
		return;
	}

	cls.serverbrowser_starttime = (Sys_FloatTime() - cls.serverbrowser_starttime); // Set the time = to the duration

	Con_DPrintf ("Debug: checking existence of \"%s\"\n", SERVER_LIST_NAME);
	if (Sys_FileTime(SERVER_LIST_NAME) == -1)
	{
		// ***************************************
		// FAILURE - AN ERROR HAPPENED
		// ***************************************
		Con_Printf ("FAILURE: Mapget failed to retrieve serverlist\n");

		// We should be checking for and displaying the reason, but we aren't
		// FIXME: code that!
		cls.serverbrowser_starttime = http_error;  // Error message
		cls.serverbrowser_nextchecktime = Sys_FloatTime() + 3.0f;  // Leave it for 3 seconds
		return;
	}

	// ***************************************
	// SUCCESS
	// ***************************************

	// Apparent success
	Con_DPrintf ("Serverlist obtained in %2.2f seconds\n", cls.serverbrowser_starttime);

	cls.serverbrowser_state = http_success;
	cls.serverbrowser_starttime = http_reconnecting;
	cls.serverbrowser_nextchecktime = Sys_FloatTime() + 0.02f; // Give chance for server browser to display success msg?
	cls.serverbrowser_lastsuccesstime = Sys_FloatTime(); // Successful time = now

	ServerBrowser_Load(); // Load our new server list

	Con_Printf ("Successfully retrieved server list\n");
}


// EXAMPLE PARSE:                 12_1234567890123456789_12345_123456_12
// quake.someserver.com:26000<tab>US Server Title        01/16 dm6    DM
// <server><tab>2_19_5_6_2 <--- sort on # players

/*
	<!-- server_start-->
	<!-- serveraddr: quake.someserver.com:26000-->
	<!-- servername: Remake Quake EU-->
	<!-- players:    00-->
	<!-- maxplayers: 00-->
	<!-- map:        &nbsp;-->
	<!-- geo:        eu-->
	<!-- mod:        RM-->
	<!-- server_end-->
*/

#define NUM_SFIELDS_7 7
#define START_MARK "<!-- server_start-->"
#define END_MARK "<!-- server_end-->"
char *mappings[] = {
	"<!-- serveraddr:",		// 0
	"<!-- servername:",		// 1
	"<!-- players:",		// 2
	"<!-- maxplayers:",		// 3
	"<!-- map:",			// 4
	"<!-- geo:",			// 5
	"<!-- mod:",			// 6
};


int Server_Browser_Util_Write_Servers (const char* buffer_in, int bufsize, const char* filename)
{
	typedef struct
	{
		qboolean	printed;
		char		address[64];
		char		geo[3];
		char		name[20];		// 19 + 1
		int			players;
		int			maxplayers;
		char		mapname[7];		// 6 + 1
		char		modcode[3];		// 2 + 1
	} server_t;

	server_t servers[MAX_SERVER_LIST];
	int numservers = 0;
	int i, j, k, n;

	// Make sure buffer_in is null terminated?
	{
		qboolean has_an_errors = false;
		char fields[NUM_SFIELDS_7][64];

		char* copybuf = Q_calloc (1, bufsize + 1);
		const char* cursor = copybuf;
		memcpy (copybuf, buffer_in, bufsize);

		// Find first occurance of "<!-- server_start-->"
		// Find next occurance of "<!-- server_end-->"
		while ( (cursor = strstr (cursor, START_MARK)) && numservers < MAX_SERVER_LIST)
		{
			if (strstr(cursor, END_MARK) == NULL)
			{
				has_an_errors = true;
				break; // Something malformed?
			}

			// We should have all the components
			// memset fields to 0.
			memset (fields, 0, sizeof(fields) );

			// Search for each of the mappings, copy to fields.
			for (n = 0; n < NUM_SFIELDS_7; n ++)
			{
				const char* begin_string = strstr (cursor, mappings[n]);
				const char* begin = begin_string ? begin_string + strlen(mappings[n]) : NULL;
				const char* end = begin ? strstr (begin, "-->") : NULL;
				int len;
				if (end == NULL)
				{
					has_an_errors = true;
					break;
				}

				// Advance begin beyond leading white space
				while (*begin && *begin <= 32)
					begin ++;

				len = end - begin;

				// We should be ok to parse
				if (len < 64 && len > 0)
					strncpy (&fields[n][0], begin, len);
				else
				{
					has_an_errors = true;
					break;
				}
			}
			if (has_an_errors) // Something went bad in parsing?
				break;

			// All fields should be populated ... now translate
			servers[numservers].maxplayers = atoi (&fields[3][0]);
			if (servers[numservers].maxplayers) // If 0 maxplayers, server is down so ignore
			{
				server_t* cur = &servers[numservers];

				strlcpy (cur->address, &fields[0][0], sizeof(cur->address));
				strlcpy (cur->geo, &fields[5][0], sizeof(cur->geo));
				strlcpy (cur->name, &fields[1][0], sizeof(cur->name));
				cur->players = atoi(&fields[2][0]);
				// cur->maxplayers = atoi(&fields[3][0]); // We already did
				strlcpy (cur->mapname , &fields[4][0], sizeof(cur->mapname));
				strlcpy (cur->modcode, &fields[6][0], sizeof(cur->modcode));

				numservers ++;
			}

			cursor += strlen(START_MARK);
		} // End of while

		numservers = numservers;
		free (copybuf);

		if (has_an_errors) // Something went bad in parsing?
			return 1; // Parse problem

	}

	// Print in order specified (geo -> # players)
	// Write fields to disk  fprintf ("%s\t%s %s %02i/%02i %s %s", cur->address, cur->name, cur->players, cur->maxplayers
	{
		FILE *f;
		f = fopen (filename, "wt");

		if (f)
		{
			char *countries[] = {"us", "eu", "br", NULL};
			for (i = 16; i >= 0; i --) // Number of players pass, conceivable we may increase scoreboard max to 64
			{
				for (k = 0; k < 4; k ++) // Geo pass 0 = US, 1 = EU, 2 = BR, 3 = anything left
				{
					const char* geo_pass = countries[k];

					// Run through every server
					for (j = 0; j < numservers; j ++)
					{
						server_t* cur = &servers[j];

						if (cur->printed) // Server already printed
							continue;

						if (cur->players != i) // Server doesn't have right # players for this pass
							continue;

						if (geo_pass && strcasecmp (cur->geo, geo_pass) != 0) // Pass has geo code and it doesn't match
							continue;

						fprintf (f, "%s\t%-2s %-19s %02i/%02i %-6s %-2s\n", cur->address, cur->geo, cur->name, cur->players, cur->maxplayers, cur->mapname, cur->modcode);

						cur->printed = true;
					}
					// End servers
				} // End geo criteria

			} // Players count

			fclose (f);
			return 0;  // Wrote file successfully
		}
	}

	return 2;  // If here, couldn't write the file
}


// Baker: From a libcurl tutorial: http://curl.haxx.se/libcurl/c/simple.html

#include "stats.h"
#include "curl.h"

struct MemoryStruct
{
  char *memory;
  size_t size;
};


static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *)userp;

	mem->memory = realloc(mem->memory, mem->size + realsize + 1);
	if (mem->memory == NULL)
	{
		// out of memory
		printf ("not enough memory (realloc returned NULL)\n");
		exit (EXIT_FAILURE);
	}

	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}


char* Server_Browser_Util_Get_Page_Alloc (const char** bufferin, const char* my_site, int* bytes)
{
	CURL *curl_handle;
	char *my_user_agent = "libcurl-agent/1.0";

	struct MemoryStruct chunk;

	chunk.memory = malloc(1);  /* will be grown as needed by the realloc above */
	chunk.size = 0;    /* no data at this point */

	curl_global_init(CURL_GLOBAL_ALL);

	/* init the curl session */
	curl_handle = curl_easy_init();

	/* specify URL to get */
	curl_easy_setopt(curl_handle, CURLOPT_URL, /*"http://www.example.com/"*/ my_site);

	/* send all data to this function  */
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

	/* we pass our 'chunk' struct to the callback function */
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);

	/* some servers don't like requests that are made without a user-agent
	 field, so we provide one */
	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, my_user_agent);

	/* get it! */
	curl_easy_perform(curl_handle);

	/* cleanup curl stuff */
	curl_easy_cleanup(curl_handle);

	/*
	* Now, our chunk.memory points to a memory block that is chunk.size
	* bytes big and contains the remote file.
	*
	* Do something nice with it!
	*
	* You should be aware of the fact that at this point we might have an
	* allocated data block, and nothing has yet deallocated that data. So when
	* you're done with it, you should free() it as a nice application.
	*/

	//  printf("%lu bytes retrieved\n", (long)chunk.size);

	/* we're done with libcurl, so clean it up */
	curl_global_cleanup();

	if (chunk.memory)
	{
		char *buffer = calloc (1,	chunk.size + 1);
		memcpy (buffer, chunk.memory, chunk.size);
		free(chunk.memory);

		*bufferin =  buffer;
		*bytes = chunk.size;
		return NULL;
	}

	*bytes = 0;
	*bufferin = NULL;

	return NULL;

}


