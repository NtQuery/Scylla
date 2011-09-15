
Scylla - x64/x86 Imports Reconstruction

******************************************************************

ImpREC, CHimpREC, Imports Fixer... this are all great tools to rebuild an import table, 
but they all have some major disadvantages, so I decided to create my own tool for this job.

Scylla's key benefits are:

 - x64 and x86 support
 - full unicode support (probably some russian or chinese will like this :-) )
 - written in C/C++
 - plugin support
 - works great with Windows 7


This tool was designed to be used with Windows 7 x64, so it is recommend to use this operating system. 
But it may work with XP and Vista, too.


Source code is licensed under GNU GENERAL PUBLIC LICENSE v3.0


******************************************************************

                           Known Bugs

******************************************************************
- Only Windows XP x64:
			Windows XP x64 has some API bugs. 100% correct imports reconstruction is impossible.
			If you still want to use XP x64, here are some hints:

			* EncodePointer/DecodePointer exported by kernel32.dll have both the same VA. Scylla, CHimpREC and 
			  other tools cannot know which API is correct. You need to fix this manually. Your fixed dump will probably 
			  run fine on XP but crash on Vista/7.

- ImpREC plugin support:
			Some ImpREC Plugins don't work with Windows Vista/7 because they don't "return 1" in the DllMain function.





******************************************************************

                           Changelog

******************************************************************

Version 0.3a:
	- Improved import resolving
	- fixed buffer overflow errors

Version 0.3:
	- ImpREC plugin support
	- minor bug fix

Version 0.2a:
	- improved disassembler dialog
	- improved iat search

Version 0.2:
	- improved process detection
	- added some options
	- new options dialog
	- improved source code

