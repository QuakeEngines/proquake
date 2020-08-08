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
// view.c -- player eye positioning

#include "quakedef.h"
/*

The view is allowed to move slightly from it's true position for bobbing,
but if it exceeds 8 pixels linear distance (spherical, not box), the list of
entities sent from the server may not include everything in the pvs, especially
when crossing a water boudnary.

*/

#ifdef MACOSX
qboolean	qMinimized;
#endif

cvar_t		lcd_x = {"lcd_x","0"};
cvar_t		lcd_yaw = {"lcd_yaw","0"};

cvar_t	scr_ofsx = {"scr_ofsx","0", false};
cvar_t	scr_ofsy = {"scr_ofsy","0", false};
cvar_t	scr_ofsz = {"scr_ofsz","0", false};



cvar_t	cl_rollspeed = {"cl_rollspeed", "200", true};
cvar_t	cl_rollangle = {"cl_rollangle", "0", true}; // Quake classic default = 2.0

cvar_t	cl_bob = {"cl_bob","0", true}; // Quake classic default = 0.02
cvar_t	cl_bobcycle = {"cl_bobcycle","0.6", false}; // Leave it
cvar_t	cl_bobup = {"cl_bobup","0", false}; // Quake classic default is 0.5

cvar_t	v_kicktime = {"v_kicktime", "0", true};  //"0.5", true};  // Baker 3.80x - Save to config
cvar_t	v_kickroll = {"v_kickroll", "0", true};  //"0.6", true};  // Baker 3.80x - Save to config
cvar_t	v_kickpitch = {"v_kickpitch", "0", true};  //"0.6", true};  // Baker 3.80x - Save to config
cvar_t	v_gunkick = {"v_gunkick", "0", true};

cvar_t	v_iyaw_cycle = {"v_iyaw_cycle", "2", false};
cvar_t	v_iroll_cycle = {"v_iroll_cycle", "0.5", false};
cvar_t	v_ipitch_cycle = {"v_ipitch_cycle", "1", false};
cvar_t	v_iyaw_level = {"v_iyaw_level", "0.3", false};
cvar_t	v_iroll_level = {"v_iroll_level", "0.1", false};
cvar_t	v_ipitch_level = {"v_ipitch_level", "0.3", false};

cvar_t	v_idlescale = {"v_idlescale", "0", false};

cvar_t	crosshair = {"crosshair", "1", true};
cvar_t r_viewmodeloffset = {"r_viewmodeloffset", "0", true};
cvar_t	cl_crosshaircentered = {"cl_crosshaircentered", "1", true}; // Baker 3.60 - Optional centered crosshair
cvar_t	cl_colorshow = {"cl_colorshow", "0", true}; // Baker 3.99n - Show pants color @ top of screen
cvar_t	cl_crossx = {"cl_crossx", "0", true};  // Baker 3.80x - Save to config
cvar_t	cl_crossy = {"cl_crossy", "0", true};  // Baker 3.80x - Save to config
cvar_t	gl_cshiftpercent = {"gl_cshiftpercent", "100", false};

// JPG 1.05 - palette changes
cvar_t	pq_waterblend = {"pq_waterblend", "0", true};
cvar_t	pq_quadblend = {"pq_quadblend", "0.3", true};
cvar_t	pq_ringblend = {"pq_ringblend", "0", true};
cvar_t	pq_pentblend = {"pq_pentblend", "0.3", true};
cvar_t	pq_suitblend = {"pq_suitblend", "0.3", true};

float	v_dmg_time, v_dmg_roll, v_dmg_pitch;

extern	int			in_forward, in_forward2, in_back;

/*
===============
V_CalcRoll

Used by view and sv_user
===============
*/
vec3_t	forward, right, up;

float V_CalcRoll (vec3_t angles, vec3_t velocity)
{
	float	sign, side;

	AngleVectors (angles, forward, right, up);
	side = DotProduct (velocity, right);
	sign = side < 0 ? -1 : 1;
	side = fabsf(side);

	side = (side < cl_rollspeed.value) ? side * cl_rollangle.value / cl_rollspeed.value : cl_rollangle.value;

	return side*sign;

}


/*
===============
V_CalcBob
===============
*/
static float V_CalcBob (void)
{
	float	bob;
	float	cycle;

	cycle = cl.time - (int)(cl.time/cl_bobcycle.value)*cl_bobcycle.value;
	cycle /= cl_bobcycle.value;
	if (cycle < cl_bobup.value)
		cycle = M_PI * cycle / cl_bobup.value;
	else
		cycle = M_PI + M_PI*(cycle-cl_bobup.value)/(1.0 - cl_bobup.value);

// bob is proportional to velocity in the xy plane
// (don't count Z, or jumping messes it up)
	bob = sqrt(cl.velocity[0]*cl.velocity[0] + cl.velocity[1]*cl.velocity[1]) * cl_bob.value;
	bob = bob*0.3 + bob*0.7*sin(cycle);
	if (bob > 4)
		bob = 4;
	else if (bob < -7)
		bob = -7;
	return bob;
}

//=============================================================================

cvar_t	v_centermove = {"v_centermove", "0.15", false};
cvar_t	v_centerspeed = {"v_centerspeed","500"};

void V_StartPitchDrift_f (void)
{
	if (cl.laststop == cl.time)
		return;		// something else is keeping it from drifting

	if (cl.nodrift || !cl.pitchvel)
	{
		cl.pitchvel = v_centerspeed.value;
		cl.nodrift = false;
		cl.driftmove = 0;
	}
}

