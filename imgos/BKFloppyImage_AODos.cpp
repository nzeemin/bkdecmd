﻿#include "../pch.h"
#include "BKFloppyImage_AODos.h"
#include "../StringUtil.h"

// TODO: Нужно что-то придумать и как-то различать между собой несовместимые форматы каталогов
// микродосовский м нордоподобный. Там разные алгоритмы обработки записей каталога, что может привести
// к порче каталога не той системы, если его обрабатывать алгоритмом другой системы

CBKFloppyImage_AODos::CBKFloppyImage_AODos(const PARSE_RESULT &image)
    : CBKFloppyImage_Prototype(image)
    , m_pDiskCat(nullptr)
{
    m_nCatSize = 10 * m_nBlockSize; // размер каталога - одна сторона нулевой дорожки.
    m_vCatBuffer.resize(m_nCatSize); // необязательно выделять массив во время чтения каталога, размер-то его известен
    m_pDiskCat = reinterpret_cast<AodosFileRecord *>(m_vCatBuffer.data() + FMT_AODOS_CAT_BEGIN); // каталог диска

    m_nMKCatSize = (static_cast<size_t>(012000) - FMT_AODOS_CAT_BEGIN) / sizeof(AodosFileRecord);
    m_nMKLastCatRecord = m_nMKCatSize - 1;
    m_bMakeAdd = true;
    m_bMakeDel = true;
    m_bMakeRename = true;
    m_bChangeAddr = true;
}

CBKFloppyImage_AODos::~CBKFloppyImage_AODos()
{
}

int CBKFloppyImage_AODos::FindRecord(AodosFileRecord *pRec)
{
    int nIndex = -1;

    for (unsigned int i = 0; i < m_nMKLastCatRecord; ++i) // цикл по всему каталогу
    {
        if (IsEndOfCat(&m_pDiskCat[i]))
        {
            break;
        }

        if ((m_pDiskCat[i].status2 & 0177) == m_sDiskCat.nCurrDirNum) // ищем только в текущей директории
        {
            if (memcmp(&m_pDiskCat[i], pRec, sizeof(AodosFileRecord)) == 0)
            {
                nIndex = static_cast<int>(i);
                break;
            }
        }
    }

    return nIndex;
}

int CBKFloppyImage_AODos::FindRecord2(AodosFileRecord *pRec, bool bFull)
{
    int nIndex = -1;

    for (unsigned int i = 0; i < m_nMKLastCatRecord; ++i)
    {
        if (IsEndOfCat(&m_pDiskCat[i]))
        {
            break;
        }

        if ((m_pDiskCat[i].status2 & 0200) || (m_pDiskCat[i].status1 & 0200))
        {
            // если удалённый файл или плохой, то игнорируем
            continue;
        }

        if ((m_pDiskCat[i].status2 & 0177) == m_sDiskCat.nCurrDirNum) // проверяем только в текущей директории
        {
            if (memcmp(pRec->name, m_pDiskCat[i].name, 14) == 0)  // проверим имя
            {
                if (bFull)
                {
                    if (pRec->len_blk == 0) // если директория
                    {
                        if ((m_pDiskCat[i].status1 & 0177) == (pRec->status1 & 0177)) // то проверяем номер директории
                        {
                            nIndex = static_cast<int>(i);
                            break;
                        }
                    }
                    else // если файл - то проверяем параметры файла
                    {
                        if ((m_pDiskCat[i].status2 & 0177) == (pRec->status2 & 0177))
                        {
                            nIndex = static_cast<int>(i);
                            break;
                        }
                    }
                }
                else
                {
                    nIndex = static_cast<int>(i);
                    break;
                }
            }
        }
    }

    return nIndex;
}

void CBKFloppyImage_AODos::ConvertAbstractToRealRecord(BKDirDataItem *pFR, bool bRenameOnly)
{
    auto pRec = reinterpret_cast<AodosFileRecord *>(pFR->pSpecificData); // Вот эту запись надо добавить

    // преобразовывать будем только если там ещё не сформирована реальная запись.
    if (pFR->nSpecificDataLength == 0 || bRenameOnly)
    {
        if (!bRenameOnly)
        {
            pFR->nSpecificDataLength = sizeof(AodosFileRecord);
            pRec->clear();
        }

        // надо сформировать мкдосную запись из абстрактной
        std::wstring strMKName = pFR->strName.stem().wstring();
        std::wstring strMKExt = strUtil::CropStr(pFR->strName.extension().wstring(), 4); // включая точку
        size_t nNameLen = (14 - strMKExt.length()); // допустимая длина имени

        if (strMKName.length() > nNameLen) // если имя длиньше
        {
            strMKName = strUtil::CropStr(strMKName, nNameLen); // обрезаем
        }

        strMKName += strMKExt; // прицепляем расширение

        if (pFR->nAttr & FR_ATTR::DIR)
        {
            std::wstring strDir = strUtil::CropStr(pFR->strName.wstring(), 14);
            imgUtil::UNICODEtoBK(strDir, pRec->name, 14, true);

            if (!bRenameOnly)
            {
                pRec->len_blk = 0; // признак каталога
                pRec->status1 = pFR->nDirNum;
                pRec->status2 = m_sDiskCat.nCurrDirNum; // номер текущего открытого подкаталога
            }
        }
        else
        {
            imgUtil::UNICODEtoBK(strMKName, pRec->name, 14, true);
            pRec->address = pFR->nAddress; // возможно, если мы сохраняем бин файл, адрес будет браться оттуда

            if (!bRenameOnly)
            {
                pRec->status1 = 0;

                if (pFR->nAttr & FR_ATTR::HIDDEN)
                {
                    pRec->status1 |= 2;
                }

                if (pFR->nAttr & FR_ATTR::PROTECTED)
                {
                    pRec->status1 |= 1;
                }

                pRec->status2 = pFR->nDirBelong;
                pRec->len_blk = ByteSizeToBlockSize_l(pFR->nSize); // размер в блоках
                pRec->length = pFR->nSize % 0200000; // остаток от длины по модулю 65535
            }
        }
    }
}

