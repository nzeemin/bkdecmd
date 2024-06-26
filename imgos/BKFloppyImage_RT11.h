﻿#pragma once

#include "BKDRT11Header.h"
#include "BKFloppyImage_Prototype.h"
#include <deque>

#pragma pack(push)
#pragma pack(1)
struct RT11FileRecord
{
    uint8_t     nFileClass;     // класс файла
    uint8_t     nStatus;        // атрибуты
    uint16_t    FileName[2];    // имя в коде RADIX-50
    uint16_t    FileExt;        // расширение в коде RADIX-50
    uint16_t    nBlkSize;       // размер файла в блоках
    uint8_t     nChanNum;       // номер канала
    uint8_t     nTaskNum;       // номер задания
    uint16_t    nFileDate;      // дата создания файла

    RT11FileRecord()
    {
        clear();
    }
    RT11FileRecord &operator = (const RT11FileRecord &src)
    {
        memcpy(this, &src, sizeof(RT11FileRecord));
        return *this;
    }
    RT11FileRecord &operator = (const RT11FileRecord *src)
    {
        memcpy(this, src, sizeof(RT11FileRecord));
        return *this;
    }
    void clear()
    {
        memset((void*)this, 0, sizeof(RT11FileRecord));
    }
};

// расширенная запись, с учётом дополнительных слов.
// макс размер всей записи 128 байтов, чтобы полезало в буфер абстрактной памяти
struct RT11FileRecordEx
{
    RT11FileRecord Record;
    uint16_t ExtraWord[(SPECIFIC_DATA_BUFFER_LENGTH - sizeof(RT11FileRecord)) / 2];
    uint16_t BeginBlock;

    RT11FileRecordEx()
    {
        clear();
    }
    RT11FileRecordEx &operator = (const RT11FileRecordEx &src)
    {
        memcpy((void*)this, &src, sizeof(RT11FileRecordEx));
        return *this;
    }
    RT11FileRecordEx &operator = (const RT11FileRecordEx *src)
    {
        memcpy((void*)this, src, sizeof(RT11FileRecordEx));
        return *this;
    }
    void clear()
    {
        memset((void*)this, 0, sizeof(RT11FileRecordEx));
    }
};
#pragma pack(pop)



class CBKFloppyImage_RT11 : public CBKFloppyImage_Prototype
{
    static const wchar_t RADIX50[050];

    uint16_t        m_nBeginBlock;     // блок, с которого начинается каталог.
    uint16_t        m_nTotalSegments;  // общее количество сегментов в каталоге
    uint16_t        m_nExtraBytes;     // количество дополнительных БАЙТОВ в записи каталога
    std::deque<RT11FileRecordEx> m_RT11Catalog; // здесь будем хранить каталог, включая доп.слова

    std::wstring    DecodeRadix50(uint16_t *buf, int len);
    int             EncodeRadix50(uint16_t *buf, int len, std::wstring &strSrc);

    bool            ReadRT11Catalog();
    bool            WriteRT11Catalog();
    bool            Squeeze();

//#ifdef _DEBUG
//		void            DebugOutCatalog();
//#endif

    /* поиск заданной записи в каталоге.
    делается поиск на полное бинарное соответствие
    выход: -1 если не найдено,
    если найдено - индекс в векторе
    */
    int             FindRecord(RT11FileRecord *pRec, bool bFull = false);

protected:
    virtual void    ConvertAbstractToRealRecord(BKDirDataItem *pFR, bool bRenameOnly = false) override;
    virtual void    ConvertRealToAbstractRecord(BKDirDataItem *pFR) override;

public:
    CBKFloppyImage_RT11(const PARSE_RESULT &image);
    virtual ~CBKFloppyImage_RT11() override;

public:  // виртуальные функции

    virtual std::wstring HasSpecificData() const override
    {
        return L"Дата создания";
    }
    virtual const std::wstring GetSpecificData(BKDirDataItem *fr) const override;

