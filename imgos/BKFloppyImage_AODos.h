﻿#pragma once
#include "BKFloppyImage_Prototype.h"

#pragma pack(push)
#pragma pack(1)
struct AodosFileRecord
{
    uint8_t     status1;   /*статус записи: бит 7 - признак "плохой", бит 1 - признак "спрятанный", бит 0 - признак "защищённый"
                            если это каталог - биты 6..0 - номер каталога, для записи-каталога */
    uint8_t     status2;   /*статус записи: бит 7 - признак "удалённый", биты 6..0 - номер родительского подкаталога */
    uint8_t     name[14];  // имя файла/каталога
    uint16_t    start_block;// начальный блок
    uint16_t    len_blk;   // длина в блоках, если 0, то это признак подкаталога
    uint16_t    address;   // стартовый адрес
    uint16_t    length;    // длина

    AodosFileRecord()
    {
        clear();
    }
    AodosFileRecord &operator = (const AodosFileRecord &src)
    {
        memcpy(this, &src, sizeof(AodosFileRecord));
        return *this;
    }
    AodosFileRecord &operator = (const AodosFileRecord *src)
    {
        memcpy(this, src, sizeof(AodosFileRecord));
        return *this;
    }
    void clear()
    {
        memset(this, 0, sizeof(AodosFileRecord));
    }
};
#pragma pack(pop)
constexpr auto FMT_AODOS_CAT_RECORD_NUMBER = 030;
constexpr auto FMT_AODOS_FIRST_FREE_BLOCK = 032;
constexpr auto FMT_AODOS_DISK_SIZE = 0466;
constexpr auto FMT_AODOS_FIRST_FILE_BLOCK = 0470;
constexpr auto FMT_AODOS_CAT_BEGIN = 0500;

class CBKFloppyImage_AODos :
    public CBKFloppyImage_Prototype
{
    std::vector<uint8_t> m_vCatBuffer;   // буфер каталога
    AodosFileRecord *m_pDiskCat;         // каталог диска в буфере каталога
    unsigned int     m_nCatSize;         // размер буфера каталога, выровнено по секторам, включая служебную область
    unsigned int     m_nMKCatSize;       // размер каталога в записях
    unsigned int     m_nMKLastCatRecord; // индекс последней записи в каталоге, чтобы определять конец каталога

    /* поиск заданной записи в каталоге.
    делается поиск на полное бинарное соответствие
    выход: -1 если не найдено,
    номер записи в каталоге - если найдено.
    */
    int     FindRecord(AodosFileRecord *pRec);
    /* поиск заданной записи в каталоге.
    если bFull==false делается поиск по имени
    если bFull==true делается поиск по имени и другим параметрам
    выход: -1 если не найдено,
    номер записи в каталоге - если найдено.
    */
    int     FindRecord2(AodosFileRecord *pRec, bool bFull = true);

    bool Squeeze();
    // проверка на конец каталога
    bool IsEndOfCat(AodosFileRecord *pRec);
//#ifdef _DEBUG
//		void DebugOutCatalog(AodosFileRecord *pRec);
//#endif
protected:
    virtual void ConvertAbstractToRealRecord(BKDirDataItem *pFR, bool bRenameOnly = false) override;
    virtual void ConvertRealToAbstractRecord(BKDirDataItem *pFR) override;
    virtual void OnReopenFloppyImage() override;

public:
    CBKFloppyImage_AODos(const PARSE_RESULT &image);
    virtual ~CBKFloppyImage_AODos() override;

    // виртуальные функции

    /* прочитать каталог образа.
     на выходе: заполненная структура m_sDiskCat */
    virtual bool ReadCurrentDir() override;
    /* записать каталог образа */
    virtual bool WriteCurrentDir() override;
    /* прочитать файл из образа
    * Параметры.
    * pFR - указатель на абстрактную запись о файле, внутри которой сохранена
    *       и реальная запись о файле, где сохранены все необходимые данные,
    *       даже с дублированием.
    * pBuffer - указатель на заранее выделенный массив памяти, куда будем сохранять файл.
    *   что мы будем делать потом с данными буфера, функцию не волнует. */
    virtual bool ReadFile(BKDirDataItem *pFR, uint8_t *pBuffer) override;
    /* записать файл в образ
    * Параметры.
    * pFR - указатель на абстрактную запись о файле, внутри которой сформирована
    *       реальная запись о файле, где по возможности сохранены все необходимые данные для записи,
    *       даже с дублированием.
    * pBuffer - указатель на заранее выделенный массив памяти, где заранее подготовлен массив сохраняемого файла. */
    virtual bool WriteFile(BKDirDataItem *pFR, uint8_t *pBuffer, bool &bNeedSqueeze) override;
    /* удалить файл в образе
    * Параметры.
    * pFR - указатель на абстрактную запись о файле, внутри которой сохранена
    *       и реальная запись о файле, где сохранены все необходимые данные,
    *       даже с дублированием. */
    virtual bool DeleteFile(BKDirDataItem *pFR, bool bForce = false) override;
    /* сменить текущую директорию в образе */
    virtual bool ChangeDir(BKDirDataItem *pFR) override;
    /* проверить, существует ли такая директория в образе */
    virtual bool VerifyDir(BKDirDataItem *pFR) override;
    /* создать директорию в образе */
    virtual bool CreateDir(BKDirDataItem *pFR) override;
    /* удалить директорию в образе */
    virtual bool DeleteDir(BKDirDataItem *pFR) override;

    virtual bool GetNextFileName(BKDirDataItem *pFR) override;

    virtual bool RenameRecord(BKDirDataItem *pFR) override;
};


