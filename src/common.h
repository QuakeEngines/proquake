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
// common.h  -- general definitions


#ifndef __COMMON_H__
#define __COMMON_H__

#if !defined BYTE_DEFINED
typedef unsigned char 		byte;
#define BYTE_DEFINED 1
#endif

#undef true
#undef false

typedef enum {false, true}	qboolean;



// Baker: these aren't used on MSVC6 (part of stdlib.h)
#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif


#define CLAMP(a, b, c) ((a) >= (c) ? (a) : (b) < (a) ? (a) : (b) > (c) ? (c) : (b))


typedef struct sizebuf_s
{
	qboolean	allowoverflow;	// if false, do a Sys_Error
	qboolean	overflowed;		// set to true if the buffer size failed
	byte	*data;
	int		maxsize;
	int		cursize;
} sizebuf_t;

void SZ_Alloc (sizebuf_t *buf, int startsize);
void SZ_Free (sizebuf_t *buf);
void SZ_Clear (sizebuf_t *buf);
void *SZ_GetSpace (sizebuf_t *buf, int length);
void SZ_Write (sizebuf_t *buf, void *data, int length);
void SZ_Print (sizebuf_t *buf, char *data);	// strcats onto the sizebuf

//============================================================================

typedef struct link_s
{
	struct link_s	*prev, *next;
} link_t;


void ClearLink (link_t *l);
void RemoveLink (link_t *l);
void InsertLinkBefore (link_t *l, link_t *before);
void InsertLinkAfter (link_t *l, link_t *after);

// (type *)STRUCT_FROM_LINK(link_t *link, type, member)
// ent = STRUCT_FROM_LINK(link,entity_t,order)
// FIXME: remove this mess!
#define	STRUCT_FROM_LINK(l,t,m) ((t *)((byte *)l - (int)&(((t *)0)->m)))

//============================================================================

#ifndef NULL
#define NULL ((void *)0)
#endif

#define Q_MAXCHAR ((char)0x7f)
#define Q_MAXSHORT ((short)0x7fff)
#define Q_MAXINT	((int)0x7fffffff)
#define Q_MAXLONG ((int)0x7fffffff)
#define Q_MAXFLOAT ((int)0x7fffffff)

#define Q_MINCHAR ((char)0x80)
#define Q_MINSHORT ((short)0x8000)
#define Q_MININT 	((int)0x80000000)
#define Q_MINLONG ((int)0x80000000)
#define Q_MINFLOAT ((int)0x7fffffff)

//============================================================================

extern	qboolean		bigendien;

extern	short	(*BigShort) (short l);
extern	short	(*LittleShort) (short l);
extern	int	(*BigLong) (int l);
extern	int	(*LittleLong) (int l);
extern	float	(*BigFloat) (float l);
extern	float	(*LittleFloat) (float l);

//============================================================================

void MSG_WriteChar (sizebuf_t *sb, int c);
void MSG_WriteByte (sizebuf_t *sb, int c);
void MSG_WriteShort (sizebuf_t *sb, int c);
void MSG_WriteLong (sizebuf_t *sb, int c);
void MSG_WriteFloat (sizebuf_t *sb, float f);
void MSG_WriteString (sizebuf_t *sb, char *s);
void MSG_WriteCoord (sizebuf_t *sb, float f);
void MSG_WriteAngle (sizebuf_t *sb, float f);
void MSG_WritePreciseAngle (sizebuf_t *sb, float f); // JPG - precise aim!!

extern	int			msg_readcount;
extern	qboolean	msg_badread;		// set if a read goes beyond end of message

void MSG_BeginReading (void);
int MSG_ReadChar (void);
int MSG_ReadByte (void);
int MSG_PeekByte (void); // JPG - need this to check for ProQuake messages
int MSG_ReadShort (void);
int MSG_ReadLong (void);
float MSG_ReadFloat (void);
char *MSG_ReadString (void);

