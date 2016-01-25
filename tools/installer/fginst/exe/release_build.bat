@echo off

SET UNPACKED_FILE=fginst.exe
SET PACKED_FILE=fginst_encrypt.exe
SET WORKSPACE=fginst.dsw
SET PROJECT="fginst - Win32 Release"

SET BUILD_PATH=C:\src\dweb\client\utilities\fginst
SET REL_PATH=C:\src\dweb\client\utilities\fginst\exe
SET PACK_PATH="C:\Program Files\EXECryptor\"

ECHO =============Remove existing files=============
DEL %REL_PATH%\%UNPACKED_FILE%
DEL %REL_PATH%\%PACKED_FILE%

ECHO =============VC6 project building=============
CD %BUILD_PATH%
msdev %WORKSPACE% /MAKE %PROJECT% /REBUILD

ECHO =============Execryptor packing=============
CD %REL_PATH%
%PACK_PATH%\EXECrypt.exe %REL_PATH%\fginst.ep2

PAUSE

SET UNPACKED_FILE=
SET PACKED_FILE=
SET REL_PATH=
SET BUILD_PATH=
SET PACK_PATH=
