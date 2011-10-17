
#pragma once

#include <Windows.h>
#include "ProcessLister.h"
#include "Thunks.h"
#include "tinyxml.h"



class TreeImportExport
{
public:
	bool exportTreeList(const WCHAR * targetXmlFile, std::map<DWORD_PTR, ImportModuleThunk> & moduleList, const Process * process, const DWORD_PTR addressOEP, const DWORD_PTR addressIAT, const DWORD sizeIAT);
	
	bool importTreeList(const WCHAR * targetXmlFile, std::map<DWORD_PTR, ImportModuleThunk> & moduleList, DWORD_PTR * addressOEP, DWORD_PTR * addressIAT, DWORD * sizeIAT);

private:
	char xmlStringBuffer[100];

	void addModuleListToRootElement( TiXmlElement * rootElement, std::map<DWORD_PTR, ImportModuleThunk> & moduleList );
	TiXmlElement * getModuleXmlElement(const ImportModuleThunk * importModuleThunk);
	TiXmlElement * getImportXmlElement(const ImportThunk * importThunk);

	bool saveXmlToFile(TiXmlDocument doc, const WCHAR * xmlFilePath);
	char * readXmlFile(const WCHAR * xmlFilePath);

	void setTargetInformation(TiXmlElement * rootElement, const Process * process, const DWORD_PTR addressOEP, const DWORD_PTR addressIAT, const DWORD sizeIAT);

	void ConvertBoolToString(const bool boolValue);
	void ConvertWordToString(const WORD dwValue);
	void ConvertDwordPtrToString(const DWORD_PTR dwValue);

	DWORD_PTR ConvertStringToDwordPtr(const char * strValue);
	WORD ConvertStringToWord(const char * strValue);
	bool ConvertStringToBool(const char * strValue);



	void parseAllElementModules( TiXmlElement * targetElement, std::map<DWORD_PTR, ImportModuleThunk> & moduleList );
	void parseAllElementImports( TiXmlElement * moduleElement, ImportModuleThunk * importModuleThunk );
};