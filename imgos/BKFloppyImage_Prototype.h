#pragma once

#include "ImgUtil.h"
#include "../BKParseImage.h"


class CBKFloppyImage_Prototype
{
public:
    const wchar_t  *m_pKoi8tbl;         // используемая в заданной ОС таблица КОИ-8

protected:
    bool            m_bMakeAdd;         // флаг, реализованы ли функции добавления
    bool            m_bMakeDel;         // флаг, реализованы ли функции удаления
    bool            m_bMakeRename;      // флаг, реализованы ли функции переименования
    bool            m_bChangeAddr;      // флаг, разрешено ли менять адрес записей
    bool            m_bFileROMode;      // режим открытия образа, только для чтения == true. или для чтения/записи == false

    uint32_t        m_nBlockSize;       // Размер блока в байтах, обычно совпадает с сектором

    IMAGE_ERROR     m_nLastErrorNumber;
    CBKImgFile      m_pFoppyImgFile;    // файл образа

    PARSE_RESULT    m_sParseImgResult;  // структура с параметрами

    DiskCatalog     m_sDiskCat;         // структура, куда собрано всё, что касается каталога

    uint8_t         m_nSector[BLOCK_SIZE];
    uint8_t         m_mBlock[COPY_BLOCK_SIZE];
    uint32_t        m_nSeekOffset;

    // переменные для GetStartFileName/GetNextFileName
    std::vector<int> m_vecPC;           // список позиций для поиска в каталоге, для того, чтобы рекурсивные алгоритмы обхода каталога работали
    uint32_t        m_nCatPos;          // глобальная позиция для поиска в каталоге

    /* Добавление номера существующей директории к списку с проверкой на дублирование
     * Выход: true - всё в порядке
     *      false - такой номер уже встречался*/
    bool            AppendDirNum(uint8_t nNum);

    /* назначить новый номер директории.
     * 0 - номеров не осталось, все заняты*/
    uint8_t         AssignNewDirNum();

    // Перемещение к заданному блоку образа
    bool            SeekToBlock(size_t nBlockNumber);
    /* чтение данных
     * Внимание!!! эта функция читает данные всегда поблочно!!!
     * Поэтому выделенная память ВСЕГДА должна быть выровнена по границе блока!!!
     * Это из-за того, что читалка совмещена с чтением с реального FDD, а там чтение
     * только посекторно. там нет возможости читать часть сектора.
    */
    bool            ReadData(void *ptr, size_t length);
    // Перемещение к заданному блоку образа и чтение данных
    bool            SeektoBlkReadData(size_t nBlockNumber, void *ptr, size_t length);
    // запись данных
    bool            WriteData(void *ptr, size_t length);
    // Перемещение к заданному блоку образа и запись данных
    bool            SeektoBlkWriteData(size_t nBlockNumber, void *ptr, size_t length);

    // преобразование абстрактной записи в конкретную. Реализация зависит от ОС
    virtual void    ConvertAbstractToRealRecord(BKDirDataItem *pFR, bool bRenameOnly = false) {}
    virtual void    ConvertRealToAbstractRecord(BKDirDataItem *pFR) {}
    virtual void    OnReopenFloppyImage() {}

public:
    CBKFloppyImage_Prototype(const PARSE_RESULT &image);
    virtual ~CBKFloppyImage_Prototype();

    // Открыть образ диска
    bool OpenFloppyImage();
    // Закрыть образ диска
    void CloseFloppyImage();

    inline std::vector<BKDirDataItem> *CurrDirectory()
    {
        return & m_sDiskCat.vecFC;
    }
    inline const int GetCurrDirNum() const // специальный костыль. чтобы узнавать о выходе из лог диска
    {
        return m_sDiskCat.nCurrDirNum;
    }
    inline const fs::path GetCurrImgName() const
    {
        return m_sParseImgResult.strName;
    }
    inline bool GetImageOpenStatus() const
    {
        return m_bFileROMode;
    }
    inline unsigned long GetBaseOffset() const
    {
        return m_sParseImgResult.nBaseOffset;
    }
    inline unsigned long GetImgSize() const
    {
        return m_sParseImgResult.nImageSize;
    }
    inline IMAGE_TYPE GetImgOSType() const
    {
        return m_sParseImgResult.imageOSType;
    }
    inline bool IsImageBootable() const
    {
        return m_sParseImgResult.bImageBootable;
    }
    inline IMAGE_ERROR GetErrorNumber() const
    {
        return m_nLastErrorNumber;
    }

    // размер проги выровняем по границе сектора
    size_t EvenSizeByBlock(size_t length) const
    {
        return length ? (((length - 1) | static_cast<size_t>(BLOCK_SIZE - 1)) + 1) : 0;
    }

    // размер проги в размерах блока
    size_t ByteSizeToBlockSize(size_t length) const
    {
        return EvenSizeByBlock(length) / BLOCK_SIZE;
    }

    // размер проги выровняем по границе блока, специфичного для заданной ОС
    int EvenSizeByBlock_l(int length) const
    {
        return length ? (((length - 1) | (m_nBlockSize - 1)) + 1) : 0;
    }

    // размер проги в размерах блока, специфичного для заданной ОС
    int ByteSizeToBlockSize_l(int length) const
    {
        return EvenSizeByBlock_l(length) / m_nBlockSize;
    }

    std::wstring CalcImageSHA1();

public:  // виртуальные функции

    // Строка с названием поля для данных, специфических для заданной ОС
    virtual std::wstring HasSpecificData() const
    {
        return L"";  // По умолчанию нет специфичных данных
    }
    // Формирование строки специфичных данных для данной записи каталога
    virtual const std::wstring GetSpecificData(BKDirDataItem *fr) const
    {
        return L"";
    }

    virtual const std::wstring GetImageInfo() const;

    virtual const size_t GetImageFreeSpace() const;


    /* прочитать каталог образа */
    virtual bool    ReadCurrentDir();
    /* записать каталог образа */
    virtual bool    WriteCurrentDir();
    /* прочитать файл из образа */
    virtual bool    ReadFile(BKDirDataItem *pFR, uint8_t *pBuffer)
    {
        return false;
    }
    // специально для ксидос вводим перехватчик,
    // который будет модифицировать имя файла
    virtual void    OnExtract(BKDirDataItem *pFR, std::wstring &strName)
    {
    }
    /* записать файл в образ */
    virtual bool    WriteFile(BKDirDataItem *pFR, uint8_t *pBuffer, bool &bNeedSqueeze)
    {
        bNeedSqueeze = false;
        return false;
    }
    /* удалить файл в образе */
    virtual bool    DeleteFile(BKDirDataItem *pFR, bool bForce = false)
    {
        return false;
    }
    /* сменить текущую директорию в образе */
    virtual bool    ChangeDir(BKDirDataItem *pFR);
    /* проверить, существует ли такая директория в образе */
    virtual bool    VerifyDir(BKDirDataItem *pFR);
    /* создать директорию в образе */
    virtual bool    CreateDir(BKDirDataItem *pFR);
    /* удалить директорию в образе */
    virtual bool    DeleteDir(BKDirDataItem *pFR);

    /*поиск и выдача всех найденных записей о файлах в текущей директории
    Вход: pFR указатель на существующий экземпляр записи
    Выход: true - найдено, при этом pFR - структура заполнена данными
    false - не найдено, pFR обнулена
    */
    virtual bool    GetStartFileName(BKDirDataItem *pFR);
    virtual bool    GetNextFileName(BKDirDataItem *pFR);

    virtual bool    RenameRecord(BKDirDataItem *pFR)
    {
        return false;
    }
};