void V_StopPitchDrift (void)
{
	cl.laststop = cl.time;
	cl.nodrift = true;
	cl.pitchvel = 0;
}

/*
===============
V_DriftPitch

Moves the client pitch angle towards cl.idealpitch sent by the server.

If the user is adjusting pitch manually, either with lookup/lookdown,
mlook and mouse, or klook and keyboard, pitch drifting is constantly stopped.

Drifting is enabled when the center view key is hit, mlook is released and
lookspring is non 0, or when
===============
*/
static void V_DriftPitch (void)
{
	float		delta, move;

	if (cl.noclip_anglehack || !cl.onground || cls.demoplayback )
	{
		cl.driftmove = 0;
		cl.pitchvel = 0;
		return;
	}

// don't count small mouse motion
	if (cl.nodrift)
	{
		if ( fabs(cl.cmd.forwardmove) < cl_forwardspeed.value)
			cl.driftmove = 0;
		else
			cl.driftmove += host_frametime;

		if ( cl.driftmove > v_centermove.value)
			V_StartPitchDrift_f ();

		return;
	}

	delta = cl.idealpitch - cl.viewangles[PITCH];

	if (!delta)
	{
		cl.pitchvel = 0;
		return;
	}

	move = host_frametime * cl.pitchvel;
	cl.pitchvel += host_frametime * v_centerspeed.value;

//Con_Printf ("move: %f (%f)\n", move, host_frametime);

	if (delta > 0)
	{
		if (move > delta)
		{
			cl.pitchvel = 0;
			move = delta;
		}
		cl.viewangles[PITCH] += move;
	}
	else if (delta < 0)
	{
		if (move > -delta)
		{
			cl.pitchvel = 0;
			move = -delta;
		}
		cl.viewangles[PITCH] -= move;
	}
}

/*
==============================================================================

						PALETTE FLASHES

==============================================================================
*/

cshift_t	cshift_empty = { {130,80,50}, 0 };
cshift_t	cshift_water = { {130,80,50}, 128 };
cshift_t	cshift_slime = { {0,25,5}, 150 };
cshift_t	cshift_lava = { {255,80,0}, 150 };

cvar_t		vold_gamma = {"gamma", "1", true};

// Baker hwgamma support


cvar_t		gl_hwblend			= {"gl_hwblend", "0"};
cvar_t		v_gamma				= {"gamma", "0.7", true};
cvar_t		v_contrast			= {"contrast", "1", true};
unsigned short	ramps[3][256];

// Baker end hwgamma support

byte		gammatable[256];	// palette is sent through this


byte		rampsold[3][256];
float		v_blend[4];		// rgba 0.0 - 1.0



void BuildGammaTable (float g)
{
	int		i, inf;

	if (g == 1.0)
	{
		for (i=0 ; i<256 ; i++)
			gammatable[i] = i;
		return;
	}

	for (i=0 ; i<256 ; i++)
	{
		inf = 255 * pow((i + 0.5) / 255.5, g) + 0.5;
		gammatable[i] =CLAMP (0, inf, 255);
	}
}


/*
=================
V_CheckGamma
=================
*/


static qboolean V_CheckGamma (void)
{
	static float oldgammavalue;
	float gamma;

	if (vold_gamma.value == oldgammavalue) // exit if gamma is unchanged
		return false;

	gamma = CLAMP (0.3, vold_gamma.value, 3);

	if (vold_gamma.value != gamma) {
		// If cvar isn't within bounds, make it so
		Cvar_SetValue("gamma", gamma);
	}

	oldgammavalue = gamma;

	BuildGammaTable (gamma); // Baker 3.99 todo: update gamma cvar if bounded

	vid.recalc_refdef = 1;				// force a surface cache flush

	return true;
}



/*
===============
V_ParseDamage
===============
*/
void V_ParseDamage (void)
{
	int		i, armor, blood;
	vec3_t	from, forward, right, up;
	entity_t	*ent;
	float	side, count;

	armor = MSG_ReadByte ();
	blood = MSG_ReadByte ();
	for (i=0 ; i<3 ; i++)
		from[i] = MSG_ReadCoord ();

	count = blood*0.5 + armor*0.5;
	if (count < 10)
		count = 10;

	cl.faceanimtime = cl.time + 0.2;		// but sbar face into pain frame

	cl.cshifts[CSHIFT_DAMAGE].percent += 3*count;
	if (cl.cshifts[CSHIFT_DAMAGE].percent < 0)
		cl.cshifts[CSHIFT_DAMAGE].percent = 0;
	if (cl.cshifts[CSHIFT_DAMAGE].percent > 150)
		cl.cshifts[CSHIFT_DAMAGE].percent = 150;

	if (armor > blood)
	{
		cl.cshifts[CSHIFT_DAMAGE].destcolor[0] = 200;
		cl.cshifts[CSHIFT_DAMAGE].destcolor[1] = 100;
		cl.cshifts[CSHIFT_DAMAGE].destcolor[2] = 100;
	}
	else if (armor)
	{
		cl.cshifts[CSHIFT_DAMAGE].destcolor[0] = 220;
		cl.cshifts[CSHIFT_DAMAGE].destcolor[1] = 50;
		cl.cshifts[CSHIFT_DAMAGE].destcolor[2] = 50;
	}
	else
	{
		cl.cshifts[CSHIFT_DAMAGE].destcolor[0] = 255;
		cl.cshifts[CSHIFT_DAMAGE].destcolor[1] = 0;
		cl.cshifts[CSHIFT_DAMAGE].destcolor[2] = 0;
	}

// calculate view angle kicks
	ent = &cl_entities[cl.viewentity];

	VectorSubtract (from, ent->origin, from);
	VectorNormalize (from);

	AngleVectors (ent->angles, forward, right, up);

	side = DotProduct (from, right);
	v_dmg_roll = count*side*v_kickroll.value;

	side = DotProduct (from, forward);
	v_dmg_pitch = count*side*v_kickpitch.value;

	v_dmg_time = v_kicktime.value;
}

