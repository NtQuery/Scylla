@echo off
SET SCYVERSION=Scylla_v0.11
if not exist .\%SCYVERSION% mkdir .\%SCYVERSION%
copy ".\Win32\Release\Scylla.exe" ".\%SCYVERSION%\Scylla_x86.exe"
copy ".\x64\Release\Scylla.exe" ".\%SCYVERSION%\Scylla_x64.exe"
copy ".\Win32\Release\Scylla.map" ".\%SCYVERSION%\Scylla_x86.map"
copy ".\x64\Release\Scylla.map" ".\%SCYVERSION%\Scylla_x64.map"
copy ".\Win32\Release\ScyllaDLL.dll" ".\%SCYVERSION%\Scylla_x86.dll"
copy ".\x64\Release\ScyllaDLL.dll" ".\%SCYVERSION%\Scylla_x64.dll"
copy ".\Win32\Release\ScyllaDLL.lib" ".\%SCYVERSION%\Scylla_x86.lib"
copy ".\x64\Release\ScyllaDLL.lib" ".\%SCYVERSION%\Scylla_x64.lib"
pause