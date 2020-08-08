//_________________________________________________________________________________________________________________________nFO
// "gl_vidosx.m" - MacOS X OpenGL Video driver
//
// Written by:	Axel 'awe' Wefers			[mailto:awe@fruitz-of-dojo.de].
//				©2001-2006 Fruitz Of Dojo 	[http://www.fruitz-of-dojo.de].
//
// Quakeª is copyrighted by id software		[http://www.idsoftware.com].
//
// Version History:
// v1.0.9: Moved functions for capturing and fading displays to "vid_shared_osx.m".
// v1.0.8: Fixed an issue with the console aspect, if apspect ratio was not 4:3.
//	       Introduces FSAA via the gl_ARB_multisample extension.
//	       Radeon users may switch FSAA on the fly.
//	       Introduces multitexture option for MacOS X v10.2 [see "video options"].
// v1.0.7: Brings back the video options menu.
//	       "vid_wait" now availavle via video options.
//	       ATI Radeon only:
//	       Added support for FSAA, via variable "gl_fsaa" or video options.
//	       Added support for Truform, via variable "gl_truform" or video options.
//	       Added support for anisotropic texture filtering, via variable "gl_anisotropic" or options.
// v1.0.5: Added "minimized in Dock" mode.
//         Displays are now catured manually due to a bug with CGReleaseAllDisplays().
//	       Reduced the fade duration to 1.0s.
// v1.0.4: Fixed continuous console output, if gamma setting fails.
//	       Fixed a multi-monitor issue.
// v1.0.3: Enables setting the gamma via the brightness slider at the options dialog.
//	       Enable/Disable VBL syncing via "vid_wait".
// v1.0.2: GLQuake/GLQuakeWorld:
//	       Fixed a performance issue [see "gl_rsurf.c"].
//         Default value of "gl_keeptjunctions" is now "1" [see "gl_rmain.c"].
//	       Added "DrawSprocket" style gamma fading at game start/end.
//         Some internal changes.
//         GLQuakeWorld:
//         Fixed console width/height bug with resolutions other than 640x480 [was always 640x480].
// v1.0.1: Initial release.
//____________________________________________________________________________________________________________________iNCLUDES

#pragma mark =Includes=

#import	<AppKit/AppKit.h>
#import <IOKit/graphics/IOGraphicsTypes.h>
#import <OpenGL/OpenGL.h>
#import <OpenGL/gl.h>
#import <OpenGL/glu.h>
#import <OpenGL/glext.h>

#import	"quakedef.h"
#import "in_osx.h"
#import "vid_osx.h"
#import	"sys_osx.h"
#import "Quake.h"
#import "QuakeView.h"
#import "FDGLScreenshot.h"

#pragma mark -

//_____________________________________________________________________________________________________________________dEFINES

#pragma mark =Defines=

#define VID_HWA_ONLY					// allow only systems with HWA OpenGL.

#define VID_WARP_WIDTH			320
#define VID_WARP_HEIGHT			200
#define VID_CONSOLE_MIN_WIDTH	320
#define VID_CONSOLE_MIN_HEIGHT	200
#define VID_FIRST_MENU_LINE		40
#define VID_ATI_FSAA_LEVEL		510		// required for CGLSetParamter () and ATI instant FSAA.

#pragma mark -

//______________________________________________________________________________________________________________________mACROS

#pragma mark =Macros=

#define	SQUARE(A)		((A) * (A))

#pragma mark -

//____________________________________________________________________________________________________________________tYPEDEFS

#pragma mark =TypeDefs=

typedef void 			(*vid_glpntrianglesiatix_t) (GLenum pname, GLint param);
typedef void			(*vid_glpntrianglesfatix_t) (GLenum pname, GLfloat param);

//typedef unsigned int	UInt;
//typedef signed int	SInt;

typedef	enum			{
                                VID_MENUITEM_WAIT,
                                VID_MENUITEM_OVERBRIGHT,
                                VID_MENUITEM_FSAA,
                                VID_MENUITEM_ANISOTROPIC,
                                VID_MENUITEM_MULTITEXTURE,
                                VID_MENUITEM_TRUFORM
                        }	vid_menuitem_t;

#pragma mark -

//__________________________________________________________________________________________________________________iNTERFACES

#pragma mark =ObjC Interfaces=

@interface NSOpenGLContext (CGLContextAccess)
- (CGLContextObj) cglContext;
@end

#pragma mark -

//___________________________________________________________________________________________________________________vARIABLES

#pragma mark =Variables=

const char						*gl_renderer,
								*gl_vendor,
								*gl_version,
								*gl_extensions;

cvar_t							vid_mode = { "vid_mode", "0", 0 },
								vid_redrawfull = { "vid_redrawfull", "0", 0 },
								vid_wait = { "vid_vsync", "1", 1 },
								vid_overbright = { "gamma_overbright", "1", 1 },
								_vid_default_mode = { "_vid_default_mode", "0", 1 },
								_vid_default_blit_mode = { "_vid_default_blit_mode", "0", 1 },
								_windowed_mouse = { "_windowed_mouse","1", 0 },
								gl_anisotropic = { "gl_anisotropic", "0", 1 },
								gl_fsaa = { "gl_fsaa", "0", 0 },
								gl_truform = { "gl_truform", "-1", 1 },
								gl_ztrick = { "gl_ztrick", "0" },
                                gl_multitexture = { "gl_multitexture", "0", 1 };

unsigned						d_8to24table[256];
unsigned char					d_15to8table[65536];

int								texture_extension_number = 1;

GLfloat							gl_texureanisotropylevel = 1.0f;
qboolean						gl_fsaaavailable = NO,
                                gl_mtexable = NO,
                               	gl_pntriangles = NO,
                                gl_texturefilteranisotropic = NO,
                                gl_luminace_lightmaps = NO,
                                gl_palettedtex = NO,
                                isPermedia = NO;

NSDictionary *					gVidDisplayMode;
CGDirectDisplayID				gVidDisplayList[VID_MAX_DISPLAYS];
CGDisplayCount					gVidDisplayCount;
NSWindow *						gVidWindow;
BOOL							gVidDisplayFullscreen,
								gVidFadeAllDisplays,
                                gVidIsMinimized = NO;
UInt32							gVidDisplay;
float							gVidWindowPosX,
                                gVidWindowPosY;
SInt32                          gGLMultiSamples = 0;

