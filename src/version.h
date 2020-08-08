/*
Copyright (C) 2002, Anton Gavrilov

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
// version.h

#ifndef VERSION_H
#define VERSION_H

// Messages: 
// MSVC: #pragma message ( "text" )
// GCC version: #warning "hello"
// #error to terminate compilation

// Create our own define for Mac OS X
#if defined(__APPLE__) && defined(__MACH__)
# define MACOSX
#endif

// Define Operating System Names

#ifdef _WIN32
# define OS_NAME "Windows"
#elif defined(MACOSX)
# define OS_NAME "Mac OSX"
#else 
# define OS_NAME "Linux"
# define LINUX // Everything else gets to be Linux for now ;)
#endif



#define CHASE_CAM_FIX
#define SUPPORTS_TRANSFORM_INTERPOLATION // We are switching to r_interpolate_transform



// Define Support For Cheat-Free Mode
#if defined(_WIN32) || defined(Linux)
# define SUPPORTS_CHEATFREE_MODE // Only Windows and Linux have security modules.
#endif


// Define Renderer Name

#if defined(DX8QUAKE) 
# define RENDERER_NAME "DX8"
#else 
# define RENDERER_NAME "GL"
#endif // end !d3dquake

// Define exceptions to the "rule"
#ifdef DX8QUAKE
//# define DX8QUAKE_NO_8BIT					// D3D8 wrapper didn't keep the 8bit support
//# define DX8QUAKE_NO_BINDTEXFUNC			// SGIS/ancient GL pathway removal
# define DX8QUAKE_NO_GL_ZTRICK				// DX8QUAKE hates gl_ztrick; clear the buffers every time
//# define DX8QUAKE_GL_READPIXELS_NO_RGBA		// Wrapper only supports GL_RGBA; not GL_RGBA like envmap command uses
# define DX8QUAKE_VSYNC_COMMANDLINE_PARAM	// Vsync command line param option ... -vsync
#endif


#ifdef MACOSX
# define MACOSX_EXTRA_FEATURES
# define MACOSX_UNKNOWN_DIFFERENCE
# define MACOSX_NETWORK_DIFFERENCE
# define MACOSX_KEYBOARD_EXTRAS
# define MACOSX_KEYBOARD_KEYPAD
# define MACOSX_PASTING

# define MACOSX_SENS_RANGE
#endif

// Define Specific General Capabilities
#ifdef _WIN32
# define SUPPORTS_AVI_CAPTURE					// Hopelessly Windows locked
# define SUPPORTS_INTERNATIONAL_KEYBOARD		// I only know how to detect and address on Windows

# define SUPPORTS_DEMO_AUTOPLAY					// Windows only.  Uses file association
# define SUPPORTS_DIRECTINPUT 
# define SUPPORTS_INTERNATIONAL_KEYBOARD		// Windows only implementation for now?; the extra key byte



# define WINDOWS_SCROLLWHEEL_PEEK				// CAPTURES MOUSEWHEEL WHEN keydest != game
# define HTTP_DOWNLOAD
//# define BUILD_MP3_VERSION


	// GLQUAKE additive features on top of _WIN32 only

//	# define SUPPORTS_GLVIDEO_MODESWITCH  		// Windows only for now.  Probably can be multiplat in future.
	# define SUPPORTS_VSYNC 					// Vertical sync; only GL does this for now
	# define SUPPORTS_TRANSPARENT_SBAR 			// Not implemented in OSX?

	# define RELEASE_MOUSE_FULLSCREEN			// D3DQUAKE gets an error if it loses focus in fullscreen, so that'd be stupid


	# define OLD_SGIS							// Old multitexture ... for now.
	# define INTEL_OPENGL_DRIVER_WORKAROUND		// Windows only issue?  Or is Linux affected too?  OS X is not affected


#endif



// Define Specific Rendering Capabilities



//# define SUPPORTS_GLHOMFIX_NEARWATER			// Specific problem and solution for GL
//# define SUPPORTS_CONSOLE_SIZING				// GL can size the console; WinQuake can't do that yet
//# define SUPPORTS_GL_DELETETEXTURES				// D3DQuake isn't emulating them at this time
# define SUPPORTS_HARDWARE_ANIM_INTERPOLATION	// The hardware interpolation route
# define SUPPORTS_2DPICS_ALPHA					// Transparency of 2D pics
# define SUPPORTS_GL_OVERBRIGHTS				// Overbright method GLQuake is using, WinQuake always had them

//# define GL_QUAKE_SKIN_METHOD					// GLQuake uses a different method for skinning


// Alternate Methods




// gl_keeptjunctions: Setting this to 0 will reduce the number of bsp polygon vertices by removing colinear points, however it produces holes in the BSP due to floating point precision errors.
// Baker: this should default to 1.  That's what FTE does.

// 0 = Remove colinear vertexes when loading the map. (will speed up the game performance, but will leave a few artifact pixels).
// Yields a few more frames per second.

// Note: gl_texsort 1 is for when multitexture is unavailable
//       gl_texsort 0 is for when multitexture is available
// Note: in glpro442 as of this time, gl_texsort 0 is going to be a fail becuase mtex is off.

// Discarded DX8QUAKE #ifdefs -- all functional but either not necessary or some such thing

//# define DX8QUAKE_CANNOT_DETECT_FULLSCREEN_BY_MODESTATE	// Detecting modestate == MS_FULLDIB can't distinguish between windowed and fullscreen modes
//# undef  SUPPORTS_GLVIDEO_MODESWITCH  // Not now, isn't working right
//#define DX8QUAKE_NO_DIALOGS				// No "starting Quake type "dialogs for DX8QUAKE, Improvement applicable to GL
//# define DX8QUAKE_NO_FRONT_BACK_BUFFER		// Baker: 4.42 - wasn't necessary, seems DX8 wrapper can do this
//# define DX8QUAKE_GL_MAX_SIZE_FAKE			// Baker  4.42 - this is no different than dx8 wrapper doing it right way
//# define DX8QUAKE_ALT_MODEL_TEXTURE				// Believe this is unnecessary skin sharpening option applicanle to GL
//# define DX8QUAKE_ALT_RESAMPLE				// Removing redundant function ... I think
//# define DX8QUAKE_NO_GL_TEXSORT_ZERO			// gl_texsort 0 is for no multitexture (can be applied to GL)
//# define DX8QUAKE_NO_GL_KEEPTJUNCTIONS_ZERO		// gl_keeptjunction 0 is (can be applied to GL) * Note below
//# define DX8QUAKE_BAKER_ALTTAB_HACK				// Possibly removeable now


#endif
