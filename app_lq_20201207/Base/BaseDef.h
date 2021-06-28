#ifndef BASE_DEF_H
#define BASE_DEF_H

#define MILLISECOND_SCALE                   1000
#define FRAME_NUM_SIZE                      (sizeof(unsigned long long))
#define WIDTH_SIZE                          (sizeof(unsigned int))
#define HEIGHT_SIZE                         (WIDTH_SIZE)
#define MAX_NET_RCV_BUF                     (4 * 1024 * 1024)

enum DataTransMethod
{
    eDTMUdp,
    eDTMTcp,
    eDTMAll,
};

#endif // BASEDEF_H