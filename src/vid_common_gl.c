/*
Copyright (C) 2002-2003 A Nourai

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
// vid_common_gl.c -- Common code for vid_wgl.c and vid_glx.c

#include "quakedef.h"
const char *gl_vendor;
const char *gl_renderer;
const char *gl_version;
const char *gl_extensions;
qboolean gl_mtexable = false;

#ifdef MACOSX
// Baker: in windows this is in gl_vidnt.c
int	texture_mode = GL_LINEAR;
#endif // Move to vid_glcommon.c  This does not belong here.
//#endif // Commenting this out to generate compiler error if I do not fix location.

float		gldepthmin, gldepthmax;


//PROC glArrayElementEXT;
//PROC glColorPointerEXT;
//PROC glTexCoordPointerEXT;
//PROC glVertexPointerEXT;

float vid_gamma = 1.0;
byte		vid_gamma_table[256];

unsigned	d_8to24table[256];
unsigned	d_8to24table2[256];
//unsigned short	d_8to16table[256]; //johnfitz -- never used

#define TEXTURE_EXT_STRING "GL_EXT_texture_object"


//qboolean isPermedia = false;

byte		color_white[4] = {255, 255, 255, 0};
byte		color_black[4] = {0, 0, 0, 0};
extern qboolean	fullsbardraw;
qboolean CheckExtension (const char *extension)
{
	char		*where, *terminator;
	const	char	*start;

	if (!gl_extensions && !(gl_extensions = glGetString(GL_EXTENSIONS)))
		return false;

	if (!extension || *extension == 0 || strchr(extension, ' '))
		return false;

	for (start = gl_extensions ; where = strstr(start, extension) ; start = terminator)
	{
		terminator = where + strlen(extension);
		if ((where == start || *(where - 1) == ' ') && (*terminator == 0 || *terminator == ' '))
			return true;
	}

	return false;
}

int		texture_mode = GL_LINEAR;
int		texture_extension_number = 1;

lpMTexFUNC		qglMultiTexCoord2f = NULL;
lpSelTexFUNC	qglActiveTexture = NULL;


void CheckMultiTextureExtensions(void)
{
	qboolean arb_mtex;

	if (COM_CheckParm("-nomtex"))
	{
		Con_Warning ("Multitexture disabled at command line\n");
		return;
	}

	arb_mtex = strstr (gl_extensions, "GL_ARB_multitexture ") != NULL;

	if (!arb_mtex) 
	{
		Con_Warning ("Multitexture extension not found\n");
		return;
	}

	qglMultiTexCoord2f	= (void *) wglGetProcAddress ("glMultiTexCoord2fARB");
	qglActiveTexture	= (void *) wglGetProcAddress ("glActiveTextureARB");

//	currenttarget = TEXTURE0_ARB;

	Con_Printf ("GL_%s_multitexture extensions found: ARB\n");
	gl_mtexable = true;
}





/*
===============
GL_SetupState -- johnfitz

does all the stuff from GL_Init that needs to be done every time a new GL render context is created
GL_Init will still do the stuff that only needs to be done once
===============
*/
void GL_SetupState (void) 
{
	glClearColor (0.15,0.15,0.15,0);  // Baker 3.60 - set to same clear color as FitzQuake for void areas
	glCullFace(GL_FRONT);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.666);
	// Get rid of Z-fighting for textures by offsetting the
	// drawing of entity models compared to normal polygons.
	// (Only works if gl_ztrick is turned off)

	// Baker: d3dquake no support this
	glPolygonOffset(0.05, 0);

	glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	glShadeModel (GL_FLAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}


