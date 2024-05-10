#include "../pch.h"
#include "BKFloppyImage_Prototype.h"
#include "../hashes/sha1.hpp"
#include "../StringUtil.h"


CBKFloppyImage_Prototype::CBKFloppyImage_Prototype(const PARSE_RESULT &image)
    : m_pKoi8tbl(imgUtil::koi8tbl10)
    , m_bMakeAdd(false), m_bMakeDel(false), m_bMakeRename(false), m_bChangeAddr(false)
    , m_bFileROMode(false)
    , m_nBlockSize(BLOCK_SIZE)
    , m_nLastErrorNumber(IMAGE_ERROR::OK_NOERRORS)
    , m_sParseImgResult(image)
    , m_nSector {}, m_mBlock {}, m_nSeekOffset(0), m_nCatPos(0)
{
}

CBKFloppyImage_Prototype::~CBKFloppyImage_Prototype()
{
    CloseFloppyImage();
}

bool CBKFloppyImage_Prototype::OpenFloppyImage()
{
    m_nLastErrorNumber = IMAGE_ERROR::OK_NOERRORS;
    CloseFloppyImage();
    m_sDiskCat.init();
    OnReopenFloppyImage(); // тут переопределяются специфичные для заданной ОС параметры в m_sDiskCat

    if (m_pFoppyImgFile.Open(m_sParseImgResult.strName, true))
    {
        m_bFileROMode = false;
        return true;
    }

    if (m_pFoppyImgFile.Open(m_sParseImgResult.strName, false))
    {
        m_bFileROMode = true;
        return true;
    }

    m_nLastErrorNumber = IMAGE_ERROR::IMAGE_CANNOT_OPEN;
    return false;
}

void CBKFloppyImage_Prototype::CloseFloppyImage()
{
    m_pFoppyImgFile.Close();
}

const std::wstring CBKFloppyImage_Prototype::GetImageInfo() const
{
    const wchar_t* strf = L"Свободно в каталоге: %d запис%ls из %d. Свободно: %d блок%ls / %d байт%ls из %d / %d";

    auto freeblocks = static_cast<unsigned int>(m_sDiskCat.nFreeBlocks);
    unsigned int bytes = freeblocks * m_nBlockSize;
    auto totalblocks = static_cast<unsigned int>(m_sDiskCat.nTotalBlocks);
    unsigned int totalbytes = totalblocks * m_nBlockSize;
    auto freerecs = static_cast<unsigned int>(m_sDiskCat.nFreeRecs);
    return imgUtil::string_format(strf,
            freerecs, imgUtil::tblStrRec[imgUtil::GetWordEndIdx(freerecs)],
            static_cast<unsigned int>(m_sDiskCat.nTotalRecs),
            freeblocks, imgUtil::tblStrBlk[imgUtil::GetWordEndIdx(freeblocks)],
            bytes, imgUtil::tblStrBlk[imgUtil::GetWordEndIdx(bytes)],
            totalblocks, totalbytes);
}

const size_t CBKFloppyImage_Prototype::GetImageFreeSpace() const
{
    return static_cast<size_t>(m_sDiskCat.nFreeBlocks) * m_nBlockSize;
}

bool CBKFloppyImage_Prototype::AppendDirNum(uint8_t nNum)
{
    bool bRet = false;

    if (m_sDiskCat.arDirNums[nNum] == 0)
    {
        m_sDiskCat.arDirNums[nNum] = 1;
        bRet = true;
    }

    // иначе - такой номер уже есть
    return bRet;
}

uint8_t CBKFloppyImage_Prototype::AssignNewDirNum()
{
    for (uint8_t i = 1; i < m_sDiskCat.nMaxDirNum; ++i)
    {
        if (m_sDiskCat.arDirNums[i] == 0)
        {
            m_sDiskCat.arDirNums[i] = 1;
            return i;
        }
    }

    return 0;
}

bool CBKFloppyImage_Prototype::SeekToBlock(size_t nBlockNumber)
{
    unsigned long nOffs = m_sParseImgResult.nBaseOffset + static_cast<unsigned long>(nBlockNumber) * m_nBlockSize;
    m_nSeekOffset = nOffs / BLOCK_SIZE;
    return true;
}

bool CBKFloppyImage_Prototype::ReadData(void *ptr, size_t length)
{
    // размер проги выровняем по границе сектора и сделаем размер в блоках
    auto nBlkLen = static_cast<unsigned int>(ByteSizeToBlockSize(length));

    if (m_pFoppyImgFile.ReadLBA(ptr, m_nSeekOffset, nBlkLen))
    {
        m_nSeekOffset += nBlkLen;
        return true;
    }

    m_nLastErrorNumber = IMAGE_ERROR::IMAGE_CANNOT_READ;
    return false;
}

bool CBKFloppyImage_Prototype::SeektoBlkReadData(size_t nBlockNumber, void *ptr, size_t length)
{
    if (SeekToBlock(nBlockNumber))
    {
        return ReadData(ptr, length);
    }

    return false;
}

bool CBKFloppyImage_Prototype::WriteData(void *ptr, size_t length)
{
    // размер проги выровняем по границе сектора и сделаем размер в блоках
    auto nBlkLen = static_cast<unsigned int>(ByteSizeToBlockSize(length));

    if (m_pFoppyImgFile.WriteLBA(ptr, m_nSeekOffset, nBlkLen))
    {
        m_nSeekOffset += nBlkLen;
        return true;
    }

    m_nLastErrorNumber = IMAGE_ERROR::IMAGE_CANNOT_WRITE;
    return false;
}

