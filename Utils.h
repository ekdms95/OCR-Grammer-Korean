#pragma once
#include "pch.h"

class Utils {
public:
	static auto UTF8ToANSI(const char* pszCode) -> char*; // UTF8 ���ڸ� ANSI�� �����ϴ� �Լ�.
	static auto ANSItoUTF8(const char* pszCode) -> char*; // ANSI ���ڸ� UTF8�� �����ϴ� �Լ�.
	static auto ReadBufferData(std::string path, unsigned char** _data, int* datalen) -> void;
};