/*
===============
GL_Init
===============
*/
qboolean IntelDisplayAdapter = false;
void GL_Init (void) 
{
	// Initialization gets GL information, extensions
	gl_vendor = glGetString (GL_VENDOR);
	Con_Printf ("GL_VENDOR: %s\n", gl_vendor);

	gl_renderer = glGetString (GL_RENDERER);
	Con_Printf ("GL_RENDERER: %s\n", gl_renderer);

	gl_version = glGetString (GL_VERSION);
	Con_Printf ("GL_VERSION: %s\n", gl_version);
	gl_extensions = glGetString (GL_EXTENSIONS);

	if (strncasecmp(gl_renderer, "Intel", 5) == 0) 
	{
		Con_Printf ("Intel Display Adapter detected\n");
		IntelDisplayAdapter = true;
		Cvar_Set ("gl_ztrick", "0");
		Cvar_Set ("gl_clear", "1");
//		Con_Printf ("values are %s and %s", gl_ztrick.value, gl_clear.value);
	}



//	CheckTextureExtensions ();
	CheckMultiTextureExtensions ();

	GL_SetupState (); //johnfitz
}

void GL_PrintExtensions_f(void) 
{
	Con_Printf ("GL_EXTENSIONS: %s\n", gl_extensions);
}


void VID_SetPaletteOld (unsigned char *palette)
{
	byte	*pal = palette;
	unsigned r,g,b, v;
	int i;
	unsigned *table = d_8to24table;

// 8 8 8 encoding

	pal = palette;
	table = d_8to24table;
	for (i = 0 ; i < 256 ; i ++, pal += 3)
	{
		r = pal[0];
		g = pal[1];
		b = pal[2];
		
		v = (255 << 24) + (r << 0) + (g << 8) + (b << 16);
		*table++ = v;
	}

	d_8to24table[255] = 0;	// 255 is transparent "MH: says this fixes pink edges"
	//d_8to24table[255] &= 0xffffff;	// 255 is transparent
}

BOOL	gammaworks;
void	VID_ShiftPaletteOld (unsigned char *palette)
{
	extern	byte rampsold[3][256];
//	VID_SetPaletteOld (palette);
//	gammaworks = SetDeviceGammaRamp (maindc, ramps);
}

//BINDTEXFUNCPTR bindTexFunc;
void VID_SetPalette (unsigned char *palette)
{
	byte		*pal;
	int		i;
	unsigned	r, g, b, *table;
// 8 8 8 encoding
	pal = palette;
	table = d_8to24table;
	for (i=0 ; i<256 ; i++) 
	{
		r = pal[0];
		g = pal[1];
		b = pal[2];
		pal += 3;
		*table++ = (255<<24) + (r<<0) + (g<<8) + (b<<16);
	}
	d_8to24table[255] = 0;	// 255 is transparent
// Tonik: create a brighter palette for bmodel textures
	pal = palette;
	table = d_8to24table2;
	for (i=0 ; i<256 ; i++) 
	{
		r = min(pal[0] * (2.0 / 1.5), 255);
		g = min(pal[1] * (2.0 / 1.5), 255);
		b = min(pal[2] * (2.0 / 1.5), 255);
		pal += 3;
		*table++ = (255<<24) + (r<<0) + (g<<8) + (b<<16);
	}
	d_8to24table2[255] = 0;	// 255 is transparent
}



void Check_GammaOld (unsigned char *pal)
{
	float	f, inf;
	unsigned char	palette[768];
	int		pnum, i;
	float	val;
	
	vid_gamma = 0.7; // Default
	if ( (pnum = COM_CheckParm("-gamma")) && pnum + 1 < com_argc && (val = atoi(com_argv[pnum + 1])) )
		vid_gamma = val;

	vid_gamma = CLAMP (0.3, vid_gamma, 1);

	for (i=0 ; i<768 ; i++)
	{
		f = pow ( (pal[i] + 1) / 256.0 , vid_gamma);
		inf = f * 255 + 0.5;
		inf = CLAMP (0, inf, 255);
		palette[i] = inf;
	}
	memcpy (pal, palette, sizeof(palette));
	BuildGammaTable(vid_gamma);
}
