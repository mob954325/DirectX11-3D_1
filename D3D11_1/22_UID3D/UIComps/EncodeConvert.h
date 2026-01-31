#pragma once
#include <string>
#include <Windows.h>
#include <vector>

// UTF-8 wsting 변환함수
static std::wstring Utf8ToWString(const std::string& utf8)
{
	if (utf8.empty()) return L"";

	int len = MultiByteToWideChar(
		CP_UTF8,
		MB_ERR_INVALID_CHARS,
		utf8.data(),
		(int)utf8.size(),
		nullptr,
		0
	);
	if (len <= 0) return L"";

	std::wstring w;
	w.resize(len);

	MultiByteToWideChar(
		CP_UTF8,
		MB_ERR_INVALID_CHARS,
		utf8.data(),
		(int)utf8.size(),
		w.data(),
		len
	);

	return w;
}

// ansi -> wstring 변환 함수
static std::wstring AnsiToWString(const std::string& ansi)
{
	if (ansi.empty()) return L"";

	int len = MultiByteToWideChar(
		CP_ACP,
		0,
		ansi.data(),
		(int)ansi.size(),
		nullptr,
		0
	);
	if (len <= 0) return L"";

	std::wstring w;
	w.resize(len);

	MultiByteToWideChar(
		CP_ACP,
		0,
		ansi.data(),
		(int)ansi.size(),
		w.data(),
		len
	);

	return w;
}