// bkdecmd.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

#include "BKParseImage.h"
#include "BKImage.h"


//////////////////////////////////////////////////////////////////////
// Preliminary function declarations

void PrintWelcome();
void PrintUsage();
bool ParseCommandLine(int argc, wchar_t* argv[]);

bool DoDiskList();
bool DoDiskExtractFile();
bool DoDiskAddFile();
bool DoDiskDeleteFile();


//////////////////////////////////////////////////////////////////////
// Globals

#ifdef _MSC_VER
#define OPTIONCHAR '/'
#define OPTIONSTR "/"
#else
#define OPTIONCHAR '-'
#define OPTIONSTR "-"
#endif

const wchar_t* g_sCommand = nullptr;
const wchar_t* g_sImageFileName = nullptr;
const wchar_t* g_sFileName = nullptr;

enum CommandRequirements
{
    CMDR_PARAM_FILENAME = 4,    // Need FileName parameter
    //CMDR_PARAM_PARTITION = 8,    // Need Partition number parameter
    CMDR_IMAGEFILERW = 32,   // Image file should be writable (not read-only)
};

struct CommandInfo
{
    const wchar_t* command;
    bool    (*commandImpl)();   // Function implementing the option
    int     requirements;       // Command requirements, see CommandRequirements enum
}
static g_CommandInfos[] =
{
    { L"l",    DoDiskList,                   0        },
    { L"e",    DoDiskExtractFile,            CMDR_PARAM_FILENAME },
    { L"a",    DoDiskAddFile,                CMDR_PARAM_FILENAME | CMDR_IMAGEFILERW },
    { L"d",    DoDiskDeleteFile,             CMDR_PARAM_FILENAME | CMDR_IMAGEFILERW },
};
static const int g_CommandInfos_count = (int)(sizeof(g_CommandInfos) / sizeof(CommandInfo));

CommandInfo* g_pCommand = nullptr;

CBKParseImage g_ParserImage;
CBKImage g_BKImage;
PARSE_RESULT g_sParseResult;


//////////////////////////////////////////////////////////////////////


void PrintWelcome()
{
    std::wcout << L"Утилита работы с дисками БК BKDEcmd на основе кода BKDE [" << __DATE__ << " " << __TIME__ << "]";
    std::wcout << std::endl;
}

void PrintUsage()
{
    std::wcout << std::endl << L"Использование:" << std::endl
            << L"  Команды для работы с образами дисков:" << std::endl
            << L"    bkdecmd l <ImageFile>  - показать содержимое каталога" << std::endl
            << L"    bkdecmd e <ImageFile> <FileName>  - извлечь файл" << std::endl
            << L"    bkdecmd a <ImageFile> <FileName>  - добавить файл" << std::endl
            << L"    bkdecmd d <ImageFile> <FileName>  - удалить файл" << std::endl;
}


bool ParseCommandLine(int argc, wchar_t* argv[])
{
    for (int argn = 1; argn < argc; argn++)
    {
        const wchar_t* arg = argv[argn];
        if (arg[0] == OPTIONCHAR)
        {
            {
                std::wcout << L"Неизвестная опция: " << arg << std::endl;
                return false;
            }
        }
        else
        {
            if (g_sCommand == nullptr)
                g_sCommand = arg;
            else if (g_sImageFileName == nullptr)
                g_sImageFileName = arg;
            else if (g_sFileName == nullptr)
                g_sFileName = arg;
            else
            {
                std::wcout << L"Неизвестный параметр: " << arg << std::endl;
                return false;
            }
        }
    }

    // Parsed options validation
    if (g_sCommand == nullptr)
    {
        std::wcout << L"Не указана команда." << std::endl;
        return false;
    }
    CommandInfo* pcinfo = nullptr;
    for (int i = 0; i < g_CommandInfos_count; i++)
    {
        if (wcscmp(g_sCommand, g_CommandInfos[i].command) == 0)
        {
            pcinfo = g_CommandInfos + i;
            break;
        }
    }
    if (pcinfo == nullptr)
    {
        std::wcout << L"Неизвестная команда: " << g_sCommand << std::endl;
        return false;
    }
    g_pCommand = pcinfo;

    // More pre-checks based on command requirements
    if (g_sImageFileName == nullptr)
    {
        std::wcout << L"Файл образа не указан." << std::endl;
        return false;
    }
    if ((pcinfo->requirements & CMDR_PARAM_FILENAME) != 0 && g_sFileName == nullptr)
    {
        std::wcout << L"Ожидалось имя файла." << std::endl;
        return false;
    }
    //if ((pcinfo->requirements & CMDR_IMAGEFILERW) != 0 && g_diskimage.IsReadOnly())
    //{
    //    std::wcout << L"Cannot perform the operation: disk image file is read-only." << std::endl;
    //    return false;
    //}

    return true;
}

