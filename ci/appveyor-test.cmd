cd %APPVEYOR_BUILD_FOLDER%

if "%PLATFORM%" EQU "x86" set RUNPATH=%CONFIGURATION%
if "%PLATFORM%" EQU "x64" set RUNPATH=x64\%CONFIGURATION%

%RUNPATH%\bkdecmd.exe i images\ANDOS330.IMG
%RUNPATH%\bkdecmd.exe l images\ANDOS330.IMG
%RUNPATH%\bkdecmd.exe lr images\ANDOS330.IMG
%RUNPATH%\bkdecmd.exe lr images\AODOS.IMG
%RUNPATH%\bkdecmd.exe lr images\BK_SYS.DSK
%RUNPATH%\bkdecmd.exe lr images\CSIDOS.IMG
%RUNPATH%\bkdecmd.exe lr images\MKDOS315.IMG
%RUNPATH%\bkdecmd.exe lr images\NORD_1.bkd
%RUNPATH%\bkdecmd.exe lm images\ANDOS330.IMG
%RUNPATH%\bkdecmd.exe lm images\AODOS.IMG
%RUNPATH%\bkdecmd.exe lm images\BK_SYS.DSK
%RUNPATH%\bkdecmd.exe lm images\CSIDOS.IMG
%RUNPATH%\bkdecmd.exe lm images\MKDOS315.IMG
%RUNPATH%\bkdecmd.exe lm images\NORD_1.bkd
%RUNPATH%\bkdecmd.exe lm -sha1 images\ANDOS330.IMG
%RUNPATH%\bkdecmd.exe lm -sha1 images\AODOS.IMG
%RUNPATH%\bkdecmd.exe lm -sha1 images\BK_SYS.DSK
%RUNPATH%\bkdecmd.exe lm -sha1 images\CSIDOS.IMG
%RUNPATH%\bkdecmd.exe lm -sha1 images\MKDOS315.IMG
%RUNPATH%\bkdecmd.exe lm -sha1 images\NORD_1.bkd
