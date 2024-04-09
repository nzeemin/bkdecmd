// pch.h: это предварительно скомпилированный заголовочный файл.
// Перечисленные ниже файлы компилируются только один раз, что ускоряет последующие сборки.
// Это также влияет на работу IntelliSense, включая многие функции просмотра и завершения кода.
// Однако изменение любого из приведенных здесь файлов между операциями сборки приведет к повторной компиляции всех(!) этих файлов.
// Не добавляйте сюда файлы, которые планируете часто изменять, так как в этом случае выигрыша в производительности не будет.

#ifndef PCH_H
#define PCH_H

// Добавьте сюда заголовочные файлы для предварительной компиляции

#define _CRT_SECURE_NO_WARNINGS

#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

#include <iostream>
#include <iomanip>
#include <string>
#include <cstring>
#include <vector>
#include <filesystem>
#include <locale>
#include <codecvt>

#include <string.h>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

#ifdef _WIN32
#define swprintf _snwprintf
#endif

namespace fs = std::filesystem;

#ifdef _DEBUG
#include <cassert>
#define ASSERT assert
#define TRACE ((void)0)
#else
#define ASSERT(condition) ((void)0)
#define TRACE(something) ((void)0)
#endif

#endif //PCH_H
