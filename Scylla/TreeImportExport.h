#pragma once

#include <windows.h>
#include "ProcessLister.h"
#include "Thunks.h"
#include <tinyxml.h>

class TreeImportExport
{
public:

	TreeImportExport(const WCHAR * targetXmlFile);

	bool exportTreeList(const std::map<DWORD_PTR, ImportModuleThunk> & moduleList, const Process * process, DWORD_PTR addressOEP, DWORD_PTR addressIAT, DWORD sizeIAT) ;
	bool importTreeList(std::map<DWORD_PTR, ImportModuleThunk> & moduleList, DWORD_PTR * addressOEP, DWORD_PTR * addressIAT, DWORD * sizeIAT);

private:

	WCHAR xmlPath[MAX_PATH];

	char xmlStringBuffer[MAX_PATH];

	void setTargetInformation(TiXmlElement * rootElement, const Process * process, DWORD_PTR addressOEP, DWORD_PTR addressIAT, DWORD sizeIAT);
	void addModuleListToRootElement(TiXmlElement * rootElement, const std::map<DWORD_PTR, ImportModuleThunk> & moduleList);

	void parseAllElementModules(TiXmlElement * targetElement, std::map<DWORD_PTR, ImportModuleThunk> & moduleList);
	void parseAllElementImports(TiXmlElement * moduleElement, ImportModuleThunk * importModuleThunk);

	TiXmlElement * getModuleXmlElement(const ImportModuleThunk * importModuleThunk);
	TiXmlElement * getImportXmlElement(const ImportThunk * importThunk);

	bool saveXmlToFile(const TiXmlDocument& doc, const WCHAR * xmlFilePath);
	bool readXmlFile(TiXmlDocument& doc, const WCHAR * xmlFilePath);

	void ConvertBoolToString(const bool boolValue);
	void ConvertWordToString(const WORD dwValue);
	void ConvertDwordPtrToString(const DWORD_PTR dwValue);

	DWORD_PTR ConvertStringToDwordPtr(const char * strValue);
	WORD ConvertStringToWord(const char * strValue);
	bool ConvertStringToBool(const char * strValue);
};