/*
==================
V_cshift_f
==================
*/
static void V_cshift_f (void)
{
	cshift_empty.destcolor[0] = atoi(Cmd_Argv(1));
	cshift_empty.destcolor[1] = atoi(Cmd_Argv(2));
	cshift_empty.destcolor[2] = atoi(Cmd_Argv(3));
	cshift_empty.percent = atoi(Cmd_Argv(4));
}

/*
==================
V_BonusFlash_f

When you run over an item, the server sends this command
==================
*/
static void V_BonusFlash_f (void)
{
	cl.cshifts[CSHIFT_BONUS].destcolor[0] = 215;
	cl.cshifts[CSHIFT_BONUS].destcolor[1] = 186;
	cl.cshifts[CSHIFT_BONUS].destcolor[2] = 69;
	cl.cshifts[CSHIFT_BONUS].percent = 50;
}

/*
=============
V_SetContentsColor

Underwater, lava, etc each has a color shift
=============
*/
void V_SetContentsColor (int contents)
{
	// Baker: scenarios
	//        glpro with gl_hwblend 1
	//		  glpro with gl_hwblend 0
	//		  glpro with -gamma
	//        d3dpro
	//        wqpro

	// Baker: I'm thinking this should 
	if (using_hwgamma /*&& vid_hwgamma_enabled && gl_hwblend.value*/) {
		// Baker begin hwgamma support
		if (!pq_waterblend.value)
		{
			cl.cshifts[CSHIFT_CONTENTS] = cshift_empty;
	
			cl.cshifts[CSHIFT_CONTENTS].percent *= 100;
	
			return;
		}
	}
	switch (contents)
	{
	case CONTENTS_EMPTY:
	case CONTENTS_SOLID:
		cl.cshifts[CSHIFT_CONTENTS] = cshift_empty;
		break;
	case CONTENTS_LAVA:
		cl.cshifts[CSHIFT_CONTENTS] = cshift_lava;
		break;
	case CONTENTS_SLIME:
		cl.cshifts[CSHIFT_CONTENTS] = cshift_slime;
		break;
	default:
		cl.cshifts[CSHIFT_CONTENTS] = cshift_water;
	}


	if (using_hwgamma /* && vid_hwgamma_enabled && gl_hwblend.value*/) 
		// Baker begin hwgamma support
		if (pq_waterblend.value > 0 && pq_waterblend.value < 1 && contents != CONTENTS_EMPTY)

			cl.cshifts[CSHIFT_CONTENTS].percent *= pq_waterblend.value;


	
	if (using_hwgamma /*&& vid_hwgamma_enabled && gl_hwblend.value*/) 
	{
		// Baker begin hwgamma support
		if (contents != CONTENTS_EMPTY)
		{
			if (!gl_polyblend.value)
				cl.cshifts[CSHIFT_CONTENTS].percent = 0;
			else
				cl.cshifts[CSHIFT_CONTENTS].percent *= gl_cshiftpercent.value;
		}
		else
		{
			cl.cshifts[CSHIFT_CONTENTS].percent *= 100;
		}
	}

}

/*
=============
V_CalcPowerupCshift
=============
*/
static void V_CalcPowerupCshift (void)
{
	if (cl.items & IT_QUAD)
	{
		cl.cshifts[CSHIFT_POWERUP].destcolor[0] = 0;
		cl.cshifts[CSHIFT_POWERUP].destcolor[1] = 0;
		cl.cshifts[CSHIFT_POWERUP].destcolor[2] = 255;
		cl.cshifts[CSHIFT_POWERUP].percent = 30 * pq_quadblend.value;	// JPG 1.05 - added pq_quadblend
	}
	else if (cl.items & IT_SUIT)
	{
		cl.cshifts[CSHIFT_POWERUP].destcolor[0] = 0;
		cl.cshifts[CSHIFT_POWERUP].destcolor[1] = 255;
		cl.cshifts[CSHIFT_POWERUP].destcolor[2] = 0;
		cl.cshifts[CSHIFT_POWERUP].percent = 20 * pq_suitblend.value; // JPG 1.05 - added pq_suidblend
	}
	else if (cl.items & IT_INVISIBILITY)
	{
		cl.cshifts[CSHIFT_POWERUP].destcolor[0] = 100;
		cl.cshifts[CSHIFT_POWERUP].destcolor[1] = 100;
		cl.cshifts[CSHIFT_POWERUP].destcolor[2] = 100;
		cl.cshifts[CSHIFT_POWERUP].percent = 100 * pq_ringblend.value; // JPG 1.05 - added pq_ringblend
	}
	else if (cl.items & IT_INVULNERABILITY)
	{
		cl.cshifts[CSHIFT_POWERUP].destcolor[0] = 255;
		cl.cshifts[CSHIFT_POWERUP].destcolor[1] = 255;
		cl.cshifts[CSHIFT_POWERUP].destcolor[2] = 0;
		cl.cshifts[CSHIFT_POWERUP].percent = 30 * pq_pentblend.value; //  JPG 1.05 - added pq_pentblend
	}
	else
		cl.cshifts[CSHIFT_POWERUP].percent = 0;
}

