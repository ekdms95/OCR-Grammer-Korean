#pragma once
#include "pch.h"

class Base64 {
private:
	std::string base64_chars =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789+/";

	static inline bool is_base64(BYTE c) {
		return (isalnum(c) || (c == '+') || (c == '/'));
	}

public:
	auto base64_encode(BYTE const* buf, unsigned int bufLen)->std::string;
	auto base64_decode(std::string const& encoded_string)->std::vector<BYTE>;
};

extern Base64* base64;