// на входе указатель на абстрактную запись.
// в ней заполнена копия реальной записи, по ней формируем абстрактную
void CBKFloppyImage_AODos::ConvertRealToAbstractRecord(BKDirDataItem *pFR)
{
    auto pRec = reinterpret_cast<AodosFileRecord *>(pFR->pSpecificData); // Вот эту запись надо преобразовать

    if (pFR->nSpecificDataLength) // преобразовываем, только если есть реальные данные
    {
        if (pRec->status2 & 0200)
        {
            pFR->nAttr |= FR_ATTR::DELETED;
        }
        else if (pRec->status1 & 0200)
        {
            pFR->nAttr |= FR_ATTR::BAD;

            if (pRec->name[0] == 0)
            {
                pRec->name[0] = 'B';
                pRec->name[1] = 'A';
                pRec->name[2] = 'D';
            }
        }

        if (pRec->len_blk == 0)
        {
            // если директория
            pFR->nAttr |= FR_ATTR::DIR;
            pFR->nRecType = BKDIR_RECORD_TYPE::DIR;
            pFR->strName = wstringToFsPath(strUtil::trim(imgUtil::BKToUNICODE(pRec->name, 14, m_pKoi8tbl)));
            pFR->nDirBelong = pRec->status2 & 0177;
            pFR->nDirNum = pRec->status1 & 0177;
            pFR->nBlkSize = 0;

            // добавим на всякий случай, вдруг такое тоже есть
            if (pRec->address && pRec->address < 0200)
            {
                pFR->nAttr |= FR_ATTR::LINK;
                pFR->nRecType = BKDIR_RECORD_TYPE::LINK;
            }

//          pFR->nAddress = 0; // если копировать address, то директории начинают притворяться линками,
            // а в микродосе их по известным мне данным, не должно быть
        }
        else
        {
            // если файл
            pFR->nRecType = BKDIR_RECORD_TYPE::FILE;
            std::wstring name = strUtil::trim(imgUtil::BKToUNICODE(pRec->name, 14, m_pKoi8tbl));
            std::wstring ext;
            size_t l = name.length();
            size_t t = name.rfind(L'.');

            if (t != std::wstring::npos) // если расширение есть
            {
                ext = name.substr(t, l);
                name = strUtil::trim(name.substr(0, t));
            }

            if (!ext.empty())
            {
                name += ext;
            }

            pFR->strName = wstringToFsPath(name);
            pFR->nDirBelong = pRec->status2 & 0177;
            pFR->nDirNum = 0;
            pFR->nBlkSize = pRec->len_blk;

            // if (pFR->nAttr & FR_ATTR::DELETED)
            // {
            //  pFR->nDirBelong = 0; // а то удалённые файлы вообще не видны. А всё потому, что у удалённых dir_num == 255 тоже.
            // }

            if (pRec->status1 & 2)
            {
                pFR->nAttr |= FR_ATTR::HIDDEN;
            }

            if (pRec->status1 & 1)
            {
                pFR->nAttr |= FR_ATTR::PROTECTED;
            }
        }

        pFR->nAddress = pRec->address;
        /*  правильный расчёт длины файла.
        т.к. в curr_record.length хранится остаток длины по модулю 0200000, нам из длины в блоках надо узнать
        сколько частей по 0200000 т.е. сколько в блоках частей по 128 блоков.(128блоков == 65536.== 0200000)

        hw = (curr_record.len_blk >>7 ) << 16; // это старшее слово двойного слова
        hw = (curr_record.len_blk / 128) * 65536 = curr_record.len_blk * m_nSectorSize
        */
        uint32_t hw = (pRec->len_blk << 9) & 0xffff0000; // это старшее слово двойного слова
        pFR->nSize = hw + pRec->length; // это ст.слово + мл.слово
        pFR->nStartBlock = pRec->start_block;
    }
}


