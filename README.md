Scylla - x64/x86 Imports Reconstruction
=======================================

ImpREC, CHimpREC, Imports Fixer... this are all great tools to rebuild an import table, 
but they all have some major disadvantages, so I decided to create my own tool for this job.

Scylla's key benefits are:

 - x64 and x86 support
 - full unicode support
 - written in C/C++
 - plugin support
 - works great with Windows 7

This tool was designed to be used with Windows 7 x64, so it is recommend to use this operating system. 
But it may work with XP and Vista, too.

Source code is licensed under GNU GENERAL PUBLIC LICENSE v3.0


Known Bugs
----------

### Windows 7 x64

Sometimes the API kernel32.dll GetProcAddress cannot be resolved, because the IAT has an entry from apphelp.dll
Solution? I don't know

### Only Windows XP x64:

Windows XP x64 has some API bugs. 100% correct imports reconstruction is impossible.
If you still want to use XP x64, here are some hints:

* EncodePointer/DecodePointer exported by kernel32.dll have both the same VA.
  Scylla, CHimpREC and other tools cannot know which API is correct. You need to fix this manually.
  Your fixed dump will probably run fine on XP but crash on Vista/7.

### ImpREC plugin support:

Some ImpREC Plugins don't work with Windows Vista/7 because they don't "return 1" in the DllMain function.


Keyboard Shortcuts
------------------

- CTRL + D: [D]ump
- CTRL + F: [F]ix Dump
- CTRL + R: PE [R]ebuild
- CTRL + O: L[o]ad Tree
- CTRL + S: [S]ave Tree
- CTRL + T: Auto[t]race
- CTRL + G: [G]et Imports
- CTRL + I: [I]AT Autosearch


Changelog
---------

Version 0.9.6

- improved iat search
- fixed bug in api resolve engine
- new option: parse APIs always from disk -> slower, useful against pe header modifications

Version 0.9.5

- Fixed virtual device bug caused by QueryDosDeviceW bug
- improved process lister
- improved module lister
- improved dump name
- improved IAT parser

Version 0.9.4 Final

- direct import scanner (LEA, MOV, PUSH, CALL, JMP) + fixer with 2 fix methods
- create new iat in section
- fixed various bugs 

Version 0.9.3

- new dll function: iat search
- new dll function: iat fix auto

Version 0.9.2

- Pick DLL -> Set DLL Entrypoint
- Advanced IAT Search Algorithm (Enable/Disable it in Options), thanks to ahmadmansoor
- Fixed bug in Options
- Added donate information, please feel free to donate some BTC to support this project

Version 0.9.1

- Fixed virtual device bug
- Fixed 2 minor bugs

Version 0.9

- updated to distorm v3.3
- added application exception handler
- fixed bug in dump engine
- improved "suspend process" feature, messagebox on exit

Version 0.8

- added OriginalFirstThunk support. Thanks to p0c
- fixed malformed dos header bug
- NtCreateThreadEx added infos from waliedassar, thanks! 

Version 0.7 Beta

- fixed bug Overlapped Headers
- fixed bug SizeOfOptionalHeader
- added feature: suspend process for dumping, more information
- improved disassembler
- fixed various bugs

Version 0.6b

- internal code changes
- added option: fix iat and oep

Version 0.6a

- fixed buffer to small bug in dump memory

Version 0.6

- added dump memory regions
- added dump pe sections -> you can edit some values in the dialog
- improved dump engine with intelligent dumping
- improved pe rebuild engine -> removed yoda's code
- fixed various bugs

Version 0.5a:

- fixed memory leak
- improved IAT search

Version 0.5:

- added save/load import tree feature
- multi-select in tree view
- fixed black icons problem in tree view
- added keyboard shortcuts
- dll dump + dll dump fix now working
- added support for scattered IATs
- pre select target path in open file dialogs
- improved import resolving engine with api scoring
- api selection dialog
- minor bug fixes and improvements

Version 0.4:

 - GUI code improvements
 - bug fixes
 - imports by ordinal

Version 0.3a:

 - Improved import resolving
 - fixed buffer overflow errors

Version 0.3:

 - ImpREC plugin support
 - minor bug fix