/*
=============
V_CalcBlend
=============
*/

extern		qboolean  using_hwgamma;


void V_CalcBlend (void)
{
	float	r, g, b, a, a2;
	int	j;

	r = g = b = a = 0;

	// Baker hwgamma support

	if (using_hwgamma) {
	if (cls.state != ca_connected)
	{
		cl.cshifts[CSHIFT_CONTENTS] = cshift_empty;
		cl.cshifts[CSHIFT_POWERUP].percent = 0;
	}
	else
	{
		V_CalcPowerupCshift ();
	}

	// drop the damage value
	cl.cshifts[CSHIFT_DAMAGE].percent -= host_frametime * 150;
	if (cl.cshifts[CSHIFT_DAMAGE].percent <= 0)
		cl.cshifts[CSHIFT_DAMAGE].percent = 0;

	// drop the bonus value
	cl.cshifts[CSHIFT_BONUS].percent -= host_frametime * 100;
	if (cl.cshifts[CSHIFT_BONUS].percent <= 0)
		cl.cshifts[CSHIFT_BONUS].percent = 0;

	for (j=0 ; j<NUM_CSHIFTS ; j++)
	{
		if ((!gl_cshiftpercent.value || !gl_polyblend.value) && j != CSHIFT_CONTENTS)
			continue;

		if (j == CSHIFT_CONTENTS)
			a2 = cl.cshifts[j].percent / 100.0 / 255.0;
		else
		a2 = ((cl.cshifts[j].percent * gl_cshiftpercent.value) / 100.0) / 255.0;

		if (!a2)
			continue;
		a = a + a2*(1-a);

		a2 /= a;
		r = r*(1-a2) + cl.cshifts[j].destcolor[0]*a2;
		g = g*(1-a2) + cl.cshifts[j].destcolor[1]*a2;
		b = b*(1-a2) + cl.cshifts[j].destcolor[2]*a2;
	}

	v_blend[0] = r/255.0;
	v_blend[1] = g/255.0;
	v_blend[2] = b/255.0;
	v_blend[3] = CLAMP (0, a, 1);
}
	else


	{  // Baker end hwgamma support
		for (j=0 ; j<NUM_CSHIFTS ; j++)	
		{
			if (!gl_cshiftpercent.value)
				continue;

			a2 = ((cl.cshifts[j].percent * gl_cshiftpercent.value) / 100.0) / 255.0;

	//		a2 = cl.cshifts[j].percent/255.0;
			if (!a2)
				continue;
			a = a + a2*(1-a);
	//Con_Printf ("j:%i a:%f\n", j, a);
			a2 = a2/a;
			r = r*(1-a2) + cl.cshifts[j].destcolor[0]*a2;
			g = g*(1-a2) + cl.cshifts[j].destcolor[1]*a2;
			b = b*(1-a2) + cl.cshifts[j].destcolor[2]*a2;
		}

		v_blend[0] = r/255.0;
		v_blend[1] = g/255.0;
		v_blend[2] = b/255.0;
		v_blend[3] = a;
		if (v_blend[3] > 1)
			v_blend[3] = 1;
		if (v_blend[3] < 0)
			v_blend[3] = 0;
	}
}


/*
=============
V_UpdatePaletteNew
=============
*/


// Baker hw gamma support
// This v_updatepalette should not get called
// except if hwgamma is being used
// classic gamma should use v_updatepaletteold
qboolean V_UpdatePalette_Hardware (void)
{
	int		i, j, c;
	qboolean	new;
	float		a, rgb[3], gamma, contrast;
	static float	prev_blend[4], old_gamma, old_contrast, old_hwblend;
	extern float	vid_gamma;
	qboolean	hardware_blend_set_off=false;

	new = false;

	// Determine
	for (i=0 ; i<4 ; i++)
	{
		if (v_blend[i] != prev_blend[i])
		{
			new = true;
			prev_blend[i] = v_blend[i];
		}
	}

	gamma = CLAMP (0.3, v_gamma.value, 3);
	if (v_gamma.value != old_gamma || !old_gamma) 
	{
		if (v_gamma.value != gamma)
			Cvar_SetValue("gamma", gamma); // Baker 3.99: Set the cvar to what it should be if out of range
		old_gamma = gamma;
		new = true;
	}

	contrast = CLAMP (1, v_contrast.value, 3);
	if (v_contrast.value != old_contrast || !old_contrast)
	{

		if (v_contrast.value != contrast) {
			Cvar_SetValue("contrast", contrast); // Baker 3.99: Set the cvar to what it should be if out of range
		}
		old_contrast = contrast;
		new = true;
	}

	if (gl_hwblend.value != old_hwblend)
	{
		new = true;

		if (!gl_hwblend.value)
			hardware_blend_set_off = true;

		old_hwblend = gl_hwblend.value;
	}

	if (!new)
		return false;

	a = v_blend[3];

	if (!vid_hwgamma_enabled || !gl_hwblend.value)
		a = 0;

	rgb[0] = 255 * v_blend[0] * a;
	rgb[1] = 255 * v_blend[1] * a;
	rgb[2] = 255 * v_blend[2] * a;

	a = 1 - a;

	if (vid_gamma < 1.0) //Baker 3.99: was vid_gamma != 1.0; 1.0 is maximum value, changed in event someone sets to 2.0 or something above 1.0
	{
		contrast = pow (contrast, vid_gamma);
		gamma /= vid_gamma;
	}

	for (i=0 ; i<256 ; i++)
	{
		for (j=0 ; j<3 ; j++)
		{
			// apply blend and contrast
			c = (i*a + rgb[j]) * contrast;
			if (c > 255)
				c = 255;
			// apply gamma
			c = 255 * pow((c + 0.5)/255.5, gamma) + 0.5;
			c = CLAMP (0, c, 255);
			ramps[j][i] = c << 8;
		}
	}

	VID_SetDeviceGammaRamp ((unsigned short *)ramps);

	return hardware_blend_set_off;
}