void CBKFloppyImage_AODos::OnReopenFloppyImage()
{
    m_sDiskCat.bHasDir = true;
    m_sDiskCat.nMaxDirNum = 0177;
}

bool CBKFloppyImage_AODos::IsEndOfCat(AodosFileRecord *pRec)
{
    if ((pRec->status1 | pRec->status2 | pRec->name[0] | pRec->name[1]) == 0)
    {
        return true;    // конец каталога для аодос 1.77
    }

    return false;
}


bool CBKFloppyImage_AODos::ReadCurrentDir()
{
    if (!CBKFloppyImage_Prototype::ReadCurrentDir())
    {
        return false;
    }

    memset(m_vCatBuffer.data(), 0, m_nCatSize);
    BKDirDataItem AFR; // экземпляр абстрактной записи
    auto pRec = reinterpret_cast<AodosFileRecord *>(AFR.pSpecificData); // а в ней копия оригинальной записи

    if (!SeektoBlkReadData(0, m_vCatBuffer.data(), m_nCatSize)) // читаем каталог
    {
        return false;
    }

    int files_count = 0;
    int files_total = *(reinterpret_cast<uint16_t *>(&m_vCatBuffer[FMT_AODOS_CAT_RECORD_NUMBER])); // читаем общее кол-во файлов.
    m_sDiskCat.nDataBegin = *(reinterpret_cast<uint16_t *>(&m_vCatBuffer[FMT_AODOS_FIRST_FILE_BLOCK])); // блок начала данных

    if (m_sDiskCat.nDataBegin <= 0 || m_sDiskCat.nDataBegin > 40) // если там некорректная информация
    {
        m_sDiskCat.nDataBegin = 024; // берём данные по умолчанию.
    }

    m_sDiskCat.nTotalRecs = m_nMKCatSize; // это у нас объём каталога, из него надо будет вычесть общее количество записей
    int used_size = 0;

    for (unsigned int i = 0; i < m_nMKCatSize; ++i) // цикл по всему каталогу
    {
        m_nMKLastCatRecord = i;

        if (files_count >= files_total) // файлы кончились, выходим
        {
            break;
        }

        if (IsEndOfCat(&m_pDiskCat[i]))
        {
            break;    // конец каталога для аодос 1.77
        }

        // преобразуем запись и поместим в массив
        AFR.clear();
        AFR.nSpecificDataLength = sizeof(AodosFileRecord);
        *pRec = m_pDiskCat[i]; // копируем текущую запись как есть
        ConvertRealToAbstractRecord(&AFR);

        if (m_pDiskCat[i].status2 & 0200)
        {
            // удалённые не считаются,
            // плохие наверное тоже не считаются, но проверить не на чём
        }
        else
        {
            // хренушки! Удалённые в аодос 2.02 тоже считаются, но как распознать версию, не ясно
            files_count++;
        }

        if (!(AFR.nAttr & FR_ATTR::DELETED))
        {
            if (m_pDiskCat[i].len_blk == 0)
            {
                // если каталог
                if (!AppendDirNum(m_pDiskCat[i].status2 & 0177))
                {
                    // встретили дублирование номеров директорий
                    m_nLastErrorNumber = IMAGE_ERROR::FS_DIR_DUPLICATE;
                }
            }
            else
            {
                // если файл
                used_size += m_pDiskCat[i].len_blk;
            }
        }

        // выбираем только те записи, которые к нужной директории принадлежат.
        if (AFR.nDirBelong == m_sDiskCat.nCurrDirNum)
        {
            m_sDiskCat.vecFC.push_back(AFR);
        }
    }

    m_sDiskCat.nFreeRecs = m_sDiskCat.nTotalRecs - files_count;
    int imgSize = *(reinterpret_cast<uint16_t *>(&m_vCatBuffer[FMT_AODOS_DISK_SIZE]));

    if ((imgSize == 0) || (imgSize > 32767)) // т.е. если там число положительное, даже если оно там случайно, то оно будет считаться размером диска в блоках. и ниипет.
    {
        /*тут такая проблема.
        800кб образ может быть меньше 800 кб, т.к. программа, делатель образов,
        могла из-за оптимизации не читать пустые дорожки в конце диска.
        А с другой стороны, в МКДОС, внутри логического диска может быть любая ОС, в том числе и АОДОС,
        и размер диска может быть меньше 800кб.
        */
        imgSize = std::max(1600, (int)(m_sParseImgResult.nImageSize / 512));
    }

//#ifdef _DEBUG
//	DebugOutCatalog(m_pDiskCat);
//#endif
    m_sDiskCat.nTotalBlocks = imgSize - m_sDiskCat.nDataBegin;
    m_sDiskCat.nFreeBlocks = m_sDiskCat.nTotalBlocks - used_size;
    return true;
}

