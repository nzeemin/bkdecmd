#pragma once

#include "BKDirDataItem.h"


extern const std::wstring g_strDir;
extern const std::wstring g_strUp;
extern const std::wstring g_strLink;

constexpr auto KNOWN_EXTS = 4;
constexpr auto BIN_EXT_IDX = 0; // индекс расширения .bin в массиве g_pstrExts
constexpr auto DSK_EXT_IDX = 1; // индекс расширения .dsk в массиве g_pstrExts
constexpr auto IMG_EXT_IDX = 2; // индекс расширения .img в массиве g_pstrExts
constexpr auto BKD_EXT_IDX = 3; // индекс расширения .bkd в массиве g_pstrExts
extern const std::wstring g_pstrExts[KNOWN_EXTS]; // массив часто используемых расширений

constexpr auto COPY_BLOCK_SIZE = 65536;

enum class IMAGE_ERROR : int
{
    OK_NOERRORS = 0,        // нет ошибок
    NOT_ENOUGHT_MEMORY,     // Недостаточно памяти.
    IMAGE_CANNOT_OPEN,      // Невозможно открыть файл образа
    FILE_CANNOT_CREATE,     // Невозможно создать файл
    IMAGE_NOT_OPEN,         // Файл образа не открыт
    IMAGE_WRITE_PROTECRD,   // Файл образа защищён от записи.
    IMAGE_CANNOT_SEEK,      // Ошибка позиционирования в файле образа
    IMAGE_CANNOT_READ,      // Ошибка чтения файла образа
    IMAGE_CANNOT_WRITE,     // Ошибка записи в файл образа
    FS_CANNOT_CREATE_DIR,   // Невозможно создать директорию
    FS_FILE_NOT_FOUND,      // Файл(запись о файле в каталоге не найдена)
    FS_FORMAT_ERROR,        // ошибка в формате файловой системы
    FS_FILE_EXIST,          // Файл с таким именем уже существует.
    FS_DIR_EXIST,           // Директория с таким именем уже существует.
    FS_DIR_NOT_EXIST,       // Директории с таким именем не существует.
    FS_CAT_FULL,            // Каталог заполнен.
    FS_DISK_FULL,           // Диск заполнен.
    FS_DISK_NEED_SQEEZE,    // Диск сильно фрагментирован, нужно провести сквизирование.
    FS_DIR_NOT_EMPTY,       // Директория не пуста.
    FS_STRUCT_ERR,          // Нарушена структура каталога.
    FS_IS_NOT_DIR,          // Это не директория. - попытка подсунуть файл функции change dir
    FS_IS_NOT_FILE,         // Это не файл. - попытка подсунуть не файл функции, работающей с файлами
    FS_NOT_SUPPORT_DIRS,    // Файловая система не поддерживает директории.
    FS_FILE_PROTECTED,      // Файл защищён от удаления
    FS_DIR_DUPLICATE,       // встретилось дублирование номеров директорий
    FS_DIRNUM_FULL,         // закончились номера для директорий

    NUMBERS
};

extern const std::wstring g_ImageErrorStr[];    // строковые сообщения об ошибках

struct PaneInfo
{
    int             nParentDir;     // номер родительской директории, -1 - нет родительской директории
    int             nCurDir;        // номер текущей директории
    int             nTopItem;       // номер верхнего элемента в таблице
    int             nCurItem;       // номер текущего элемента, на котором стоял курсор
    std::wstring    strCurrPath;    // текущий виртуальный путь.

    PaneInfo()
    {
        clear();
    }

    void clear()
    {
        nParentDir = -1;
        nCurItem = nTopItem = nCurDir = 0;
        strCurrPath = L"/";
    }
};

