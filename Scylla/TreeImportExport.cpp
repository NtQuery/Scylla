
#include "TreeImportExport.h"
#include "definitions.h"
#include "Logger.h"

#define DEBUG_COMMENTS

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

bool TreeImportExport::importTreeList(const WCHAR * targetXmlFile, std::map<DWORD_PTR, ImportModuleThunk> & moduleList, DWORD_PTR * addressOEP, DWORD_PTR * addressIAT, DWORD * sizeIAT)
{
	TiXmlElement * targetElement;
	TiXmlDocument doc;
	char * buffer = readXmlFile(targetXmlFile);
	int count = 0;

	moduleList.clear();

	if (buffer)
	{
		doc.Parse(buffer);
		if (doc.Error())
		{
			Logger::printfDialog(TEXT("Load Tree :: Error parsing xml %S: %S\r\n"), doc.Value(), doc.ErrorDesc());
			delete [] buffer;
			return false;
		}

		targetElement = doc.FirstChildElement();

		*addressOEP = ConvertStringToDwordPtr(targetElement->Attribute("oep_va"));
		*addressIAT = ConvertStringToDwordPtr(targetElement->Attribute("iat_va"));
		*sizeIAT = (DWORD)ConvertStringToDwordPtr(targetElement->Attribute("iat_size"));

		//test = targetElement->Attribute("filename");

		parseAllElementModules(targetElement, moduleList);

		delete [] buffer;
	}

	return true;
}

void TreeImportExport::setTargetInformation(TiXmlElement * rootElement, const Process * process, const DWORD_PTR addressOEP, const DWORD_PTR addressIAT, const DWORD sizeIAT)
{
	size_t stringLength = 0;

	wcstombs_s(&stringLength, xmlStringBuffer, (size_t)_countof(xmlStringBuffer), process->filename, (size_t)_countof(process->filename));


	rootElement->SetAttribute("filename", xmlStringBuffer);

	ConvertDwordPtrToString(addressOEP);
	rootElement->SetAttribute("oep_va",xmlStringBuffer);

	ConvertDwordPtrToString(addressIAT);
	rootElement->SetAttribute("iat_va",xmlStringBuffer);

	ConvertDwordPtrToString(sizeIAT);
	rootElement->SetAttribute("iat_size",xmlStringBuffer);
}

char * TreeImportExport::readXmlFile(const WCHAR * xmlFilePath)
{
	FILE * pFile = 0;
	long lSize = 0;
	char * buffer = 0;

	if (_wfopen_s(&pFile,xmlFilePath,L"r") == NULL)
	{
		fseek(pFile, 0, SEEK_END);
		lSize = ftell(pFile);
		fseek(pFile, 0, SEEK_SET);

		if (lSize > 2)
		{
			buffer = new char[lSize + sizeof(char)];

			ZeroMemory(buffer, lSize + sizeof(char));

			fread(buffer, sizeof(char), lSize, pFile);

			if (!feof(pFile) || ferror(pFile))
			{
				delete [] buffer;
				buffer = 0;
			}
		}

		fclose (pFile);
		return buffer;
	}
	else
	{
		return 0;
	}
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

	ConvertDwordPtrToString(importModuleThunk->getFirstThunk());
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

		ConvertWordToString(importThunk->ordinal);
		importElement->SetAttribute("ordinal",xmlStringBuffer);

		ConvertWordToString(importThunk->hint);
		importElement->SetAttribute("hint",xmlStringBuffer);

		ConvertBoolToString(importThunk->suspect);
		importElement->SetAttribute("suspect", xmlStringBuffer);
	}
	else
	{
		importElement = new TiXmlElement("import_invalid");
	}

	ConvertDwordPtrToString(importThunk->rva);
	importElement->SetAttribute("iat_rva", xmlStringBuffer);

	ConvertDwordPtrToString(importThunk->apiAddressVA);
	importElement->SetAttribute("address_va",xmlStringBuffer);

	return importElement;
}

