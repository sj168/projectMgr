
#ifndef DATAPKG_H
#define DATAPKG_H

typedef struct DataPkg
{
    DataPkg();
    ~DataPkg();

    unsigned long long ullFrameNum;
    unsigned int nWidth;
    unsigned int nHeight;
    unsigned char *pucImageData;
    unsigned int nImageDataTotalLen;
    unsigned int nImageDataCurrentLen;
    bool bDataHandled;

    DataPkg & operator=(const DataPkg & tDataPkg);

} T_DataPkg, * PT_DataPkg;

#endif // DATAPKG_H
