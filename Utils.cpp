#include "Utils.h"

auto Utils::UTF8ToANSI(const char* pszCode) -> char* { // UTF8 문자를 ANSI로 변경하는 함수.
    BSTR    bstrWide;
    char* pszAnsi;
    int     nLength;

    nLength = MultiByteToWideChar(CP_UTF8, 0, pszCode, strlen(pszCode) + 1,
        NULL, NULL);
    bstrWide = SysAllocStringLen(NULL, nLength);

    MultiByteToWideChar(CP_UTF8, 0, pszCode, strlen(pszCode) + 1, bstrWide, nLength);

    nLength = WideCharToMultiByte(CP_ACP, 0, bstrWide, -1, NULL, 0, NULL, NULL);
    pszAnsi = new char[nLength];

    WideCharToMultiByte(CP_ACP, 0, bstrWide, -1, pszAnsi, nLength, NULL, NULL);
    SysFreeString(bstrWide);

    return pszAnsi;
}

auto Utils::ANSItoUTF8(const char* pszCode) -> char* { // ANSI 문자를 UTF8로 변경하는 함수.
    BSTR bstrCode;
    char* pszUTFCode = NULL;
    int  nLength, nLength2;

    nLength = MultiByteToWideChar(CP_ACP, 0, pszCode, lstrlen(pszCode), NULL, NULL);
    bstrCode = SysAllocStringLen(NULL, nLength);
    MultiByteToWideChar(CP_ACP, 0, pszCode, lstrlen(pszCode), bstrCode, nLength);

    nLength2 = WideCharToMultiByte(CP_UTF8, 0, bstrCode, -1, pszUTFCode, 0, NULL, NULL);
    pszUTFCode = (char*)malloc(nLength2 + 1);
    WideCharToMultiByte(CP_UTF8, 0, bstrCode, -1, pszUTFCode, nLength2, NULL, NULL);

    return pszUTFCode;
}

auto Utils::ReadBufferData(std::string path, unsigned char** _data, int* datalen) -> void {
    std::ifstream in;
    in.open(path, std::ios::in | std::ios::binary); // 경로의 파일을 binary로 읽습니다.

    // seekg를 이용한 파일 크기 추출 합니다.
    in.seekg(0, in.end);
    int length = (int)in.tellg();
    in.seekg(0, in.beg);

    unsigned char* buffer = (unsigned char*)malloc(length); // malloc으로 메모리 할당

    // 데이타를 읽습니다.
    in.read((char*)buffer, length);
    in.close();

    // 포인터에 값을 담아주고 끝
    *_data = buffer;
    *datalen = length;
}