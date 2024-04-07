// pch.h: это предварительно скомпилированный заголовочный файл.
// Перечисленные ниже файлы компилируются только один раз, что ускоряет последующие сборки.
// Это также влияет на работу IntelliSense, включая многие функции просмотра и завершения кода.
// Однако изменение любого из приведенных здесь файлов между операциями сборки приведет к повторной компиляции всех(!) этих файлов.
// Не добавляйте сюда файлы, которые планируете часто изменять, так как в этом случае выигрыша в производительности не будет.

#ifndef PCH_H
#define PCH_H

// Добавьте сюда заголовочные файлы для предварительной компиляции

#include <iostream>
#include <iomanip>
#include <string>
#include <cstring>
#include <vector>
#include <filesystem>
#include <string.h>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#else
#include <locale>
#endif

#ifdef _WIN32
#define swprintf _snwprintf
#endif

namespace fs = std::filesystem;

#ifdef _DEBUG
#include <cassert>
#define ASSERT assert
#define TRACE ;
#else
#define ASSERT
#define TRACE
#endif

#endif //PCH_H
