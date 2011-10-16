
#pragma once

#include <Windows.h>
#include "ProcessLister.h"
#include "Thunks.h"
#include "tinyxml.h"

class TreeImportExport
{
public:
	bool exportTreeList(const WCHAR * targetXmlFile, std::map<DWORD_PTR, ImportModuleThunk> & moduleList, const Process * process, const DWORD_PTR addressOEP, const DWORD_PTR addressIAT, const DWORD sizeIAT);
	
private:

	char xmlStringBuffer[100];

	void addModuleListToRootElement( TiXmlElement * rootElement, std::map<DWORD_PTR, ImportModuleThunk> & moduleList );
	TiXmlElement * getModuleXmlElement(const ImportModuleThunk * importModuleThunk);
	TiXmlElement * getImportXmlElement(const ImportThunk * importThunk);
	bool saveXmlToFile(TiXmlDocument doc, const WCHAR * xmlFilePath);
	void setTargetInformation(TiXmlElement * rootElement, const Process * process, const DWORD_PTR addressOEP, const DWORD_PTR addressIAT, const DWORD sizeIAT);

	void boolToString(const bool boolValue);
	void WordToString(const WORD dwValue);
	void DwordPtrToString(const DWORD_PTR dwValue);
};