bool CBKFloppyImage_AODos::WriteCurrentDir()
{
    if (!CBKFloppyImage_Prototype::WriteCurrentDir())
    {
        return false;
    }

//#ifdef _DEBUG
//	DebugOutCatalog(m_pDiskCat);
//#endif

    if (!SeektoBlkWriteData(0, m_vCatBuffer.data(), m_nCatSize)) // сохраняем каталог как есть
    {
        return false;
    }

    if (!WriteData(m_vCatBuffer.data(), m_nCatSize)) // и вторую копию.
    {
        return false;
    }

    return true;
}

bool CBKFloppyImage_AODos::ChangeDir(BKDirDataItem *pFR)
{
    if (CBKFloppyImage_Prototype::ChangeDir(pFR))
    {
        return true;
    }

    if ((pFR->nAttr & FR_ATTR::LOGDISK) && (pFR->nRecType == BKDIR_RECORD_TYPE::LOGDSK))
    {
        m_nLastErrorNumber = IMAGE_ERROR::OK_NOERRORS;
        return true;
    }

    m_nLastErrorNumber = IMAGE_ERROR::FS_IS_NOT_DIR;
    return false;
}

bool CBKFloppyImage_AODos::VerifyDir(BKDirDataItem *pFR)
{
    if (pFR->nAttr & FR_ATTR::DIR)
    {
        ConvertAbstractToRealRecord(pFR);
        auto pRec = reinterpret_cast<AodosFileRecord *>(pFR->pSpecificData); // Вот эту запись надо добавить
        // проверим, вдруг такая директория уже есть
        int nIndex = FindRecord2(pRec, false); // мы тут не знаем номер директории. мы можем только по имени проверить.

        if (nIndex >= 0)
        {
            return true;
        }
    }

    return false;
}

bool CBKFloppyImage_AODos::CreateDir(BKDirDataItem *pFR)
{
    m_nLastErrorNumber = IMAGE_ERROR::OK_NOERRORS;
    bool bRet = false;

    if (m_bFileROMode)
    {
        // Если образ открылся только для чтения,
        m_nLastErrorNumber = IMAGE_ERROR::IMAGE_WRITE_PROTECRD;
        return bRet; // то записать в него мы ничего не сможем.
    }

    if (m_sDiskCat.nFreeRecs <= 0)
    {
        m_nLastErrorNumber = IMAGE_ERROR::FS_CAT_FULL;
        return false;
    }

    if (pFR->nAttr & FR_ATTR::DIR)
    {
        ConvertAbstractToRealRecord(pFR);
        auto pRec = reinterpret_cast<AodosFileRecord *>(pFR->pSpecificData); // Вот эту запись надо добавить
        pFR->nDirBelong = pRec->status2 = m_sDiskCat.nCurrDirNum; // номер текущего открытого подкаталога
        // проверим, вдруг такая директория уже есть
        int nInd = FindRecord2(pRec, false); // мы тут не знаем номер директории. мы можем только по имени проверить.

        if (nInd >= 0)
        {
            m_nLastErrorNumber = IMAGE_ERROR::FS_DIR_EXIST;
            pFR->nDirNum = m_pDiskCat[nInd].status2 & 0177; // и заодно узнаем номер директории
        }
        else
        {
            unsigned int nIndex = 0;
            // найдём свободное место в каталоге.
            bool bHole = false;
            bool bFound = false;

            for (unsigned int i = 0; i < m_nMKLastCatRecord; ++i)
            {
                if (IsEndOfCat(&m_pDiskCat[i]))
                {
                    break;
                }

                if ((m_pDiskCat[i].status2 & 0200) && (m_pDiskCat[i].len_blk == 0))
                {
                    // если нашли дырку в которой было имя директории
                    bHole = true;
                    nIndex = i;
                    bFound = true;
                    break;
                }
            }

            if (!bFound)
            {
                nIndex = m_nMKLastCatRecord;

                if (nIndex < m_nMKCatSize) // если в конце каталога вообще есть место
                {
                    bFound = true; // то нашли, иначе - нет
                }
            }

            if (bFound)
            {
                pFR->nDirNum = pRec->status1 = AssignNewDirNum(); // назначаем номер директории.

                if (pFR->nDirNum == 0)
                {
                    m_nLastErrorNumber = IMAGE_ERROR::FS_DIRNUM_FULL;
                }

                // если ошибок не произошло, сохраним результаты
                if (bHole)
                {
                    // сохраняем нашу запись вместо удалённой директории
                    m_pDiskCat[nIndex] = pRec;
                }
                else
                {
                    // если нашли свободную область в конце каталога
                    int nHoleSize = m_pDiskCat[nIndex].len_blk;
                    int nStartBlock = m_pDiskCat[nIndex].start_block;
                    // сохраняем нашу запись
                    m_pDiskCat[nIndex] = pRec;
                    // сохраняем признак конца каталога
                    AodosFileRecord hole;
                    hole.start_block = nStartBlock + pRec->len_blk;
                    hole.len_blk = nHoleSize - pRec->len_blk;
                    // и запись с инфой о свободной области
                    m_pDiskCat[nIndex + 1] = hole;
                    m_nMKLastCatRecord++;
                }

                // сохраняем каталог
                *(reinterpret_cast<uint16_t *>(&m_vCatBuffer[FMT_AODOS_CAT_RECORD_NUMBER])) += 1; // поправим параметры - количество файлов
                bRet = WriteCurrentDir();
                m_sDiskCat.nFreeRecs--;
            }
            else
            {
                m_nLastErrorNumber = IMAGE_ERROR::FS_CAT_FULL;
            }
        }
    }
    else
    {
        m_nLastErrorNumber = IMAGE_ERROR::FS_IS_NOT_DIR;
    }

    return bRet;
}

