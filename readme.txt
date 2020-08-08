ProQuake 4.91 - Change Log - Baker   wwww.quakeone.com - January 23, 2008
------------------------------------------------------------------------------------
This is an effort to update the ProQuake engine with bug-fixes and some add modern conveniences without altering any of the 
look or feel of the ProQuake engine.  Conservative improvements like making the brightness controls work in GLPro, fixing 
major rendering bugs like the historical GLQuake FOV bug, and simple things like command completion are in the 
scope of this; things like adding 24-bit texture support, new particle effects or unnecessary things (like an MP3 player) 
are not in the scope of this project.

This is merely an attempt to improve the best engine for an authentic 1999-style deathmatch experience.  

ProQuake is modification of GLQuake/WinQuake by JP Grossman that really evolved into the gold standard classic Quake client 
for online play.  In December 2007, this project became the official continuation of ProQuake.

Source code is available at http://quakeone.com/proquake under the GPL v3 or later, binaries are distributed under the 
GPL v3.

V4.93 - October 24, 2012
============================================================================================================================
1. Server browser.  Press F5 or Menu->Multiplayer->Join Game.  Refreshes every 80 seconds.
2. Support for a QuakeOne.com player id.
3. MH fixed a DX8 wrapper screenshot issue reported by Arkon.  I pressed CTRL-V.  Thanks MH!
4. Removed ancient SGIS OpenGL stuff and used ARB.
5. Miscellaneous improvements robbed from FitzQuake Mark V and Engine X.  Demo tweaks, history file refinement, zzzzz.
6. Timedemo automatically closes console.
7. Added 2 testing command line parameters for potentially addressing Polarite keyboard issue.
8. General code cleanups and streamlining.

V4.91 Beta - October 5, 2012
============================================================================================================================
1. Sounds and .loc files download.
2. r_colored_bodies 0 turns colored dead bodies off (some people are used to that).
3. Windows stickykeys is handled like DirectQ --- which means it is likely disabled when ProQuake is active.
4. The "Windows" key is handled in the same manner.  Disabled while using ProQuake (cvar is gone)
5. Stripped out cd support, ipx support, serial port support, etc.
6. Switched some compiler settings to get the file sizes back down on par with 4.5x file sizes.
7. Changed model frustum culling to use the Engine X method.  Fixes rare bug introduced in 2010.


V4.90 Beta - September 25, 2012
============================================================================================================================
Going to focus efforts on ProQuake for what it is best at: multiplayer NetQuake.  And how it is actually used and what it
is at good at doing.

1. Colored dead bodies support.  Similar to the way GL Quakeworld did it.  Inflexible, but effective.
2. Increased performance when a ton of particles are on the screen with smoother and consistent frames-per-second.
3. DX8 build optimizations and fixes.
4. Map download stays connected to server.  Won't lose score, weapons, place in line in Rocket Arena, etc. on map change.
5. Dropped support for WinQuake build.  No one uses it.  Eliminated ancient D3D build, since DX8 Pro is better, faster, flexible.
6. Mac build will return at some point, probably nearer version 5.0.
7. Eliminated features like alpha, fog and skybox support.  They may return at some point, done correctly. Half-Life map 
support has likewise been eliminated.

The support for a WinQuake renderer will not return, it requires a lot of work to maintain with no one using it and someone
wishing to play around with WinQuake can use an older ProQuake or some other engine.  Likewise, with the DX8 version being
extremely fast, high performance, stable and versatile, the ancient D3D build will not return.