void TreeImportExport::ConvertBoolToString(const bool boolValue)
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

bool TreeImportExport::ConvertStringToBool(const char * strValue)
{
	if (strValue)
	{
		if (strValue[0] == '1')
		{
			return true;
		}
	}
	
	return false;
}

void TreeImportExport::ConvertDwordPtrToString(const DWORD_PTR dwValue)
{
	sprintf_s(xmlStringBuffer, _countof(xmlStringBuffer), PRINTF_DWORD_PTR_FULL, dwValue);
}

DWORD_PTR TreeImportExport::ConvertStringToDwordPtr(const char * strValue)
{
	DWORD_PTR result = 0;

	if (strValue)
	{
#ifdef _WIN64
		result = _strtoi64(strValue, NULL, 16);
#else
		result = strtoul(strValue, NULL, 16);
#endif
	}

	return result;
}

void TreeImportExport::ConvertWordToString(const WORD dwValue)
{
	sprintf_s(xmlStringBuffer, _countof(xmlStringBuffer), "%04X", dwValue);
}

WORD TreeImportExport::ConvertStringToWord(const char * strValue)
{
	WORD result = 0;

	if (strValue)
	{
		result = (WORD)strtoul(strValue, NULL, 16);
	}

	return result;
}

void TreeImportExport::parseAllElementModules( TiXmlElement * targetElement, std::map<DWORD_PTR, ImportModuleThunk> & moduleList )
{
	TiXmlElement * moduleElement = 0;
	ImportModuleThunk importModuleThunk;
	size_t convertedChars = 0;
	const char * filename = 0;

	for(moduleElement = targetElement->FirstChildElement(); moduleElement; moduleElement = moduleElement->NextSiblingElement())
	{
		filename = moduleElement->Attribute("filename");

		if (filename)
		{

			mbstowcs_s(&convertedChars, importModuleThunk.moduleName, _countof(importModuleThunk.moduleName), filename, _TRUNCATE);
			importModuleThunk.firstThunk = ConvertStringToDwordPtr(moduleElement->Attribute("first_thunk_rva"));

			importModuleThunk.thunkList.clear();

			parseAllElementImports(moduleElement, &importModuleThunk);

			moduleList.insert(std::pair<DWORD_PTR,ImportModuleThunk>(importModuleThunk.firstThunk, importModuleThunk));

		}
	}
}

void TreeImportExport::parseAllElementImports( TiXmlElement * moduleElement, ImportModuleThunk * importModuleThunk )
{
	TiXmlElement * importElement = 0;
	ImportThunk importThunk;
	const char * temp = 0;

	for(importElement = moduleElement->FirstChildElement(); importElement; importElement = importElement->NextSiblingElement())
	{
		temp = importElement->Value();

		if (!strcmp(temp, "import_valid"))
		{
			temp = importElement->Attribute("name");
			if (temp)
			{
				strcpy_s(importThunk.name, _countof(importThunk.name),temp);
			}
			else
			{
				importThunk.name[0] = 0;
			}

			wcscpy_s(importThunk.moduleName,_countof(importThunk.moduleName), importModuleThunk->moduleName);

			importThunk.suspect = ConvertStringToBool(importElement->Attribute("suspect"));
			importThunk.ordinal = ConvertStringToWord(importElement->Attribute("ordinal"));
			importThunk.hint = ConvertStringToWord(importElement->Attribute("hint"));

			importThunk.valid = true;
		}
		else
		{
			importThunk.valid = false;
			importThunk.suspect = true;
		}

		importThunk.apiAddressVA = ConvertStringToDwordPtr(importElement->Attribute("address_va"));
		importThunk.rva = ConvertStringToDwordPtr(importElement->Attribute("iat_rva"));

		if (importThunk.rva != 0)
		{
			importModuleThunk->thunkList.insert(std::pair<DWORD_PTR,ImportThunk>(importThunk.rva, importThunk));
		}
		
	}
}