bool CBKFloppyImage_AODos::DeleteDir(BKDirDataItem *pFR)
{
    m_nLastErrorNumber = IMAGE_ERROR::OK_NOERRORS;
    bool bRet = false;

    if (m_bFileROMode)
    {
        // Если образ открылся только для чтения,
        m_nLastErrorNumber = IMAGE_ERROR::IMAGE_WRITE_PROTECRD;
        return bRet; // то записать в него мы ничего не сможем.
    }

    if (pFR->nAttr & FR_ATTR::DELETED)
    {
        return true; // уже удалённое не удаляем повторно
    }

    if (pFR->nAttr & FR_ATTR::DIR)
    {
        ConvertAbstractToRealRecord(pFR);
        // узнать есть ли файлы в директории.
        bool bExist = false;

        for (unsigned int i = 0; i < m_nMKLastCatRecord; ++i)
        {
            if (IsEndOfCat(&m_pDiskCat[i]))
            {
                break;
            }

            if (m_pDiskCat[i].status2 & 0200)
            {
                // если удалённый файл - дырка, то игнорируем
                continue;
            }

            if ((m_pDiskCat[i].status2 & 0177) == pFR->nDirNum)
            {
                bExist = true; // нашли файл, принадлежащий этой директории
                break;
            }
        }

        // если есть - не удалять.
        if (bExist)
        {
            m_nLastErrorNumber = IMAGE_ERROR::FS_DIR_NOT_EMPTY;
        }
        else
        {
            // удалить можно пустую директорию.
            auto pRec = reinterpret_cast<AodosFileRecord *>(pFR->pSpecificData); // Вот эту запись надо удалить
            int nIndex = FindRecord(pRec);

            if (nIndex >= 0) // если нашли
            {
                m_pDiskCat[nIndex].status2 |= 0200; // пометим как удалённую
                *(reinterpret_cast<uint16_t *>(&m_vCatBuffer[FMT_AODOS_CAT_RECORD_NUMBER])) -= 1; // поправим параметры - количество файлов
                bRet = WriteCurrentDir(); // сохраним директорию
                m_sDiskCat.nFreeRecs++;
            }
            else
            {
                m_nLastErrorNumber = IMAGE_ERROR::FS_FILE_NOT_FOUND;
            }
        }
    }
    else
    {
        m_nLastErrorNumber = IMAGE_ERROR::FS_IS_NOT_DIR;
    }

    return bRet;
}