    /* прочитать каталог образа */
    virtual bool    ReadCurrentDir() override;
    /* записать каталог образа */
    virtual bool    WriteCurrentDir() override;
    /* прочитать файл из образа
     * Параметры.
     * pFR - указатель на абстрактную запись о файле, внутри которой сохранена
     *       и реальная запись о файле, где сохранены все необходимые данные,
     *       даже с дублированием.
     * pBuffer - указатель на заранее выделенный массив памяти, куда будем сохранять файл.
     *   что мы будем делать потом с данными буфера, функцию не волнует. */
    virtual bool    ReadFile(BKDirDataItem *pFR, uint8_t *pBuffer) override;
    /* записать файл в образ
     * Параметры.
     * pFR - указатель на абстрактную запись о файле, внутри которой сформирована
     *       реальная запись о файле, где по возможности сохранены все необходимые данные для записи,
     *       даже с дублированием.
     * pBuffer - указатель на заранее выделенный массив памяти, где заранее подготовлен массив сохраняемого файла. */
    virtual bool    WriteFile(BKDirDataItem *pFR, uint8_t *pBuffer, bool &bNeedSqueeze) override;
    /* удалить файл в образе
     * Параметры.
     * pFR - указатель на абстрактную запись о файле, внутри которой сохранена
     *       и реальная запись о файле, где сохранены все необходимые данные,
     *       даже с дублированием. */
    virtual bool    DeleteFile(BKDirDataItem *pFR, bool bForce = false) override;
    /* сменить текущую директорию в образе
    нужно для обработки логических дисков
    */
    virtual bool    ChangeDir(BKDirDataItem *pFR) override;
    virtual bool    RenameRecord(BKDirDataItem *pFR) override;
};

