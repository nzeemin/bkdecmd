#include "pch.h"
#include "BKImgFile.h"

#pragma warning(disable:4996)

constexpr auto BLOCK_SIZE = 512;

CBKImgFile::CBKImgFile()
    : m_f(nullptr)
    , m_nCylinders(83), m_nHeads(2), m_nSectors(10)
{
}

CBKImgFile::CBKImgFile(const std::wstring &strName, const bool bWrite)
    : m_f(nullptr)
    , m_nCylinders(83), m_nHeads(2), m_nSectors(10)
{
    Open(strName, bWrite);
}

CBKImgFile::~CBKImgFile()
{
    Close();
}

bool CBKImgFile::Open(const fs::path &pathName, const bool bWrite)
{
    bool bNeedFDRaw = false;
    bool bFloppy = false; // флаг, true - обращение к реальному флопику, false - к образу
    bool bFDRaw = false;

    std::wstring strName = pathName.wstring();

    // сперва проверим, что за имя входного файла
    if (strName.length() >= 4)
    {
        std::wstring st = strName.substr(0, 4);

        //if (st == L"\\\\.\\")   // если начинается с этого
        //{
        //}
    }

    bool bRet = false;

    // открываем образ
    std::wstring strMode = bWrite ? L"r+b" : L"rb";
    m_f = _wfopen(pathName.c_str(), strMode.c_str());

    if (m_f)
    {
        bRet = true;
    }

    if (bRet)
    {
        m_strName = pathName;
    }

    return bRet;
}

void CBKImgFile::Close()
{
    if (m_f)
    {
        fclose(m_f);
        m_f = nullptr;
    }
}

void CBKImgFile::SetGeometry(const uint8_t c, const uint8_t h, const uint8_t s)
{
    if (c != 0xff)
    {
        m_nCylinders = c;
    }

    if (h != 0xff)
    {
        m_nHeads = h;
    }

    if (s != 0xff)
    {
        m_nSectors = s;
    }
}

CBKImgFile::CHS CBKImgFile::ConvertLBA(const uint32_t lba) const
{
    CHS ret;
    // превратить смещение в байтах в позицию в c:h:s;
    // поскольку формат строго фиксирован: c=80 h=2 s=10, а перемещения предполагаются только по границам секторов
    uint32_t t = static_cast<uint32_t>(m_nHeads) * static_cast<uint32_t>(m_nSectors);
    ret.c = lba / t;
    t = lba % t;
    ret.h = static_cast<uint8_t>(t / static_cast<uint32_t>(m_nSectors));
    ret.s = static_cast<uint8_t>(t % static_cast<uint32_t>(m_nSectors)) + 1;
    return ret;
}

uint32_t CBKImgFile::ConvertCHS(const CHS chs) const
{
    return ConvertCHS(chs.c, chs.h, chs.s);
}

uint32_t CBKImgFile::ConvertCHS(const uint8_t c, const uint8_t h, const uint8_t s) const
{
    uint32_t lba = (static_cast<uint32_t>(c) * static_cast<uint32_t>(m_nHeads) + static_cast<uint32_t>(h)) * static_cast<uint32_t>(m_nSectors) + static_cast<uint32_t>(s) - 1;
    return lba;
}

//bool CBKImgFile::ReadCHS(void *buffer, const uint8_t cyl, const uint8_t head, const uint8_t sector, const UINT numSectors) const
//{
//	if (m_f)
//	{
//		UINT lba = ConvertCHS(cyl, head, sector) * BLOCK_SIZE;
//
//		if (0 == fseek(m_f, lba, SEEK_SET))
//		{
//			lba = numSectors * BLOCK_SIZE;
//			return (lba == fread(buffer, 1, lba, m_f));
//		}
//
//		return false;
//	}
//
//	if (m_h != INVALID_HANDLE_VALUE)
//	{
//		FD_READ_WRITE_PARAMS rwp = { FD_OPTION_MFM, head, cyl, head, sector, 2, sector, 0x0a, 0xff };
//		return IOOperation(IOCTL_FDCMD_READ_DATA, &rwp, buffer, numSectors);
//	}
//
//	return false;
//}

//bool CBKImgFile::WriteCHS(void *buffer, const uint8_t cyl, const uint8_t head, const uint8_t sector, const UINT numSectors) const
//{
//	if (m_f)
//	{
//		UINT lba = ConvertCHS(cyl, head, sector) * BLOCK_SIZE;
//
//		if (0 == fseek(m_f, lba, SEEK_SET))
//		{
//			lba = numSectors * BLOCK_SIZE;
//			return (lba == fwrite(buffer, 1, lba, m_f));
//		}
//
//		return false;
//	}
//
//	if (m_h != INVALID_HANDLE_VALUE)
//	{
//		FD_READ_WRITE_PARAMS rwp = { FD_OPTION_MFM, head, cyl, head, sector, 2, sector, 0x0a, 0xff };
//		return IOOperation(IOCTL_FDCMD_WRITE_DATA, &rwp, buffer, numSectors);
//	}
//
//	return false;
//}

bool CBKImgFile::ReadLBA(void *buffer, const uint32_t lba, const uint32_t numSectors) const
{
    if (m_f)
    {
        if (0 == fseek(m_f, lba * BLOCK_SIZE, SEEK_SET))
        {
            uint32_t size = numSectors * BLOCK_SIZE;
            return (size == fread(buffer, 1, size, m_f));
        }

        return false;
    }

    return false;
}

bool CBKImgFile::WriteLBA(void *buffer, const uint32_t lba, const uint32_t numSectors) const
{
    if (m_f)
    {
        if (0 == fseek(m_f, lba * BLOCK_SIZE, SEEK_SET))
        {
            uint32_t size = numSectors * BLOCK_SIZE;
            return (size == fwrite(buffer, 1, size, m_f));
        }

        return false;
    }

    return false;
}

long CBKImgFile::GetFileSize() const
{
    if (m_f)
    {
// #ifdef TARGET_WINXP
//      FILE *f = _tfopen(m_strName.c_str(), _T("rb"));
//
//      if (f)
//      {
//          fseek(f, 0, SEEK_END);
//          long sz = ftell(f);
//          fclose(f);
//          return sz;
//      }
//
//      return -1;
// #else
//      // вот этого нет в Windows XP, оно возвращает -1 вместо размера.
//      // оказывается это древний баг в рантайме, который даже чинили
//      // но он всё равно просочился и на него ужа забили из-за прекращения
//      // поддержки WinXP
//      struct _stat stat_buf;
//      int rc = _wstat(m_strName.c_str(), &stat_buf);
//      return rc == 0 ? stat_buf.st_size : -1;
// #endif
        std::error_code ec;
        long rc = fs::file_size(m_strName, ec);
        return (ec) ? -1 : rc;
    }

    return -1;
}

bool CBKImgFile::SeekTo00() const
{
    if (m_f)
    {
        return (0 == fseek(m_f, 0, SEEK_SET));
    }

    return false;
}

bool CBKImgFile::IsFileOpen() const
{
    if (m_f)
    {
        return true;
    }

    return false;
}
