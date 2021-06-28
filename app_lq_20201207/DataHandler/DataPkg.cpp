
#include <cstring>
#include <cstdio>

#include "DataPkg.h"

DataPkg::DataPkg()
{
    ullFrameNum = 0;
    nWidth = 0;
    nHeight = 0;
    pucImageData = nullptr;
    nImageDataTotalLen = 0;
    nImageDataCurrentLen = 0;
    bDataHandled = true;
}

DataPkg::~DataPkg()
{
    if (nullptr != pucImageData)
    {
        delete [] pucImageData;
        pucImageData = nullptr;
        nImageDataTotalLen = 0;
        nImageDataCurrentLen = 0;
        bDataHandled = true;
    }
}

DataPkg & DataPkg::operator=(const DataPkg & tDataPkg)
{
    if (this != &tDataPkg)
    {
        if (!bDataHandled)
        {
            printf("Data have not been handled\n");
            return *this;
        }

        ullFrameNum = tDataPkg.ullFrameNum;
        nWidth = tDataPkg.nWidth;
        nHeight = tDataPkg.nHeight;
        bDataHandled = tDataPkg.bDataHandled;
        unsigned int nLen = tDataPkg.nImageDataCurrentLen + 1 + sizeof(unsigned long long) + 2 * sizeof(unsigned int);
        
        if (nullptr == pucImageData)
        {
            pucImageData = new unsigned char[nLen];
            if (nullptr == pucImageData)
            {
                nImageDataTotalLen = 0;
                nImageDataCurrentLen = 0;
                return *this;
            }

            nImageDataTotalLen = nLen;
        }
        else if (nImageDataTotalLen < nLen)
        {
            delete [] pucImageData;
            pucImageData = new unsigned char[nLen];
            if (nullptr == pucImageData)
            {
                nImageDataTotalLen = 0;
                nImageDataCurrentLen = 0;
                return *this;
            }

            nImageDataTotalLen = nLen;
        }

    }
    memcpy(pucImageData, tDataPkg.pucImageData, tDataPkg.nImageDataCurrentLen);
    nImageDataCurrentLen = tDataPkg.nImageDataCurrentLen;
//    printf("alloc pucImageData:%d\n", nImageDataCurrentLen);

    return *this;
}