static const float				gGLTruformAmbient[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
static GLuint					gGLGrowboxTexture;
static BOOL						gGLGrowboxInitialised = NO;
static NSDictionary *			gGLOriginalMode;
NSOpenGLContext *				gGLContext;
static QuakeView *				gGLWindowView = NULL;
static NSImage *				gGLMiniWindow = NULL;
static NSBitmapImageRep *		gGLMiniWindowBuffer = NULL;
static NSRect					gGLMiniWindowRect;
static BOOL						gGLDisplayIs8Bit,
                                gGLAnisotropic = NO,
                                gGLMultiTextureAvailable = NO,
                                gGLMultiTexture = NO;
UInt32							gGLDisplayWidth, 
                                gGLDisplayHeight;
static float					gGLVideoWait = 0.0f,
                                gGLFSAALevel = 1.0f,
                                gGLPNTriangleLevel = -1.0f;
static SInt8					gGLMenuMaxLine,
                                gGLMenuLine = VID_FIRST_MENU_LINE;
static vid_menuitem_t           gGLMenuItem = VID_MENUITEM_WAIT;
static vid_glpntrianglesiatix_t	gGLPNTrianglesiATIX = NULL;
static vid_glpntrianglesfatix_t	gGLPNTrianglesfATIX = NULL;

#pragma mark -

//_________________________________________________________________________________________________________fUNCTION_pROTOTYPES

#pragma mark =Function ProtoTypes =

static	void		VID_CheckGamma (unsigned char *);
static	void		VID_SetWait (UInt32);
static	BOOL		VID_SetDisplayMode (void);
static	void 		VID_MenuDraw (void);
static	void		VID_MenuKey (int theKey);

BOOL				GL_CheckARBMultisampleExtension (CGDirectDisplayID theDisplay);

static	BOOL		GL_ExtensionSupported (const char *theExtension);
static	void		GL_CheckMultiTextureExtensions (void);
static	void		GL_CheckPalettedTexture (void);
static	void		GL_CheckPNTrianglesExtensions (void);
static	void		GL_CheckSwitchFSAAOnTheFly (void);
static	void		GL_CheckTextureFilterAnisotropic (void);
static	void		GL_CheckLuminanceLightmaps (void);
static	void		GL_Init (void);
static	void		GL_SetFSAA (UInt32 theFSAALevel);
static	void		GL_SetTextureFilterAnisotropic (UInt32 theState);
static	void		GL_SetPNTriangles (SInt32 thePNTriangleLevel);
static	void		GL_SetMultiTexture (UInt32 theState);
static	void		GL_RenderInsideDock (void);

#pragma mark -

//____________________________________________________________________________________________________________VID_LockBuffer()

#ifdef QUAKE_WORLD

void 	VID_LockBuffer (void)
{
}

#endif /* QUAKE_WORLD */

//__________________________________________________________________________________________________________VID_UnlockBuffer()

#ifdef QUAKE_WORLD

void	VID_UnlockBuffer (void)
{
}

#endif /* QUAKE_WORLD */

//________________________________________________________________________________________________________________VID_Is8bit()

qboolean VID_Is8bit (void)
{
    return (gGLDisplayIs8Bit);
}

//____________________________________________________________________________________________________________VID_CheckGamma()

void	VID_CheckGamma (unsigned char *thePalette)
{
    float			myGamma;
    float			myNewValue;
    unsigned char	myPalette[768];
    SInt			i;

    if ((i = COM_CheckParm ("-gamma")) == 0)
    {
        if ((gl_renderer && strstr (gl_renderer, "Voodoo")) || (gl_vendor && strstr (gl_vendor, "3Dfx")))
        {
            myGamma = 1.0f;
        }
		else
        {
            myGamma = 0.7f;
        }
    }
    else
    {
		myGamma = Q_atof (com_argv[i+1]);
    }
    
    for (i = 0 ; i < 768 ; i++)
    {
        myNewValue = pow ((thePalette[i] + 1) / 256.0f, myGamma) * 255 + 0.5f;
        
		if (myNewValue < 0.0f)
            myNewValue = 0.0f;
            
        if (myNewValue > 255.0f)
            myNewValue = 255.0f;
            
		myPalette[i] = (unsigned char) myNewValue;
    }
    
    memcpy (thePalette, myPalette, sizeof (myPalette));
}

//______________________________________________________________________________________________________________VID_SetGamma()

void	VID_SetGamma (void)
{
    static float	myOldGamma = 0.0f,
					myOldOverbright = 1.0f;

    if (myOldGamma != v_gamma.value || myOldOverbright != vid_overbright.value)
    {
        if (vid_overbright.value == 0.0f)
        {
            if (gVshGammaTable != NULL)
            {
                static vid_gammatable_t		myNewTable;
                UInt16 				i;

                myOldGamma = (1.4f - v_gamma.value) * 2.5f;
                if (myOldGamma < 1.0f)
                {
                    myOldGamma = 1.0f;
                }
                else
                {
                    if (myOldGamma > 2.25f)
                    {
                        myOldGamma = 2.25f;
                    }
                }
                
                for (i = 0; i < gVshGammaTable->count; i++)
                {
                    myNewTable.red[i]   = myOldGamma * gVshGammaTable->red[i];
                    myNewTable.green[i] = myOldGamma * gVshGammaTable->green[i];
                    myNewTable.blue[i]  = myOldGamma * gVshGammaTable->blue[i];
                }
                
                CGSetDisplayTransferByTable (gVidDisplayList[gVidDisplay], gVshGammaTable->count,
                                             myNewTable.red, myNewTable.green, myNewTable.blue);
            }
            else
            {
                Con_Printf ("Can\'t set the requested gamma value!\n");
            }
        }
        else
        {
            if (gVshOriginalGamma != NULL)
            {
                // set the current gamma:
                myOldGamma = 1.0f - v_gamma.value;
                if (myOldGamma < 0.0f)
                {
                    myOldGamma = 0.0f;
                }
                else
                {
                    if (myOldGamma >= 1.0f)
                    {
                        myOldGamma = 0.999f;
                    }
                }
                CGSetDisplayTransferByFormula (gVshOriginalGamma[gVidDisplay].displayID,
                                               myOldGamma,
                                               1.0f,
                                               gVshOriginalGamma[gVidDisplay].component[2],
                                               myOldGamma,
                                               1.0f,
                                               gVshOriginalGamma[gVidDisplay].component[5],
                                               myOldGamma,
                                               1.0f,
                                               gVshOriginalGamma[gVidDisplay].component[8]);
            }
            else
            {
                Con_Printf ("Can\'t set the requested gamma value!\n");
            }
            
            // has to be 0 or 1:
            if (vid_overbright.value != 1.0f)
            {
                Cvar_SetValue (vid_overbright.name, 1.0f);
            }
        }
        myOldGamma = v_gamma.value;
        myOldOverbright = vid_overbright.value;
    }
}

//____________________________________________________________________________________________________________VID_SetPalette()

void	VID_SetPalette (UInt8 *thePalette)
{
    UInt		myRedComp, myGreenComp, myBlueComp,
				myColorValue, myBestValue,
				*myColorTable;
    SInt		myNewRedComp, myNewGreenComp, myNewBlueComp,
				myCurDistance, myBestDistance;
    UInt16		i;
    UInt8		*myPalette;

    myPalette	 = thePalette;
    myColorTable = d_8to24table;
    
    for (i = 0; i < 256; i++)
    {
        myRedComp       = myPalette[0];
        myGreenComp     = myPalette[1];
		myBlueComp      = myPalette[2];

#ifdef __LITTLE_ENDIAN__
		myColorValue	= (myRedComp <<  0) + (myGreenComp <<  8) + (myBlueComp << 16) + (0xFF << 24);
#else
		myColorValue    = (myRedComp << 24) + (myGreenComp << 16) + (myBlueComp <<  8) + (0xFF <<  0);
#endif // __LITTLE_ENDIAN__

		*myColorTable++ = myColorValue;
		myPalette       += 3;
    }
    
#ifdef __LITTLE_ENDIAN__
    d_8to24table[255] &= 0x00ffffff;
#else
    d_8to24table[255] &= 0xffffff00;
#endif // __LITTLE_ENDIAN__
    
    for (i = 0; i < (1 << 15); i++)
    {
        myRedComp   = ((i & 0x001F) << 3) + 4;
		myGreenComp = ((i & 0x03E0) >> 2) + 4;
		myBlueComp  = ((i & 0x7C00) >> 7) + 4;
        
		myPalette   = (UInt8 *) d_8to24table;
        
		for (myColorValue = 0 , myBestValue = 0, myBestDistance = SQUARE(10000); myColorValue < 256; myColorValue++, myPalette += 4)
        {
            myNewRedComp   = (SInt) myRedComp   - (SInt) myPalette[0];
            myNewGreenComp = (SInt) myGreenComp - (SInt) myPalette[1];
            myNewBlueComp  = (SInt) myBlueComp  - (SInt) myPalette[2];
            
            myCurDistance  = SQUARE(myNewRedComp) + SQUARE(myNewGreenComp) + SQUARE(myNewBlueComp);
            
            if (myCurDistance < myBestDistance)
            {
                myBestValue = myColorValue;
				myBestDistance = myCurDistance;
            }
		}
		d_15to8table[i] = myBestValue;
    }
}

//__________________________________________________________________________________________________________VID_ShiftPalette()

void	VID_ShiftPalette (UInt8 *thePalette)
{
    // we could set gamma here [like at the "gl_vidnt.c" code],
    // but this would cause unnecessary gamma table updates. so do nothing...
}

//_______________________________________________________________________________________________________________VID_SetMode()

SInt 	VID_SetMode (SInt modenum, UInt8 *thePalette)
{
    return (1);
}

//________________________________________________________________________________________________________VID_SetWindowTitle()

#ifdef QUAKE_WORLD

void	VID_SetWindowTitle (char *theTitle)
{
    if (gVidWindow != NULL)
    {
        NSString	*myTitle = [NSString stringWithCString: theTitle];
        
        if (myTitle != NULL)
        {
            [gVidWindow setTitle: myTitle];
        }
    }
}

#endif /* QUAKE_WORLD */

//___________________________________________________________________________________________________VID_CreateGLPixelFormat()

NSOpenGLPixelFormat *	VID_CreateGLPixelFormat (BOOL theFullscreenMode)
{
    NSOpenGLPixelFormat				*myPixelFormat;
    NSOpenGLPixelFormatAttribute	myAttributeList[32];
    UInt32							myDisplayDepth;
    UInt16							i = 0;

    myAttributeList[i++] = NSOpenGLPFANoRecovery;

    myAttributeList[i++] = NSOpenGLPFAClosestPolicy;

#ifdef VID_HWA_ONLY
    myAttributeList[i++] = NSOpenGLPFAAccelerated;
#endif /* VID_HWA_ONLY */

    myAttributeList[i++] = NSOpenGLPFADoubleBuffer;

    myAttributeList[i++] = NSOpenGLPFADepthSize;
    myAttributeList[i++] = 1;

    myAttributeList[i++] = NSOpenGLPFAAlphaSize;
    myAttributeList[i++] = 0;
    
    myAttributeList[i++] = NSOpenGLPFAStencilSize;
    myAttributeList[i++] = 0;

    myAttributeList[i++] = NSOpenGLPFAAccumSize;
    myAttributeList[i++] = 0;

    if (theFullscreenMode == YES)
    {
        myDisplayDepth = [[gVidDisplayMode objectForKey: (id)kCGDisplayBitsPerPixel] intValue];
        
        myAttributeList[i++] = NSOpenGLPFAFullScreen;
        myAttributeList[i++] = NSOpenGLPFAScreenMask;
        myAttributeList[i++] = CGDisplayIDToOpenGLDisplayMask (gVidDisplayList[gVidDisplay]);
    }
    else
    {
        myDisplayDepth = [[gGLOriginalMode objectForKey: (id)kCGDisplayBitsPerPixel] intValue];
        myAttributeList[i++] = NSOpenGLPFAWindow;
    }

    myAttributeList[i++] = NSOpenGLPFAColorSize;
    myAttributeList[i++] = myDisplayDepth;

    if (gGLMultiSamples > 0)
    {
        if (gGLMultiSamples > 8)
        {
            gGLMultiSamples = 8;
        }
        myAttributeList[i++] = NSOpenGLPFASampleBuffers;
        myAttributeList[i++] = 1;
        myAttributeList[i++] = NSOpenGLPFASamples;
        myAttributeList[i++] = gGLMultiSamples;
    }
        
    myAttributeList[i++] = 0;
  
    myPixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes: myAttributeList];
    gGLDisplayIs8Bit = (myDisplayDepth == 8);

    return (myPixelFormat);
}

