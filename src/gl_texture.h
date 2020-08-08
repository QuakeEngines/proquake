/*

Copyright (C) 2001-2002       A Nourai

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the included (GNU.txt) GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// gl_texture.h

// Baker ...
/*
Basically texture related functions and variables go here
What doesn't go here is general OpenGL stuff.

And Cvars really shouldn't go here except in the
most exception of circumstances, right?
*/



// GL constants and API stuffs (not internal vars)

#ifndef GL_TEXTURE_H
#define GL_TEXTURE_H


#define	MAX_GLTEXTURES	1024

// Multitexture

//multitexturing
#define	GL_TEXTURE0_ARB 		0x84C0
#define	GL_TEXTURE1_ARB 		0x84C1
#define	GL_TEXTURE2_ARB 		0x84C2
#define	GL_TEXTURE3_ARB 		0x84C3
#define GL_MAX_TEXTURE_UNITS_ARB	0x84E2

// GL external vars, functions
typedef void (APIENTRY *lpMTexFUNC)(GLenum, GLfloat, GLfloat);
typedef void (APIENTRY *lpSelTexFUNC)(GLenum);

extern lpMTexFUNC qglMultiTexCoord2f;
extern lpSelTexFUNC qglActiveTexture;


#define GL_MAX_TEXTURE_UNITS_ARB	0x84E2

// Engine internal vars

extern qboolean gl_mtexable;
extern	int		gl_max_size;


extern	int texture_extension_number;

extern	texture_t	*r_notexture_mip;
extern	int		d_lightstylevalue[256];	// 8.8 fraction of base light value

extern	qboolean	envmap;
extern	int	cnttextures[2];
extern	int	current_texture_num;
extern	int	particletexture;
extern	int	playertextures;

extern	int	skytexturenum;		// index in cl.loadmodel, not gl texture object

extern	int			mirrortexturenum;	// quake texturenum, not gltexturenum
extern	qboolean	mirror;
extern	mplane_t	*mirror_plane;

extern	int		texture_mode;
extern	int		gl_lightmap_format;


// Engine internal functions

void GL_Bind (int texnum);

void GL_SelectTexture (GLenum target);
void GL_DisableMultitexture(void);
void GL_EnableMultitexture(void);




void GL_Upload32 (unsigned *data, int width, int height, int mode);
void GL_Upload8 (byte *data, int width, int height, int mode);
int GL_LoadTexture (char *identifier, int width, int height, byte *data, int mode);
int GL_FindTexture (char *identifier);
int GL_LoadTexture32 (char *identifier, int width, int height, byte *data, int mode);

#endif