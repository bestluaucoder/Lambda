#pragma once
#include "IAppSystem.h"

#include <string>

class ILocalize : public IAppSystem
{
public:
	
	virtual bool AddFile(const char* fileName, const char* pPathID = 0, bool bIncludeFallbackSearchPaths = false) = 0;

	
	virtual void RemoveAll() = 0;

	
	virtual wchar_t* Find(const char* tokenName) = 0;

	
	virtual const wchar_t* FindSafe(const char* tokenName) = 0;

	
	
	virtual int ConvertANSIToUnicode(const char* ansi, wchar_t* unicode, int unicodeBufferSizeInBytes) = 0;

	
	
	
	virtual int ConvertUnicodeToANSI(const wchar_t* unicode, char* ansi, int ansiBufferSize) = 0;

	std::string utf16le_to_utf8(const std::wstring& utf16le_str) {
		int utf8_len = WideCharToMultiByte(CP_UTF8, 0, utf16le_str.c_str(), -1, nullptr, 0, nullptr, nullptr);
		std::string utf8_str(utf8_len - 1, '\0');
		WideCharToMultiByte(CP_UTF8, 0, utf16le_str.c_str(), -1, &utf8_str[0], utf8_len, nullptr, nullptr);
		return utf8_str;
	}
};