//_______________________________________________________________________________________________________________VID_SetWait()

void VID_SetWait (UInt32 theState)
{
	const long params = theState;
	
    // set theState to 1 to enable, to 0 to disable VBL syncing.
    [gGLContext makeCurrentContext];
    if(CGLSetParameter (CGLGetCurrentContext (), kCGLCPSwapInterval, &params) == CGDisplayNoErr)
    {
        gGLVideoWait = vid_wait.value;
        if (theState == 0)
        {
            Con_Printf ("video wait successfully disabled!\n");
        }
        else
        {
            Con_Printf ("video wait successfully enabled!\n");
        }
    }
    else
    {
        vid_wait.value = gGLVideoWait;
        Con_Printf ("Error while trying to change video wait!\n");
    }    
}

//_______________________________________________________________________________________________________________VID_SetMode()

BOOL	VID_SetDisplayMode (void)
{
    NSOpenGLPixelFormat		*myPixelFormat;

    // just for security:
    if (gVidDisplayList == NULL || gVidDisplayCount == 0)
    {
        Sys_Error ("Invalid list of active displays!");
    }

    // save the old display mode:
    gGLOriginalMode = (NSDictionary *) CGDisplayCurrentMode (gVidDisplayList[gVidDisplay]);
    
    if (gVidDisplayFullscreen == YES)
    {
        // hide cursor:
        IN_ShowCursor (NO);

        // fade the display(s) to black:
        VSH_FadeGammaOut (gVidFadeAllDisplays, VID_FADE_DURATION);
        
        // capture display(s);
        if (VSH_CaptureDisplays (gVidFadeAllDisplays) == NO)
            Sys_Error ("Unable to capture display(s)!");
        
        // switch the display mode:
        if (CGDisplaySwitchToMode (gVidDisplayList[gVidDisplay], (CFDictionaryRef) gVidDisplayMode)
            != CGDisplayNoErr)
        {
            Sys_Error ("Unable to switch the displaymode!");
        }
    }
    
    // get the pixel format:
    if ((myPixelFormat = VID_CreateGLPixelFormat (gVidDisplayFullscreen)) == NULL)
        Sys_Error ("Unable to find a matching pixelformat. Please try other displaymode(s).");

    // initialize the OpenGL context:
    if (!(gGLContext = [[NSOpenGLContext alloc] initWithFormat: myPixelFormat shareContext: nil]))
        Sys_Error ("Unable to create an OpenGL context. Please try other displaymode(s).");

    // get rid of the pixel format:
    [myPixelFormat release];

    if (gVidDisplayFullscreen)
    {
        // set the OpenGL context to fullscreen:
        if (CGLSetFullScreen ([gGLContext cglContext]) != CGDisplayNoErr)
            Sys_Error ("Unable to use the selected displaymode for fullscreen OpenGL.");
        
        // fade the gamma back:
        VSH_FadeGammaIn (gVidFadeAllDisplays, 0.0f);
    }
    else
    {
		// setup the window according to our settings:
		NSRect				myContentRect	= NSMakeRect (0, 0, gGLDisplayWidth, gGLDisplayHeight);

        gVidWindow = [[NSWindow alloc] initWithContentRect: myContentRect
                                                 styleMask: NSTitledWindowMask |
                                                            NSClosableWindowMask |
                                                            NSMiniaturizableWindowMask |
                                                            NSResizableWindowMask
                                                   backing: NSBackingStoreBuffered
                                                     defer: NO];
#ifdef QUAKE_WORLD
        [gVidWindow setTitle: @"GLQuakeWorld"];
#else
        [gVidWindow setTitle: @"GLQuake"];
#endif /* QUAKE_WORLD */

        gGLWindowView = [[QuakeView alloc] initWithFrame: myContentRect];
		
        if (gGLWindowView == NULL)
        {
            Sys_Error ("Unable to create content view!\n");
        }

        // setup the view for tracking the window location:
        [gVidWindow setDocumentEdited: YES];
        [gVidWindow setMinSize: myContentRect.size];
        [gVidWindow setShowsResizeIndicator: NO];
        [gVidWindow setBackgroundColor: [NSColor blackColor]];
        [gVidWindow useOptimizedDrawing: NO];
        [gVidWindow setContentView: gGLWindowView];
        [gVidWindow makeFirstResponder: gGLWindowView];
        [gVidWindow setDelegate: gGLWindowView];

        // attach the OpenGL context to the window:
        [gGLContext setView: [gVidWindow contentView]];
        
        // finally show the window:
		[gVidWindow center];
        [gVidWindow setAcceptsMouseMovedEvents: YES];
        [gVidWindow makeKeyAndOrderFront: nil];
        [gVidWindow makeMainWindow];
		[gVidWindow center];
        [gVidWindow flushWindow];

        // setup the miniwindow [if one alloc fails, the miniwindow will not be drawn]:
        gGLMiniWindow = [[NSImage alloc] initWithSize: NSMakeSize (128.0f, 128.0f)];
        gGLMiniWindowRect = NSMakeRect (0.0f, 0.0f, [gGLMiniWindow size].width, [gGLMiniWindow size].height);
        [gGLMiniWindow setFlipped: YES];
        VSH_DisableQuartzInterpolation (gGLMiniWindow);
        
        // obtain window buffer:
        gGLMiniWindowBuffer = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes: NULL
                                                                      pixelsWide: NSWidth (gGLMiniWindowRect)
                                                                      pixelsHigh: NSHeight (gGLMiniWindowRect)
                                                                   bitsPerSample: 8
                                                                 samplesPerPixel: 4
                                                                        hasAlpha: YES
                                                                        isPlanar: NO
                                                                  colorSpaceName: NSDeviceRGBColorSpace
                                                                     bytesPerRow: NSWidth (gGLMiniWindowRect) * 4
                                                                    bitsPerPixel: 32];
        if (gGLMiniWindowBuffer == NULL)
        {
            Sys_Error ("Unabled to allocate the window buffer!\n");
        }
    }
    
    // Lock the OpenGL context to the refresh rate of the display [for clean rendering], if desired:
    VID_SetWait((UInt32) vid_wait.value);
    
    return (YES);
}

