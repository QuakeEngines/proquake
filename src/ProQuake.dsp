# Microsoft Developer Studio Project File - Name="ProQuake" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=ProQuake - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ProQuake.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ProQuake.mak" CFG="ProQuake - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ProQuake - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "ProQuake - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "ProQuake - Win32 DX8 Release" (based on "Win32 (x86) Application")
!MESSAGE "ProQuake - Win32 DX8 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ProQuake - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release_MSVC6"
# PROP BASE Intermediate_Dir "Release_MSVC6"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_MSVC6"
# PROP Intermediate_Dir "Release_MSVC6"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G5 /W3 /GX /O2 /I ".\dxsdk\sdk8\include" /D "NDEBUG" /D "GLQUAKE" /D "WIN32" /D "_WINDOWS" /Fr /YX /FD /c
# ADD CPP /nologo /G5 /W3 /GX /O2 /I ".\dxsdk\sdk\inc" /I ".\curl\include" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 advapi32.lib comctl32.lib dsound.lib dxguid.lib gdi32.lib libcurl.lib kernel32.lib ole32.lib oleaut32.lib opengl32.lib shell32.lib strmiids.lib user32.lib winmm.lib wsock32.lib libpng.lib zlib.lib libjpeg.lib /nologo /subsystem:windows /profile /map /machine:I386 /out:"..\dx8_Engine_X_473.exe" /libpath:".\fmod" /libpath:".\curl" /libpath:".\zlib" /libpath:".\png" /libpath:".\jpeg" /libpath:".\dxsdk\sdk8\lib"
# ADD LINK32 advapi32.lib gdi32.lib kernel32.lib dxguid.lib ole32.lib opengl32.lib oleaut32.lib shell32.lib user32.lib winmm.lib wsock32.lib curl/lib/libcurl.lib zlib/lib/zlib.lib /nologo /subsystem:windows /machine:I386 /out:"C:\quake2\glpro493.exe"
# SUBTRACT LINK32 /profile /incremental:yes /map

!ELSEIF  "$(CFG)" == "ProQuake - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Debug_MSVC6"
# PROP BASE Intermediate_Dir "Debug_MSVC6"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Debug_MSVC6"
# PROP Intermediate_Dir "Debug_MSVC6"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G5 /W3 /GX /ZI /Od /I ".\dxsdk\sdk8\include" /I ".\png" /I ".\zlib" /I ".\fmod" /I ".\curl" /I ".\jpeg" /I ".\dxsdk\sdk7\include" /D "NDEBUG" /D "GLQUAKE" /D "WIN32" /D "_WINDOWS" /D "_DEBUG" /Fr /YX /FD /c
# ADD CPP /nologo /G5 /MT /W3 /GX /ZI /Od /I ".\dxsdk\sdk\inc" /I ".\curl\include" /D "_DEBUG" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 advapi32.lib comctl32.lib dsound.lib dxguid.lib gdi32.lib libcurl.lib kernel32.lib ole32.lib oleaut32.lib opengl32.lib shell32.lib strmiids.lib user32.lib winmm.lib wsock32.lib libpng.lib zlib.lib libjpeg.lib Comctl32.lib /nologo /subsystem:windows /profile /map /debug /machine:I386 /out:"..\dx8_Engine_X_473_dbg.exe" /libpath:".\fmod" /libpath:".\curl" /libpath:".\zlib" /libpath:".\png" /libpath:".\jpeg" /libpath:".\dxsdk\sdk8\lib"
# ADD LINK32 advapi32.lib dsound.lib dxguid.lib gdi32.lib kernel32.lib ole32.lib oleaut32.lib opengl32.lib shell32.lib user32.lib winmm.lib wsock32.lib curl/lib/libcurl.lib zlib/lib/zlib.lib /nologo /subsystem:windows /incremental:yes /map /debug /machine:I386 /out:"C:\quake2\glpro493_debug.exe"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "ProQuake - Win32 DX8 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ProQuake___Win32_DX8_Release"
# PROP BASE Intermediate_Dir "ProQuake___Win32_DX8_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ProQuake___Win32_DX8_Release"
# PROP Intermediate_Dir "ProQuake___Win32_DX8_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G5 /W3 /GX /O2 /I ".\dxsdk\sdk\inc" /I ".\curl\include" /D "WIN32" /FR /YX /FD /c
# ADD CPP /nologo /G5 /W3 /GX /O2 /I ".\dxsdk\sdk8\include" /I ".\curl\include" /D "DX8QUAKE" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 advapi32.lib gdi32.lib kernel32.lib dxguid.lib ole32.lib opengl32.lib oleaut32.lib shell32.lib user32.lib winmm.lib wsock32.lib curl/lib/libcurl.lib zlib/lib/zlib.lib /nologo /subsystem:windows /incremental:yes /machine:I386 /out:"C:\quake\ProQuake5.exe"
# SUBTRACT BASE LINK32 /profile /map
# ADD LINK32 advapi32.lib gdi32.lib kernel32.lib dxguid.lib ole32.lib oleaut32.lib shell32.lib user32.lib winmm.lib wsock32.lib curl/lib/libcurl.lib zlib/lib/zlib.lib dxsdk/sdk8/lib/d3d8.lib ./dxsdk/sdk8/lib/d3dx8.lib /nologo /subsystem:windows /machine:I386 /out:"C:\quake2\dx8pro493.exe" /libpath:".\dxsdk\sdk8\lib"
# SUBTRACT LINK32 /pdb:none /incremental:yes