bool CBKFloppyImage_AODos::GetNextFileName(BKDirDataItem *pFR)
{
    pFR->clear();
    auto pRec = reinterpret_cast<AodosFileRecord *>(pFR->pSpecificData); // оригинальная запись
//#ifdef _DEBUG
//	DebugOutCatalog(m_pDiskCat);
//#endif

    // и выводим всё что найдём. но по одному.
    while (m_nCatPos < m_nMKLastCatRecord) // цикл по всему каталогу
    {
        if (IsEndOfCat(&m_pDiskCat[m_nCatPos]))
        {
            break;
        }

        if ((m_pDiskCat[m_nCatPos].status2 & 0177) == m_sDiskCat.nCurrDirNum) // ищем только в текущей директории
        {
            *pRec = m_pDiskCat[m_nCatPos];
            pFR->nSpecificDataLength = sizeof(AodosFileRecord);
            ConvertRealToAbstractRecord(pFR);
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

bool CBKFloppyImage_AODos::RenameRecord(BKDirDataItem *pFR)
{
    auto pRec = reinterpret_cast<AodosFileRecord *>(pFR->pSpecificData); // оригинальная запись
    int nIndex = FindRecord(pRec); // сперва найдём её

    if (nIndex >= 0)
    {
        ConvertAbstractToRealRecord(pFR, true); // теперь скорректируем имя реальной записи
        m_pDiskCat[nIndex] = *pRec; // теперь скорректируем в каталоге
        return WriteCurrentDir(); // сохраним каталог
    }

    // что-то не так
    return false;
}


bool CBKFloppyImage_AODos::ReadFile(BKDirDataItem *pFR, uint8_t *pBuffer)
{
    m_nLastErrorNumber = IMAGE_ERROR::OK_NOERRORS;
    bool bRet = true;

    if (pFR->nAttr & (FR_ATTR::DIR | FR_ATTR::LINK))
    {
        m_nLastErrorNumber = IMAGE_ERROR::FS_IS_NOT_FILE;
        return false; // если это не файл - выход с ошибкой
    }

    ConvertAbstractToRealRecord(pFR);
    size_t len = pFR->nSize;
    size_t sector = pFR->nStartBlock;

    if (SeekToBlock(sector)) // перемещаемся к началу файла
    {
        // поскольку файл располагается единым куском, прочитаем его целиком
        if (len > 0)
        {
            if (!ReadData(pBuffer, len))
            {
                m_nLastErrorNumber = IMAGE_ERROR::IMAGE_CANNOT_READ;
                bRet = false;
            }
        }
    }
    else
    {
        bRet = false;
    }

    return bRet;
}

bool CBKFloppyImage_AODos::WriteFile(BKDirDataItem *pFR, uint8_t *pBuffer, bool &bNeedSqueeze)
{
    // если попадаем сюда с такой ошибкой - то мы в цикле пытаемся просквизировать диск
    if (bNeedSqueeze)
    {
        if (!Squeeze()) // если сквизирование неудачное
        {
            bNeedSqueeze = false;
            return false; // то выходим с ошибкой
        }
    }

    bNeedSqueeze = false;
    m_nLastErrorNumber = IMAGE_ERROR::OK_NOERRORS;

    if (m_bFileROMode)
    {
        // Если образ открылся только для чтения,
        m_nLastErrorNumber = IMAGE_ERROR::IMAGE_WRITE_PROTECRD;
        return false; // то записать в него мы ничего не сможем.
    }

    if (pFR->nAttr & (FR_ATTR::DIR | FR_ATTR::LINK))
    {
        m_nLastErrorNumber = IMAGE_ERROR::FS_IS_NOT_FILE;
        return false; // если это не файл - выход с ошибкой
    }

    ConvertAbstractToRealRecord(pFR);
    auto pRec = reinterpret_cast<AodosFileRecord *>(pFR->pSpecificData); // Вот эту запись надо добавить
    pFR->nDirBelong = pRec->status2 = m_sDiskCat.nCurrDirNum; // номер текущего открытого подкаталога

    if (m_sDiskCat.nFreeBlocks < pRec->len_blk)
    {
        m_nLastErrorNumber = IMAGE_ERROR::FS_DISK_FULL;
        return false;
    }

    if (m_sDiskCat.nFreeRecs <= 0)
    {
        m_nLastErrorNumber = IMAGE_ERROR::FS_CAT_FULL;
        return false;
    }

    // поищем, вдруг такое имя файла уже существует.
    int nInd = FindRecord2(pRec, true);

    if (nInd >= 0)
    {
        m_nLastErrorNumber = IMAGE_ERROR::FS_FILE_EXIST;
        *pRec = m_pDiskCat[nInd];
        ConvertRealToAbstractRecord(pFR); // и сделаем найденную запись актуальной.
        return false;
    }

    bool bRet = false;
    unsigned int nIndex = 0;
    // найдём свободное место в каталоге.
    bool bFound = false;
    bool bHole = false; // что нашли, дырку или место в конце, т.к. разные способы обработки каталога

    for (unsigned int i = 0; i < m_nMKLastCatRecord; ++i)
    {
        if (IsEndOfCat(&m_pDiskCat[i]))
        {
            break;
        }

        if ((m_pDiskCat[i].status2 & 0200) && (m_pDiskCat[i].len_blk >= pRec->len_blk))
        {
            // если нашли дырку подходящего размера
            bHole = true;
            nIndex = i;
            bFound = true;
            break;
        }
    }

    if (!bFound)
    {
        nIndex = m_nMKLastCatRecord;

        if (nIndex < m_nMKCatSize) // если в конце каталога вообще есть место
        {
            bFound = true; // то нашли, иначе - нет
        }
    }

    if (bFound)
    {
        // можно сохранять
        int nFirstFreeBlock = pRec->start_block = m_pDiskCat[nIndex].start_block;
        int nHoleSize = m_pDiskCat[nIndex].len_blk;

        if (bHole)
        {
            // если нашли дырку в каталоге, надо раздвинуть каталог, или не раздвигать, если размер дырки совпадает
            if (nHoleSize > pRec->len_blk)
            {
                // раздвинем каталог
                unsigned int i = m_nMKLastCatRecord + 1; // найдём конец каталога

                if (i >= m_nMKCatSize) // если каталог раздвинуть нельзя
                {
                    m_nLastErrorNumber = IMAGE_ERROR::FS_CAT_FULL;
                    goto l_squeeze;
                }
                else
                {
                    // раздвигаем
                    int cnt = 0; // признак

                    while (i > nIndex)
                    {
                        m_pDiskCat[i] = m_pDiskCat[i - 1];
                        i--;
                        cnt++;
                    }

                    // сохраняем нашу запись
                    m_pDiskCat[nIndex] = pRec;
                    AodosFileRecord hole;
                    hole.status1 = 0377;
                    hole.status2 = 0377;
                    hole.start_block = pRec->start_block + pRec->len_blk;
                    hole.len_blk = nHoleSize - pRec->len_blk;
                    imgUtil::UNICODEtoBK(L"<HOLE>", hole.name, 14, true);
                    hole.length = (hole.len_blk * m_nBlockSize) % 0200000;
                    // и запись с инфой о дырке
                    m_pDiskCat[nIndex + 1] = hole;
                    m_nMKLastCatRecord++;

                    // если мы в каталоге раздвигая, раздвинули только на одну запись - то мы оказались в конце каталога
                    // при этом надо поправить начало первого свободного блока в ячейке 032
                    if (cnt == 1)
                    {
                        *(reinterpret_cast<uint16_t *>(&m_vCatBuffer[FMT_AODOS_FIRST_FREE_BLOCK])) = hole.start_block;
                    }
                }
            }
            else
            {
                // ничего раздвигать не надо, просто
                // сохраняем нашу запись
                m_pDiskCat[nIndex] = pRec;
            }
        }
        else
        {
            // если нашли свободную область в конце каталога
            // сохраняем нашу запись
            m_pDiskCat[nIndex] = pRec;
            // сохраняем признак конца каталога
            AodosFileRecord hole;
            hole.start_block = pRec->start_block + pRec->len_blk;
            hole.len_blk = nHoleSize - pRec->len_blk;
            // и запись с инфой о свободной области
            m_pDiskCat[nIndex + 1] = hole;
            m_nMKLastCatRecord++;
        }

        if (m_nLastErrorNumber == IMAGE_ERROR::OK_NOERRORS)
        {
            // перемещаемся к месту
            if (SeekToBlock(nFirstFreeBlock))
            {
                int nReaded;
                int nLen = pFR->nSize; // размер файла

                if (nLen == 0)
                {
                    nLen++; // запись файла нулевой длины недопускается, поэтому скорректируем.
                }

                while (nLen > 0)
                {
                    memset(m_mBlock, 0, COPY_BLOCK_SIZE);
                    nReaded = (nLen >= COPY_BLOCK_SIZE) ? COPY_BLOCK_SIZE : nLen;
                    memcpy(m_mBlock, pBuffer, nReaded);
                    pBuffer += nReaded;

                    // пишем с выравниванием по секторам.
                    if (nReaded > 0)
                    {
                        if (!WriteData(m_mBlock, EvenSizeByBlock_l(nReaded)))
                        {
                            return false;
                        }
                    }

                    nLen -= nReaded;
                }

                if (m_nLastErrorNumber == IMAGE_ERROR::OK_NOERRORS)
                {
                    // если ошибок не произошло, сохраним результаты
                    *(reinterpret_cast<uint16_t *>(&m_vCatBuffer[FMT_AODOS_CAT_RECORD_NUMBER])) += 1;
                    // наконец сохраняем каталог
                    bRet = WriteCurrentDir();
                    m_sDiskCat.nFreeRecs--;
                    m_sDiskCat.nFreeBlocks -= pRec->len_blk;
                }
            }
        }
    }
    else
    {
        // если нет дырки, но суммарное свободное место на диске есть - делаем сквизирование и пишем в конец.
        m_nLastErrorNumber = IMAGE_ERROR::FS_DISK_NEED_SQEEZE;
l_squeeze:
        bNeedSqueeze = true;
    }

    return bRet;
}


bool CBKFloppyImage_AODos::DeleteFile(BKDirDataItem *pFR, bool bForce)
{
    m_nLastErrorNumber = IMAGE_ERROR::OK_NOERRORS;
    bool bRet = false;

    if (m_bFileROMode)
    {
        // Если образ открылся только для чтения,
        m_nLastErrorNumber = IMAGE_ERROR::IMAGE_WRITE_PROTECRD;
        return bRet; // то записать в него мы ничего не сможем.
    }

    if (pFR->nAttr & (FR_ATTR::DIR | FR_ATTR::LINK))
    {
        m_nLastErrorNumber = IMAGE_ERROR::FS_IS_NOT_FILE;
        return false; // если это не файл - выход с ошибкой
    }

    if (pFR->nAttr & FR_ATTR::DELETED)
    {
        return true; // уже удалённое не удаляем повторно
    }

    ConvertAbstractToRealRecord(pFR);
    auto pRec = reinterpret_cast<AodosFileRecord *>(pFR->pSpecificData); // Вот эту запись надо удалить
    // найдём её в каталоге
    int nIndex = FindRecord(pRec);

    if (nIndex >= 0) // если нашли
    {
        if ((m_pDiskCat[nIndex].status1 & 2) && !bForce)
        {
            m_nLastErrorNumber = IMAGE_ERROR::FS_FILE_PROTECTED;
        }
        else
        {
            m_pDiskCat[nIndex].status1 = 0377; // пометим как удалённую
            m_pDiskCat[nIndex].status2 = 0377;
            *(reinterpret_cast<uint16_t *>(&m_vCatBuffer[FMT_AODOS_CAT_RECORD_NUMBER])) -= 1; // поправим параметры - количество файлов
            bRet = WriteCurrentDir(); // сохраним директорию
            m_sDiskCat.nFreeRecs++;
            m_sDiskCat.nFreeBlocks += m_pDiskCat[nIndex].len_blk;
        }
    }
    else
    {
        m_nLastErrorNumber = IMAGE_ERROR::FS_FILE_NOT_FOUND;
    }

    return bRet;
}

// сквизирование диска
bool CBKFloppyImage_AODos::Squeeze()
{
    m_nLastErrorNumber = IMAGE_ERROR::OK_NOERRORS;
    bool bRet = true;
    unsigned int p = 0;

    while (p <= m_nMKLastCatRecord)
    {
        if (m_pDiskCat[p].status2 == 0200) // если нашли дырку
        {
            unsigned int n = p + 1; // индекс следующей записи

            if (n >= m_nMKLastCatRecord) // если p - последняя запись
            {
                // Тут надо обработать выход
                m_pDiskCat[p].status1 = m_pDiskCat[p].status2 = 0;
                memset(m_pDiskCat[p].name, 0, 14);
                m_pDiskCat[p].address = m_pDiskCat[p].length = 0;
                m_nMKLastCatRecord--;
                break;
            }

            if (m_pDiskCat[n].status2 & 0200) // и за дыркой снова дырка
            {
                m_pDiskCat[p].len_blk += m_pDiskCat[n].len_blk; // первую - укрупним

                // а вторую удалим.
                while (n < m_nMKLastCatRecord) // сдвигаем каталог
                {
                    m_pDiskCat[n] = m_pDiskCat[n + 1];
                    n++;
                }

                m_pDiskCat[m_nMKLastCatRecord--].clear();
                continue; // и всё сначала
            }

            size_t nBufSize = static_cast<size_t>(m_pDiskCat[n].len_blk) * m_nBlockSize;
            auto pBuf = std::vector<uint8_t>(nBufSize);

            if (pBuf.data())
            {
                if (SeekToBlock(m_pDiskCat[n].start_block))
                {
                    if (!ReadData(pBuf.data(), nBufSize))
                    {
                        m_nLastErrorNumber = IMAGE_ERROR::IMAGE_CANNOT_READ;
                        bRet = false;
                        break;
                    }

                    if (SeekToBlock(m_pDiskCat[p].start_block))
                    {
                        if (!WriteData(pBuf.data(), nBufSize))
                        {
                            m_nLastErrorNumber = IMAGE_ERROR::IMAGE_CANNOT_READ;
                            bRet = false;
                            break;
                        }

                        // теперь надо записи местами поменять.
                        std::swap(m_pDiskCat[p], m_pDiskCat[n]); // обменяем записи целиком
                        std::swap(m_pDiskCat[p].start_block, m_pDiskCat[n].start_block); // начальные блоки вернём как было.
                        m_pDiskCat[n].start_block = m_pDiskCat[p].start_block + m_pDiskCat[p].len_blk;
                    }
                    else
                    {
                        bRet = false;
                    }
                }
                else
                {
                    bRet = false;
                }
            }
            else
            {
                m_nLastErrorNumber = IMAGE_ERROR::NOT_ENOUGHT_MEMORY;
                bRet = false;
                break;
            }
        }

        p++;
    }

    WriteCurrentDir(); // сохраним
    return bRet;
}

//#ifdef _DEBUG
//// отладочный вывод каталога
//void CBKFloppyImage_AODos::DebugOutCatalog(AodosFileRecord *pRec)
//{
//	auto strModuleName = std::vector<wchar_t>(_MAX_PATH);
//	::GetModuleFileName(AfxGetInstanceHandle(), strModuleName.data(), _MAX_PATH);
//	fs::path strName = fs::path(strModuleName.data()).parent_path() / L"dirlog.txt";
//	FILE *log = _wfopen(strName.c_str(), L"wt");
//	fprintf(log, "N#  Stat.Dir.Name           start  lenblk  address length\n");
//	uint8_t name[15];
//
//	for (unsigned int i = 0; i < m_nMKLastCatRecord; ++i)
//	{
//		memcpy(name, pRec[i].name, 14);
//		name[14] = 0;
//		fprintf(log, "%03d %03d  %03d %-14s %06o %06o  %06o  %06o\n", i, pRec[i].status1, pRec[i].status2, name, pRec[i].start_block, pRec[i].len_blk, pRec[i].address, pRec[i].length);
//	}
//
//	fclose(log);
//}
//#endif