float MSG_ReadCoord (void);
float MSG_ReadAngle (void);

float MSG_ReadPreciseAngle (void); // JPG - precise aim!!





//============================================================================

extern	char		com_token[1024];
extern	qboolean	com_eof;

char *COM_Parse (char *data);


extern	int		com_argc;
extern	char	**com_argv;

int COM_CheckParm (char *parm);
void COM_Init (char *path);
void COM_InitArgv (int argc, char **argv);

char *COM_SkipPath (char *pathname);
void COM_StripExtension (char *in, char *out);
char *COM_FileExtension (char *in);
void COM_FileBase (char *in, char *out);
void COM_DefaultExtension (char *path, char *extension);

char *va (const char *format, ...);
// does a varargs printf into a temp buffer

char *CopyString (char *s);
void COM_SlashesForward_Like_Unix (char *WindowsStylePath);
void COM_Reduce_To_Parent_Path (char* myPath);
char *COM_NiceFloatString (float floatvalue);

//============================================================================

extern int com_filesize;
extern char	com_basedir[MAX_OSPATH];


struct cache_user_s;

extern	char	com_gamedir[MAX_OSPATH];

void COM_ForceExtension (char *path, char *extension);	// by joe


void COM_WriteFile (char *filename, void *data, int len);


void COM_CreatePath (char *path);

int COM_OpenFile (char *filename, int *hndl);
int COM_FOpenFile (char *filename, FILE **file);
void COM_CloseFile (int h);

byte *COM_LoadStackFile (char *path, void *buffer, int bufsize);
byte *COM_LoadTempFile (char *path);
byte *COM_LoadHunkFile (char *path);
void COM_LoadCacheFile (char *path, struct cache_user_s *cu);
#ifdef HTTP_DOWNLOAD

void COM_GetFolder (char *in, char *out);//R00k

#endif


// Misc
char *COM_Quakebar (int len);
int COM_Minutes (int seconds);
int COM_Seconds (int seconds);
char *VersionString (void);
void Host_Version_f (void);

extern struct cvar_s	registered;
extern struct cvar_s	cmdline;


extern qboolean		standard_quake, rogue, hipnotic;
//extern qboolean		mod_deeplair, mod_conhide,  mod_nosoundwarn;






#if defined(_WIN32) && defined(_MSC_VER)

// vc++ snprintf and vsnprintf are non-standard and not compatible with C99.
size_t SNPrintf		(char *str,    const size_t n,     const char *format, ...);
size_t VSNPrintf	(char *buffer, const size_t count, const char *format, va_list argptr);

// MSVC has a different name for several standard functions

#define strcasecmp stricmp
#define strncasecmp strnicmp

#else

#define SNPrintf snprintf
#define VSNPrintf vsnprintf


#endif // GNUC even on Windows has this you know ...


// strlcat and strlcpy, from OpenBSD
// Most (all?) BSDs already have them
#if defined(__OpenBSD__) || defined(__NetBSD__) || defined(__FreeBSD__) || defined(MACOSX)
# define HAVE_STRLCAT 1
# define HAVE_STRLCPY 1
#endif

#ifndef HAVE_STRLCAT
/*!
 * Appends src to string dst of size siz (unlike strncat, siz is the
 * full size of dst, not space left).  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz <= strlen(dst)).
 * Returns strlen(src) + MIN(siz, strlen(initial dst)).
 * If retval >= siz, truncation occurred.
 */
size_t strlcat(char *dst, const char *src, size_t siz);
#endif  // #ifndef HAVE_STRLCAT

#ifndef HAVE_STRLCPY
/*!
 * Copy src to string dst of size siz.  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz == 0).
 * Returns strlen(src); if retval >= siz, truncation occurred.
 */
size_t strlcpy(char *dst, const char *src, size_t siz);

#endif  // #ifndef HAVE_STRLCPY
char *strltrim(char *s);
//============================================================================

#endif // __COMMON_H__
