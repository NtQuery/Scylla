
#pragma once

#define APPNAME "Scylla"

#ifdef _WIN64

#define ARCHITECTURE "x64"
#define PRINTF_DWORD_PTR "%I64X"
#define PRINTF_DWORD_PTR_FULL "%016I64X"
#define PRINTF_DWORD_PTR_HALF "%08I64X"
#define PRINTF_INTEGER "%I64u"
#define MAX_HEX_VALUE_EDIT_LENGTH 16

#else

#define ARCHITECTURE "x86"
#define PRINTF_DWORD_PTR "%X"
#define PRINTF_DWORD_PTR_FULL "%08X"
#define PRINTF_DWORD_PTR_HALF "%08X"
#define PRINTF_INTEGER "%u"
#define MAX_HEX_VALUE_EDIT_LENGTH 8

#endif

#define APPVERSION "v0.4"

#define RECOMMENDED_OS "This tool was designed to work with Windows 7 x64"
#define DEVELOPED "Developed with Microsoft Visual Studio 2010, written in pure C/C++"
#define CREDIT_DISTORM "This tool uses the diStorm disassembler library -> http://code.google.com/p/distorm/"
#define CREDIT_YODA "The PE Rebuilder engine is based on the Realign DLL version 1.5 by yoda"
#define CREDIT_SILK "The small icons are taken from the Silk icon package -> http://www.famfamfam.com"
#define GREETINGS "Greetz: metr0, G36KV and all from the gRn Team"
#define VISIT "Visit  http://kickme.to/grn  and  http://forum.tuts4you.com "


#define PLUGIN_MENU_BASE_ID 0x10