!ELSEIF  "$(CFG)" == "ProQuake - Win32 DX8 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ProQuake___Win32_DX8_Debug"
# PROP BASE Intermediate_Dir "ProQuake___Win32_DX8_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ProQuake___Win32_DX8_Debug"
# PROP Intermediate_Dir "ProQuake___Win32_DX8_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G5 /MT /W3 /GX /ZI /Od /I ".\dxsdk\sdk\inc" /I ".\curl\include" /D "DEBUG" /FR /YX /FD /c
# ADD CPP /nologo /G5 /MT /W3 /GX /ZI /Od /I ".\dxsdk\sdk8\include" /I ".\curl\include" /D "DX8QUAKE" /D "_DEBUG" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 advapi32.lib dsound.lib dxguid.lib gdi32.lib kernel32.lib ole32.lib oleaut32.lib opengl32.lib shell32.lib user32.lib winmm.lib wsock32.lib curl/lib/libcurl.lib zlib/lib/zlib.lib /nologo /subsystem:windows /incremental:yes /map /debug /machine:I386 /out:"C:\quake\ProQuake5_debug.exe"
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 advapi32.lib dsound.lib dxguid.lib gdi32.lib kernel32.lib ole32.lib oleaut32.lib shell32.lib user32.lib winmm.lib wsock32.lib curl/lib/libcurl.lib zlib/lib/zlib.lib dxsdk/sdk8/lib/d3d8.lib ./dxsdk/sdk8/lib/d3dx8.lib /nologo /subsystem:windows /incremental:yes /map /debug /machine:I386 /out:"C:\quake2\dx8pro493_debug.exe"
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "ProQuake - Win32 Release"
# Name "ProQuake - Win32 Debug"
# Name "ProQuake - Win32 DX8 Release"
# Name "ProQuake - Win32 DX8 Debug"
# Begin Group "Source Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\anorm_dots.h
# End Source File
# Begin Source File

SOURCE=.\anorms.h
# End Source File
# Begin Source File

SOURCE=.\bspfile.h
# End Source File
# Begin Source File

SOURCE=.\chase.c
# End Source File
# Begin Source File

SOURCE=.\cl_demo.c
# End Source File
# Begin Source File

SOURCE=.\cl_input.c
# End Source File
# Begin Source File

SOURCE=.\cl_main.c
# End Source File
# Begin Source File

SOURCE=.\cl_parse.c
# End Source File
# Begin Source File

SOURCE=.\cl_server_browser.c
# End Source File
# Begin Source File

SOURCE=.\cl_server_browser.h
# End Source File
# Begin Source File

SOURCE=.\cl_tent.c
# End Source File
# Begin Source File