//__________________________________________________________________________________________________________________VID_Init()
void VID_Consize_f(void);
extern cvar_t vid_consize;
void	VID_Init (unsigned char *thePalette)
{
    char		myGLDir[MAX_OSPATH];
    UInt		i;

    // register miscelanous vars:
    Cvar_RegisterVariable (&vid_mode, NULL);
    Cvar_RegisterVariable (&_vid_default_mode, NULL);
    Cvar_RegisterVariable (&_vid_default_blit_mode, NULL);
    Cvar_RegisterVariable (&vid_wait, NULL);
    Cvar_RegisterVariable (&vid_redrawfull, NULL);
    Cvar_RegisterVariable (&vid_overbright, NULL);
    Cvar_RegisterVariable (&vid_consize, &VID_Consize_f); //Baker 3.97: this supercedes vid_conwidth/vid_conheight cvars
    Cvar_RegisterVariable (&_windowed_mouse, NULL);
    Cvar_RegisterVariable (&gl_anisotropic, NULL);
    Cvar_RegisterVariable (&gl_fsaa, NULL);
    Cvar_RegisterVariable (&gl_truform, NULL);
    Cvar_RegisterVariable (&gl_multitexture, NULL);
    Cvar_RegisterVariable (&gl_ztrick, NULL);
        
    // setup basic width/height:
    vid.maxwarpwidth = VID_WARP_WIDTH;
    vid.maxwarpheight = VID_WARP_HEIGHT;
    vid.colormap = host_colormap;
    vid.fullbright = 256 - LittleLong (*((SInt *)vid.colormap + 2048));

    gGLDisplayWidth  = [[gVidDisplayMode objectForKey: (id)kCGDisplayWidth] intValue];
    gGLDisplayHeight = [[gVidDisplayMode objectForKey: (id)kCGDisplayHeight] intValue];

    // get width from command line parameters [only for windowed mode]:
    if ((i = COM_CheckParm("-width")))
        gGLDisplayWidth = atoi (com_argv[i+1]);
    vid.width = gGLDisplayWidth;

    // get height from command line parameters [only for windowed mode]:
    if ((i = COM_CheckParm("-height")))
        gGLDisplayHeight = atoi (com_argv[i+1]);
    vid.height = gGLDisplayHeight;
    
    vid.aspect = ((float) vid.height / (float) vid.width) * (320.0f / 240.0f);
    vid.numpages = 2;

    // switch the video mode:
    if (VID_SetDisplayMode() == NO)
        Sys_Error ("Can\'t initialize video!");

    // setup console width according to display width:
    if ((i = COM_CheckParm("-conwidth")))
        vid.conwidth = Q_atoi (com_argv[i+1]);
    else
        vid.conwidth = vid.width;
    vid.conwidth &= 0xfff8;

    // setup console height according to display height:
    if ((i = COM_CheckParm ("-conheight")))
        vid.conheight = Q_atoi (com_argv[i+1]);
    else
        vid.conheight = vid.height;

    // check the console size:
    if (vid.conwidth > gGLDisplayWidth)
        vid.conwidth = gGLDisplayWidth;
    if (vid.conheight > gGLDisplayHeight)
        vid.conheight = gGLDisplayHeight;

    if (vid.conwidth < VID_CONSOLE_MIN_WIDTH)
        vid.conwidth = VID_CONSOLE_MIN_WIDTH;
    if (vid.conheight < VID_CONSOLE_MIN_HEIGHT)
        vid.conheight = VID_CONSOLE_MIN_HEIGHT;

    VID_Consize_f();
    
    // setup OpenGL:
    GL_Init ();

//    // setup the "glquake" folder within the "id1" folder:
//    snprintf (myGLDir, MAX_OSPATH, "%s/glquake", com_gamedir);
//    Sys_mkdir (myGLDir);

    // enable the video options menu:
    vid_menudrawfn = VID_MenuDraw;
    vid_menukeyfn = VID_MenuKey;
    
    // finish up initialization:
    VID_CheckGamma (thePalette);
    VID_SetPalette (thePalette);
    Con_SafePrintf ("Video mode %dx%d initialized.\n", gGLDisplayWidth, gGLDisplayHeight);
    vid.recalc_refdef = 1;
}

//______________________________________________________________________________________________________________VID_Shutdown()

void	VID_Shutdown (void)
{
	if (gGLGrowboxInitialised == YES)
	{
		glDeleteTextures(1, &gGLGrowboxTexture);
	}

    // release the miniwindow:
    if (gGLMiniWindow != NULL)
    {
        [gGLMiniWindow release];
        gGLMiniWindow = NULL;
    }

    // release the buffer of the mini window:
    if (gGLMiniWindowBuffer != NULL)
    {
        [gGLMiniWindowBuffer release];
        gGLMiniWindowBuffer = NULL;
    }

    // close the old window [will automagically be released when closed]:
    if (gVidWindow != NULL)
    {
        [gVidWindow close];
        gVidWindow = NULL;
    }

    // close the content view:
    if (gGLWindowView != NULL)
    {
        [gGLWindowView release];
        gGLWindowView = NULL;
    }
    
    // fade gamma out, to avoid splash:
    if (gVidDisplayFullscreen == YES && gGLOriginalMode != NULL)
    {
        VSH_FadeGammaOut (gVidFadeAllDisplays, 0.0f);
    }

    // clean up the OpenGL context:
    if (gGLContext != NULL)
    {
        [NSOpenGLContext clearCurrentContext];
        [gGLContext clearDrawable];
        [gGLContext release];
        gGLContext = NULL;
    }
    
    // restore the old display mode:
    if (gVidDisplayFullscreen == YES)
    {
        if (gGLOriginalMode != NULL)
        {
            CGDisplaySwitchToMode (gVidDisplayList[gVidDisplay], (CFDictionaryRef) gGLOriginalMode);
        
            VSH_ReleaseDisplays (gVidFadeAllDisplays);

            VSH_FadeGammaIn (gVidFadeAllDisplays, VID_FADE_DURATION);
            
            // clean up the gamma fading:
            VSH_FadeGammaRelease ();
        }
    }
    else
    {
        if (gVidWindow != NULL)
        {
            [gVidWindow release];
        }
    }
}

//______________________________________________________________________________________________________________VID_MenuDraw()

