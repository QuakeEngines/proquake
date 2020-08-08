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
// gl_mesh.c: triangle model functions

#include "quakedef.h"

extern	stvert_t	stverts[MAXALIASVERTS];
extern	mtriangle_t	triangles[MAXALIASTRIS];
extern	trivertx_t	*poseverts[MAXALIASFRAMES];

/*
=================================================================

ALIAS MODEL DISPLAY LIST GENERATION

=================================================================
*/


static qboolean	used[8192];

// the command list holds counts and s/t values that are valid for
// every frame
static int		commands[8192];
static int		numcommands;

// all frames will have their vertexes rearranged and expanded
// so they are in the order expected by the command list
static int		vertexorder[8192];
static int		numorder;

static int		allverts, alltris;

static int		stripverts[128];
static int		striptris[128];
static int		stripcount;

/*
================
StripLength
================
*/
static int StripLength (const aliashdr_t *paliashdr_strip, int starttri, int startv)
{
	int		m1, m2, j, k;
	mtriangle_t	*last, *check;

	used[starttri] = 2;

	last = &triangles[starttri];

	stripverts[0] = last->vertindex[(startv+0)%3];
	stripverts[1] = last->vertindex[(startv+1)%3];
	stripverts[2] = last->vertindex[(startv+2)%3];

	striptris[0] = starttri;
	stripcount = 1;

	m1 = last->vertindex[(startv+2)%3];
	m2 = last->vertindex[(startv+1)%3];

	// look for a matching triangle
nexttri:
	for (j=starttri+1, check=&triangles[starttri+1] ; j<paliashdr_strip->numtris ; j++, check++)
	{
		if (check->facesfront != last->facesfront)
			continue;

		for (k=0 ; k<3 ; k++)
		{
			if (check->vertindex[k] != m1)
				continue;
			if (check->vertindex[ (k+1)%3 ] != m2)
				continue;

			// this is the next part of the fan

			// if we can't use this triangle, this tristrip is done
			if (used[j])
				goto done;

			// the new edge
			if (stripcount & 1)
				m2 = check->vertindex[ (k+2)%3 ];
			else
				m1 = check->vertindex[ (k+2)%3 ];

			stripverts[stripcount+2] = check->vertindex[ (k+2)%3 ];
			striptris[stripcount] = j;
			stripcount++;

			used[j] = 2;
			goto nexttri;
		}
	}

done:
	// clear the temp used flags
	for (j=starttri+1 ; j<paliashdr_strip->numtris ; j++)
		if (used[j] == 2)
			used[j] = 0;

	return stripcount;
}

/*
===========
FanLength
===========
*/
static int FanLength (const aliashdr_t *paliashdr_fan, int starttri, int startv)
{
	int		m1, m2, j, k;
	mtriangle_t	*last, *check;

	used[starttri] = 2;

	last = &triangles[starttri];

	stripverts[0] = last->vertindex[(startv+0)%3];
	stripverts[1] = last->vertindex[(startv+1)%3];
	stripverts[2] = last->vertindex[(startv+2)%3];

	striptris[0] = starttri;
	stripcount = 1;

	m1 = last->vertindex[(startv+0)%3];
	m2 = last->vertindex[(startv+2)%3];

	// look for a matching triangle
nexttri:
	for (j=starttri+1, check=&triangles[starttri+1] ; j<paliashdr_fan->numtris ; j++, check++)
	{
		if (check->facesfront != last->facesfront)
			continue;

		for (k=0 ; k<3 ; k++)
		{
			if (check->vertindex[k] != m1)
				continue;
			if (check->vertindex[ (k+1)%3 ] != m2)
				continue;

			// this is the next part of the fan

			// if we can't use this triangle, this tristrip is done
			if (used[j])
				goto done;

			// the new edge
			m2 = check->vertindex[ (k+2)%3 ];

			stripverts[stripcount+2] = m2;
			striptris[stripcount] = j;
			stripcount++;

			used[j] = 2;
			goto nexttri;
		}
	}

done:
	// clear the temp used flags
	for (j=starttri+1 ; j<paliashdr_fan->numtris ; j++)
		if (used[j] == 2)
			used[j] = 0;

	return stripcount;
}

/*
================
BuildTris

Generate a list of trifans or strips for the model, which holds for all frames
================
*/
static void BuildTris (const aliashdr_t *paliashdr_tris)
{
	int		i, j, k, startv, len, bestlen, besttype, type, bestverts[1024], besttris[1024];
	float		s, t;

	// build tristrips
	numorder = 0;
	numcommands = 0;
	memset (used, 0, sizeof(used));
	for (i=0 ; i<paliashdr_tris->numtris ; i++)
	{
		// pick an unused triangle and start the trifan
		if (used[i])
			continue;

		bestlen = 0;
		for (type = 0 ; type < 2 ; type++)
//	type = 1;
		{
			for (startv =0 ; startv < 3 ; startv++)
			{
				len = (type == 1) ? StripLength (paliashdr_tris, i, startv) : FanLength (paliashdr_tris, i, startv);
				if (len > bestlen)
				{
					besttype = type;
					bestlen = len;
					for (j=0 ; j<bestlen+2 ; j++)
						bestverts[j] = stripverts[j];
					for (j=0 ; j<bestlen ; j++)
						besttris[j] = striptris[j];
				}
			}
		}

		// mark the tris on the best strip as used
		for (j=0 ; j<bestlen ; j++)
			used[besttris[j]] = 1;

		commands[numcommands++] = (besttype == 1) ? (bestlen+2) : -(bestlen+2);

		for (j=0 ; j<bestlen+2 ; j++)
		{
			// emit a vertex into the reorder buffer
			k = bestverts[j];
			vertexorder[numorder++] = k;

			// emit s/t coords into the commands stream
			s = stverts[k].s;
			t = stverts[k].t;
			if (!triangles[besttris[0]].facesfront && stverts[k].onseam)
				s += paliashdr_tris->skinwidth / 2;	// on back side
			s = (s + 0.5) / paliashdr_tris->skinwidth;
			t = (t + 0.5) / paliashdr_tris->skinheight;

			*(float *)&commands[numcommands++] = s;
			*(float *)&commands[numcommands++] = t;
		}
	}

	commands[numcommands++] = 0;		// end of list marker

	Con_DPrintf ("%3i tri %3i vert %3i cmd\n", paliashdr_tris->numtris, numorder, numcommands);

	allverts += numorder;
	alltris += paliashdr_tris->numtris;
}


void GL_MakeAliasModelDisplayLists (aliashdr_t *paliashdr) 
{
	int		i, j, *cmds;
	trivertx_t	*verts;

	// Tonik: don't cache anything, because it seems just as fast
	// (if not faster) to rebuild the tris instead of loading them from disk
		BuildTris (paliashdr);		// trifans or lists

	// save the data out
	paliashdr->poseverts = numorder;

	cmds = Hunk_Alloc (numcommands * 4);
	paliashdr->commands = (byte *)cmds - (byte *)paliashdr;
	memcpy (cmds, commands, numcommands * 4);

	verts = Hunk_Alloc (paliashdr->numposes * paliashdr->poseverts * sizeof(trivertx_t) );
	paliashdr->posedata = (byte *)verts - (byte *)paliashdr;
	for (i=0 ; i<paliashdr->numposes ; i++)
		for (j=0 ; j<numorder ; j++)
			*verts++ = poseverts[i][vertexorder[j]];
}