SOURCE=.\client.h
# End Source File
# Begin Source File

SOURCE=.\cmd.c
# End Source File
# Begin Source File

SOURCE=.\cmd.h
# End Source File
# Begin Source File

SOURCE=.\common.c
# End Source File
# Begin Source File

SOURCE=.\common.h
# End Source File
# Begin Source File

SOURCE=.\conproc.c
# End Source File
# Begin Source File

SOURCE=.\conproc.h
# End Source File
# Begin Source File

SOURCE=.\console.c
# End Source File
# Begin Source File

SOURCE=.\console.h
# End Source File
# Begin Source File

SOURCE=.\crc.c
# End Source File
# Begin Source File

SOURCE=.\crc.h
# End Source File
# Begin Source File

SOURCE=.\cvar.c
# End Source File
# Begin Source File

SOURCE=.\cvar.h
# End Source File
# Begin Source File

SOURCE=.\draw.h
# End Source File
# Begin Source File

SOURCE=.\dx8_fakegl.h

!IF  "$(CFG)" == "ProQuake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ProQuake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ProQuake - Win32 DX8 Release"

!ELSEIF  "$(CFG)" == "ProQuake - Win32 DX8 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dx8_gl_fakegl.c

!IF  "$(CFG)" == "ProQuake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ProQuake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ProQuake - Win32 DX8 Release"

!ELSEIF  "$(CFG)" == "ProQuake - Win32 DX8 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\gl_draw.c
# End Source File
# Begin Source File

SOURCE=.\gl_fullbright.c
# End Source File
# Begin Source File

SOURCE=.\gl_fullbright.h
# End Source File
# Begin Source File

SOURCE=.\gl_mesh.c
# End Source File
# Begin Source File

SOURCE=.\gl_model.c
# End Source File
# Begin Source File

SOURCE=.\gl_model.h
# End Source File
# Begin Source File

SOURCE=.\gl_refrag.c
# End Source File
# Begin Source File

SOURCE=.\gl_rlight.c
# End Source File
# Begin Source File

SOURCE=.\gl_rmain.c
# End Source File
# Begin Source File

SOURCE=.\gl_rmisc.c
# End Source File
# Begin Source File

SOURCE=.\gl_rsurf.c
# End Source File
# Begin Source File

SOURCE=.\gl_screen.c
# End Source File
# Begin Source File

SOURCE=.\gl_test.c
# End Source File
# Begin Source File

SOURCE=.\gl_texture.h
# End Source File
# Begin Source File

SOURCE=.\gl_vid_w32.c
# End Source File
# Begin Source File

SOURCE=.\gl_vidnt.c
# End Source File
# Begin Source File

SOURCE=.\gl_warp.c
# End Source File
# Begin Source File

SOURCE=.\gl_warp_sin.h
# End Source File
# Begin Source File

SOURCE=.\glquake.h
# End Source File
# Begin Source File

SOURCE=.\host.c
# End Source File
# Begin Source File

SOURCE=.\host_cmd.c
# End Source File
# Begin Source File

SOURCE=.\in_win.c
# End Source File
# Begin Source File

SOURCE=.\input.h
# End Source File
# Begin Source File

SOURCE=.\iplog.c
# End Source File
# Begin Source File

SOURCE=.\iplog.h
# End Source File
# Begin Source File

SOURCE=.\keys.c
# End Source File
# Begin Source File

SOURCE=.\keys.h
# End Source File
# Begin Source File

SOURCE=.\location.c
# End Source File
# Begin Source File

SOURCE=.\location.h
# End Source File
# Begin Source File

SOURCE=.\mathlib.c
# End Source File
# Begin Source File

SOURCE=.\mathlib.h
# End Source File
# Begin Source File

SOURCE=.\matrix.c
# End Source File
# Begin Source File

SOURCE=.\menu.c
# End Source File
# Begin Source File

SOURCE=.\menu.h
# End Source File
# Begin Source File

SOURCE=.\modelgen.h
# End Source File
# Begin Source File

SOURCE=.\movie.c
# End Source File
# Begin Source File

SOURCE=.\movie.h
# End Source File
# Begin Source File

