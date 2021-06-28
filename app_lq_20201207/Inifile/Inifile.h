
//
// 支持中文，注意在Linux下，
//	将ini文件的编码为utf-8
//

#ifndef INIFILE_H
#define INIFILE_H

#include <arpa/inet.h>
#include <sys/time.h>
#include <pthread.h>

//常用接口
int read_profile_string(const char *section, const char *key, char *value, int size, const char *default_value, const char *file);
int read_profile_int(const char *section, const char *key, int default_value, const char *file);
double read_profile_double(const char *section, const char *key, double default_value, const char *file);
int write_profile_string(const char *section, const char *key, const char *value, const char *file);
int write_profile_int(const char *section, const char *key, int defalt_value, const char *file);
int write_profile_double(const char *section, const char *key, double defalt_value, const char *file);
int set_cpu(int index);
double GET_MS();
unsigned long us_timer_init(void);
unsigned long ms_timer_init(void);
int ms_timer_interval_elapsed(unsigned long origin, unsigned long timeout);

#define LINUX

#ifdef LINUX /* Remove CR, on unix systems. */
#define INI_REMOVE_CR
#define DONT_HAVE_STRUPR
#endif

#ifndef CCHR_H
#define CCHR_H
typedef const char cchr;
#endif

#ifndef __cplusplus
typedef char bool;
#define true  1
#define TRUE  1
#define false 0
#define FALSE 0
#endif

#define tpNULL       0
#define tpSECTION    1
#define tpKEYVALUE   2
#define tpCOMMENT    3
#define FILE_INI_NAME "setting.ini"

struct ENTRY
{
   char   Type;
   char  *Text;
   struct ENTRY *pPrev;
   struct ENTRY *pNext;
};

typedef struct
{
   struct ENTRY *pSec;
   struct ENTRY *pKey;
   char          KeyText [128];
   char          ValText [128];
   char          Comment [255];
} EFIND;

/* Macros */
#define ArePtrValid(Sec,Key,Val) ((Sec!=NULL)&&(Key!=NULL)&&(Val!=NULL))

/* Connectors of this file (Prototypes) */

bool OpenIniFile (cchr *FileName);

bool ReadBool(cchr *Section, cchr *Key, bool Default);
int ReadInt(cchr *Section, cchr *Key, int Default);
double ReadDouble(cchr *Section, cchr *Key, double Default);
cchr *ReadString(cchr *Section, cchr *Key, cchr *Default);

void WriteBool(cchr *Section, cchr *Key, bool Value);
void WriteInt(cchr *Section, cchr *Key, int Value);
void WriteDouble(cchr *Section, cchr *Key, double Value);
void WriteString(cchr *Section, cchr *Key, cchr *Value);

bool DeleteKey(cchr *Section, cchr *Key);

void CloseIniFile();
bool WriteIniFile(cchr *FileName);

#endif


