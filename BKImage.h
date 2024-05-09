#pragma once

#include "imgos/BKFloppyImage_Prototype.h"

/*
Класс, где будут все основные методы для работы с образом.
Открытие образа, закрытие,
добавление файлов/директорий (групповое)
удаление файлов/директорий (групповое и рекурсивное)
создание директорий
извлечение файлов и преобразование форматов
*/

enum class ADD_ERROR : int
{
    OK_NOERROR = 0, // нет ошибок
    IMAGE_NOT_OPEN, // файл образа не открыт
    FILE_TOO_LARGE, // файл слишком большой
    USER_CANCEL,    // операция отменена пользователем
    IMAGE_ERROR,    // ошибку смотри в nImageErrorNumber
    NUMBERS
};

enum class LISTING_FORMAT : int
{
    DEFAULT = 0,    // Формат по умолчанию, похожий на тот что показывается в BKDE GUI
    RAR_LIKE,       // Формат подобный тому что выдаёт архиватор RAR; только файлы, директории не перечисляются
};

extern std::wstring g_AddOpErrorStr[];

// Результат операции добавления объекта
struct ADDOP_RESULT
{
    bool            bFatal;     // флаг необходимости прервать работу.
    ADD_ERROR       nError;     // номер ошибки в результате добавления объекта в образ
    IMAGE_ERROR     nImageErrorNumber; // номер ошибки в результате операций с образом
    BKDirDataItem   afr;        // экземпляр абстрактной записи, которая вызвала ошибку.
    // Она нам нужна будет для последующей обработки ошибок
    ADDOP_RESULT()
        : bFatal(false)
        , nError(ADD_ERROR::OK_NOERROR)
        , nImageErrorNumber(IMAGE_ERROR::OK_NOERRORS)
    {
        afr.clear();
    }
};


class CBKImage
{
    std::unique_ptr<CBKFloppyImage_Prototype> m_pFloppyImage;

    fs::path m_strStorePath;        // путь, куда будем сохранять файлы
    bool m_bCheckUseBinStatus;      // состояние чекбоксов "использовать формат бин"
    bool m_bCheckUseLongBinStatus;  // состояние чекбоксов "использовать формат бин"
    bool m_bCheckLogExtractStatus;  // и "создавать лог извлечения" соответственно, проще их тут хранить, чем запрашивать сложными путями у родителя
    LISTING_FORMAT m_nListingFormat;

    //PaneInfo                m_PaneInfo;  //TODO: Убрать
    std::vector<PaneInfo>   m_vSelItems;  //TODO: Убрать
    std::vector<std::unique_ptr<CBKFloppyImage_Prototype>> m_vpImages; // Стек образов дисков, для захода в логические диски

public:
    CBKImage();
    ~CBKImage();

    uint32_t Open(PARSE_RESULT &pr, const bool bLogDisk = false); // открыть файл по результатам парсинга
    uint32_t ReOpen(); // переинициализация уже открытого образа
    void Close(); // закрыть текущий файл
    std::wstring GetImgFormatName(IMAGE_TYPE nType = IMAGE_TYPE::UNKNOWN);
    void ClearImgVector();
    void PushCurrentImg();
    bool PopCurrentImg();

    struct ItemPanePos
    {
        int nTopItem;
        int nFocusedItem;
        ItemPanePos() : nTopItem(0), nFocusedItem(0) {}
        ItemPanePos(int t, int f) : nTopItem(t), nFocusedItem(f) {}
    };

    CBKImage::ItemPanePos GetTopItemIndex();

    inline bool IsImageOpen() const
    {
        return (m_pFloppyImage != nullptr);
    }
    // Открыт как read-only (true) или как read-write (false)
    inline bool GetImageOpenStatus() const
    {
        return m_pFloppyImage->GetImageOpenStatus();
    }
    inline unsigned long GetBaseOffset() const
    {
        return m_pFloppyImage->GetBaseOffset();
    }
    inline unsigned long GetImgSize() const
    {
        return m_pFloppyImage->GetImgSize();
    }
    inline unsigned long GetImageFreeSpace() const
    {
        return m_pFloppyImage->GetImageFreeSpace();
    }

    inline void SetCheckBinExtractStatus(bool bStatus = false)
    {
        m_bCheckUseBinStatus = bStatus;
    }
    inline void SetCheckUseLongBinStatus(bool bStatus = false)
    {
        m_bCheckUseLongBinStatus = bStatus;
    }
    inline void SetCheckLogExtractStatus(bool bStatus = false)
    {
        m_bCheckLogExtractStatus = bStatus;
    }
    inline void SetListingFormat(LISTING_FORMAT format) { m_nListingFormat = format; }

    // Печать общей информации об образе диска
    bool PrintImageInfo();

    // Выдать на печать текущую директорию;
    // при level == 0 печатает заголовок и концевик таблицы;
    // при recursive вызывается рекурсивно для под-директорий
    bool PrintCurrentDirectory(const int level = 0, const bool recursive = false, std::wstring dirpath = L"");

    // Найти объект (файл/директория) по имени в текущей директории
    BKDirDataItem* FindRecordByName(std::wstring strName);

    // Найти указанный файл в текущей директории
    BKDirDataItem* FindFileRecord(std::wstring strFileName);

    // Найти в текущей директории указанный файл и извлечь его
    bool FindAndExtractFile(std::wstring strFileName);

    // Найти в текущей директории указанный файл и удалить его
    bool FindAndDeleteFile(std::wstring strFileName);

    // Добавление файла в текущую директорию
    ADDOP_RESULT AddFile(const fs::path& findFile);

    //void ItemProcessing(int nItem, BKDirDataItem* fr);
    void RenameRecord(BKDirDataItem *fr);
    void DeleteSelected();

    // добавить в образ файл/директорию
    ADDOP_RESULT AddObject(const fs::path &findFile, bool bExistDir = false);
    // удалить из образа файл/директорию
    ADDOP_RESULT DeleteObject(BKDirDataItem *fr, bool bForce = false);

protected:
    // Печать шапки каталога
    void PrintCatalogTableHead();
    // Печать концевика каталога
    void PrintCatalogTableTail();
    // Печать одной строки для вывода одного элемента каталога: файла/директории
    void PrintItem(BKDirDataItem& fr, const int level, std::wstring dirpath);

    void StepIntoDir(BKDirDataItem* fr);
    bool StepUptoDir(BKDirDataItem* fr);

    bool ExtractObject(BKDirDataItem* fr);
    bool ExtractFile(BKDirDataItem* fr);
    bool DeleteRecursive(BKDirDataItem* fr);
    bool AnalyseExportFile(AnalyseFileStruct* a);

    void SetStorePath(const fs::path& str)
    {
        m_strStorePath = str;
    }

};