/*
формат каталога аодос 2.02, 2.10

смещения от начала нулевого блока

смещение значение
-------------------------
0       код 0240, признак загрузочного диска
2       команда br xxx
4*      код 032 - признак расширенного формата
5*      байт описатель диска:
        012 - 10 секторов на дорожку, 2 стороны
        0212 - 10 секторов на дорожку, 1 сторона
        011 - 9 секторов на дорожку, 2 стороны
        0211 - 9 секторов на дорожку, 1 сторона
06*     размер сектора в байтах
010*    кол-во дорожек
012*    зарезервировано
014     метка диска (14 символов)
030     кол-во записей в каталоге
032     номер блока, указываемого последней записью (по сути - начало свободной области в конце диска)
044     кол-во и состояние копий каталогов на диске (для версии 2.10)
046     признак заполнения виртуального диска Е: (для версии 2.10)
0400    код 0123456 - признак формата микродос
0466    Размер диска в блоках
0470    Номер блока первого файла - враньё. нету там такой информации
0500    начало области каталога - размер каталога строго фиксирован
012000  начало области копии каталога данных
024000  начало области данных

 * - смещения поддерживаются расширенным форматом, могут
использоваться в будущих версиях системы

Область данных содержит 030 байтные записи. структура записи

смещение    значение
------------------------------------
0..1        слово признаков:
            бит 15. - признак "удалённый"
            бит 14..8 - номер родительского подкаталога
            бит 7 - признак "сбойный блок"
            бит 1 - признак "спрятанный"
            бит 0 - признак "защищённый"
            бит 6..0 - номер подкаталога для записи, содержащей имя подкаталога
2..017      имя файла (коды 0..040, 0200..0240 допустимы, но не рекомендуются)
020..021    номер первого блока, занимаемого файлом
022..023    количество блоков, отведённых файлу, если это поле содержит 0,
            то запись трактуется как имя подкаталога
024..025    адрес загрузки (уменьшается до чётного)
026..027    длина в байтах (должна быть меньше 77777, т.е. остаток по модулю 0100000 чтоль?)

При инициализации 40-дорожечного диска под данные отводится 01414 блоков,
80-дорожечного - 03054 блока. Каталог занимает блоки 0..011 на нулевой дорожке,
страховочная копия, обновляемая при каждом изменении каталога - блоки 012..023
на нулевой дорожке.

в общем тут выяснилось, что у каталога есть явно задаваемый конец каталога.
это последняя запись с такими параметрами
слово признаков == 0177777 (!!! для ранних аодосов так помечается удалённая запись!)
имя файла  == 14 случайных байтов
номер блока == номер первого свободного блока
длина в блоках == размер свободной области
адрес == случайное значение
длина == случайное значение
т.е. последняя запись помечает оставшуюся свободную область как удалённый файл, занимающий
всё свободное пространство

аодос 1.77
подкаталогов не имеет. биты 8..14 зарезервированы и каталог там занимает
блоки 0..6 и может содержать до 0210(136.) записей.

при всём при этом нельзя однозначно детектировать аодос.
признак - с байта 4 по 14 может быть расширенный формат, начало - байт 032
либо там строка " AO-DOS ", но это не гарантия. версия 1.77 точно не определится

Эксперимент показал, что аодосу плевать на размер каталога. туда можно добавлять
сколько угодно файлов, просто новые записи будут затирать старые и каталог начинает
выглядеть глючнейше. Максимальный физический размер каталога 178 записей (пока не заполнится 9й сектор), но
при превышении 153 записи начинаются глюки: в каталог данные заносятся, но вместо них отображаются
директории с пустым именем.

При создании нового системного диска в версии 2.10 строка " AO-DOS " автоматом не появляется.
Итак, существует 3 разных аодоса
аодос 1.77 - формат каталога скорее всего как у микродос
аодос 2.02 - формат каталога свой
аодос 2.10 - формат каталога якобы совместим с нордом, только формат директорий - свой.


*/