/*
0-й блок - ЗАГОЛОВОК ТОМА
    Заголовок тома располагается в нулевом блоке и начинается с кода 000240.
В случае отсутствия этого кода аппаратный загрузчик будет пытаться прочитать
этот том снова. В заголовке тома располагается первичный загрузчик, который
знает где находится основной загрузчик системы и как работать со своим
устройством. Вся его задача сводится к загрузке основного загрузчика системы.
В RT-11 основной загрузчик располагается в блоках со 2-го по 5-й включительно.
Если том не системный и загрузка, как операция, не определена, то в нулевом
блоке вместо загрузчика располагается программа вывода на терминал сообщения:
BOOT NO VOLUME — загрузчика нет на томе. Содержимое блоков 2-го 5-й в таком
случае не определено.
    Нулевой блок системного тома содержит начальный загрузчик системы,
который копируется в него из последнего блока драйвера системного устройства.

1-й блок - ИДЕНТИФИКАТОР ТОМА
    Располагается в первом блоке и содержит: количество сегментов каталога;
номер блока, с которого начинается каталог; версию системы в коде RADIX-50,
которая инициализировала каталог; идентификатор тома и имя владельца,
введённые оператором при инициализации каталога; название фирмы-разработчика
и название операционной системы.

смещение длина  значение
----------------------------------
0       0202    Таблица замены сбойных блоков
0204    046     Область данных инициализации/восстановления
0252    022     BUP information area (чё за фигня и зачем нужно - неясно)
0700    2       Зарезервировано DEC
0702    2       Зарезервировано DEC
0722    2       Pack cluster size (что такое - нигде не поясняется)
0724    2       блок начала каталога (по умолчанию - 6, поэтому иногда тут 0)
0726    2       версия ОС в коде RADIX-50
0730    12.     идентификатор тома
0744    12.     имя владельца
0760    12.     идентификатор системы
0776    2       контрольная сумма

Контрольная сумма рассчитывается так:
        MOV Header__address,R0
        CLR R1
        MOV #255.,R2
10$:    ADD (R0)+,R1
        SOB R2,10$
        MOV R1,@R0


2-й блок - ОСНОВНОЙ ЗАГРУЗЧИК МОНИТОРА
    Со 2-го по 5-й блок идёт загрузчик монитора, служит для первоначальной
или повторной загрузки операционной системы. Он кроме загрузки тестирует
аппаратную и программную части системы и выполняет другие подготовительные
операции, необходимые для работы монитора. Этот загрузчик переписывается из
1-4-го блоков файла монитора.

6-й блок - КАТАЛОГ ТОМА, размер каталога N*2+5, где N - число сегментов в каталоге
которое устанавливается при инициализации диска (допустимые значения 1..31,
по умолчанию - 4), длина сегмента - 2 сектора.

    Состоит из сегментов. размер сегмента 512. слов, т.е. 2 сектора.

Структура каталога

смещ.   длина   значение
----------------------------------
0       2       число сегментов, отведённых под каталог. Допустимый диапазон:
                1-31., число сегментов задаётся по умолчанию (разное для
                различных устройств, в зависимости от объёма тома) или по
                указанию пользователя при инициализации каталога.
2       2       номер следующего сегмента каталога. Это слово является
                связующим между логически смежными сегментами каталога.
                В последнем используемом сегменте это слово содержит нуль.
4       2       счётчик сегментов, имеющих записи. При открытии нового сегмента
                счётчик корректируется только в первом сегменте. В последующих
                сегментах содержимое этого слова не изменяется и равно нулю,
                а RT-11 вообще игнорирует это слово в остальных сегментах.
6       2       число дополнительных БАЙТОВ в записи о файле в каталоге.
                Может задаваться пользователем при инициализации каталога.
010     2       номер блока, с которого начинается самый первый файл,
                описанный в данном сегменте. Т.к. все файлы следуют друг за
                другом, и их размеры известны, то можно вычислить начальные
                блоки остальных файлов.
012     14.     запись о файле
.... и дальше записи о файлах, всего 72. записи в сегменте.
    Однако маркер конца сегмента имеет такой же формат, как и запись о файле.
Кроме того, сегмент должен заканчиваться записью о неиспользуемой области,
хотя бы и с нулевым размером. Итого в одном сегменте можно описать 70 файлов.
    В описании RT-11 v5.6 написано, что максимум - 69 файлов, а зарезервировано
3 записи - конец сегмента, запись о неиспользуемой области и ещё одна зарезервированная
запись, используемая при создании временных файлов.
    В ОСБК сегмент заполняется под завязку, все 72 записи. Так что всё таки расчёт
не ясно как делать.
    В описании RT-11 v5.6 написано, что маркер конца сегмента может быть только одним словом,
даже вообще последним словом в сегменте, за ним нет больше никаких данных.

    А теперь давай разбираться дальше. Допустим, что у нас есть 200 файлов
по два блока, и взяли мы для каталога 3 сегмента. И записывали файлы подряд
без промежутков. А потом понадобилось удалить файл размером 2 блока,
записанный в первом сегменте, и вместо него записать два файла по одному
блоку. А место для записи освободилось только одно. Вот и придумали
разработчики фирмы DEC полностью заполнять только последний сегмент в этой
цепочке. А предыдущие сегменты заполнять только наполовину, т.е. между
каждым файлом можно расположить ещё один файл, например неиспользуемый.
И тогда во всех сегментах, кроме последнего, может быть не более 36 записей
о постоянных файлах.
    Таким образом, при заполнении последнего сегмента открывается следующий,
и половина записей из предыдущего переписывается в него. Значит, для 200
файлов потребуется не три, а пять сегментов. Единственный случай, когда все
сегменты, имеющие записи, заполняются полностью, возникает при "сборке мусора",
т.е. если мы попросим систему собрать все свободные области в одну,
а заполненное файлами пространство сжать в начало тома.


формат записи
смещ.   длина   значение
----------------------------------
0       1       класс файла
                0 - обычный
                020 - индикатор префикса. Означает, что у файла есть как минимум 1
                префиксный блок
1       1       атрибуты
                000400   001 - бит 0 - временный (открытый и не закрытый пока)
                001000   002 - бит 1 - неиспользуемый
                002000   004 - бит 2 - постоянный
                004000   010 - бит 3 - маркер конца сегмента
                010000   020 - бит 4
                020000   040 - бит 5
                040000   100 - бит 6 - только для чтения
                100000   200 - бит 7 - защищённый, от всего, в том числе и от удаления.
2       4       имя файла в коде RADIX-50, 2 слова - 6 символов
6       2       тип файла в коде RADIX-50, 1 слово - три символа (расширение)
010     2       размер файла в блоках
012     1       номер канала - используется только для временных
013     1       номер задания - используется только для временных
014     2       дата создания
                     xx xxxx xxxxx xxxxx
                     +| +--| +---| +---|- год, биты 0-4
                      |    |     -------- день, биты 5-9
                      |    -------------- месяц, биты 10-13
                      ------------------- эра, биты 14-15 для преодоления проблемы 2000
                      Вычисление года: биты (04-00) + 1972 + биты (15-14) * 32

Radix-50

Код трёхсимвольной последовательности рассчитывается по следующей формуле:

A = 1600. * код_левого_символа + 40. * код_среднего_символа + код_правого_символа


код символ  код символ  код символ  код символ  код символ
0   пробел  8   H       16  P       24  X       32  2
1   A       9   I       17  Q       25  Y       33  3
2   B       10  J       18  R       26  Z       34  4
3   C       11  K       19  S       27  $       35  5
4   D       12  L       20  T       28  .       36  6
5   E       13  M       21  U       29  резерв  37  7
6   F       14  N       22  V       30  0       38  8
7   G       15  O       23  W       31  1       39  9

Существуют логические диски, как опознать по атрибутам нет информации,
предположительно можно опознать по стандартному расширению .DSK

*/
