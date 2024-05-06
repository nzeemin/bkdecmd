#include "../pch.h"

#include "ImgUtil.h"
#include "../StringUtil.h"

const std::wstring g_strDir = L"DIR";
const std::wstring g_strUp = L"UP";
const std::wstring g_strLink = L"LINK";


const std::wstring g_pstrExts[KNOWN_EXTS] =
{
    L".bin",
    L".dsk",
    L".img",
    L".bkd"
};

const std::wstring g_ImageErrorStr[] =
{
    L"Успешно.",
    L"Недостаточно памяти.",
    L"Невозможно открыть файл образа.",
    L"Невозможно создать файл образа.",
    L"Файл образа не открыт.",
    L"Файл образа защищён от записи.",
    L"Ошибка позиционирования в файле образа.",
    L"Ошибка чтения файла образа.",
    L"Ошибка записи в файл образа.",
    L"Невозможно создать директорию.",
    L"Файл не найден.",
    L"Ошибки в формате файловой системы.",
    L"Файл с таким именем уже существует.",
    L"Директория с таким именем уже существует.",
    L"Директории с таким именем не существует.",
    L"Каталог заполнен.",
    L"Диск заполнен.",
    L"Диск сильно фрагментирован, нужно провести сквизирование.",
    L"Директория не пуста.",
    L"Нарушена структура каталога.",
    L"Это не директория.",
    L"Это не файл.",
    L"Файловая система не поддерживает директории.",
    L"Файл защищён от удаления.",
    L"Обнаружено дублирование номеров директорий.",
    L"Закончились номера для директорий.",
    L"" // на будущее
};


const std::wstring imgUtil::tblStrRec[3] =
{
    L"ь", L"и", L"ей"
};
const std::wstring imgUtil::tblStrBlk[3] =
{
    L"", L"а", L"ов"
};


// это правильная таблица
const wchar_t imgUtil::koi8tbl_RFC1489[128] =
{
    // {200..237}
    0x2500, 0x2502, 0x250C, 0x2510, 0x2514, 0x2518, 0x251C, 0x2524,
    0x252C, 0x2534, 0x253C, 0x2580, 0x2584, 0x2588, 0x258C, 0x2590,
    0x2591, 0x2592, 0x2593, 0x2320, 0x25A0, 0x2219, 0x221A, 0x2248,
    0x2264, 0x2265, 0xA0,   0x2321, 0xB0,   0xB2,   0xB7,   0xF7,
    // {240..277}
    0x2550, 0x2551, 0x2552, 0x451,  0x2553, 0x2554, 0x2555, 0x2556,
    0x2557, 0x2558, 0x2559, 0x255A, 0x255B, 0x255C, 0x255D, 0x255E,
    0x255F, 0x2560, 0x2561, 0x401,  0x2562, 0x2563, 0x2564, 0x2565,
    0x2566, 0x2567, 0x2568, 0x2569, 0x256A, 0x256B, 0x256C, 0xA9,
    // {300..337}
    L'ю', L'а', L'б', L'ц', L'д', L'е', L'ф', L'г',
    L'х', L'и', L'й', L'к', L'л', L'м', L'н', L'о',
    L'п', L'я', L'р', L'с', L'т', L'у', L'ж', L'в',
    L'ь', L'ы', L'з', L'ш', L'э', L'щ', L'ч', L'ъ',
    // {340..377}
    L'Ю', L'А', L'Б', L'Ц', L'Д', L'Е', L'Ф', L'Г',
    L'Х', L'И', L'Й', L'К', L'Л', L'М', L'Н', L'О',
    L'П', L'Я', L'Р', L'С', L'Т', L'У', L'Ж', L'В',
    L'Ь', L'Ы', L'З', L'Ш', L'Э', L'Щ', L'Ч', L'Ъ'
};