void V_UpdatePalette_Static (qboolean forced)
{
	int		i, j;
	qboolean	blend_changed;
	byte	*basepal, *newpal;
	byte	pal[768];
	float	r,g,b,a;
	int		ir, ig, ib;

//	Baker: eliminate shifts when disconnected
	if (cls.state != ca_connected)
	{
		cl.cshifts[CSHIFT_CONTENTS] = cshift_empty;
		cl.cshifts[CSHIFT_POWERUP].percent = 0;
	}
	else
	{
		V_CalcPowerupCshift ();
	}

	blend_changed = false;

	for (i=0 ; i<NUM_CSHIFTS ; i++)
	{
		if (cl.cshifts[i].percent != cl.prev_cshifts[i].percent)
		{
			blend_changed = true;
			cl.prev_cshifts[i].percent = cl.cshifts[i].percent;
		}
		for (j=0 ; j<3 ; j++)
			if (cl.cshifts[i].destcolor[j] != cl.prev_cshifts[i].destcolor[j])
			{
				blend_changed = true;
				cl.prev_cshifts[i].destcolor[j] = cl.cshifts[i].destcolor[j];
			}
	}

// drop the damage value
	cl.cshifts[CSHIFT_DAMAGE].percent -= host_frametime*150;
	if (cl.cshifts[CSHIFT_DAMAGE].percent <= 0)
		cl.cshifts[CSHIFT_DAMAGE].percent = 0;

// drop the bonus value
	cl.cshifts[CSHIFT_BONUS].percent -= host_frametime*100;
	if (cl.cshifts[CSHIFT_BONUS].percent <= 0)
		cl.cshifts[CSHIFT_BONUS].percent = 0;

	if (!blend_changed && !forced)
		return;



	a = v_blend[3];
	r = 255*v_blend[0]*a;
	g = 255*v_blend[1]*a;
	b = 255*v_blend[2]*a;

	a = 1-a;
	for (i=0 ; i<256 ; i++)
	{
		ir = i*a + r;
		ig = i*a + g;
		ib = i*a + b;
		if (ir > 255)
			ir = 255;
		if (ig > 255)
			ig = 255;
		if (ib > 255)
			ib = 255;

		rampsold[0][i] = gammatable[ir];
		rampsold[1][i] = gammatable[ig];
		rampsold[2][i] = gammatable[ib];
	}

	basepal = host_basepal;
	newpal = pal;

	for (i=0 ; i<256 ; i++)
	{
		ir = basepal[0];
		ig = basepal[1];
		ib = basepal[2];
		basepal += 3;

		newpal[0] = rampsold[0][ir];
		newpal[1] = rampsold[1][ig];
		newpal[2] = rampsold[2][ib];
		newpal += 3;
	}

	VID_ShiftPaletteOld (pal);
}

/*
==============================================================================

						VIEW RENDERING

==============================================================================
*/

float angledelta (float a)
{
	a = anglemod(a);
	if (a > 180)
		a -= 360;
	return a;
}

/*
==================
CalcGunAngle
==================
*/
void CalcGunAngle (void)
{
	float	yaw, pitch, move;
	static float oldyaw = 0;
	static float oldpitch = 0;

	yaw = r_refdef.viewangles[YAW];
	pitch = -r_refdef.viewangles[PITCH];

	yaw = angledelta(yaw - r_refdef.viewangles[YAW]) * 0.4;
	if (yaw > 10)
		yaw = 10;
	if (yaw < -10)
		yaw = -10;
	pitch = angledelta(-pitch - r_refdef.viewangles[PITCH]) * 0.4;
	if (pitch > 10)
		pitch = 10;
	if (pitch < -10)
		pitch = -10;
	move = host_frametime*20;
	if (yaw > oldyaw)
	{
		if (oldyaw + move < yaw)
			yaw = oldyaw + move;
	}
	else
	{
		if (oldyaw - move > yaw)
			yaw = oldyaw - move;
	}

	if (pitch > oldpitch)
	{
		if (oldpitch + move < pitch)
			pitch = oldpitch + move;
	}
	else
	{
		if (oldpitch - move > pitch)
			pitch = oldpitch - move;
	}

	oldyaw = yaw;
	oldpitch = pitch;

	cl.viewent.angles[YAW] = r_refdef.viewangles[YAW] + yaw;
	cl.viewent.angles[PITCH] = - (r_refdef.viewangles[PITCH] + pitch);

	cl.viewent.angles[ROLL] -= v_idlescale.value * sin(cl.time*v_iroll_cycle.value) * v_iroll_level.value;
	cl.viewent.angles[PITCH] -= v_idlescale.value * sin(cl.time*v_ipitch_cycle.value) * v_ipitch_level.value;
	cl.viewent.angles[YAW] -= v_idlescale.value * sin(cl.time*v_iyaw_cycle.value) * v_iyaw_level.value;
}

