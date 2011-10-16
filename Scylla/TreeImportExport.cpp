
#include "TreeImportExport.h"
#include "definitions.h"

bool TreeImportExport::exportTreeList(const WCHAR * targetXmlFile, std::map<DWORD_PTR, ImportModuleThunk> & moduleList, const Process * process, const DWORD_PTR addressOEP, const DWORD_PTR addressIAT, const DWORD sizeIAT)
{
	TiXmlDocument doc;

	TiXmlDeclaration * decl = new TiXmlDeclaration( "1.0", "", "" );
	doc.LinkEndChild(decl);

	TiXmlElement * rootElement = new TiXmlElement("target");

	setTargetInformation(rootElement, process,addressOEP,addressIAT,sizeIAT);

	addModuleListToRootElement(rootElement, moduleList);
	
	doc.LinkEndChild(rootElement);

	return saveXmlToFile(doc,targetXmlFile);
}

void TreeImportExport::setTargetInformation(TiXmlElement * rootElement, const Process * process, const DWORD_PTR addressOEP, const DWORD_PTR addressIAT, const DWORD sizeIAT)
{
	size_t stringLength = 0;

	wcstombs_s(&stringLength, xmlStringBuffer, (size_t)_countof(xmlStringBuffer), process->filename, (size_t)_countof(process->filename));


	rootElement->SetAttribute("filename", xmlStringBuffer);

	DwordPtrToString(addressOEP);
	rootElement->SetAttribute("oep_va",xmlStringBuffer);

	DwordPtrToString(addressIAT);
	rootElement->SetAttribute("iat_va",xmlStringBuffer);

	DwordPtrToString(sizeIAT);
	rootElement->SetAttribute("iat_size",xmlStringBuffer);
}

bool TreeImportExport::saveXmlToFile(TiXmlDocument doc, const WCHAR * xmlFilePath)
{
	FILE * pFile = 0;

	if (_wfopen_s(&pFile,xmlFilePath,L"w") == NULL)
	{
		doc.Print(pFile);
		fclose (pFile);
		return true;
	}
	else
	{
		return false;
	}
}

void TreeImportExport::addModuleListToRootElement( TiXmlElement * rootElement, std::map<DWORD_PTR, ImportModuleThunk> & moduleList )
{
	std::map<DWORD_PTR, ImportModuleThunk>::iterator mapIt;
	std::map<DWORD_PTR, ImportThunk>::iterator mapIt2;
	ImportModuleThunk * importModuleThunk = 0;
	ImportThunk * importThunk = 0;

	TiXmlElement * moduleElement;
	TiXmlElement * importElement;

	for ( mapIt = moduleList.begin() ; mapIt != moduleList.end(); mapIt++ )
	{
		importModuleThunk = &((*mapIt).second);

		moduleElement = getModuleXmlElement(importModuleThunk);

		for ( mapIt2 = (*mapIt).second.thunkList.begin() ; mapIt2 != (*mapIt).second.thunkList.end(); mapIt2++ )
		{
			importThunk = &((*mapIt2).second);

			importElement = getImportXmlElement(importThunk);
			moduleElement->LinkEndChild(importElement);
		}

		rootElement->LinkEndChild(moduleElement);
	}
}

TiXmlElement * TreeImportExport::getModuleXmlElement(const ImportModuleThunk * importModuleThunk)
{
	size_t stringLength = 0;
	TiXmlElement * moduleElement = new TiXmlElement("module");

	wcstombs_s(&stringLength, xmlStringBuffer, (size_t)_countof(xmlStringBuffer), importModuleThunk->moduleName, (size_t)_countof(importModuleThunk->moduleName));

	moduleElement->SetAttribute("filename", xmlStringBuffer);

	DwordPtrToString(importModuleThunk->getFirstThunk());
	moduleElement->SetAttribute("first_thunk_rva",xmlStringBuffer);

	return moduleElement;
}

TiXmlElement * TreeImportExport::getImportXmlElement(const ImportThunk * importThunk)
{
	TiXmlElement * importElement = 0;
	
	if (importThunk->valid)
	{
		importElement = new TiXmlElement("import_valid");

		if(importThunk->name[0] != '\0')
		{
			importElement->SetAttribute("name",importThunk->name);
		}

		WordToString(importThunk->ordinal);
		importElement->SetAttribute("ordinal",xmlStringBuffer);

		WordToString(importThunk->hint);
		importElement->SetAttribute("hint",xmlStringBuffer);

		boolToString(importThunk->suspect);
		importElement->SetAttribute("suspect", xmlStringBuffer);
	}
	else
	{
		importElement = new TiXmlElement("import_invalid");
	}

	DwordPtrToString(importThunk->rva);
	importElement->SetAttribute("iat_rva", xmlStringBuffer);

	DwordPtrToString(importThunk->apiAddressVA);
	importElement->SetAttribute("address_va",xmlStringBuffer);

	return importElement;
}

void TreeImportExport::boolToString(const bool boolValue)
{
	if (boolValue)
	{
		strcpy_s(xmlStringBuffer,_countof(xmlStringBuffer),"1");
	}
	else
	{
		strcpy_s(xmlStringBuffer,_countof(xmlStringBuffer),"0");
	}
}

void TreeImportExport::DwordPtrToString(const DWORD_PTR dwValue)
{
	sprintf_s(xmlStringBuffer, _countof(xmlStringBuffer), PRINTF_DWORD_PTR_FULL, dwValue);
}

void TreeImportExport::WordToString(const WORD dwValue)
{
	sprintf_s(xmlStringBuffer, _countof(xmlStringBuffer), "%04X", dwValue);
}