// это таблица кои8 БК11М
// таблица соответствия верхней половины аскии кодов с 128 по 255, включая псевдографику.
// выяснилось, что на бк11 таблица тоже нестандартная, но от бк10 тоже отличается.
const wchar_t imgUtil::koi8tbl11M[128] =
{
    // {200..237}
    0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556,
    0x2555, 0x2563, 0x2551, 0x2557, 0x255d, 0x255c, 0x255b, 0x2510,
    0x2514, 0x2534, 0x252c, 0x251c, 0x2500, 0x253c, 0x255e, 0x255f,
    0x255a, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256c, 0x2567,
    // {240..277}
    0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256b,
    0x256a, 0x2518, 0x250c, 0x2588, 0x2584, 0x258c, 0x2590, 0x2580,
    0x25ba, 0x25c4, 0x2195, 0x203c, 0xb6,   0xa7,   0x25ac, 0x21a8,
    0x2191, 0x2193, 0x2192, 0x2190, 0x2fe,  0x2194, 0x25b2, 0x25bc,
    // {300..337}
    L'ю', L'а', L'б', L'ц', L'д', L'е', L'ф', L'г',
    L'х', L'и', L'й', L'к', L'л', L'м', L'н', L'о',
    L'п', L'я', L'р', L'с', L'т', L'у', L'ж', L'в',
    L'ь', L'ы', L'з', L'ш', L'э', L'щ', L'ч', L'ъ',
    // {340..377}
    L'Ю', L'А', L'Б', L'Ц', L'Д', L'Е', L'Ф', L'Г',
    L'Х', L'И', L'Й', L'К', L'Л', L'М', L'Н', L'О',
    L'П', L'Я', L'Р', L'С', L'Т', L'У', L'Ж', L'В',
    L'Ь', L'Ы', L'З', L'Ш', L'Э', L'Щ', L'Ч', L'Ъ'
};

// таблица соответствия верхней половины аскии кодов с 128 по 255, включая псевдографику
const wchar_t imgUtil::koi8tbl10[128] =
{
    // {200..237} этих символов на бк10 нету.
    L' ', L' ', L' ', L' ', L' ', L' ', L' ', L' ',
    L' ', L' ', L' ', L' ', L' ', L' ', L' ', L' ',
    L' ', L' ', L' ', L' ', L' ', L' ', L' ', L' ',
    L' ', L' ', L' ', L' ', L' ', L' ', L' ', L' ',
    // {240..277}
    0xb6,    0x2534,  0x2665,  0x2510,  0x2561,  0x251c,  0x2514,  0x2550,
    0x2564,  0x2660,  0x250c,  0x252c,  0x2568,  0x2193,  0x253c,  0x2551,
    0x2524,  0x2190,  0x256c,  0x2191,  0x2663,  0x2500,  0x256b,  0x2502,
    0x2666,  0x2518,  0x256a,  0x2565,  0x2567,  0x255e,  0x2192,  0x2593,
    // {300..337}
    L'ю', L'а', L'б', L'ц', L'д', L'е', L'ф', L'г',
    L'х', L'и', L'й', L'к', L'л', L'м', L'н', L'о',
    L'п', L'я', L'р', L'с', L'т', L'у', L'ж', L'в',
    L'ь', L'ы', L'з', L'ш', L'э', L'щ', L'ч', L'ъ',
    // {340..377}
    L'Ю', L'А', L'Б', L'Ц', L'Д', L'Е', L'Ф', L'Г',
    L'Х', L'И', L'Й', L'К', L'Л', L'М', L'Н', L'О',
    L'П', L'Я', L'Р', L'С', L'Т', L'У', L'Ж', L'В',
    L'Ь', L'Ы', L'З', L'Ш', L'Э', L'Щ', L'Ч', L'Ъ'
};


// получим номер окончания слова в зависимости от числа num (предполагаем, что нет отрицательных чисел)
uint32_t imgUtil::GetWordEndIdx(uint32_t num)
{
    static const uint32_t nn[10] = { 2, 0, 1, 1, 1, 2, 2, 2, 2, 2 };

    // исключение 10..19, здесь всегда 2
    if ((num >= 10) && (num <= 19))
    {
        return 2;
    }

    // все остальные окончания определяются по одной крайней правой цифре.
    // вот её и получим
    num %= 10;
    return nn[num];
}

