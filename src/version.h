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
//#error no touch
#elif defined(MACOSX)
# define OS_NAME "Mac OSX"
#elif defined(FLASH)
# define OS_NAME "Flash"
#elif defined(PSP)
# define OS_NAME "Sony PSP"
#else // Everything else gets to be Linux for now
# define OS_NAME "Linux"
# define LINUX // Everything else gets to be Linux for now ;)
#endif


// Define platforms where we do not use assembly

#if defined(MACOSX) || defined(FLASH)
# define NO_ASSEMBLY
#endif

#ifdef GLQUAKE
# define NO_ASSEMBLY
#endif

#if defined(_WIN32) && !defined(WINDED) && defined(_M_IX86)
#define __i386__	1
#endif

#if defined(__i386__)  && !defined(NO_ASSEMBLY)
#define id386	1
#else
#define id386	0
#endif

// Define platforms supporting HTTP_DOWNLOAD (I have this working on OSX for Intel Macs, but let's hold out)
#ifdef _WIN32
# define HTTP_DOWNLOAD
# define BUILD_MP3_VERSION
#endif

// Define support for platforms that can do HLBSP
#ifdef GLQUAKE
# define SUPPORTS_HLBSP // Only GL
#endif

// Smooth rotation test
//#define SMOOTH_SINGLEPLAYER_TEST
#define QCEXEC
#define SUPPORTS_MULTIMAP_DEMOS
#define CHASE_CAM_FIX



// Define Support For Cheat-Free Mode
#if defined(_WIN32) || defined(Linux)
# define SUPPORTS_CHEATFREE_MODE // Only Windows and Linux have security modules.
#endif


// Define Renderer Name

#ifdef D3DQUAKE
# define RENDERER_NAME "D3D"
#elif defined(DX8QUAKE) // !d3dquake
# define RENDERER_NAME "DX8"
#elif defined (FLASH)
# define RENDERER_NAME "Software"
#elif defined(GLQUAKE) // !d3dquake
# define RENDERER_NAME "GL"
#elif defined(PSP_HARDWARE_VIDEO)
# define RENDERER_NAME "PSPGU"
#else
# define RENDERER_NAME "Win"
#endif // end !d3dquake

// Special markings for D3DQuake

#ifdef D3DQUAKE
# define D3DQ_EXTRA_FEATURES // (D3D_FEATURE)
# define D3DQ_CANNOT_DO_THIS // D3D_NOT_CAPABLE
# define D3DQ_WORKAROUND     // D3DQ_WORKAROUND
#endif

#ifdef MACOSX
# define MACOSX_EXTRA_FEATURES
# define MACOSX_TEXRAM_CHECK
# define MACOSX_UNKNOWN_DIFFERENCE
# define MACOSX_QUESTIONABLE_VALUE
# define MACOSX_NETWORK_DIFFERENCE
# define MACOSX_KEYBOARD_EXTRAS
# define MACOSX_KEYBOARD_KEYPAD
# define MACOSX_PASTING

# define MACOSX_SENS_RANGE
#endif

// Define Specific General Capabilities
#ifdef _WIN32
# define SUPPORTS_AVI_CAPTURE // Hopelessly Windows locked
# define SUPPORTS_INTERNATIONAL_KEYBOARD // I only know how to detect and address on Windows
# define SUPPORTS_CD_PLAYER // Windows yes, maybe Linux too.  Not MACOSX, Fruitz of Dojo not support it.
# define SUPPORTS_DEMO_AUTOPLAY // Windows only.  Uses file association
# define SUPPORTS_DIRECTINPUT 
# define SUPPORTS_INTERNATIONAL_KEYBOARD // Windows only implementation for now?; the extra key byte
# define SUPPORTS_SYSSLEEP // Make this work on OS X sometime; "usleep"
# define SUPPORTS_CLIPBOARD

