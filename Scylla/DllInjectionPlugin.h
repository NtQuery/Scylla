#include "DllInjection.h"
#include "PluginLoader.h"
#include "Thunks.h"
#include "ApiReader.h"

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
	BYTE status; //return a status, default 0xFF
	DWORD_PTR imageBase; //image base
	DWORD_PTR imageSize; //size of the image
	DWORD_PTR numberOfUnresolvedImports; //number of unresolved imports in this structure
	BYTE offsetUnresolvedImportsArray;
} SCYLLA_EXCHANGE, *PSCYLLA_EXCHANGE;

class DllInjectionPlugin : public DllInjection {

public:
	static const WCHAR * FILE_MAPPING_NAME;
	static HANDLE hProcess;

	ApiReader * apiReader;
	HANDLE hMapFile;
	LPVOID lpViewOfFile;

	DllInjectionPlugin()
	{
		hMapFile = 0;
		lpViewOfFile = 0;
		apiReader = 0;
	}

	~DllInjectionPlugin()
	{
		closeAllHandles();
	}

	void injectPlugin(Plugin & plugin, std::map<DWORD_PTR, ImportModuleThunk> & moduleList, DWORD_PTR imageBase, DWORD_PTR imageSize);

private:
	bool createFileMapping(DWORD mappingSize);
	void closeAllHandles();
	DWORD_PTR getNumberOfUnresolvedImports( std::map<DWORD_PTR, ImportModuleThunk> & moduleList );
	void addUnresolvedImports( PUNRESOLVED_IMPORT firstUnresImp, std::map<DWORD_PTR, ImportModuleThunk> & moduleList );
	void handlePluginResults( PSCYLLA_EXCHANGE scyllaExchange, std::map<DWORD_PTR, ImportModuleThunk> & moduleList );
	void updateImportsWithPluginResult( PUNRESOLVED_IMPORT firstUnresImp, std::map<DWORD_PTR, ImportModuleThunk> & moduleList );
};