/*
преобразование имени файла из кои8 в юникод. с удалением запрещённых символов
*/
std::wstring imgUtil::BKToUNICODE(const uint8_t *pBuff, const size_t size, const wchar_t table[])
{
    std::wstring strRet;

    for (size_t i = 0; i < size; ++i)
    {
        wchar_t tch = BKToWIDEChar(pBuff[i], table);

        if (tch == 0)
        {
            break;
        }

        strRet.push_back(tch);
    }

    return strRet;
}

wchar_t imgUtil::BKToWIDEChar(const uint8_t b, const wchar_t table[])
{
    if (b == 0)
    {
        return 0;
    }

    if (b < 32)
    {
        return L' ';
    }

    if (b < 127)
    {
        return static_cast<wchar_t>(b);
    }

    if (b == 127)
    {
        return wchar_t(0x25a0);
    }

    return table[b - 128];
}


/*
преобразование юникодной строки в бкшный кои8.
вход:
ustr - преобразуемая строка
pBuff - буфер, куда выводится результат
bufSize - размер буфера
bFillBuf - флаг. если строка короче размера буфера, буфер до конца забивается пробелами. (конца строки - 0 нету.)
*/
void imgUtil::UNICODEtoBK(const std::wstring &ustr, uint8_t *pBuff, const size_t bufSize, const bool bFillBuf)
{
    uint8_t b;
    size_t bn = 0;

    for (auto ch : ustr)
    {
        if (ch < 32) // если символ меньше пробела,
        {
            b = ' ';    // то будет пробел
        }
        else if (ch < 127) // если буквы-цифры- знаки препинания
        {
            b = static_cast<uint8_t>(ch & 0xff); // то буквы-цифры- знаки препинания
        }
        else if (ch == 0x25a0) // если такое
        {
            b = 127; // то это, это единственное исключение в нижней половине аски кодов
        }
        else // если всякие другие символы
        {
            // то ищем в таблице нужный нам символ, а его номер - будет кодом кои8
            b = ' '; // если такого символа нету в таблице - будет пробел

            for (uint8_t i = 0; i < 128; ++i)
            {
                if (ch == imgUtil::koi8tbl10[i])
                {
                    b = i + 0200;
                    break;
                }
            }
        }

        pBuff[bn] = b;

        if (++bn >= bufSize)
        {
            break;
        }
    }

    if (bFillBuf)
    {
        while (bn < bufSize)
        {
            pBuff[bn++] = ' ';
        }
    }
}

// меняем страшные символы '>','<',':','|','?','*','\','/' на нестрашную '_'
// и все коды меньше пробела - на '_'
std::wstring imgUtil::SetSafeName(const std::wstring &str)
{
    const wchar_t rch = L'_';

    // если файл имеет имя "." или "..", то его заменим на подчёркивания.
    if (str == L".")
    {
        return { L"_" };
    }

    if (str == L"..")
    {
        return { L"__" };
    }

    // res = strUtil::replaceChars(res, std::wstring(L"\"*><|:?\\/"), rch);
    std::wstring res = strUtil::replaceChars(str, std::wstring(L"*"), 0x2022);
    res = strUtil::replaceChars(res, std::wstring(L">"), 0x203a);
    res = strUtil::replaceChars(res, std::wstring(L"<"), 0x2039);
    res = strUtil::replaceChars(res, std::wstring(L"\""), 0x201D);
    res = strUtil::replaceChars(res, std::wstring(L"|"), 0xA6);
    res = strUtil::replaceChars(res, std::wstring(L":?\\/"), rch);

    // а теперь заменим все символы с кодом меньше 32
    for (auto & wch : res)
    {
        if (wch < 32)
        {
            wch = rch;
        }
    }

    // ещё проверка на зарезервированные имена
    const size_t n = res.find(L'.'); // находим первую точку
    std::wstring name = res; // имя без расширений

    if (n != std::wstring::npos)
    {
        name = res.substr(0, n);
    }

    name = strUtil::strToLower(name);

    // отлавливаем строго трёхсимвольные имена
    if (name == L"aux" || name == L"nul" || name == L"con" || name == L"prn")
    {
        res.insert(3, L"#");    // вставляем после запрещённого имени этот символ, и всё
        // становится разрешённым
    }
    else if (name.length() == 4) // теперь отлавливаем строго 4х символьные имена
    {
        std::wstring str3 = name.substr(0, 3); // первые 3 символа имени
        wchar_t ch4 = name.at(3); // 4й символ

        if ((str3 == L"com" || str3 == L"lpt") && (L'0' <= ch4 && ch4 <= L'9'))
        {
            res.insert(3, L"#"); // вставляем после запрещённого имени этот символ, и всё
            // становится разрешённым
        }
    }

    // всё остальное, даже если содержит часть зарезервированного имени, не является запрещённым
    return res;
}


