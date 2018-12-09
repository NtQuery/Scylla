
#include <windows.h>

const char FILE_MAPPING_NAME[] = "ScyllaPluginExchange";

#define SCYLLA_STATUS_SUCCESS 0
#define SCYLLA_STATUS_UNKNOWN_ERROR 1
#define SCYLLA_STATUS_UNSUPPORTED_PROTECTION 2
#define SCYLLA_STATUS_IMPORT_RESOLVING_FAILED 3
#define SCYLLA_STATUS_MAPPING_FAILED 0xFF

/* Important note:
 *
 * If you write a plugin for the x86 (32-Bit) edition: DWORD_PTR address has 32 bit (4 byte)
 * If you write a plugin for the x64 (64-Bit) edition: DWORD_PTR address has 64 bit (8 byte)
 */
typedef struct _UNRESOLVED_IMPORT {       // Scylla Plugin exchange format
	DWORD_PTR ImportTableAddressPointer;  //in VA, address in IAT which points to an invalid api address
	DWORD_PTR InvalidApiAddress;          //in VA, invalid api address that needs to be resolved
} UNRESOLVED_IMPORT, *PUNRESOLVED_IMPORT;

typedef struct _SCYLLA_EXCHANGE {
	BYTE status; //return a status, default 0xFF == SCYLLA_STATUS_MAPPING_FAILED
	DWORD_PTR imageBase; //image base
	DWORD_PTR imageSize; //size of the image
	DWORD_PTR numberOfUnresolvedImports; //number of unresolved imports in this structure
	BYTE offsetUnresolvedImportsArray;
} SCYLLA_EXCHANGE, *PSCYLLA_EXCHANGE;



#define DllExport   __declspec(dllexport)

#ifdef __cplusplus
extern "C" {
#endif

#ifdef UNICODE
	DllExport wchar_t * __cdecl ScyllaPluginNameW();
#else
	DllExport char * __cdecl ScyllaPluginNameA();
#endif

#ifdef __cplusplus
}
#endif