The DX8 version (powered by MH's OpenGL to Direct3D 8.0 wrapper) performs embarassingly well, on older machines gets
80%-90% performance of GL ProQuake.  On newer machines, DX8 ProQuake seems to beat GL ProQuake by as much as %30 to 100% in 
speed.  I don't believe MH ever thought the DX8 wrapper would actually out-perform the native OpenGL version, it was meant
to allow those with bad OpenGL drivers to be able to use Quake.


V4.52 Beta - August 7, 2010
============================================================================================================================
Absorbed Enhanced GLQuake's skybox code almost wholesale to support all sizes of skyboxes correctly in gl/dx8/d3dpro
Results: maps like back2forwards should load skybox correctly, the RMQ single player demo skybox should be right
Raised efrags to 2048.
Results: maps like some of the Travail ones won't display messages like too many efrags.

V4.51 Beta - August 3, 2010
============================================================================================================================
Maxplayers can be increased at any time
developer 2 displays world position x,y,z on-screen
gl_xflip - Ability to play maps rendered in reverse.
Player color range in menu increased to 15.
Resolved issue causing Half-Life maps to crash when dedicated server was running.
Resolved issue that could cause white textures when switching to windowed mode.
Added several new model and map load warnings from Enhanced GLQuake.

V4.50 Beta - July 30, 2010
============================================================================================================================
Fixed view blends issues once and for all (?).  Code is still a kludge.
Raised MAX_CHANNELS and MAX_DYNAMIC_CHANNELS to mirror FitzQuake 0.85
Con_Debugf so for development Con_Printfs can be added during testing and find them to remove them easily.
wqpro: Made 640x480 in windowed mode non-stretched
wqpro: Made wqpro window captioning same as glpro, etc.
wqpro: Rewired ALT-ENTER behavior to use the vid_fullscreen_mode and vid_windowed_mode features originally in WinQuake
wqpro: Maximize button in titlebar in windowed mode will change mode to this vid_fullscreen_mode setting.
Removed separate application of gamma table to Half-Life textures
Made all pr_<name>.c use Con_SafePrintf for messages for speed.  (Buffer overrun risk on maps with tons of bad fields?)


V4.49 Beta - July 28, 2010
============================================================================================================================
Bug fix:  I had jacked stored command history to 64.  It only saved the first 32 = dumb = fixed.
glpro/d3dpro/dx8pro - Overbright lighting on/off switches in real time now.
glpro/d3dpro/dx8pro - r_shadows actually works if interpolation is off.  It was broke.  
wqpro - r_wateralpha support (from the awesome Makaqu engine).  r_novis support.
wqpro - can be positioned anywhere on the screen and on dual monitors.
wqpro - ALT TAB now switches between full screen mode and windowed mode!
wqpro - Now gets positioned like glpro in the center of screen if windowed.

V4.48 Beta - July 27, 2010
============================================================================================================================
More dxpro improvments.
PCX skybox support in wqpro
"FTE stains" in wqpro
gl_ringalpha is now r_ringalpha in preparation for some sort of wqpro weapon transparency.

V4.47 Beta - July 22, 2010
============================================================================================================================
Reconciled a lot of WinQuake vs. GLQuake code including versus MAC OSX (gl_screen.c, r_screen.c).
dx8pro in windowed mode should no longer minimize if it loses focus (happened only AFTER first being fullscreen upon start).
dx8pro in windowed mode should no longer move to top left of screen (happened only AFTER first being fullscreen upon start).
host_sleep defaults to 0 again.  Polarite reported it was a problem with his frames per second dropping.
Bug fix: Starting demos should no longer start going if, for instance, you are using Qview.  Reported by Polarite.
Code maintenance: chase.c --> cl_chase.c and sbar.c --> cl_sbar.c  ... renamed files for consistency.
Long story short: dx8pro should be as good as glpro in functionality and reliability now.
Eliminated sound studdering on video mode switch.

V4.46 Beta - July 19, 2010
============================================================================================================================
Mac OSX build: Fixed console resizing in windowed mode, host_sleep added, Quake sleeps when minimized

V4.45 Beta - July 18, 2010
============================================================================================================================
Mac OSX Build.

V4.44 Beta - July 17, 2010
============================================================================================================================
Relased binaries in 4.43 didn't actually have gl_overbright cvar.  gl_overbright 1 to enable; requires map restart.
Added contrast capability to d3dpro.  Now all 4 Windows builds have contrast control.  Type contrast 2 for example (default 1).
version command prints full version details now.
Minor change in the way dx8pro handles ALT-TAB.
Vertical sync will work on more video cards, at least Intel Display Adapters.  Added check for extension GL_WIN_swap_hint.

V4.43 Beta - July 16, 2010
============================================================================================================================
dx8pro changes.  Super-fast, same features as glpro, nearly as fast, really debugged!
A few bug fixes.  Water blends not showing = fixed.  Too bright lightmaps = fixed.  Etc.
gl_overbright 1 support.  Restart the map if you change the value. (I'm rushed ... I should reload the lightmaps.)
Special thanks to MH.  

V4.42 Beta - July 15, 2010
============================================================================================================================
Exhaustively examined several changes in MH's various Direct3D8 engine modifications plus overbright lightning engine sample.

Result: went through and reworked a lot of things.

The DX8PRO build is much faster now and I've killed a lot of ifdefs.  Should be approaching near GLQUAKE speeds.  I'm showing
it performs about 88% of GLQuake.

Bugs: waterblends in glpro are foobar, lightmaps in all hardware accelerated versions are foobar.

V4.41 Beta - July 15, 2010
============================================================================================================================
Bug fix regarding truncated command line parameters.

V4.40 Beta - July 14, 2010
============================================================================================================================
Chase Cam fix.  [Rook's version] chase_active 1 won't poke through map walls any more.
Namemaker command is back. Type in console to access namemaker.
Namemaker can be used with the mouse in glpro and dxpro. [Not in wqpro or d3dpro except in -windowed mode].
Ability to bind mouse buttons and mousewheel despite how the mouse is released by Quake ;-)
gl_fadescreen_alpha cvar.  Surprisingly at values under 0.66 it doesn't even draw.  I think the fadescreen is too dark.

V4.39 Beta - July 13, 2010
============================================================================================================================
String safety.  Removed all instances of strcat, Q_atoi, Q_atof, strncat, q_strncasecmp, q_strcasecmp, q_strncpyz.
Made a fair number of functions "static".   For clarity.
Other minor source finessing such as making all command functions end with _f.   For clarity.
Changed messages for hardware gamma.
Profile command when no server running explains that there is no server running.
Experiments regarding the console.  Try the -conhide parameter to see.

V4.38 Beta - July 12, 2010
============================================================================================================================
Colors 14/15 enabled.  sv_allcolors 0 to disable.
gl_nearwater_fix defaults 1.  You can set it to 0 if you are "old school".
host_sleep defaults to 1.  Should save a lot of cpu and therefore battery life on laptops.  No noticeable performance difference.
-fullwindow works with dx8pro version.
r_clearcolor for gl and direct3d builds
The demo loop continues to play even if the menu is up.
With developer 1, this min and max bounds of the world will print to cNonsole.
wqpro build bugfix; drops the view blend when disconnected if player had quad or suit, etc.
gl_print_extensions prints the gl extensions to console.
startdemos gives about disconnecting if already connected.

V4.36 Beta - July 9, 2010
============================================================================================================================
ProQuake defaults to NOT using hardware blends [gl_hwblend 0].  Major FPS gain in glpro (when not using -gamma) and dx8pro.
Upgraded dx8 wrapper.
WQPro now has contrast control.  Type contrast 2 in the console, for example.
Waterblends work in glpro + dx8pro when using hardware gamma.  Gunter reported this bug, was inherited from JoeQuake [waterblends don't work in it either, at least for me.]
Switching gl_texturemode is instant .. I broke it a few versions ago inadvertently.  However, I have made it better and a cvar.
Disconnect clears music, ambient looping sounds and any view blends in all builds.  

gl_overbright is temporarily out (except in dx8pro where it is natively a feature [on accident, I accidentally ported it from dx8 Enhanced GLQuake not realizing it]).


V4.35 Beta - July 6, 2010
============================================================================================================================
(GLPRO only) Experimental gl_overbright 1 capability added to OpenGL build.  Needs further integration as at the moment switching from
SGIS to ARB breaks r_lightmap 1 and Half-Life map texture upload somehow.

V4.34 Beta - July 6, 2010
============================================================================================================================
Finally fixed the problem where ProQuake would always set "Enhanced Pointer Precision" in ProQuake and fail to restore it
on exit.  Independent verification needed.

V4.33 Beta - July 6, 2010
============================================================================================================================
- Added software renderer transform courtesy of code from DirectQ by MH.  r_interpolate_transform 1 to active. (wqpro)
- Plays multimap demos.  For example, start recording on E1M1 and stop recording on E1M7 ... it can play back it ALL!
- Fixed an issue with really long folder names and web download noticed by Mr. Burns.
- gl_nearwater_fix 1 will fix underwater drawing when underneath water near the surface. (source = aguirRe's EnhancedGL/FitzQuake)

V4.32 Beta - June 29, 2010
============================================================================================================================
- scr_autoid 1 displays player names in ALL Windows builds now (wqpro, d3dpro, dx8pro).  Thanks to FTEQW mathlib.c
- Fixed cl_keys_altenter bug -- it wasn't available in glpro or dx8pro despite being a long-time feature due to a typo.

V4.31 Beta - June 29, 2010
============================================================================================================================
- scr_autoid 1 displays player names overhead in demo playback (GL build only).

============================================================================================================================
V4.30 Beta - June 26, 2010

- Software renderer animation interpolation courtesy of MH.  Set r_interpolate_animation 1 to activate. (wqpro)
- Smooth coop!  No more jerkiness with monsters. On by default.  (cl_gameplayhack_monster_lerp and sv_gameplayfix_monster_lerp)
- Download map progress bar.
- A couple of fixes via bug reports (clipboard copy didn't work, for instance: i.e. the "copy" command) and maintaince upgrades (fixed a D3D menu item).

V4.27 Beta - June 25, 2010
- Session to session command history added.  Saves to quake\id1\proquake_history.txt

V4.26 Beta - June 24, 2010

- gl_free_world_textures support.  Set to 1 and world textures are cleared on map load. (Rook/aguirRe) Not in D3D build.
- qcexec support.
- Rotation support fix (Thanks MH!)
- Fullscreen releases mouse cursor upon menu or console in Windows version (maybe make this a cvar)
- Eliminated all the Windows compiler warnings in MSVC6
- mp3 command ... plays files in id1\music folder (example: mp3 mymusic will play "mymusic.mp3" in the id1\music folder)
- Fixed r_wateralpha issue with DX8 version
- Gave up on the idea of "perfection" and changed the default web download url to http://downloads.quake-1.com/ for now.

V4.25 Beta - June 22, 2010

NOTE:  Binaries are now distributed under the GPL v3.  No binaries happen to be in this source update so I'll update the
docs appropriately on the next distribution.

Rotation support, DX8 build, chase_active 2, sv_progs support, cl_bobbing, general code clean-up and improvements.

Planned additions:

- Free world textures option (allows engine to avoid running out of texture slots when changing maps countless times)
- Probable scale support
- Quake.cmdline file support
- Possibly r_interpolate_transform support for software
- Maybe even r_skybox support for software
- Long shot: r_interpolate_animation support for software
- Get host_sleep working on all platforms

Known issues (or things that I do not like):

- Still have the "Enhanced Pointer Precision" issue with Windows build.
- OS X build I do need to fix the console resizing issue that can occur in windowed mode
- MP3 music should stop playing upon disconnect
- Flash -nomouse support


V4.15 Beta - June 15, 2010

Rather integrated Windows, OSX, PSP and Flash source code.  More to do, but close.

Bugs: 

Windows:  Still enables "Enhanced Pointer Precision"
OS X:     Console sizing issues when not in fullscreen sometimes.  Half-Life bsp support somehow is broken.
PSP:      Can't run this one mod I have for unknown reasons.
Flash:    None?

Future:   

All:     cl_bobbing 1 support and maybe cl_scalebobbing for OpenGL.  Targa and PCX load/save into own file.   Chase_active 3 plus possible chasecam fix.

Windows: MP3 support via MH/Reckless tutorial.  Re-add DX8 build into code base.
OS X:    Integrated menu and keys.  Close!  Plus I have a private build with map download, but it isn't a Universal Binary. :(
Flash:   Add -nomouse support and quake.cmdline style support.
PSP:     Create a graphic or 2 for it.  More integration.  Possible Half-Life BSP support.
Non-Win: Make sys_sleep work, etc.

V4.00 Zeta - August 11, 2009

- Map/Model Web Download support added (does not download sounds or non-mdl/non-bsp files)
- Half-Life support added (limited: no lits, no alpha texture support)
- Skybox support added (sky "morose_" or loadsky "nightfall" ... skyboxes go in quake\id1\gfx\env folder)
- Skybox and fog automatically set from map worldspawn (single player releases)

NOTE: zlib1.dll and libcurl.dll are now REQUIRED for the executable!

No Mac OSX build yet ... will be forthcoming ... I hope.

V3.99x-3 (beta) - December 1, 2008
- cycle command enabled
- entities are transparency sorted (fixes issue with monsters behind glass not being seen)
- sv_altnoclip 1 (default) - better noclip

V3.99x-2 (beta) - November 21, 2008
- alpha brush model entity support (i.e., transparent glass) [Nehahra compatible protocol]

V3.99x-1 (beta) - November 21, 2008
- r_drawentities 0 fix in wqpro version where it no longer draws view model.
- profile command will no longer crash as a client.
- glpro399.exe can now run as a dedicated server.
- Intel display adapter fix enhanced to include time refresh.
- Intel displays default to gl_ztrick 0 and gl_clear 1.
- r_viewmodeloffset support (set like, say, r_viewmodeloffset 5 for side view weapon).
- optional centerprint logging (con_logcenterprint 1).
- host_sleep 1 option, uses less CPU than normal
- DSL fix upgraded to dsl fix 3.0.
- Centerprint logging in single player

V3.99x (beta) - October 30, 2008 - Comparison vs. 3.99s release

- Q_version only responds once per 20 seconds. 
- Not specifying a port will connect to port 26000 (this might break compatibility with Qview, gamespy and other launchers).
- Mac OS X version has nearly all Windows ProQuake features (the menu isn't quite as advanced, main difference).
- OS X version releases the mouse cursor in Windows mode when console or menu are pulled up.
- gl_fullbright capability (off by default, set to 1 to enable.  Tiny FPS hit when gl_fullbright is set to 1)
  This features will enable GL versions to display fullbright textures like WinQuake.  
  (This isn't a cheat; this makes textures that should be visible in the dark, like glowing buttons, visible like they should be.)
- Custom crosshair capability using crosshair 2 thru 6, crosshaircolor, crosshairalpha.  Doesn't save to config.

THERE IS NO SERVER BROWSER OR MAP DOWNLOAD IN THIS RELEASE.   It will be back, I just want to do it properly.

That feature will be re-integrated in a future release.  The version with that was an experimental release that
used an operating system specific and sort of hacky method that did not work for a few ppl.  


============================================================================================================================

V3.99sx11 


V3.99sx8 (beta)
- More heavy code integration
- WinQuake windowed mouse defaults to on
- Maybe fixed a gamma issue with GL


V3.99sx7 (beta)
- More heavy code integration
- gl_ztrick 0 is default now
- -no8bit is the default, use -8bit to use old method; white-screen issue with some video cards that say they support 8bit palette but don't.
- Note: when you have a powerup, the client ignores all contrast and brightness changes; inherited from JoeQuake I believe.  Check Qrack and ezQuake.

V3.99sx6 (beta)
- "Correct Bounding Box on Models" per Quakesrc.org tutorial (reversed in 3.99s7, weird effect on viewmodel; revisit).
- Heavy code integration
Bug: Seemed to have introduced an issue with view model from one of the above, track down and kill

V3.99sx5 (beta)
- Primitive fog capability (needs a lot of work)

V3.99sx4 (beta) - October 28, 2008

- Heavy integration with the Mac version
- Fullbright texture support like WinQuake - use gl_overbright 1
- Another z-fighting fix attempt using glPolygonOffset (see E1M1 Quad area)
- ipmasking is now sv_ipmasking 0/1 instead -noipmasking command line parameter

V3.99s (beta)

- Intel display adapter fix (glpro should now run on every Intel display adapter)
- Key release fix (-aliases no longer inappropriately toggled)
- Namemaker command added
- Demo autoplay by file association has been made bulletproof (previously paths with spaces COULD sometimes cause problems)

todo: (minor) improve Intel fix a little with detection and address timerefresh thing too 
todo: WinQuake does not release mouse cursor in Windowed mode when console is up (make it behave like GLQuake)
todo: early read of display settings for single point initialization
maybe: default to desktop settings?
maybe: require -newgamma parameter and default -gamma system?
maybe: support for multi-mod.pak?
maybe: MH's automatic underwater transparency
maybe: move FPS settings into video options for GL only?
maybe: disable ALT-ENTER for Intel display adapters (Intel display adapters don't like it)
add: join game -> server browser
add: client map not found initiates map download using mapget utility
add: afterwards use Rook's curl lib code to get HTTP into the client


V3.99r (beta)

- sys_disablewinkeys 1 will disable the annoying Windows keys (set to 0 to enable them)
- sys_highpriority 1 will give ProQuake's thread cpu priority over other applications (set to 0 for normal, -1 for low)

V3.99q (beta)

- Extensive audit of input code; should resolve all issues.
- "-mem" command line parameter supported
- Restored some very old keyboard code to cure an ALT-ENTER issue
- cl_key_altenter 0 disables ALT-ENTER video mode change capability in glpro
- Optional clean scoreboard cl_scoreboard_clean 1 (doesn't display centerprint if scoreboard is on)

- Boring code stuff:
** malloc -> Q_malloc

V3.99n (beta)
- ALT-Enter changes video mode in glpro
- "copy" command, copies console to clipboard! (Like condump command or condebug, but more convenient)
- "sysinfo" command, prints system info to console (like f_system stuff in ezQuake, etc.)
- "homepage" command, causes your default web browser to open http://www.quakeone.com/proquake (is this the final home?  Don't know!)
- "openquakefolder" command, opens your Quake folder in explorer

V3.99k (beta)

- Fixed cmdline cvar.  
- cl_sbar<1 workaround that hopefully fixes ALT-TAB problem for certain video cards.
- cvar change: cvar_reset_all -> resetall; cvar_reset_one-> resetcvar
- Tweaked default_fov option behavior, it was reseting fov when it was 90 and not accepting 0 value (off).
- Defaulted always run to on.  An oversight, meant to do that a long time ago.

- Boring code stuff:
** #IFDEF WIN32 -> #IFDEF _WIN32
** vsprintf -> vsnprintf
** Q_memcpy -> memcpy
** Q_memcmp -> memcmp
** Q_memset -> memset
** Q_strlen -> strlen
** Q_strcmp -> strcmp
** Q_strncmp -> strncmp
** Q_strrchr -> strrchr
** Q_strcat  -> strcat

Bug fixes needed:

1. -dinput and -window oddness.
2. -dinput isn't triggering mousewheel up/down binds
3. International keyboard support completion
4. Lennox mentions something about the mouse behavior.

Testing Required:

1. Server testing to ensure stability.

To do list:

1. Cvar_SetDefault capability and change code to do this.
2. Maps menu and demos menu; not in the menu, but activated by command like "maps" or "playdemo (with no parameter)"
3. Optional scoreboard overlay ("easy read" scoreboard)
4. Maybe a JoeQuake style command completion option
5. Maybe having holding +showscores (TAB usually) down more than a few seconds displays diagnostics

Refinements, maybe:

0. First use notification for oldschoolers about the need to use -gamma for classic behavior and about -bpp 16 not being default.
1. Get screenshots and captured video to use correct hardware gamma palette (they don't!).
2. How to widescreen users cope with gun positioning?  Is r_viewmodelsize needed?
3. ProQuake has always parsed ping wrong in certain situations; DarkPlaces doesn't.  Explore!
4. Mac address hash
5. D3DQuake likes to lose the surface with DDERR_SURFACELOST if ALT-TAB out, can this be fixed easily? 
6. -classic command line parameter to make defaults the single player defaults (with bobbing, mouselook off, old hues, etc.)
7. Have CTRL -/+ minus adjust gamma on the fly; have SHIFT -/+ adjust viewsize on the fly

Code:

1. Check for anywhere that a cvar is set outside the cvar handling functions.
2. Code cleanup (safe string handling to overflow situations like eliminating sprintf, strcat, strcpy, etc.

V3.99j (beta)

- Clock overspeeding fix added (from JoeQuake).
- Added -oldkeys parameter, allows normal use of ALT and CTRL in the console
- The -joy parameter has become -joystick to be consistent with other engines that require an explicit command line parameter for the joystick.
- Fixed fov 0 error.

V3.99h (beta)

- Video mode is now permitted if -bpp differs from desktop (but toggling fullscreen is disabled).
- Fixed mistake regarding a string that hadn't been allocated memory that had weird side effects.

V3.99g (beta)

- Automatic ip selection works perfectly now (disabled for dedicated server)
- Use -oldnet to disable automatic selection (maybe for a LAN game if you have an active dialup connection)
- ALT-TAB now always corrects the hardware gamma, even if minimized
- pq_connectmute <seconds>; newly connected players can't speak until x seconds after connect (defaults to 3 seconds if dedicated server)

V3.99f (beta - testing version)

- Note: a problem from 3.99e still exists, depending on your internet setup ProQuake may crash upon attempting to connect to server.
  This will only affect some setups, but will happen everytime if it does happen.  Easily resolved, just need more info to walkthrough
  why apparently the auto-ip selector is apparently choosing a bad connection for some ppl.
- ALT-TAB in and out of ProQuake with hardware gamma being used now restores the gamma.
- Problem with using the optional sbar transparency (cl_sbar 0 or cl_sbar 0.x) and ALT-TAB might be resolved; verification needed.

V3.99e (beta)

- Using a new concept for solving network setups that result in "can't connect to server" 
- Video modes screen shows aspect ratios 
- restored -nocdaudio command line parameter (instead of defaulting it) because ...
- CD audio can be turned off from menu
- A couple of minor bugfixes
- tracepos and unalias from FitzQuake
- sv_cullentities replaces sv_cullplayers_trace (because multiple anti-wallhack modes may end up existing because of Rook's work)
- May have solved a crashing problem with d3dpro (was acknowledging vid_restart command, and d3dpro can't vid_restart)

V3.99c (beta)
- sv_cullplayers_trace and sv_cullplayers_notify are now "server" variables (print if changed, show in Qview and test2).
- sys_ticrate is now a server variable (shows in Qview).
- restored cmdline cvar (was going to supercede with a commandline command, but test2 command needs it to be a cvar).
- added anti-wallhack state to STATUS command.

V3.99b (beta)

- v_gunkick added (off by default).  Set to 1 for DOSQuake style jittery fire when firing, especially the super-nailgun.
- Engine support for mod-side anti-lag capabilities for dialup users (thanks to Slot Zero).
- sv_defaultmap from Rook's modified ProQuake server (one reason to want this is using rcon to change a map and the map doesn't exist crashes the server).
- Settled on the proper method for the "mapname" command to ensure it works as client and dedicated server.

Todo: figure out why windowed winquake can't be positioned on 2nd dual monitor.
Todo: Check winquake behavior for windowed mouse when in console.

V3.99a (beta)
- Improved connectivity with DSL modems!
  Removed -dslhack command line parameter in favor of new automatic DSL modem workaround.  Automatically
  ignores the 169.254.x.x ip address of a DSL modem.  Should eliminate "can't connect to server" problems
  and need for -ip parameter in most cases involving a DSL modem with no router capabilities.  (net_wins.c)
  
  Thanks to tryno for the code for -ip workaround he attempted similar to -dslhack.  This ended up building
  on that idea.

- Made small adjustments to when smoothfont kicks in.
- automatic console sizing added (vid_consize -1); can be changed to other settings in Advanced Settings menu in Options
- gl_smoothfont command re-enabled, gl_smoothfont 0 turns it off.  This does not save to config and is more used for testing purposes
- locked down some cvars that were being evaluated against 1 (gl_triplebuffer, r_wateralpha, etc.)
- Set consistent max/min bounds on gamma in wqpro, d3dpro, glpro to avoid whitescreen, including -gamma parameter


V3.98 (beta)

- vid_consize now controls 2D size; vid_conwidth/vid_conheight removed
- Video startup change that should eliminate invalid refresh rates for displays that don't support 60hz.
- cl_sbar 0 now activates the Quakeworld-style HUD; cl_sbar 1 is classic and anything in-between is transparent
- Remove console speed from menu; added consize width

V3.97 (Beta)

- transparent HUD capability; cl_sbar 1 (default) = normal HUD.  cl_sbar 0.5 will be 50% transparent.
- startdemos now clears the demo queue; add +startdemos to your commandline to startup to console w/o watching demos.
- Commented out something that was spamming mouse i/o information to the console.

V3.96 (Beta)

- r_farclip added; defaults to 16384
- command line parameter -noassocdem added

January 2, 2008 - Unofficial ProQuake is now officially a continuation of ProQuake.

V3.95 (Beta)
- gl_smoothfont added
- interpolation set to off by default

V3.94 (Beta)

- Switch gamedir on the fly using gamedir command.
- Removed glquake .ms2 meshing
- Fixed weapon model transparency with animation smoothing on.
- Implemented "shot1sid fix" that eliminates white outline at bottom of shell boxes.

V3.93 (Beta)
- Direct3D Version returns.  In-game video mode changing is disabled with Direct3D version; must use -width/-height.
- Optional animation smoothing (interpolation) returns (off by default).  Set r_interpolate_model_animation 1, r_interpolate_model_transform 1, r_interpolate_model_weapon 1.

Note: Direct3D Version is a port of the (sometimes buggy) functionality of Direct3DQuake and is unsupported.
Known issues: still has international keyboard/keypad issue.
Todo: Make animation smoothing a menu item.  

V3.92 (Stable?)
- Video resolution changing in-client is now fully supported. (Disabled if started with -window in command line)
- Authentic looking in-game brightness hardware brightness adjustment.  (-gamma command line parameter allowed, but disables adjustment in-game)
- Upped 2 rendering defaults in wqpro so maps with a lot of brush model entities (teleporters, doors, platforms) like the RQuake intro map or the Undergate display correctly.
- BPP defaults to your desktop bits per pixel setting.  Use -bpp 16 in command line if you prefer that.
- vid_conwidth and vid_conheight are supported.  If using a large resolution that makes letters too small for your liking like 1280, type vid_conwidth 640 in the console.

Known issues: still need to address international keyboard/keypad issue.  Possibly disable vid_fullscreen 0 if using non desktop bpp.

V3.91 (Beta):
- Fixed QView/Server browser reporting of IP address with default IP masking on.

V3.90 (Beta):

- Anti-wallhack code added, sv_cullplayers_trace (1=on by default, set to 0 to turn it off).
- sv_cullplayers_notify 1/0; for testing only.  Prints whether or not a player is visible to server console.
- Small (say_team) display issue fixed; 3.89 had changed messagemode2 prompt from "(say)" to "(say_team)" for clarity.

V3.89 (Beta):

- Automatically play demos (.dem) by clicking on them (in your Quake folder, in a zip file, on the internet, etc.)
- Demo rewind/fast forward
- Todo Fix list:  international keyboard + keypad; finish in-client video mode changing to completion; don't allow FF/rewind during pause.

V3.88 (Beta):

- Fixed AVI capture audio bug
- Software renderer (wqpro) now supports AVI capture as well
- Possible fix for an uncommon "mystery characters printing on scoreboard" in deathmatch problem. (Needs verification, since I don't have the problem).
- Fix for international keyboard issue with keypad.

V3.87 (Beta):

- Added AVI capture capability from JoeQuake Build 1862.
- Resolved situation where ctrl/alt/shift state was not reset upon switching in and out of client.

V3.86:

- Ability to change video mode in-game in glpro via Video Options. (From FitzQuake/Qrack).  I want to refine the startup more like Qrack does, instead of setting the video state twice like FitzQuake does.
- Set the console width in realtime with vid_conwidth and vid_conheight.  (From Qrack/FitzQuake).
- TODO:  Make video mode options unavailable in -window mode or implement full FitzQuake texture manager process so color depth can be changed.

V3.85:

Probably the last of the minor updates ...

- Added a confirmation dialog to "Reset to defaults"
- Implemented default_fov (idea from FuhQuake) that will intepret a server sending "fov 90" as a request to set your default fov.  Some mods do this and it is real annoying.
- cl_bindprotect (default value 2).  Added protection against admin-abuse where "unbindall" gets sent to the client.  2 = Client must confirm unbindall keys, 1 = Ignore unbindall keys but print a notification to console, 0 = normal (unbind all your keys).
- Increased the maximum screenshot count from 100 to 10000.
- Can bind tilde (~) to a key in the console by typing "tilde" so if you accidentally unbind tilde, it can be rebound.

V3.84:

- Automatic international keyboard mapping can be turned off (strange results on Belgium AZERTY keyboards).  
  This is now in the Advanced Settings menu and also be changed with in_keymap in the console 0= OFF, 1 = ON (default).

(US users are unaffected by this change, it doesn't matter if it is on or off).

V3.83:

- Added Quake Name Maker (from JoeQuake / Qrack).  Go to Options -> Multiplayer -> Name Maker to create a name.
- Commandline parameters -nojoy, -nocdaudio, -noipx have become -joy, -cdaudio, -ipx.  These now must be explicitly enabled instead of the reverse (because almost no one uses these).
- IPLog is enabled by default and always is logged in id1\ folder.  Use -noiplog to disable instead of -iplog to enable.
- Commandline command instead of cmdline cvar.  Type "commandline" in console to see your startup command line.
- Boring notes: Commented out numerous unused variables in the GL build engine code.  I'll be removing these entirely eventually, but I've left them commented out for now (not cosmetically pleasing when reading the code, sorry).

V3.82:

- DirectInput can now be turned on/off in the menu.
- Added a "time" command.  Type in console to display current time.
- Discovered a fixed 2 minor bugs with wqpro (mouse button issue, default resolution issue).

V3.80

Very recent additions:

- q_version support (from Qrack)
- server ipmasking (use -noipmask to disable usage)
- the -nosoundkeys parameter to disable default volumeup/volumedown functionality of -/+ keys

This is a rebuild off the ProQuake 3.50 base incrementally adding features because 3.77 introduced some difficult to identify bugs.

Main features of 3.80

- In-game gamma correction removed because it did not look ProQuake authentic.
- VSync ON|OFF now works properly
- Automatic 8 button mouse support
- Conwidth and conheight parameters work again

Some "extras" features have temporarily been removed to make it easier to generate a Linux version.

These features include the time command, gamedir switching, Direct3D renderer support, maplist and demolist commands.

Additionally, some rarely used "extras" features have been temporarily removed but will return in subsequent version:

interpolation, demo rewinding, demoplay via file association, cl_autodemo, rquake sv_dmflag, server side ip-masking, cl_mute.

All of these features will return in subsequent versions after a Linux and possibly a Mac version are able to be compiled.  Removal of these features increase to the odds of a rock-solid release and decreases potential issues with compiling on non-Windows operating systems.

V3.77

- Animation interpolation added (from GLQuake 113).  Right now it is on by default just for testing purposes.
- Fixed super-annoying ProQuake "chase_active 1 while demo is playing bug"; the fix is from Qrack (the problem is rather complicated to traceback, thank Rook for writing the fix in Qrack).
- conwidth/conheight currently doesn't do anything in the commandline in this version and 3.76

V3.76 

- Hunk_print command from FitzQuake
- Keybindlist (FitzQuake) "bindlist" (pacify the formatting)
- Condump command (FitzQuake)
- Lightning gun beam freezes when game is paused. (FitzQuake)
- Now using FitzQuake-style viewpos.
- Cvar cvar_reset_all, cvar_reset_one, toggle from FitzQuake
- LH Aim fix - Provided by LordHavoc
- "Reset to Defaults" in Options does what is expected; in GLQuake/GLProQuake it doesn't actually reset most things to the real defaults. (from FitzQuake)
- Playdemo by file association capability (ezQuake)
- Basic demo playback control keys (PGUP = fast forward toggle, PGDN = rewind).  Plan to adjust this more in the future.
- "Can't find gfx.wad" can no longer happen.  Implemented ezQuake's "use the exe directory" if the basedir is wrong.

V3.75

- Limited gamma in wqpro to minium of 0.1 to avoid full white screen from ever occurring.
- Fixed WinQuake chase_active 1 bug.  Apparently id source release didn't mirror the real official WinQuake in that code.  Fix from Enhanced GLQuake.
- Weapon now is always drawn in wqpro if fov > 90.  r_drawviewmodel 0 is the appropriate way to turn view model rendering off.
- Default hunk allocation is 32MB.
- Cmdlist/cvarlist now sorted. (From FitzQuake)
- Procrastination:  Direct3D version ALT-TAB issue (DDERR_SURFACELOST) can be fixed; it is a little complex so I'll do that sometime in the future.
- Demo rewind and fast forward (from JoeQuake).
- PAUSE key now pauses demos.
- ALT bronzing and CTRL special characters work with international keyboard support; the international keyboard support "broke".
- Skill level shows on-screen in single player when pressing showscores key (typically TAB).

Todo:

- Implement DarkPlaces-style more precise aim code LordHavoc provided.
- Fix vsync so it can be turned off and on in-client.
- Revisit the automatic conwidth/conheight method and improve it (it gets confused sometimes right now).
- Linux build.
- Direct3D ALT-TAB issue when full-screen (doesn't affect windowed mode).  Low priority.
- Better parameter completion.
- Hardwire -/+ as sound keys instead of the method currently used.

V3.703 

- International keyboard support (from DarkPlaces) and now numeric keypad always types numbers when the console is down (from DarkPlaces).

V3.702

- Added a Direct3D build, so those that can't use glpro.exe have an alternative, so they don't have to use wqpro.exe.  Building the D3D version requires the Direct3D Quake libraries http://dxquake.sourceforge.net/

V3.701

- Changed compiler options to resolve an issue in wqpro370.
V3.70

- Added full JoeQuake bronzing/coloring supporting via CTRL/ALT.
- Showspeed command (show_speed 1)
- Built-in aliases for convenience:  +quickgrenade, +quickrocket, +quickshot, +quickshaft, bestsafe, teamloc
- Enabled colors 14/15 for client (IHOC style), colors only work online on supporting servers like IHOC, Whitehot, RQuake
- Removed old maplist/demolist commands and reworked (because it could crash in certain circumstances)
- Created new maplist/demolist commands based on the JoeQuake dir command.
- The - and + keys are now volume adjustment keys.  Press - or + to change the volume at any time.

V3.66

- Intelligent resolution setting/detection.  When started with a plain command line (no -width/-height), glpro will search to find a resolution available for your display aspect ratio that will give you the highest FPS and performance, so it will look great on-screen and get a good FPS.

V3.65 

- Made the console width/height logical by default if not specified for nice on-screen appearance (from Qrack) and legible (50% conwidth) @ high resolutions (expanded on the concept).
- Vid_vsync is always off if your hardware permits; added max framerate to menu.

V3.64

- Enabled binding to the "Windows" keys, the left/right and the context menu one.  But the left/right cannot really be used at this time because it will switch you out of the application.

V3.63

- "-nodemolist" and "-nomaplist" command line options; to postpone working on the best solution to address excessively long filenames or situations with huge numbers of maps/demos.

V3.62

- GL extensions no longer print.

V3.61  

- Bidirectional use of advanced settings menu.
- Mirroralpha fix in GL is only used when mirroralpha < 1, hopefully this fixes an issue with some video cards that.
- r_drawviewmodel / gl_ringalpha was adjusted; it was drawing the weapon with r_drawviewmodel 0 + gl_ringalpha 1 when it was not supposed to.

V3.60

Bug Fixes or Really Annoying Things Fixed

- GLQuake high FOV rendering issue in some video cards appears to be fixed; fix from Enhanced GLQuake (but that code was from FitzQuake).
- Fixed ALT-Tabbing bug for Nvidia/GeForce cards -- so very annoying of a problem -- yeah technically it is Nvidia's driver fault, but that doesn't change the pain. (From JoeQuake).
- Mirror alpha fix.  For some reason, this seems historically important to me.  Set
- Using -window with wqpro.exe actually starts wqpro.exe in windowed mode (ranks up there with one of WinQuake's most annoying shortcomings).
- GL_Loadtexture: GLQuake cache mismatch bug eliminated.  Would generate an engine-stopping message going from maps with different textures, like E4M6 and then DM4.  Thanks to Reckless for provided the code, fix code by LordHavoc.
- gl_clear 1 doesn't have status bar flicker with the color FitzQuake uses. 
- Notifications of "Mousewheel is unbound" and such have been removed (except for currently non-supported mouse buttons 5-8, don't feel like implementing an external DLL fix right now).
- Allow TAB to be pressed during demoplayback to see scoreboard. (Consider expanding a la JoeQuake where any key can be pressed).
- Adjusted default values of pq_spam_rate and pq_spam_grace pq_tempmute as the defaults be very meddlesome in single player, coop, loc and mod testing.
- Shadow flicker fix.  It's gone now.  Particularly noticeable at E1M1 "earth" you shoot to open access to Quad. (From Unofficial GLQuake-113)
- Notification of non-26000 port connect failure so it doesn't take 5 minutes to figure out you are trying to connect to a non-existent port/server combination (Recommends to try port 26000 if the port is not 26000 -- from Rook's QRack).
- 640 x 480 is now the default full-screen wqpro.exe resolution instead of the tiny 320 x 200 (240?).
- Turning on "always run" increases cl_upspeed and cl_backspeed as well. The poor unknowing wonder why they don't move as fast -- they don't!

Developer Convenience

- Added viewpos command to tell current location. (This has been SOOO BADLY needed for ages.)
- FitzQuake's give armor command (ex: give "a" 200), so you can now give yourself only armor in single player.
- NO.  Reversed for assure dedicated server compatibility --> did not do this --> Max edicts raised from 600 to 2048, allowing ProQuake to play most single player map for lazy players that don't/won't use a real single player engine for that.
- RocketGuy's sv_dmflag, which allows RQuake to effectively use a DM scoreboard in coop (todo:  add cvars to support/use his other changes like color 14/15 support, sv_tellenabled, "quit!" command [which I don't know what it does], anything else I forgot) and Rook's "the server doesn't crash if it doesn't have the map" fix.

Convenience

- Vanilla cmdlist / cvarlist support from Quakesrc.org tutorial (todo: JoeQuake style command/parameter completion)
- Maplist/demolist commands.  
- cl_confirmquit 0 - quit without the prompt.
- time command - type in console to display current time.
- full screen windowed mode (from aguirRe's Enhanced GLQuake).
- Freelook cvar -- this has pretty much evolved to be the standard.
- Zone default increased to 1024 KB.  Yeah, 512 KB usually suffices, but 5% of the time it doesn't and this is a trivial amount in the modern day.
- Added cl_mute from Qrack and con_notifylines.  If someone is super annoying, just completely ignore them.  Also helps for turning demos into movies.
- "-fullwindow" option for GL to do a full-screen windowed mode.  To do:  Make it work in Linux.
- Made fov, r_drawviewmodel, scr_conspeed, cl_crossx/y, ambient sound, r_waterwarp, etc. save to config.
- cl_crosshaircentered - actually centers the crosshair.
- writeconfig command 
- Reorganized and cleaned up "customize controls".  All the same options are there, it's just much cleaner and the order makes sense.
- Menu to allow the easy ability to set all of the most important settings via the menu.

New Functionality

- In-game brightness adjustment -- it could use some more work. (From TyrQuake)
- Bronzing by holding ALT down in console (From JoeQuake).  Largely eliminates the need for a Quake name maker. (Question: Do all saved names get saved properly.)
- gl_ringalpha, set to less than 1 for weapon transparency - make weapon visible during invisibility (to do: option to draw weapon model in GL), but transparent.
- Added cl_autodemo from Qrack.
- FitzQuake style keypad binding support (off by default). (Think about: super extended support like JoeQuake where you can bind Left and Right CTRL/ALT separately).
- Added JoeQuake adjust volume on the fly suppport [volumeup/volumedown]. (Need to test in wqpro and implement; todo: mute-toggle type of command?).
- r_truegunangle - optionally render the gun angle "properly" (idea from how DarkPlaces/FitzQuake render weapon).

==============================
==============================
To Do List for Future Versions
==============================
==============================

1. Video capture
3. Support Total Conversions
4. Discreet demos menu under multiplayer??? If so, put the path on the screen dammit!  So newbies know where demos go!
5. Don't crash server on no map fix.
6. RocketGuy's RQuake server enhancements not already implemented as options. (Quit!, FRIK_FILE)
7. Optional Interpolation. (Smooths movement very noticeable in single player).
8. Add maps path to maplist command?  Add demo path to demolist command?
9. About my Quake option in discreet menu?
10. Make winquake and glquake versions save each other's cvars somehow so your config isn't nuked!

12. Clipboard copy
13. Diagnostics command or setting
14. Universal config system
15. Investigate ProQuake's cheat-free code and it's interference with mods.
16. Some sort of superzoom implementation.
17. Yugo2Heck's single port option??????  It hasn't been tested but seems to be how other engines do it.
18. Auto-connect to server from web page.
19. Deal better with minimizing/maximizing -windowed windows; I can't see seem to unminimze winquake sometimes.
20. 2 requested features from ORL
21. Look at FuhQuake's disable syswin keys feature, which makes it so the Windows key won't exit you.

Possibly

1. Optional dead body color fix. 
2. Extended key bind menu for mods, messagemode2, etc.
3. Frik_File / EBFS support (Rocketguy says EBFS will break QCCX compatibility:  cannot do EBFS)
4. PNG screenshots or JPG screenshots or both (it's not hard)
5. Better command or parameter completion like JoeQuake
6. Make IPLOG ON|OFF be a preference menu item, not command line option.
8. Client protection from "unbindallkeys" to protect from abusive admins
11. Optional server word filter (client word filter?) for admins.
12. 8 button mouse support and fixing bind button issues with -dinput (requires an extra DLL :(  ).
13. Support for playing .dz demos
14. Neil's demos over multiple maps fix?
15. In-game server browser

Somewhat Unlikely

1. Startup menu to select resolution or in-game video mode changing
2. Play Demo Via File Association
3. DarkPlaces type console where console text size can be set independently
4. Optional sbar alpha support for a DarkPlaces style transparent HUD.
6. Optional Centerprint logging. (Easy to do, does it fit the feel of ProQuake though?)
7. Convert demos to .dz format upon completion of recording?

/ End Unofficial Changelog

============================================================================================
ProQuake change log
============================================================================================

V3.50

- Fixed cheat-free connection lag
- added cheatfree status command
- added "cheat-free" to status message
- added (cheat-free) to host hame
- don't send fullpitch messages to qsmack clients
- allow tell for qsmack clients
- use current server if rcon_server is not set

V3.40

- Fixed NAT!

V3.30

- iplog supports multiple servers; logs are automatically merged
- limit of 64 entries with a single IP address
- fullbright shaft
- upped Sys_Printf buffer from 1024 to 2048
- check for !cls.netcon in Con_Printf
- upped MAX_MODE_LIST from 30 to 64 in GL
- extended wheel support
- 4 button mouse support
- r_polyblend
- bestweapon
- can connect to cheat free server without map

V3.20

- Fixed incoming message buffer overflow error
- added pq_tempmute
- added pq_cheatfree
- removed -cheatfree command-line argument
- added pq_removecr
- added pq_logbinds
- added %d
- recognize ip:port
- commented out slist if connect fails
- better connection error messages
- fixed parsing of pqc_team messages to prevent team scores from dropping to 0 in crmod 6.3
- use original game directory for file verification

V3.11

- fixed auto ping when there's a gap in client numbers
- added pq_showedict for Slot Zero
- early exit from Host_Color_f if colour isn't actually changing

V3.02 = V3.1

- fixed spelling of gl_doubleeys
- removed gl_eyes BS!!
- support for qsmack
- fixed GL memory leak
- gamma correction!
- chase_active fix
- remove '\n' from names
- fixed "allways"
- fixed winquake screenshot gamma correction

- fixed "c:\quake\glpro.exe" not found bug
- fixed "server is full" bug
- fixed listen server bug
- fixed autorepeat
- fixed rcon buffer overflow
- added rcon_server
- made auto-ping checking more robust ("unconnected" and '\n' doesn't mess it up)

V3.00

- CSR -ip patch
- fixed buffer overrun problem for NVidia 5.16 drivers
- put [] around score in scoreboard
- added %w (weapons)
- don't print "I have" if you're dead!
- don't allow say or say_team immediately after changing name/colour
- ipmerge (Klas Nyblom)
- identify <name>
- fixed win2k compatability
- added poll counter to test2!
- added error messages to test, test2
- rcon!!
- fixed consecutive demo bug
- CHEAT FREE!!!
- changed pq_spam_rate to 1.5
- "known aliases for <15 chars mas>"
- added pq_smoothcam for Slot Zero

V2.01

- commented out reverse lookup code
- server side fullpitch correction
- net_connecttimeout

V1.05

- added pq_moveup
- changed default zone size to 512K
- added menu option for changing _windowed_mouse under X11
- pq_waterblend, pq_quadblend, pq_ringblend, pq_pentblend, pq_suitblend
- pq_fullpitch
- pq_timestamp
- pq_confilter
- PGUP, PGDOWN, TAB autorepeat (only in console!!)
- improved location logic (nearest rectangle)
- qflood protection
- fixed command completion (includes aliases)
- added lg, gl to dm3.loc
- translation to plain text in dedicated server console
- ip logging!
- demo recording!!
- pq_maxfps

V1.04

- added spam protection
	- pq_spam_rate
	- pq_spam_grace
- added %x (rockets), %c (cells)
- added pq_haverl, pq_needrl, pq_needrox
- added %p
	- pq_quad
	- pq_pent
	- pq_ring
- fixed -ip parameter for sqpro

V1.03

- added say, say_team formatting: %a, %h, %r, %l (with location files)
	- credit: Ramirez, http://nqctf.gamepoint.net/
- added pasting to console and messagemode
	- credit: FricaC, www.quakesource.org
- fixed stupid bug in call to Con_Debuglog, in Con_Printf (forgot "%s"!!!)
- removed coop 0 from demos

V1.02

- added pq_drawfps
- fixed 0 packet size bug
- added -ip for linux

V1.00

Client:

- precise aim for ProQuake connection
- ProQuake message on connect
- ProQuake version
- Auto-smooth for camera/chasecam/eyecam
- longer mm1, mm2 messages
- (say): for messagemode2
- support for mods in qsocket (EXPLAIN - byte after accept server->client; byte after connect client->server)
- fixed losing backscroll on print
- fixed HOME and END in console
- Added proquake commands to CL_Parse
- pq_teamscores: teamscores in status bar!!!
- pq_timer: timer in status bar!!
- put sound back to DOSquake levels!
- added ping to scoreboard
- added ProQuake help page
- added cl_crossx, cl_crossy for GL
- queries server for pings if not connected to crmod6.0 server
- added pq_lag - synthetic lag!
- recognizes ping +N
- grab match time from crmod 4.0-5.1
- added r_waterwarp to GL

Server:

- disabled tell
- () in mm2 (server side)
- Allow multiple connects from any IP (server side)
- precise aim for ProQuake connection
- added hook for QuakeC to turn off server flag in cvars
- Added ProQuake console variable (so mods can detect it)
- can set logfile name; can use %d for multiple logs
- log file header
- dprint(proquake client connected) if proquake client detected


Other:
- removed "backup past 0"
- removed "stuck", "unstuck" messages
- restored old winquake icon