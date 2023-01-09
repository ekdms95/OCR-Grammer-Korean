#pragma once
#include "pch.h"

class Utils {
public:
	static auto UTF8ToANSI(const char* pszCode) -> char*; // UTF8 문자를 ANSI로 변경하는 함수.
	static auto ANSItoUTF8(const char* pszCode) -> char*; // ANSI 문자를 UTF8로 변경하는 함수.
	static auto ReadBufferData(std::string path, unsigned char** _data, int* datalen) -> void;
};
