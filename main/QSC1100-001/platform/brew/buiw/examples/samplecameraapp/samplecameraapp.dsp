# Microsoft Developer Studio Project File - Name="samplecameraapp" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=samplecameraapp - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "samplecameraapp.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "samplecameraapp.mak" CFG="samplecameraapp - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "samplecameraapp - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "samplecameraapp - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "samplecameraapp"
# PROP Scc_LocalPath "..\widget"
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "samplecameraapp - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "samplecameraapp_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /WX /GX /O2 /I "$(BUITDIR)\widgets\inc" /I "..\..\framewidget\inc" /I "$(BUITDIR)\xmod" /I "$(BREWDIR)/inc" /D "NDEBUG" /D "AEE_SIMULATOR" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "samplecameraapp_EXPORTS" /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386

!ELSEIF  "$(CFG)" == "samplecameraapp - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "samplecameraapp_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /WX /Gm /GX /ZI /Od /I "$(BUITDIR)\widgets\inc" /I "..\..\framewidget\inc" /I "$(BUITDIR)\xmod" /I "$(BREWDIR)/inc" /D "_DEBUG" /D "AEE_SIMULATOR" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "samplecameraapp_EXPORTS" /FR /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /pdb:"./samplecameraapp.pdb" /debug /machine:I386 /out:"samplecameraapp.dll" /implib:"./samplecameraapp.lib" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build
TargetPath=.\samplecameraapp.dll
TargetName=samplecameraapp
InputPath=.\samplecameraapp.dll
SOURCE="$(InputPath)"

"$(TargetName).dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy $(TargetPath) .

# End Custom Build

!ENDIF 

# Begin Target

# Name "samplecameraapp - Win32 Release"
# Name "samplecameraapp - Win32 Debug"
# Begin Group "App Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE="$(BUITDIR)\widgets\src\AEEBase.c"
# End Source File
# Begin Source File

SOURCE=./src/samplecameraapp.c
# End Source File
# Begin Source File

SOURCE=.\src\widget_stubs.c
# End Source File
# Begin Source File

SOURCE="$(BUITDIR)\xmod\xmod.h"
# End Source File
# Begin Source File

SOURCE="$(BUITDIR)\xmod\xmodimpl.c"
# End Source File
# Begin Source File

SOURCE="$(BUITDIR)\xmod\xmodstub.c"
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\res\samplecameraapp.brx

!IF  "$(CFG)" == "samplecameraapp - Win32 Release"

# Begin Custom Build
InputPath=.\res\samplecameraapp.brx

BuildCmds= \
	"$(BREWSDKTOOLSDIR)\ResourceEditor\brewrc.exe" -s -o samplecameraapp.bar -h samplecameraapp_res.h $(InputPath)

"samplecameraapp.bar" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"samplecameraapp_res.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "samplecameraapp - Win32 Debug"

# Begin Custom Build
InputPath=.\res\samplecameraapp.brx

BuildCmds= \
	"$(BREWSDKTOOLSDIR)\ResourceEditor\brewrc.exe" -s -o ./samplecameraapp.bar -h ./inc/samplecameraapp_res.h $(InputPath)

"samplecameraapp.bar" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"samplecameraapp_res.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mif\samplecameraapp.mfx

!IF  "$(CFG)" == "samplecameraapp - Win32 Release"

# Begin Custom Build
TargetName=samplecameraapp
InputPath=.\mif\samplecameraapp.mfx

"..\$(TargetName).mif" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	"$(BREWSDKTOOLSDIR)\ResourceEditor\brewrc.exe" -o ..\$(TargetName).mif -i mif $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "samplecameraapp - Win32 Debug"

# Begin Custom Build
InputPath=.\mif\samplecameraapp.mfx

"..\samplecameraapp.mif" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	"$(BREWSDKTOOLSDIR)\ResourceEditor\brewrc.exe" -nh  -o ..\samplecameraapp.mif -i mif $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# End Group
# End Target
# End Project
