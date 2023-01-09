#include "Utils.h"

auto Utils::UTF8ToANSI(const char* pszCode) -> char* { // UTF8 ���ڸ� ANSI�� �����ϴ� �Լ�.
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

auto Utils::ANSItoUTF8(const char* pszCode) -> char* { // ANSI ���ڸ� UTF8�� �����ϴ� �Լ�.
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
    in.open(path, std::ios::in | std::ios::binary); // ����� ������ binary�� �н��ϴ�.

    // seekg�� �̿��� ���� ũ�� ���� �մϴ�.
    in.seekg(0, in.end);
    int length = (int)in.tellg();
    in.seekg(0, in.beg);

    unsigned char* buffer = (unsigned char*)malloc(length); // malloc���� �޸� �Ҵ�

    // ����Ÿ�� �н��ϴ�.
    in.read((char*)buffer, length);
    in.close();

    // �����Ϳ� ���� ����ְ� ��
    *_data = buffer;
    *datalen = length;
}