//std::wstring imgUtil::LoadStringFromResource(__in unsigned int stringID, __in_opt HINSTANCE instance /*= nullptr*/)
//{
//	wchar_t *pBuf = nullptr;
//	int len = ::LoadString(instance, stringID, reinterpret_cast<LPWSTR>(&pBuf), 0);
//
//	if (len)
//	{
//		return std::wstring(pBuf, len);
//	}
//
//	return {};
//}


/*
анализ импортируемого файла, проверка bin это или нет
вход:
a - указатель на структуру данных, которые корректируются в процессе анализа
выход: поля в структуре AnalyseFileStr:
file - открытый файл
strName - имя файла
strExt - расширение файла
nAddr - адрес загрузки
nLen - длина файла

true - данные скорректированы
false - нет, можно игнорировать
*/
bool imgUtil::AnalyseImportFile(AnalyseFileStruct *a)
{
    bool bRet = false;
    uint16_t mBinHeader[2];
    int nOffset = 0;

    // проверим, а не бин формат ли подсунули?
    if (sizeof(mBinHeader) == fread(mBinHeader, 1, sizeof(mBinHeader), a->file))
    {
        a->bIsCRC = false;

        // если формат действительно бин, то nOffset будет != 0,
        // но есть конечно шанс случайного совпадения
        if (mBinHeader[1] == a->nLen - 4)
        {
            nOffset = 4;
        }
        else if (mBinHeader[1] == a->nLen - 6)
        {
            a->bIsCRC = true;
            nOffset = 4;
        }
        else if (mBinHeader[1] == a->nLen - 22)
        {
            a->bIsCRC = true;
            nOffset = 20;
        }

        // если действительно бин
        if (nOffset)
        {
            // да, бин однако
            a->nAddr = mBinHeader[0];
            a->nLen = mBinHeader[1];

            if (nOffset == 20) // прочитаем оригинальное имя
            {
                fread(a->OrigName, 1, 16, a->file);
            }

            // тут можно ещё попробовать убрать расширение .bin
            std::wstring ext = strUtil::strToLower(a->strExt);

            if (ext == g_pstrExts[BIN_EXT_IDX])
            {
                size_t t = a->strName.rfind(L'.'); // ищем, может ещё есть расширения

                if (t != std::wstring::npos) // если расширение есть
                {
                    size_t l = a->strName.length();
                    a->strExt = a->strName.substr(t, l);
                    a->strName = a->strName.substr(0, t);
                }
                else // если расширения нету, то просто убираем бин
                {
                    a->strExt.clear();
                }

                a->strName = strUtil::trim(a->strName);
                a->strExt = strUtil::trim(a->strExt);
            }

            bRet = true;
        }
    }

    fseek(a->file, nOffset, SEEK_SET); // пропустить заголовок
    return bRet;
}

uint16_t imgUtil::CalcCRC(uint8_t *buffer, size_t len)
{
    uint32_t crc = 0;

    for (size_t i = 0; i < len; ++i)
    {
        crc += buffer[i];

        if (crc & 0xFFFF0000)
        {
            // если случился перенос в 17 разряд (т.е. бит С для word)
            crc &= 0x0000FFFF; // его обнулим
            crc++; // но прибавим к сумме
        }
    }

    return static_cast<uint16_t>(crc & 0xffff);
}
