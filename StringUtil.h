#pragma once
#include <string>

namespace strUtil
{
    // удаление заданного символа с обоих концов строки
    std::wstring trim(const std::wstring &str, const wchar_t trim_char = L' ');
    // удаление заданного символа в начале строки
    std::wstring trimLeft(const std::wstring &str, const wchar_t trim_char = L' ');
    // удаление заданного символа в конце строки
    std::wstring trimRight(const std::wstring &str, const wchar_t trim_char = L' ');
    // замена всех символов src в строке на символы dst
    std::wstring replaceChar(const std::wstring &str, const wchar_t src, const wchar_t dst);
    // замена всех символов, входящих в src в строке на символы dst
    std::wstring replaceChars(const std::wstring &str, const std::wstring &src, const wchar_t dst);
    // преорбазование регистра, сделать все буквы большими
    std::wstring strToUpper(const std::wstring &str);
    // преорбазование регистра, сделать все буквы маленькими
    std::wstring strToLower(const std::wstring &str);
    // сокращение строки до заданной длины.
    std::wstring CropStr(const std::wstring &str, const size_t len);
    // сравнение строк без учёта регистра
    bool equalNoCase(const std::wstring &a, const std::wstring &b);
}