/*
==============
V_BoundOffsets
==============
*/
void V_BoundOffsets (void)
{
	entity_t	*ent;

	ent = &cl_entities[cl.viewentity];

// absolutely bound refresh relative to entity clipping hull
// so the view can never be inside a solid wall
	r_refdef.vieworg[0] = max(r_refdef.vieworg[0], ent->origin[0] - 14);
	r_refdef.vieworg[0] = min(r_refdef.vieworg[0], ent->origin[0] + 14);
	r_refdef.vieworg[1] = max(r_refdef.vieworg[1], ent->origin[1] - 14);
	r_refdef.vieworg[1] = min(r_refdef.vieworg[1], ent->origin[1] + 14);
	r_refdef.vieworg[2] = max(r_refdef.vieworg[2], ent->origin[2] - 22);
	r_refdef.vieworg[2] = min(r_refdef.vieworg[2], ent->origin[2] + 30);
}

/*
==============
V_AddIdle

Idle swaying
==============
*/
void V_AddIdle (void)
{
	r_refdef.viewangles[ROLL] += v_idlescale.value * sinf(cl.time*v_iroll_cycle.value) * v_iroll_level.value;
	r_refdef.viewangles[PITCH] += v_idlescale.value * sinf(cl.time*v_ipitch_cycle.value) * v_ipitch_level.value;
	r_refdef.viewangles[YAW] += v_idlescale.value * sinf(cl.time*v_iyaw_cycle.value) * v_iyaw_level.value;
}


/*
==============
V_CalcViewRoll

Roll is induced by movement and damage
==============
*/
void V_CalcViewRoll (void)
{
	float		side;

	side = V_CalcRoll (cl_entities[cl.viewentity].angles, cl.velocity);
	r_refdef.viewangles[ROLL] += side;

	if (v_dmg_time > 0)
	{
		r_refdef.viewangles[ROLL] += v_dmg_time/v_kicktime.value*v_dmg_roll;
		r_refdef.viewangles[PITCH] += v_dmg_time/v_kicktime.value*v_dmg_pitch;
		v_dmg_time -= host_frametime;
	}

	if (cl.stats[STAT_HEALTH] <= 0)
	{
		r_refdef.viewangles[ROLL] = 80;	// dead view angle
		return;
	}

}

/*
==================
V_CalcIntermissionRefdef
==================
*/
void V_CalcIntermissionRefdef (void)
{
	entity_t	*ent, *view;
	float		old;

// ent is the player model (visible when out of body)
	ent = &cl_entities[cl.viewentity];
// view is the weapon model (only visible from inside body)
	view = &cl.viewent;

	VectorCopy (ent->origin, r_refdef.vieworg);
	VectorCopy (ent->angles, r_refdef.viewangles);
	view->model = NULL;

// always idle in intermission
	old = v_idlescale.value;
	v_idlescale.value = 1;
	V_AddIdle ();
	v_idlescale.value = old;
}