void	VID_MenuDraw (void)
{
    qpic_t	*myPicture;
    UInt8	myRow = VID_FIRST_MENU_LINE;
	char	myString[16];

    myPicture = Draw_CachePic ("gfx/vidmodes.lmp");
    M_DrawPic ((320 - myPicture->width) / 2, 4, myPicture);
    
    // draw vid_wait option:
    M_Print (VID_FONT_WIDTH, myRow, "Video Sync:");
    if (vid_wait.value) M_Print ((39 - 2) * VID_FONT_WIDTH, myRow, "On");
        else M_Print ((39 - 3) * VID_FONT_WIDTH, myRow, "Off");
    if (gGLMenuLine == myRow) gGLMenuItem = VID_MENUITEM_WAIT;
    myRow += VID_FONT_HEIGHT;

    // draw vid_overbright option:
    M_Print (VID_FONT_WIDTH, myRow, "Overbright Gamma:");
    if (vid_overbright.value) M_Print ((39 - 2) * VID_FONT_WIDTH, myRow, "On");
        else M_Print ((39 - 3) * VID_FONT_WIDTH, myRow, "Off");
    if (gGLMenuLine == myRow) gGLMenuItem = VID_MENUITEM_OVERBRIGHT;    
    
    // draw FSAA option:
    if (gl_fsaaavailable == YES)
    {
        myRow += VID_FONT_HEIGHT;
        M_Print (VID_FONT_WIDTH, myRow, "FSAA:");
        if (gl_fsaa.value == 0.0f)
        {
             M_Print ((39 - 3) * VID_FONT_WIDTH, myRow, "Off");
        }
        else
        {
            snprintf (myString, 16, "%dx", (int) gl_fsaa.value);
            M_Print ((39 - strlen (myString)) * VID_FONT_WIDTH, myRow, myString);
        }
		
        if (gGLMenuLine == myRow)
		{
			gGLMenuItem = VID_MENUITEM_FSAA;
		}
    }
    
    // draw anisotropic option:
    if (gl_texturefilteranisotropic == YES)
    {
        myRow += VID_FONT_HEIGHT;
        M_Print (VID_FONT_WIDTH, myRow, "Anisotropic Texture Filtering:");
		
        if (gl_anisotropic.value)
		{
			M_Print ((39 - 2) * VID_FONT_WIDTH, myRow, "On");
		}
		else
		{
            M_Print ((39 - 3) * VID_FONT_WIDTH, myRow, "Off");
		}
		
        if (gGLMenuLine == myRow)
		{
			gGLMenuItem = VID_MENUITEM_ANISOTROPIC;
		}
    }
    
    // draw multitexture option:
    if (gGLMultiTextureAvailable == YES)
    {
        myRow += VID_FONT_HEIGHT;
        M_Print (VID_FONT_WIDTH, myRow, "Multitexturing:");
		
        if (gl_multitexture.value)
		{
			M_Print ((39 - 2) * VID_FONT_WIDTH, myRow, "On");
		}
		else
		{
			M_Print ((39 - 3) * VID_FONT_WIDTH, myRow, "Off");
		}
		
        if (gGLMenuLine == myRow)
		{
			gGLMenuItem = VID_MENUITEM_MULTITEXTURE;
		}
    }
    
    // draw truform option:
    if (gl_pntriangles == YES)
    {
        myRow += VID_FONT_HEIGHT;
        M_Print (VID_FONT_WIDTH, myRow, "ATI Truform Tesselation Level:");
		
        if (gl_truform.value < 0)
        {
             M_Print ((39 - 3) * VID_FONT_WIDTH, myRow, "Off");
        }
        else
        {
            snprintf (myString, 16, "%dx", (int) gl_truform.value);
            M_Print ((39 - strlen (myString)) * VID_FONT_WIDTH, myRow, myString);
        }
		
        if (gGLMenuLine == myRow)
		{
			gGLMenuItem = VID_MENUITEM_TRUFORM;
		}
    }

    M_Print (4 * VID_FONT_WIDTH + 4, 36 + 23 * VID_FONT_HEIGHT, "Video modes must be set at the");
    M_Print (11 * VID_FONT_WIDTH + 4, 36 + 24 * VID_FONT_HEIGHT, "startup dialog!");
    
    M_DrawCharacter (0, gGLMenuLine, 12 + ((int)(realtime * 4) & 1));
    gGLMenuMaxLine = myRow;
}

//_______________________________________________________________________________________________________________VID_MenuKey()

void	VID_MenuKey (int theKey)
{
    switch (theKey)
    {
        case K_ESCAPE:
            S_LocalSound ("misc/menu1.wav");
            M_Menu_Options_f ();
            break;
		case K_UPARROW:
            S_LocalSound ("misc/menu1.wav");
            gGLMenuLine -= VID_FONT_HEIGHT;
            if (gGLMenuLine < VID_FIRST_MENU_LINE)
                gGLMenuLine = gGLMenuMaxLine;
            break;
		case K_DOWNARROW:
            S_LocalSound ("misc/menu1.wav");
            gGLMenuLine += VID_FONT_HEIGHT;
            if (gGLMenuLine > gGLMenuMaxLine)
                gGLMenuLine = VID_FIRST_MENU_LINE;
            break;
        case K_LEFTARROW:
            S_LocalSound ("misc/menu1.wav");
			
            switch (gGLMenuItem)
            {
                case VID_MENUITEM_WAIT:
                    Cvar_SetValue (vid_wait.name, (vid_wait.value == 0.0f) ? 1.0f : 0.0f);
                    break;
                case VID_MENUITEM_OVERBRIGHT:
                    Cvar_SetValue (vid_overbright.name, (vid_overbright.value == 0.0f) ? 1.0f : 0.0f);
                    break;
                case VID_MENUITEM_FSAA:
                    Cvar_SetValue (gl_fsaa.name, (gl_fsaa.value <= 0.0f) ? 8.0f : gl_fsaa.value - 4.0f);
                    break;
                case VID_MENUITEM_ANISOTROPIC:
                    Cvar_SetValue (gl_anisotropic.name, (gl_anisotropic.value == 0.0f) ? 1.0f : 0.0f);
                    break;
                case VID_MENUITEM_MULTITEXTURE:
                    Cvar_SetValue (gl_multitexture.name, (gl_multitexture.value == 0.0f) ? 1.0f : 0.0f);
                    break;
                case VID_MENUITEM_TRUFORM:
                    Cvar_SetValue (gl_truform.name, (gl_truform.value <= -1.0f) ? 7.0f : gl_truform.value-1.0f);
                    break;
            }
            break;
        case K_RIGHTARROW:
		case K_ENTER:
            S_LocalSound ("misc/menu1.wav");
			
            switch (gGLMenuItem)
            {
                case VID_MENUITEM_WAIT:
                    Cvar_SetValue (vid_wait.name, (vid_wait.value == 0.0f) ? 1.0f : 0.0f);
                    break;
                case VID_MENUITEM_OVERBRIGHT:
                    Cvar_SetValue (vid_overbright.name, (vid_overbright.value == 0.0f) ? 1.0f : 0.0f);
                    break;
                case VID_MENUITEM_FSAA:
                    Cvar_SetValue (gl_fsaa.name, (gl_fsaa.value >= 8.0f) ? 0.0f : gl_fsaa.value + 4.0f);
                    break;
                case VID_MENUITEM_ANISOTROPIC:
                    Cvar_SetValue (gl_anisotropic.name, (gl_anisotropic.value == 0.0f) ? 1.0f : 0.0f);
                    break;
                case VID_MENUITEM_MULTITEXTURE:
                    Cvar_SetValue (gl_multitexture.name, (gl_multitexture.value == 0.0f) ? 1.0f : 0.0f);
                    break;
                case VID_MENUITEM_TRUFORM:
                    Cvar_SetValue (gl_truform.name, (gl_truform.value >= 7.0f) ? -1.0f : gl_truform.value+1.0f);
                    break;
            }
            break;
        default:
            break;
    }
}

//_____________________________________________________________________________________________________GL_ExtensionSupported()

BOOL	GL_ExtensionSupported (const char *theExtension)
{
    const char	*myCurPos;
    char		*myExtStart,
				*myTerminator;
    UInt32		myLength;

    if (theExtension == NULL				||
        gl_extensions == NULL				||
        strchr (theExtension, ' ') != NULL	||
        *theExtension == '\0')
    {
        return (NO);
    }

    myCurPos = gl_extensions;
    myLength = strlen(theExtension);
    
    while (1)
    {
        myExtStart = strstr (myCurPos, theExtension);
		
        if (myExtStart == NULL)
        {
            break;
        }

        myTerminator = myExtStart + myLength;
		
        if ((myExtStart == myCurPos || *(myExtStart - 1) == ' ') && (*myTerminator == ' ' || *myTerminator == '\0'))
        {
            return (YES);
        }

        myCurPos = myTerminator;
    }

    return (NO);
}

//____________________________________________________________________________________________GL_CheckMultiTextureExtensions()