SOURCE=.\movie_avi.c
# End Source File
# Begin Source File

SOURCE=.\movie_avi.h
# End Source File
# Begin Source File

SOURCE=.\net.h
# End Source File
# Begin Source File

SOURCE=.\net_dgrm.c
# End Source File
# Begin Source File

SOURCE=.\net_dgrm.h
# End Source File
# Begin Source File

SOURCE=.\net_loop.c
# End Source File
# Begin Source File

SOURCE=.\net_loop.h
# End Source File
# Begin Source File

SOURCE=.\net_main.c
# End Source File
# Begin Source File

SOURCE=.\net_vcr.c
# End Source File
# Begin Source File

SOURCE=.\net_vcr.h
# End Source File
# Begin Source File

SOURCE=.\net_win.c
# End Source File
# Begin Source File

SOURCE=.\net_wins.c
# End Source File
# Begin Source File

SOURCE=.\net_wins.h
# End Source File
# Begin Source File

SOURCE=.\pr_cmds.c
# End Source File
# Begin Source File

SOURCE=.\pr_comp.h
# End Source File
# Begin Source File

SOURCE=.\pr_edict.c
# End Source File
# Begin Source File

SOURCE=.\pr_exec.c
# End Source File
# Begin Source File

SOURCE=.\progdefs.h
# End Source File
# Begin Source File

SOURCE=.\progs.h
# End Source File
# Begin Source File

SOURCE=.\protocol.h
# End Source File
# Begin Source File

SOURCE=.\quakedef.h
# End Source File
# Begin Source File

SOURCE=.\r_part.c
# End Source File
# Begin Source File

SOURCE=.\render.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\sbar.c
# End Source File
# Begin Source File

SOURCE=.\sbar.h
# End Source File
# Begin Source File

SOURCE=.\screen.h
# End Source File
# Begin Source File

SOURCE=.\security.c
# End Source File
# Begin Source File

SOURCE=.\security.h
# End Source File
# Begin Source File

SOURCE=.\server.h
# End Source File
# Begin Source File

SOURCE=.\snd_dma.c
# End Source File
# Begin Source File

SOURCE=.\snd_mem.c
# End Source File
# Begin Source File

SOURCE=.\snd_mix.c
# End Source File
# Begin Source File

SOURCE=.\snd_win.c
# End Source File
# Begin Source File

SOURCE=.\sound.h
# End Source File
# Begin Source File

SOURCE=.\spritegn.h
# End Source File
# Begin Source File

SOURCE=.\sv_main.c
# End Source File
# Begin Source File

SOURCE=.\sv_move.c
# End Source File
# Begin Source File

SOURCE=.\sv_phys.c
# End Source File
# Begin Source File

SOURCE=.\sv_user.c
# End Source File
# Begin Source File

SOURCE=.\sys.h
# End Source File
# Begin Source File

SOURCE=.\sys_win.c
# End Source File
# Begin Source File

SOURCE=.\version.h
# End Source File
# Begin Source File

SOURCE=.\vid.h
# End Source File
# Begin Source File

SOURCE=.\vid_common_gl.c
# End Source File
# Begin Source File

SOURCE=.\view.c
# End Source File
# Begin Source File

SOURCE=.\view.h
# End Source File
# Begin Source File

SOURCE=.\wad.c
# End Source File
# Begin Source File

SOURCE=.\wad.h
# End Source File
# Begin Source File

SOURCE=.\webdownload.c
# End Source File
# Begin Source File

SOURCE=.\webdownload.h
# End Source File
# Begin Source File

SOURCE=.\winquake.h
# End Source File
# Begin Source File

SOURCE=.\world.c
# End Source File
# Begin Source File

SOURCE=.\world.h
# End Source File
# Begin Source File

SOURCE=.\zone.c
# End Source File
# Begin Source File

SOURCE=.\zone.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\ProQuake.ico
# End Source File
# Begin Source File

SOURCE=.\ProQuake.rc
# End Source File
# End Group
# Begin Source File

SOURCE=.\notes.txt
# End Source File
# End Target
# End Project