/*
==================
V_CalcRefdef
==================
*/
void V_CalcRefdef (void)
{
	entity_t	*ent, *view;
	int			i;
	vec3_t		forward, right, up;
	vec3_t		angles;
	float		bob;
	static float oldz = 0;
	extern cvar_t r_truegunangle;

	V_DriftPitch ();

// ent is the player model (visible when out of body)
	ent = &cl_entities[cl.viewentity];
// view is the weapon model (only visible from inside body)
	view = &cl.viewent;

// transform the view offset by the model's matrix to get the offset from
// model origin for the view
	// JPG - viewangles -> lerpangles
	ent->angles[YAW] = cl.lerpangles[YAW];	// the model should face the view dir
	ent->angles[PITCH] = -cl.lerpangles[PITCH];	// the model should face the view dir


	bob = V_CalcBob ();

	// set up the refresh position
	VectorCopy (ent->origin, r_refdef.vieworg);
	r_refdef.vieworg[2] += cl.viewheight + bob;

// never let it sit exactly on a node line, because a water plane can
// disappear when viewed with the eye exactly on it.
// the server protocol only specifies to 1/16 pixel, so add 1/32 in each axis
	r_refdef.vieworg[0] += 1.0/32;
	r_refdef.vieworg[1] += 1.0/32;
	r_refdef.vieworg[2] += 1.0/32;

	VectorCopy (cl.lerpangles, r_refdef.viewangles); // JPG - viewangles -> lerpangles
	V_CalcViewRoll ();
	V_AddIdle ();

// offsets
	angles[PITCH] = -ent->angles[PITCH];	// because entity pitches are actually backward
	angles[YAW] = ent->angles[YAW];
	angles[ROLL] = ent->angles[ROLL];

	AngleVectors (angles, forward, right, up);

	for (i=0 ; i<3 ; i++)
		r_refdef.vieworg[i] += scr_ofsx.value*forward[i] + scr_ofsy.value*right[i] + scr_ofsz.value*up[i];

	V_BoundOffsets ();

// set up gun position
	VectorCopy (cl.lerpangles, view->angles); // JPG - viewangles -> lerpangles

	CalcGunAngle ();

	VectorCopy (ent->origin, view->origin);
	view->origin[2] += cl.viewheight;



   VectorCopy (r_refdef.vieworg, view->origin);
   VectorMA (view->origin, bob * 0.4, forward, view->origin);

   if (r_viewmodeloffset.string[0]) {
      float offset[3];
      int size = sizeof(offset)/sizeof(offset[0]);

      ParseFloats(r_viewmodeloffset.string, offset, &size);
      VectorMA (view->origin,  offset[0], right,   view->origin);
      VectorMA (view->origin, -offset[1], up,      view->origin);
      VectorMA (view->origin,  offset[2], forward, view->origin);
   }


// fudge position around to keep amount of weapon visible
// roughly equal with different FOV
	if (!r_truegunangle.value) // Baker 3.80x - True gun angle option (FitzQuake/DarkPlaces style)
	{
	if (scr_viewsize.value == 110)
		view->origin[2] += 1;
	else if (scr_viewsize.value == 100)
		view->origin[2] += 2;
	else if (scr_viewsize.value == 90)
		view->origin[2] += 1;
	else if (scr_viewsize.value == 80)
		view->origin[2] += 0.5;
	}

	view->model = cl.model_precache[cl.stats[STAT_WEAPON]];
	view->frame = cl.stats[STAT_WEAPONFRAME];
	view->colormap = 0;

// set up the refresh position
	if (v_gunkick.value)
		VectorAdd (r_refdef.viewangles, cl.punchangle, r_refdef.viewangles);

// smooth out stair step ups
	if (cl.onground && ent->origin[2] - oldz > 0)

	{
		float steptime;

		steptime = cl.time - cl.oldtime;
		if (steptime < 0)
	//FIXME		I_Error ("steptime < 0");
			steptime = 0;

		oldz += steptime * 80;
		if (oldz > ent->origin[2])
			oldz = ent->origin[2];
		if (ent->origin[2] - oldz > 12)
			oldz = ent->origin[2] - 12;
		r_refdef.vieworg[2] += oldz - ent->origin[2];
		view->origin[2] += oldz - ent->origin[2];
	}
	else
		oldz = ent->origin[2];

		if (chase_active.value && !pq_cheatfree)	// JPG 3.20 - added pq_cheatfree
			Chase_Update ();
}


/*
===============
SCR_DrawVolume - Baker 3.80x -- from JoeQuake
===============
*/
void SCR_DrawCoords (void)
{
	if (developer.value < 2)
		return;
	
	Draw_String (16, 16, va("Position xyz = %i %i %i",
	(int)cl_entities[cl.viewentity].origin[0],
	(int)cl_entities[cl.viewentity].origin[1],
	(int)cl_entities[cl.viewentity].origin[2]));

}


/*
===============
SCR_DrawVolume - Baker 3.80x -- from JoeQuake
===============
*/
void SCR_DrawVolume (void)
{
	int		i, yofs;
	float		j;
	char		bar[11];
	static	float	volume_time = 0;
	extern qboolean	volume_changed;

	if (realtime < volume_time - 2.0)
	{
		volume_time = 0;
		return;
	}

	if (volume_changed)
	{
		volume_time = realtime + 2.0;
		volume_changed = false;
	}
	else if (realtime > volume_time)
	{
		return;
	}

	for (i = 1, j = 0.1 ; i <= 10 ; i++, j += 0.1)
		bar[i-1] = ((volume.value +0.05) >= j) ? 139 : 11; // Baker 3.60 + 0.0.5 hack for now

	bar[10] = 0;

		yofs = 8;

	Draw_String (vid.width - 88, yofs, bar);
	Draw_String (vid.width - 88, yofs + 8, "volume");
}

#ifdef SUPPORTS_AUTOID_SOFTWARE
void R_DrawNameTags(void)
{
	int i;
	vec3_t center;
	vec3_t tagcenter;
	vec3_t waste, waste2;
//	frame_t *frame;
	entity_t	*state;
	vec3_t	OurViewPoint;
	vec3_t  ThisClientPoint;
	vec3_t	stop;
	extern cvar_t scr_autoid;

	if (!scr_autoid.value || cls.state != ca_connected || !cls.demoplayback)
		return;

	for (i = 0; i < cl.maxclients; i++)
	{
		state = &cl_entities[1+i];

		if (!state->model->name)		// NULL model
			continue;

		if (!(state->modelindex == cl_modelindex[mi_player]))	// Not a player model
			continue;

		if (ISDEAD(state->frame)) // Dead
			continue;

//		if (strcmp(state->model->name, "progs/player.mdl"))
//			continue;


//		if (R_CullSphere(state->origin, 0))
//			continue;

		VectorCopy (r_refdef.vieworg, OurViewPoint);
		VectorCopy (state->origin, ThisClientPoint);

		TraceLine (OurViewPoint, ThisClientPoint, stop);

		if (stop[0] != 0 || stop[1] != 0 || stop[2] != 0)  // Quick and dirty traceline
			continue;


		VectorCopy(state->origin, tagcenter);
		tagcenter[2] += 32;
		ML_Project(tagcenter, center, r_refdef.viewangles, r_refdef.vieworg, (float)r_refdef.vrect.width/r_refdef.vrect.height, r_refdef.fov_y);
		if (center[2] > 1)
			continue;
		//Con_Printf("Center is x, y, z %f %f %f\n", center[0], center[1], center[2]);
		Draw_String(center[0]*r_refdef.vrect.width+r_refdef.vrect.x, (1-center[1])*r_refdef.vrect.height+r_refdef.vrect.y, cl.scores[i].name);
		//Con_Printf("Drawing tag for number %i = %s\n", i, cl.scores[i].name);
		//Con_Printf("Drawing at x %i,y i% \n", center[0]*r_refdef.vrect.width+r_refdef.vrect.x, (1-center[1])*r_refdef.vrect.height+r_refdef.vrect.y);
		//Con_Printf("Our screen %i %i with center0/1 %f %f\n",  r_refdef.vrect.width, r_refdef.vrect.height, center[0], center[1]);
	}
}
#endif


