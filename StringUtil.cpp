﻿#include "pch.h"

#include "StringUtil.h"
#include <cctype>    // std::tolower
#include <algorithm> // std::equal

// для работы с std::wstring нет многих стандартных нужных функций.

static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> utfconverter;

std::string strUtil::wstringToString(const std::wstring& str)
{
    return utfconverter.to_bytes(str);
}
std::wstring strUtil::stringToWstring(const std::string& str)
{
    return utfconverter.from_bytes(str.c_str());
}

std::wstring strUtil::trim(const std::wstring &str, const wchar_t trim_char)
{
    std::wstring res = trimLeft(str, trim_char);
    return trimRight(res, trim_char);
}

std::wstring strUtil::trimLeft(const std::wstring &str, const wchar_t trim_char)
{
    std::wstring res = str;

    while (!res.empty() && (res.front() == trim_char))
    {
        res.erase(res.begin());
    }

    return res;
}

std::wstring strUtil::trimRight(const std::wstring &str, const wchar_t trim_char)
{
    std::wstring res = str;

    while (!res.empty() && (res.back() == trim_char))
    {
        res.pop_back();
    }

    return res;
}

std::wstring strUtil::replaceChar(const std::wstring &str, const wchar_t src, const wchar_t dst)
{
    if (!str.empty())
    {
        std::wstring res;

        for (auto wch : str)
        {
            if (wch == src)
            {
                wch = dst;
            }

            res.push_back(wch);
        }

        return res;
    }

    return str;
}

std::wstring strUtil::replaceChars(const std::wstring &str, const std::wstring &src, const wchar_t dst)
{
    if (!str.empty() && !src.empty())
    {
        std::wstring res;

        for (auto wch : str)
        {
            for (auto s : src)
            {
                if (wch == s)
                {
                    wch = dst;
                    break;
                }
            }

            res.push_back(wch);
        }

        return res;
    }

    return str;
}

std::wstring strUtil::strToUpper(const std::wstring &str)
{
    std::wstring res;

    if (!str.empty())
    {
        std::locale loc = std::locale("ru-ru");
        for (auto n : str)
        {
            res.push_back(toupper(n, loc));
        }
    }

    return res;
}

std::wstring strUtil::strToLower(const std::wstring &str)
{
    std::wstring res;

    if (!str.empty())
    {
        std::locale loc = std::locale("ru-ru");
        for (auto n : str)
        {
            res.push_back(tolower(n, loc));
        }
    }

    return res;
}


// сокращение строки до заданной длины.
// Вход: str - строка
// len - желаемая длина
// Выход: сформированная строка
std::wstring strUtil::CropStr(const std::wstring &str, const size_t len)
{
    std::wstring res = trim(str);

    if (res.length() >= len)
    {
        std::wstring crop = res.substr(0, len - 1); // берём первые len-1 символов
        crop += res.back(); // и берём самый последний символ
        return crop;
    }

    return res;
}

bool strUtil::equalNoCase(const std::wstring &a, const std::wstring &b)
{
    return std::equal(a.begin(), a.end(), b.begin(), b.end(), [](wchar_t a, wchar_t b)
    {
        return std::tolower(a) == std::tolower(b);
    });
}