void	GL_CheckMultiTextureExtensions (void) 
{
    // look for the extension:
    if (GL_ExtensionSupported ("GL_ARB_multitexture") == YES &&
        gl_luminace_lightmaps == YES &&
        !COM_CheckParm ("-nomtex"))
    {
        // attach symbols:
        qglSelectTextureSGIS = Sys_GetProcAddress ("glActiveTextureARB", NO);
		qglMTexCoord2fSGIS = Sys_GetProcAddress ("glMultiTexCoord2fARB", NO);
        
        // symobls present?
		if (qglMTexCoord2fSGIS != NULL && qglSelectTextureSGIS != NULL)
        {
            GLint	myMaxTextureUnits;
    
            glGetIntegerv (GL_MAX_TEXTURE_UNITS_ARB, &myMaxTextureUnits); 
            
            // allow multitexturing only if we have at least two texture units:
            if (myMaxTextureUnits >= 2)
            {
                Con_Printf ("Found GL_ARB_multitexture...\n(%d texture units)\n", myMaxTextureUnits);
                gGLMultiTextureAvailable = YES;
            }
            else
            {
                Con_Printf ("Too less texture units (%d).\nGL_ARB_multitexture disabled.\n", myMaxTextureUnits);
                gGLMultiTextureAvailable = NO;
            }
		}
        else
        {
            Con_Printf ("Failed to look up symbol.\nGL_ARB_multitexture disabled.\n");
            gGLMultiTextureAvailable = NO;
        }
    }
    else
    {
        gGLMultiTextureAvailable = NO;
    }
    gl_mtexable = NO;
}

//___________________________________________________________________________________________________GL_CheckPalettedTexture()

void	GL_CheckPalettedTexture (void)
{
    // will never be supported under MacOS X. nevertheless here we go...
    if (GL_ExtensionSupported ("GL_EXT_paletted_texture") == YES)
    {
        gl_palettedtex = YES;
        Con_Printf ("Found GL_EXT_paletted_texture...\n");
    }
    else
    {
        gl_palettedtex = NO;
    }
}

//_____________________________________________________________________________________________GL_CheckPNTrianglesExtensions()

void	GL_CheckPNTrianglesExtensions (void)
{
    if (GL_ExtensionSupported ("GL_ATIX_pn_triangles") == YES)
    {
        // get the required symbols from the dyld:
        gGLPNTrianglesiATIX = Sys_GetProcAddress ("glPNTrianglesiATIX", NO);
        gGLPNTrianglesfATIX = Sys_GetProcAddress ("glPNTrianglesfATIX", NO);

        // are the symbols present?
        if (gGLPNTrianglesiATIX != NULL && gGLPNTrianglesfATIX != NULL)
        {
            Con_Printf ("Found GL_ATIX_pn_triangles...\n");       
            gl_pntriangles = YES;
        }
        else
        {
            Con_Printf ("Failed to look up symbol. GL_ATIX_pn_triangles disabled.\n");
            gl_pntriangles = NO;
        }
    }
    else
    {
	gl_pntriangles = NO;    
    }
}

//___________________________________________________________________________________________GL_CheckARBMultisampleExtension()

BOOL	GL_CheckARBMultisampleExtension (CGDirectDisplayID theDisplay)
{
    CGLRendererInfoObj		myRendererInfo;
    CGLError				myError;
    UInt64					myDisplayMask;
    long					myCount,
							myIndex,
							mySampleBuffers,
							mySamples,
							myMaxSampleBuffers = 0,
                            myMaxSamples = 0;

    // retrieve the renderer info for the selected display:
    myDisplayMask = CGDisplayIDToOpenGLDisplayMask (theDisplay);
    myError = CGLQueryRendererInfo (myDisplayMask, &myRendererInfo, &myCount);
    
    if (myError == kCGErrorSuccess)
    {
        // loop through all renderers:
        for (myIndex = 0; myIndex < myCount; myIndex++)
        {
            // check if the current renderer supports sample buffers:
            myError = CGLDescribeRenderer (myRendererInfo, myIndex, kCGLRPMaxSampleBuffers, &mySampleBuffers);
            if (myError == kCGErrorSuccess && mySampleBuffers > 0)
            {
                // retrieve the number of samples supported by the current renderer:
                myError = CGLDescribeRenderer (myRendererInfo, myIndex, kCGLRPMaxSamples, &mySamples);
                if (myError == kCGErrorSuccess && mySamples > myMaxSamples)
                {
                    myMaxSampleBuffers = mySampleBuffers;
                    myMaxSamples = mySamples;
                }
            }
        }
        
        // get rid of the renderer info:
        CGLDestroyRendererInfo (myRendererInfo);
    }
    
    // NOTE: we could return the max number of samples at this point, but unfortunately there is a bug
    //       with the ATI Radeon/PCI drivers: We would return 4 instead of 8. So we assume that the
    //       max samples are always 8 if we have sample buffers and max samples is greater than 1.
    
    if (myMaxSampleBuffers > 0 && myMaxSamples > 1)
	{
        return (YES);
	}
	
    return (NO);
}

//________________________________________________________________________________________________GL_CheckSwitchFSAAOnTheFly()

void	GL_CheckSwitchFSAAOnTheFly (void)
{
    // Changing the FSAA samples is only available for Radeon boards
    // [a sample buffer is not required at the pixelformat].
    //
    // We don't want to support FSAA under 10.1.x because the current driver will crash the WindowServer.
    // Thus we check for the Radeon string AND the GL_ARB_multisample extension, which is only available for
    // Radeon boards under 10.2 or later.
    
    if (strstr (gl_renderer, "ATI Radeon") && GL_ExtensionSupported ("GL_ARB_multisample") == YES)
    {
        Con_Printf ("Found ATI FSAA...\n");
        gl_fsaaavailable = YES;
    }
    else
    {
        gl_fsaaavailable = NO;
    }

    gGLFSAALevel = (float) gGLMultiSamples;
    Cvar_SetValue (gl_fsaa.name, gGLFSAALevel);
}

//__________________________________________________________________________________________GL_CheckTextureFilterAnisotropic()

void	GL_CheckTextureFilterAnisotropic (void)
{
    if (GL_ExtensionSupported ("GL_EXT_texture_filter_anisotropic") == YES)
    {
        Con_Printf ("Found GL_EXT_texture_filter_anisotropic...\n");
        gl_texturefilteranisotropic = YES;
    }
    else
    {
        gl_texturefilteranisotropic = NO; 
    }
}

//________________________________________________________________________________________________GL_CheckLuminanceLightmaps()

void	GL_CheckLuminanceLightmaps (void)
{
    // We allow luminance lightmaps only on MacOS X v10.2 or later because of a driver related
    // performance issue.
    //
    // NSAppKitVersionNumber10_1 is defined as 620.0. 10.2 has 663.0. So test against 663.0:
    
    if (NSAppKitVersionNumber >= 663.0)
    {
        gl_luminace_lightmaps = YES;
        Con_Printf ("Found MacOS X v10.2 or later. Using luminance lightmaps...\n");
    }
    else
    {
        gl_luminace_lightmaps = NO;
        Con_Printf ("Found MacOS X v10.1 or earlier.  Using RGBA lightmaps...\n");
    }
}

//________________________________________________________________________________________________________GL_CheckTextureRAM()

void	GL_CheckTextureRAM (GLenum theTarget, GLint theLevel, GLint theInternalFormat, GLsizei theWidth,
                            GLsizei theHeight, GLsizei theDepth , GLint theBorder, GLenum theFormat,
                            GLenum theType)
{
    GLint	myWidth = -1;
    GLenum	myError,
			myTarget;
    
    // flush existing errors:
    glGetError ();

    // check our target texture type:
    switch (theTarget)
    {
        case GL_TEXTURE_1D:
        case GL_PROXY_TEXTURE_1D:
            myTarget = GL_PROXY_TEXTURE_1D;
            glTexImage1D (myTarget, theLevel, theInternalFormat, theWidth, theBorder, theFormat, theType, NULL); 
            break;
        case GL_TEXTURE_2D:
        case GL_PROXY_TEXTURE_2D:
            myTarget = GL_PROXY_TEXTURE_2D;
            glTexImage2D (myTarget, theLevel, theInternalFormat, theWidth, theHeight, theBorder, theFormat,
                          theType, NULL); 
            break;
        case GL_TEXTURE_3D:
        case GL_PROXY_TEXTURE_3D:
            myTarget = GL_PROXY_TEXTURE_3D;
            glTexImage3D (myTarget, theLevel, theInternalFormat, theWidth, theHeight, theDepth, theBorder,
                          theFormat, theType, NULL); 
            break;
        default:
            return;
    }
    
    myError = glGetError ();

    // get the width of the texture [should be zero on failure]:
    glGetTexLevelParameteriv (myTarget, theLevel, GL_TEXTURE_WIDTH, &myWidth);
    
    // now let's see if the width is equal to our requested value:
    if (myError != GL_NO_ERROR || theWidth != myWidth)
    {
        Sys_Error ("Out of texture RAM. Please try a lower resolution and/or depth!");
    }
}