int wmain(int argc, wchar_t* argv[])
{
#ifdef _WIN32
    _setmode(_fileno(stdout), _O_U16TEXT);
#else // #D
    std::locale::global(std::locale(""));
    std::wcout.imbue(std::locale());
#endif

    PrintWelcome();

    if (!ParseCommandLine(argc, argv))
    {
        PrintUsage();
        return 255;
    }

    std::wcout << L"Команда: " << g_sCommand << std::endl;
    std::wcout << L"Образ диска: " << g_sImageFileName << std::endl;

    // Подключение к файлу образа
    g_sParseResult = g_ParserImage.ParseImage(g_sImageFileName, 0);
    if (g_sParseResult.imageOSType == IMAGE_TYPE::ERROR_NOIMAGE)
    {
        std::wcout << L"Какая-то ошибка при чтении файла образа, либо он повреждён, либо недоступен по чтению, из-за блокирования другой программой." << std::endl;
        return 255;
    }
    if (g_sParseResult.imageOSType == IMAGE_TYPE::UNKNOWN)
    {
        std::wcout << L"Неопознанная файловая система образа." << std::endl;
        return 255;
    }

    // теперь, если образ опознался, надо создать объект, соответствующий файловой системе
    g_BKImage.ClearImgVector();

    uint32_t flg = g_BKImage.Open(g_sParseResult);
    if (flg == 0)
    {
        std::wcout << L"Недостаточно памяти!" << std::endl;
        return 255;
    }

    std::wcout << L"Формат: " << g_BKImage.GetImgFormatName() << L"  ";
    std::wcout << L"Размер: " << g_BKImage.GetImgSize() << L"  ";
    //std::wcout << L"Свободно: " << g_BKImage.GetImageFreeSpace() << L"  ";
    std::wcout << L"Режим: " << (g_BKImage.GetImageOpenStatus() ? L"RO" : L"RW") << std::endl;
    std::wcout << std::endl;

    // Main task
    bool result = g_pCommand->commandImpl();

    g_BKImage.Close();

    if (result)
        std::wcout << std::endl << L"Done." << std::endl;
    return 0;
}


//////////////////////////////////////////////////////////////////////


bool DoDiskList()
{
    // Читаем и печатаем список файлов в корневой папке
    CBKImage::ItemPanePos pp(0, 0);
    return g_BKImage.PrintCurrentDir(pp);
}

bool DoDiskExtractFile()
{
    return g_BKImage.FindAndExtractFile(g_sFileName);
}

bool DoDiskAddFile()
{
    // Проверяем, есть ли у нас такой файл/директория
    if (fs::is_directory(g_sFileName))  // это директория
    {
        std::wcout << L"Добавление директорий пока не реализовано." << std::endl;
        return false;
    }
    if (!fs::is_regular_file(g_sFileName))
    {
        std::wcout << L"Добавляемый файл не найден: " << g_sFileName << std::endl;
        return false;
    }

    // Сначала поищем файл/директорию с таким именем
    auto fr = g_BKImage.FindRecordByName(g_sFileName);
    if (fr != nullptr)
    {
        std::wcout << L"Уже существует файл или директория с таким именем: " << g_sFileName << std::endl;
        return false;
    }

    //TODO: Если существующий объект это файл и мы добавляем файл, можно предлагать перезаписать

    ADDOP_RESULT ret;
    ret = g_BKImage.AddFile(g_sFileName);

    if (ret.nError != ADD_ERROR::OK_NOERROR)
    {
        std::wstring serror = g_AddOpErrorStr[(int)ret.nError];
        std::wcout << L"Не удалось добавить файл, ошибка: " << serror << std::endl;
        return false;
    }

    std::wcout << L"Файл добавлен: " << g_sFileName << std::endl;
    return true;
}

bool DoDiskDeleteFile()
{
    return g_BKImage.FindAndDeleteFile(g_sFileName);
}
