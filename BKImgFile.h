#pragma once


// Класс, инкапсулирующий работу с файлом образа
class CBKImgFile
{
    uint8_t         m_nCylinders;
    uint8_t         m_nHeads;
    uint8_t         m_nSectors;

    FILE           *m_f;        // файловый дескриптор для файла образа
    std::wstring    m_strName;  // имя файла образа

    struct CHS
    {
        uint8_t c, h, s;
        CHS() : c(0), h(0), s(0) {}
    };

    CHS             ConvertLBA(const uint32_t lba) const;   // LBA -> CHS
    uint32_t        ConvertCHS(const CHS chs) const;    // CHS -> LBA
    uint32_t        ConvertCHS(const uint8_t c, const uint8_t h, const uint8_t s) const;    // CHS -> LBA
    // операция ввода вывода, т.к. оказалось, что fdraw не умеет блочный обмен мультитрековый,
    // да и позиционирование нужно делать вручную
    //bool            IOOperation(const uint32_t cmd, FD_READ_WRITE_PARAMS *rwp, void *buffer, const uint32_t numSectors) const;

public:
    CBKImgFile();
    CBKImgFile(const std::wstring &strName, const bool bWrite);
    ~CBKImgFile();
    bool            Open(const fs::path &pathName, const bool bWrite);
    void            Close();

    // установка новых значений CHS
    // если какое-то значение == 255, то заданное значение не меняется
    void            SetGeometry(const uint8_t c, const uint8_t h, const uint8_t s);

    /*
    buffer - куда читать/писать, о размере должен позаботиться пользователь
    cyl - номер дорожки
    head - номер головки
    sector - номер сектора
    numSectors - количество читаемых/писаемых секторов
    */
    //bool            ReadCHS(void *buffer, const uint8_t cyl, const uint8_t head, const uint8_t sector, const uint32_t numSectors) const;
    //bool            WriteCHS(void *buffer, const uint8_t cyl, const uint8_t head, const uint8_t sector, const uint32_t numSectors) const;
    bool            ReadLBA(void *buffer, const uint32_t lba, const uint32_t numSectors) const;
    bool            WriteLBA(void *buffer, const uint32_t lba, const uint32_t numSectors) const;

    long            GetFileSize() const;
    bool            SeekTo00() const;
    bool            IsFileOpen() const;
};