//___________________________________________________________________________________________________________________GL_Init()

void	GL_Init (void)
{
    // show OpenGL stats at the console:
    gl_vendor = (const char *) glGetString (GL_VENDOR);
    Con_Printf ("GL_VENDOR: %s\n", gl_vendor);
    
    gl_renderer = (const char *) glGetString (GL_RENDERER);
    Con_Printf ("GL_RENDERER: %s\n", gl_renderer);
    
    gl_version = (const char *) glGetString (GL_VERSION);
    Con_Printf ("GL_VERSION: %s\n", gl_version);
    
    gl_extensions = (const char *) glGetString (GL_EXTENSIONS);
    Con_Printf ("GL_EXTENSIONS: %s\n", gl_extensions);
    
    // not required for MacOS X, but nevertheless:
    if (!strncasecmp ((char*) gl_renderer, "Permedia", 8))
        isPermedia = YES;

    // check if we have fast luminance lightmaps:
    GL_CheckLuminanceLightmaps ();

    // check for multitexture extensions:
    GL_CheckMultiTextureExtensions ();

    // check for pn_triangles extension:
    GL_CheckPNTrianglesExtensions ();

    // check for texture filter anisotropic extension:
    GL_CheckTextureFilterAnisotropic ();

    // check if FSAA is available:
    GL_CheckSwitchFSAAOnTheFly ();

    // setup OpenGL:    
    glClearColor (1,0,0,0);
    glEnable (GL_TEXTURE_2D);
    glAlphaFunc (GL_GREATER, 0.666f);
    // Get rid of Z-fighting for textures by offsetting the
    // drawing of entity models compared to normal polygons.
    // (Only works if gl_ztrick is turned off)
    glPolygonOffset(0.05, 0);
    glEnable (GL_ALPHA_TEST);
    glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
    glCullFace (GL_FRONT);
    glShadeModel (GL_FLAT);

    if (gGLMultiSamples > 0)
        glEnable (GL_MULTISAMPLE_ARB);
    
    glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}

//____________________________________________________________________________________________________GL_SetMiniWindowBuffer()

void	GL_SetMiniWindowBuffer (void)
{
    if (gGLMiniWindowBuffer == NULL ||
        [gGLMiniWindowBuffer pixelsWide] != gGLDisplayWidth ||
        [gGLMiniWindowBuffer pixelsHigh] != gGLDisplayHeight)
    {
        [gGLMiniWindowBuffer release];
        gGLMiniWindowBuffer = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes: NULL
                                                                      pixelsWide: gGLDisplayWidth
                                                                      pixelsHigh: gGLDisplayHeight
                                                                   bitsPerSample: 8
                                                                 samplesPerPixel: 4
                                                                        hasAlpha: YES
                                                                        isPlanar: NO
                                                                  colorSpaceName: NSDeviceRGBColorSpace
                                                                     bytesPerRow: gGLDisplayWidth * 4
                                                                    bitsPerPixel: 32];
        if (gGLMiniWindowBuffer == NULL)
        {
            Sys_Error ("Unabled to allocate the window buffer!\n");
        }
    }
}

//_________________________________________________________________________________________________________GL_SaveScreenshot()

qboolean GL_SaveScreenshot (const char *theFilename)
{
    return ([FDGLScreenshot writeToPNG: [NSString stringWithCString: theFilename]] == YES ? true : false);
}

//_____________________________________________________________________________________________________GL_InitGrowboxTexture()

void	GL_InitGrowboxTexture (void)
{
    NSString			*myGrowboxPath = [[NSBundle mainBundle] pathForResource: @"GrowBox" ofType: @"tiff"];
    NSImage				*myGrowboxImage = [[NSImage alloc] initWithContentsOfFile: myGrowboxPath],
                        *myTextureImage = [[NSImage alloc] initWithSize: NSMakeSize (16.0f, 16.0f)];
    NSBitmapImageRep	*myBitmapRep;
    NSSize				myGrowboxSize = [myGrowboxImage size],
                        mxTextureSize = [myTextureImage size];
    GLint				myUnpackAlignment,
                        myUnpackRowLength;
    
    // copy the growbox image to an image of size 16x16:
    [myGrowboxImage setFlipped: YES];
    [myTextureImage lockFocus];
    [myGrowboxImage compositeToPoint: NSMakePoint (mxTextureSize.width - myGrowboxSize.width, 0.0f) operation: NSCompositeCopy];
    [myTextureImage unlockFocus];
    
    // safe old pixel storage modes:
    glGetIntegerv (GL_UNPACK_ALIGNMENT, &myUnpackAlignment);
    glGetIntegerv (GL_UNPACK_ROW_LENGTH, &myUnpackRowLength);

    // create the texture from the image:
    myBitmapRep = [NSBitmapImageRep imageRepWithData: [myTextureImage TIFFRepresentation]];
        
    glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei (GL_UNPACK_ROW_LENGTH, ([myBitmapRep bytesPerRow] << 3) / [myBitmapRep bitsPerPixel]);
    
    glGenTextures (1, &gGLGrowboxTexture);
    glBindTexture (GL_TEXTURE_2D, gGLGrowboxTexture);	
    glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, [myBitmapRep pixelsWide], [myBitmapRep pixelsHigh], 0,
                  [myBitmapRep hasAlpha] ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, [myBitmapRep bitmapData]);
                  
    // set the texture parameters:
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    
    // restore pixel storage modes:
    glPixelStorei (GL_UNPACK_ALIGNMENT, myUnpackAlignment);
    glPixelStorei (GL_UNPACK_ROW_LENGTH, myUnpackRowLength);
	
	gGLGrowboxInitialised = YES;
}

//______________________________________________________________________________________________________GL_RenderInnsideDock()

void	GL_RenderInsideDock (void)
{
    if ([gVidWindow isMiniaturized] == YES)
    {
        UInt8 *	myBitmapBuffer = (UInt8 *) [gGLMiniWindowBuffer bitmapData];

        CGLFlushDrawable ([gGLContext cglContext]);//CGLGetCurrentContext ());                
        if (myBitmapBuffer != NULL)
        {
            UInt8 *	myBitmapBufferEnd = myBitmapBuffer + (gGLDisplayWidth << 2) * gGLDisplayHeight;

            // get the OpenGL buffer:
            glReadPixels (0, 0, gGLDisplayWidth, gGLDisplayHeight, GL_RGBA, GL_UNSIGNED_BYTE, myBitmapBuffer);
            
            // set all alpha to 1.0. instead we could use "glPixelTransferf (GL_ALPHA_BIAS, 1.0f)", but it's slower!
            myBitmapBuffer += 3;
            while (myBitmapBuffer < myBitmapBufferEnd)
            {
                *myBitmapBuffer = 0xFF;
                myBitmapBuffer += sizeof (UInt32);
            }

            // draw the Dock image:
            [gGLMiniWindow lockFocus];
            [gGLMiniWindowBuffer drawInRect: gGLMiniWindowRect];
            [gGLMiniWindow unlockFocus];
            [gVidWindow setMiniwindowImage: gGLMiniWindow];
        }
    }
    else
    {
        // draw the growbox here!
        if (gGLGrowboxInitialised == NO)
        {
            GL_InitGrowboxTexture ();
        }

		glPushMatrix ();
		glMatrixMode (GL_PROJECTION);
		glPushMatrix ();	
		glLoadIdentity ();
		glOrtho (0.0f, gGLDisplayWidth, 0.0f, gGLDisplayHeight, -1.0f, 1.0f);
		glMatrixMode (GL_TEXTURE);
		glPushMatrix ();
		glMatrixMode (GL_MODELVIEW);
		glLoadIdentity ();
		//glDisable (GL_CULL_FACE);
		//glDisable (GL_BLEND);
		glBindTexture (GL_TEXTURE_2D, gGLGrowboxTexture);
		glEnable (GL_TEXTURE_2D);
		glColor4f (1.0f, 1.0f, 1.0f, 1.0f);
		glBegin (GL_TRIANGLE_STRIP);
			glTexCoord2f (0.0f, 0.0f); glVertex2i (gGLDisplayWidth - 16,  0);
			glTexCoord2f (1.0f, 0.0f); glVertex2i (gGLDisplayWidth,       0);
			glTexCoord2f (0.0f, 1.0f); glVertex2i (gGLDisplayWidth - 16, 16);
			glTexCoord2f (1.0f, 1.0f); glVertex2i (gGLDisplayWidth,      16);
		glEnd ();
		glMatrixMode (GL_PROJECTION);
		glPopMatrix ();
		glMatrixMode (GL_TEXTURE);
		glPopMatrix ();
		glMatrixMode (GL_MODELVIEW);
		glPopMatrix ();
		
		CGLFlushDrawable ([gGLContext cglContext]);
    }
}