/*
==================
V_RenderView

The player's clipping box goes from (-16 -16 -24) to (16 16 32) from
the entity origin, so any view position inside that will be valid
==================
*/
extern vrect_t	scr_vrect;

void V_RenderView (void)
{
	// Baker hwgamma support
	if (using_hwgamma) {
		if (cls.state != ca_connected) {
			V_CalcBlend ();
			return;
		}
	}

	if (con_forcedup)
		return;

	
// don't allow cheats in multiplayer
	if (cl.maxclients > 1)
	{
		Cvar_Set ("scr_ofsx", "0");
		Cvar_Set ("scr_ofsy", "0");
		Cvar_Set ("scr_ofsz", "0");
	}

	if (cl.intermission) // intermission / finale rendering
	{
		V_CalcIntermissionRefdef ();
	}
	else
	{
		if (!cl.paused)
			V_CalcRefdef ();
	}

	R_PushDlights ();

	if (lcd_x.value)
	{
		// render two interleaved views
		int		i;


		r_refdef.viewangles[YAW] -= lcd_yaw.value;
		for (i=0 ; i<3 ; i++)
			r_refdef.vieworg[i] -= right[i]*lcd_x.value;
		R_RenderView ();


		R_PushDlights ();

		r_refdef.viewangles[YAW] += lcd_yaw.value*2;
		for (i=0 ; i<3 ; i++)
			r_refdef.vieworg[i] += 2*right[i]*lcd_x.value;
		R_RenderView ();


		r_refdef.vrect.height <<= 1;

	}
	else
	{
		R_RenderView ();
	}



}

//============================================================================

/*
=============
V_Init
=============
*/
void V_Init (void)
{
	Cmd_AddCommand ("v_cshift", V_cshift_f);
	Cmd_AddCommand ("bf", V_BonusFlash_f);
	Cmd_AddCommand ("centerview", V_StartPitchDrift_f);

	Cvar_RegisterVariable (&lcd_x, NULL);
	Cvar_RegisterVariable (&lcd_yaw, NULL);

	// Baker hwgamma support
	Cvar_RegisterVariable (&gl_hwblend, NULL);
	Cvar_RegisterVariable (&v_gamma, NULL);
	Cvar_RegisterVariable (&v_contrast, NULL);
	// Baker end hwgamma support


	Cvar_RegisterVariable (&v_centermove, NULL);
	Cvar_RegisterVariable (&v_centerspeed, NULL);

	Cvar_RegisterVariable (&v_iyaw_cycle, NULL);
	Cvar_RegisterVariable (&v_iroll_cycle, NULL);
	Cvar_RegisterVariable (&v_ipitch_cycle, NULL);
	Cvar_RegisterVariable (&v_iyaw_level, NULL);
	Cvar_RegisterVariable (&v_iroll_level, NULL);
	Cvar_RegisterVariable (&v_ipitch_level, NULL);

	Cvar_RegisterVariable (&v_idlescale, NULL);
	Cvar_RegisterVariable (&crosshair, NULL);
	Cvar_RegisterVariable (&r_viewmodeloffset, NULL);

	Cvar_RegisterVariable (&cl_crossx, NULL);
	Cvar_RegisterVariable (&cl_crossy, NULL);
	Cvar_RegisterVariable (&cl_crosshaircentered, NULL); // Baker 3.60 - centered crosshair
	Cvar_RegisterVariable (&cl_colorshow, NULL);
	Cvar_RegisterVariable (&gl_cshiftpercent, NULL);

	Cvar_RegisterVariable (&scr_ofsx, NULL);
	Cvar_RegisterVariable (&scr_ofsy, NULL);
	Cvar_RegisterVariable (&scr_ofsz, NULL);
	Cvar_RegisterVariable (&cl_rollspeed, NULL);
	Cvar_RegisterVariable (&cl_rollangle, NULL);

	Cvar_RegisterVariable (&cl_bob, NULL);
	Cvar_RegisterVariable (&cl_bobcycle, NULL);
	Cvar_RegisterVariable (&cl_bobup, NULL);

	Cvar_RegisterVariable (&v_kicktime, NULL);
	Cvar_RegisterVariable (&v_kickroll, NULL);
	Cvar_RegisterVariable (&v_kickpitch, NULL);
	Cvar_RegisterVariable (&v_gunkick, NULL);

	Cvar_RegisterVariable (&vold_gamma, NULL);


	BuildGammaTable (1.0);	// no gamma yet


	// JPG 1.05 - colour shifts
	Cvar_RegisterVariable (&pq_waterblend, NULL);
	Cvar_RegisterVariable (&pq_quadblend, NULL);
	Cvar_RegisterVariable (&pq_pentblend, NULL);
	Cvar_RegisterVariable (&pq_ringblend, NULL);
	Cvar_RegisterVariable (&pq_suitblend, NULL);

}