// GLQUAKE additive features on top of _WIN32 only
#if defined(GLQUAKE) && !defined(D3DQUAKE)
# define SUPPORTS_ENHANCED_GAMMA // Windows only for now.  Probably can be multiplat in future.
# define SUPPORTS_GLVIDEO_MODESWITCH  // Windows only for now.  Probably can be multiplat in future.
# define SUPPORTS_VSYNC // Vertical sync; only GL does this for now
#endif

#endif


#if defined(_WIN32) && defined(GLQUAKE) && !defined(DX8QUAKE)
#define OLD_SGIS
#endif

#ifdef GLQUAKE
#define NO_MGRAPH
#endif

//  Audio Capabilities

#ifdef MACOSX
# define FULL_BACKGROUND_VOLUME_CONTROL
#endif

#ifdef BUILD_MP3_VERSION
# define FULL_BACKGROUND_VOLUME_CONTROL
#endif

// Define Specific Rendering Capabilities

#ifdef GLQUAKE
# define SUPPORTS_SKYBOX // Only GL
# define SUPPORTS_GLHOMFIX_NEARWATER
#endif

#if defined(GLQUAKE) && !defined(D3DQUAKE)
# define SUPPORTS_GL_DELETETEXTURES
#endif

#if defined(_WIN32) && defined(GLQUAKE) && !defined(D3DQUAKE) // WinQuake you can't see cursor so no point
# define RELEASE_MOUSE_FULLSCREEN // D3DQUAKE gets an error if it loses focus in fullscreen, so that'd be stupid
#endif

#if defined(GLQUAKE) && !defined(D3DQUAKE)
# define SUPPORTS_FOG // Only GL, D3D hates fog
#endif

#ifdef GLQUAKE
# define SUPPORTS_ENTITY_ALPHA
#endif

#define SUPPORTS_TRANSFORM_INTERPOLATION // We are switching to r_interpolate_transform
#ifdef GLQUAKE
# define SUPPORTS_HARDWARE_ANIM_INTERPOLATION
#else
# define SUPPORTS_SOFTWARE_ANIM_INTERPOLATION
#endif

#define SUPPORTS_AUTOID

#ifdef SUPPORTS_AUTOID
#if defined(GLQUAKE) && !defined(D3DQUAKE) && !defined(DX8QUAKE)
# define SUPPORTS_AUTOID_HARDWARE
#else
# define SUPPORTS_AUTOID_SOFTWARE
#endif
#endif

#ifdef GLQUAKE
# define SUPPORTS_2DPICS_ALPHA
#endif

#ifdef GLQUAKE
# define SUPPORTS_CONSOLE_SCALING
#endif

// Alternate Methods

#ifdef FLASH
# define FLASH_FILE_SYSTEM
# define FLASH_CONSOLE_TRACE_ECHO
# define FLASH_SOUND_DIFFERENCE
#endif

// Define Deficiencies and Workarounds

#if defined(_WIN32) && defined(GLQUAKE)
# define INTEL_OPENGL_DRIVER_WORKAROUND // Windows only issue?  Or is Linux affected too?  OS X is not affected
# define GL_QUAKE_SKIN_METHOD
#endif

#ifdef DX8QUAKE
//# undef  SUPPORTS_GLVIDEO_MODESWITCH  // Not now, isn't working right
# define DX8QUAKE_NO_DIALOGS  // No dialogs for DX8QUAKE
# define DX8QUAKE_NO_8BIT
# define DX8QUAKE_GET_GLMAXSIZE
# define DX8QUAKE_NO_FRONT_BACK_BUFFER
# define DX8QUAKE_GL_MAX_SIZE_FAKE
# define DX8QUAKE_ALT_MODEL_TEXTURE
# define DX8QUAKE_ALT_RESAMPLE
# define DX8QUAKE_NO_BINDTEXFUNC
# define DX8QUAKE_NO_GLTEXSORT
# define DX8QUAKE_BAKER_ALTTAB_HACK
#endif

int build_number (void);
void Host_Version_f (void);
char *VersionString (void);



#endif