//________________________________________________________________________________________________________________GL_SetFSAA()

void	GL_SetFSAA (UInt32 theFSAALevel)
{
	long	myFSAALevel;
    
    // check the level value:
    if (theFSAALevel != 0 && theFSAALevel != 4 && theFSAALevel != 8)
    {
        Cvar_SetValue (gl_fsaa.name, gGLFSAALevel);
        Con_Printf ("Invalid FSAA level, accepted values are 0, 4 or 8!\n");
        return;
    }
    
    // check if FSAA is available:
    if (gl_fsaaavailable == NO)
    {
        gGLFSAALevel = gl_fsaa.value;
        if (theFSAALevel != 0)
        {
            Con_Printf ("FSAA not supported with the current graphics board!\n");
            Cvar_SetValue (gl_fsaa.name, gGLFSAALevel);
        }
        return;
    }
    
    // convert the ARB_multisample value for the ATI hack:
    if (theFSAALevel == 0)
        myFSAALevel = 1;
    else
        myFSAALevel = theFSAALevel >> 1;

    // set the level:
    [gGLContext makeCurrentContext];
    if (CGLSetParameter (CGLGetCurrentContext (), VID_ATI_FSAA_LEVEL, &myFSAALevel) == CGDisplayNoErr)
    {
        gGLFSAALevel = theFSAALevel;
        Con_Printf ("FSAA level set to: %d!\n", theFSAALevel);
    }
    else
    {
        Con_Printf ("Error while trying to set the new FSAA Level!\n");
    }
    Cvar_SetValue (gl_fsaa.name, gGLFSAALevel);
}

//_________________________________________________________________________________________________________GL_SetPNTriangles()

void	GL_SetPNTriangles (SInt32 thePNTriangleLevel)
{
    // check if the pntriangles extension is available:
    if (gl_pntriangles == NO)
    {
        if (thePNTriangleLevel != -1)
        {
            Con_Printf ("pntriangles not supported with the current graphics board!\n");
        }
        
		gGLPNTriangleLevel = (gl_truform.value > 7.0f) ? 7.0f : gl_truform.value;
        
		if (gGLPNTriangleLevel < 0.0f)
		{
			gGLPNTriangleLevel = -1.0f;
		}
		
        Cvar_SetValue (gl_truform.name, gGLPNTriangleLevel);
        return;
    }

    if (thePNTriangleLevel >= 0)
    {
        if (thePNTriangleLevel > 7)
        {
            thePNTriangleLevel = 7;
            Con_Printf ("Clamping to max. pntriangle level 7!\n");
        }
        
        // enable pn_triangles. lightning required due to a bug of OpenGL!
        glEnable (GL_PN_TRIANGLES_ATIX);
        glEnable (GL_LIGHTING);
        glLightModelfv (GL_LIGHT_MODEL_AMBIENT, gGLTruformAmbient);
        glEnable (GL_COLOR_MATERIAL);

        // point mode:
        gGLPNTrianglesiATIX (GL_PN_TRIANGLES_POINT_MODE_ATIX, GL_PN_TRIANGLES_POINT_MODE_CUBIC_ATIX);
                    
        // normal mode (no normals used at all by Quake):
        gGLPNTrianglesiATIX (GL_PN_TRIANGLES_NORMAL_MODE_ATIX, GL_PN_TRIANGLES_NORMAL_MODE_QUADRATIC_ATIX);

        // tesselation level:
        gGLPNTrianglesiATIX (GL_PN_TRIANGLES_TESSELATION_LEVEL_ATIX, thePNTriangleLevel);

        Con_Printf ("Truform enabled, current tesselation level: %d!\n", thePNTriangleLevel);
    }
    else
    {
        thePNTriangleLevel = -1;
        glDisable (GL_PN_TRIANGLES_ATIX);
        glDisable (GL_LIGHTING);
        Con_Printf ("Truform disabled!\n");
    }
    gGLPNTriangleLevel = thePNTriangleLevel;
    Cvar_SetValue (gl_truform.name, gGLPNTriangleLevel);
}

//____________________________________________________________________________________________GL_SetTextureFilterAnisotropic()

void	GL_SetTextureFilterAnisotropic (UInt32 theState)
{
    // clamp the value to 1 [= enabled]:
    gGLAnisotropic = theState ? YES : NO;
    Cvar_SetValue (gl_anisotropic.name, gGLAnisotropic);
    
    // check if anisotropic filtering is available:
    if (gl_texturefilteranisotropic == NO)
    {
        gl_texureanisotropylevel = 1.0f;
        if (theState != 0)
        {
            Con_Printf ("Anisotropic tetxure filtering not supported with the current graphics board!\n");
        }
        return;
    }
    
    // enable/disable anisotropic filtering:
    if (theState == 0)
    {
        gl_texureanisotropylevel = 1.0f;
    }
    else
    {
        glGetFloatv (GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &gl_texureanisotropylevel);
    }
}

//________________________________________________________________________________________________________GL_SetMultiTexture()

void	GL_SetMultiTexture (UInt32 theState)
{
    // clamp the value to 1 [= enabled]:
    gGLMultiTexture = theState ? YES : NO;
    Cvar_SetValue (gl_multitexture.name, gGLMultiTexture);
    
    // check if multitexturing is available:
    if (gGLMultiTextureAvailable == YES)
    {
        gl_mtexable = gGLMultiTexture;
        if (gl_mtexable == YES)
        {
            Con_Printf ("Multitexturing enabled!\n");
        }
        else
        {
            Con_Printf ("Multitexturing disabled!\n");
        }
    }
    else
    {
        Con_Printf ("Multitexturing not available!\n");
    }
}

//_________________________________________________________________________________________________________GL_BeginRendering()

void	GL_BeginRendering (SInt *x, SInt *y, SInt *width, SInt *height)
{
    *x = *y = 0;
    *width = gGLDisplayWidth;
    *height = gGLDisplayHeight;
}

//___________________________________________________________________________________________________________GL_EndRendering()

void	GL_EndRendering (void)
{
    // set the gamma if fullscreen:
    if (gVidDisplayFullscreen == YES)
    {
        VID_SetGamma ();
        CGLFlushDrawable ([gGLContext cglContext]);//CGLGetCurrentContext ());
    }
    else
    {
        // if minimized, render inside the Dock!
        GL_RenderInsideDock ();
    }

    // check if video_wait changed:
    if(vid_wait.value != gGLVideoWait)
    {
        VID_SetWait ((UInt32) vid_wait.value);
    }

    // check if anisotropic texture filtering changed:
    if (gl_anisotropic.value != gGLAnisotropic)
    {
        GL_SetTextureFilterAnisotropic ((UInt32) gl_anisotropic.value);
    }

    // check if vid_fsaa changed:
    if (gl_fsaa.value != gGLFSAALevel)
    {
        GL_SetFSAA ((UInt32) gl_fsaa.value);
    }

    // check if truform changed:
    if (gl_truform.value != gGLPNTriangleLevel)
    {
        GL_SetPNTriangles ((SInt32) gl_truform.value);
    }

    // check if multitexture changed:
    if (gl_multitexture.value != gGLMultiTexture)
    {
        GL_SetMultiTexture ((UInt32) gl_multitexture.value);
    }
}

#pragma mark -

//______________________________________________________________________________________________iMPLEMENTATION_NSOpenGLContext 

@implementation NSOpenGLContext (CGLContextAccess)

//_________________________________________________________________________________________________________________cglContext:

- (CGLContextObj) cglContext;
{
    return (_contextAuxiliary);
}

@end

//_________________________________________________________________________________________________________________________eOF 