struct DiskCatalog
{
    std::vector<BKDirDataItem> vecFC; // текущий каталог
    std::vector<int>vecDir;     // вектор номеров вложенных каталогов
    int         nCurrDirNum;    // номер текущего отображаемого каталога
    bool        bHasDir;        // флаг, имеет ли файловая система директории
    bool        bTrueDir;       // флаг, что файловая система имеет настояшие директории, как в MS-DOS например
    uint8_t     nMaxDirNum;     // максимальный номер директории для заданной ОС
    uint8_t     arDirNums[256]; // список номеров директорий, найденных в каталоге. Нужно, чтобы создавать новые директории.
    // переменные для отображения информации о ФС диска
    int         nTotalRecs;     // всего записей в каталоге
    int         nTotalBlocks;   // общее количество блоков/кластеров (зависит от ФС)
    int         nFreeRecs;      // количество свободных записей в каталоге
    int         nFreeBlocks;    // количество свободных блоков/кластеров (зависит от ФС) на основе чего высчитывается кол-во свободных байтов
    int         nDataBegin;     // блок, с которого начинаются данные на диске (зависит от ФС)

    DiskCatalog() :
        nCurrDirNum(0), bHasDir(false), bTrueDir(false), nMaxDirNum(255),
        arDirNums {}, nTotalRecs(0), nTotalBlocks(0), nFreeRecs(0), nFreeBlocks(0), nDataBegin(0)
    {
        memset(&arDirNums[0], 0, 256);
    }

    void init()
    {
        vecFC.clear();
        vecDir.clear();
        nCurrDirNum = 0;
        bHasDir = false;
        bTrueDir = false;
        nMaxDirNum = 255;
        memset(&arDirNums[0], 0, 256);
        nTotalRecs = 0;
        nTotalBlocks = 0;
        nFreeRecs = 0;
        nFreeBlocks = 0;
        nDataBegin = 0;
    }
};

// Структура для передачи параметров анализатору файлов
struct AnalyseFileStruct
{
    FILE           *file;       // обрабатываемый файл
    std::wstring    strName;    // имя файла
    std::wstring    strExt;     // расширение файла
    int             nAddr;      // адрес загрузки
    int             nLen;       // размер
    uint8_t         OrigName[16]; // оригинальное БКшное имя из бин файла
    bool            bIsCRC;     // флаг наличия КС в бин файле
    uint16_t        nCRC;       // КС файла
    AnalyseFileStruct() : file(nullptr), nAddr(0), nLen(0), OrigName { 0 }, bIsCRC(false), nCRC(0) {};
};

namespace imgUtil
{
    extern const std::wstring tblStrRec[3]; // окончания для записей
    extern const std::wstring tblStrBlk[3]; // окончания для блоков

    // получим номер окончания слова в зависимости от числа num
    uint32_t GetWordEndIdx(uint32_t num);

    // это правильная таблица
    extern const wchar_t koi8tbl_RFC1489[128];

    // это таблица кои8 БК11М
    // таблица соответствия верхней половины аскии кодов с 128 по 255, включая псевдографику.
    // выяснилось, что на бк11 таблица тоже нестандартная, но от бк10 тоже отличается.
    extern const wchar_t koi8tbl11M[128];

    // таблица соответствия верхней половины аскии кодов с 128 по 255, включая псевдографику
    extern const wchar_t koi8tbl10[128];

    std::wstring BKToUNICODE(const uint8_t *pBuff, const size_t size, const wchar_t table[]);
    wchar_t BKToWIDEChar(const uint8_t b, const wchar_t table[]);

    void UNICODEtoBK(const std::wstring &ustr, uint8_t *pBuff, const size_t bufSize, const bool bFillBuf = false);

    std::wstring SetSafeName(const std::wstring &str);


    template<typename ... Args>
    std::wstring string_format(const std::wstring &format, Args ... args)
    {
        size_t size = _snwprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'

        if (size <= 0)
        {
            throw std::runtime_error("Error during formatting.");
        }

        auto buf = std::vector<wchar_t>(size);
        _snwprintf(buf.data(), size, format.c_str(), args ...);
        return std::wstring(buf.data(), buf.data() + size - 1); // We don't want the '\0' inside
    }

    // получение строки из ресурсов. платформо- и системозависимая функция
    //std::wstring LoadStringFromResource(__in unsigned int stringID, __in_opt HINSTANCE instance = nullptr);

    bool AnalyseImportFile(AnalyseFileStruct *a);
    uint16_t CalcCRC(uint8_t *buffer, size_t len);
}