bool CBKFloppyImage_Prototype::SeektoBlkWriteData(size_t nBlockNumber, void *ptr, size_t length)
{
    if (SeekToBlock(nBlockNumber))
    {
        return WriteData(ptr, length);
    }

    return false;
}

std::wstring CBKFloppyImage_Prototype::CalcImageSHA1()
{
    return m_pFoppyImgFile.CalcImageSHA1();
}

std::wstring CBKFloppyImage_Prototype::CalcFileSHA1(BKDirDataItem *fr)
{
    ASSERT(fr != nullptr);
    if (fr == nullptr || (fr->nAttr & (FR_ATTR::DIR | FR_ATTR::LINK)) != 0)
        return L"";

    std::vector<uint8_t> vec(fr->nSize);
    if (!ReadFile(fr, vec.data()))
    {
        //TODO: Показать ошибку
        return L"";
    }

    SHA1 hash;
    hash.update(vec.data(), fr->nSize);

    return strUtil::stringToWstring(hash.final());
}

// виртуальная функция, для каждой ФС - своя реализация.
bool CBKFloppyImage_Prototype::ReadCurrentDir()
{
    m_nLastErrorNumber = IMAGE_ERROR::OK_NOERRORS;
    m_sDiskCat.vecFC.clear();
    m_sDiskCat.nTotalRecs = m_sDiskCat.nTotalBlocks = m_sDiskCat.nFreeRecs = m_sDiskCat.nFreeBlocks = -1;
    memset(&m_sDiskCat.arDirNums[0], 0, 256);

    if (m_pFoppyImgFile.IsFileOpen())
    {
        return true;
    }

    m_nLastErrorNumber = IMAGE_ERROR::IMAGE_NOT_OPEN;
    return false;
}

bool CBKFloppyImage_Prototype::WriteCurrentDir()
{
    m_nLastErrorNumber = IMAGE_ERROR::OK_NOERRORS;

    if (m_pFoppyImgFile.IsFileOpen())
    {
        return true;
    }

    m_nLastErrorNumber = IMAGE_ERROR::IMAGE_NOT_OPEN;
    return false;
}

bool CBKFloppyImage_Prototype::ChangeDir(BKDirDataItem *pFR)
{
    ConvertAbstractToRealRecord(pFR);
    m_nLastErrorNumber = IMAGE_ERROR::OK_NOERRORS;

    if (pFR->nAttr & FR_ATTR::DIR)
    {
        switch (pFR->nRecType)
        {
        case BKDIR_RECORD_TYPE::UP:
            {
                // выход из директории или логдиска
                m_sDiskCat.nCurrDirNum = pFR->nDirNum; // если тут будет -1, то надо выгрузить образ.

                if (!m_sDiskCat.vecDir.empty())
                {
                    m_sDiskCat.vecDir.pop_back();
                }

                return true;
            }

        case BKDIR_RECORD_TYPE::DIR:
            {
                // заход в директорию
                if (VerifyDir(pFR))
                {
                    m_sDiskCat.vecDir.push_back(m_sDiskCat.nCurrDirNum);
                    m_sDiskCat.nCurrDirNum = pFR->nDirNum;
                    return true;
                }

                m_nLastErrorNumber = IMAGE_ERROR::FS_DIR_NOT_EXIST;
                return false;
            }
        }
    }

    m_nLastErrorNumber = IMAGE_ERROR::FS_IS_NOT_DIR;
    return false;
}

bool CBKFloppyImage_Prototype::VerifyDir(BKDirDataItem *pFR)
{
    m_nLastErrorNumber = IMAGE_ERROR::FS_NOT_SUPPORT_DIRS;
    return false;
}

bool CBKFloppyImage_Prototype::CreateDir(BKDirDataItem *pFR)
{
    m_nLastErrorNumber = IMAGE_ERROR::FS_NOT_SUPPORT_DIRS;
    return false;
}

bool CBKFloppyImage_Prototype::DeleteDir(BKDirDataItem *pFR)
{
    m_nLastErrorNumber = IMAGE_ERROR::FS_NOT_SUPPORT_DIRS;
    return false;
}

bool CBKFloppyImage_Prototype::GetStartFileName(BKDirDataItem *pFR)
{
    m_vecPC.push_back(m_nCatPos); // сохраним предыдущую позицию
    m_nCatPos = 0; // инициализация - именно поэтому надо сперва эту функцию вызывать, а потом другую.
    return GetNextFileName(pFR);
}

bool CBKFloppyImage_Prototype::GetNextFileName(BKDirDataItem *pFR)
{
    pFR->clear();
    auto size = static_cast<unsigned int>(m_sDiskCat.vecFC.size());

    // и выводим всё что найдём. но по одному.
    while (m_nCatPos < size)
    {
        if (m_sDiskCat.vecFC.at(m_nCatPos).nDirBelong == m_sDiskCat.nCurrDirNum)
        {
            *pFR = m_sDiskCat.vecFC.at(m_nCatPos);
            m_nCatPos++;
            return true;
        }

        m_nCatPos++;
    }

    if (!m_vecPC.empty())
    {
        m_nCatPos = m_vecPC.back();
        m_vecPC.pop_back();
    }

    return false;
}
