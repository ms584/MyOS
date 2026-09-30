#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef void *SbStorageRecord;
extern SbStorageRecord SbStorageOpenRecord(const char *);
extern bool SbStorageCloseRecord(SbStorageRecord);
extern int64_t SbStorageGetRecordSize(SbStorageRecord);
extern int64_t SbStorageReadRecord(SbStorageRecord,char *,int64_t);
extern bool SbStorageWriteRecord(SbStorageRecord,const char *,int64_t);
extern bool SbStorageDeleteRecord(const char *);
extern int64_t write(int,const void *,size_t);

static const char name[]="phase33-storage";
#if STORAGE_MODE != 4
static const char payload[]="Cobalt storage survives reboot; atomic bank v1";
#endif
static void print(const char *text)
{size_t length=0;while(text[length])++length;(void)write(1,text,length);}

uint64_t _start(void)
{
    SbStorageRecord record=SbStorageOpenRecord(name);if(!record)return 1;
    int64_t size=SbStorageGetRecordSize(record);
#if STORAGE_MODE == 1
    if(size!=0)return 2;
    if(SbStorageOpenRecord(name)||SbStorageOpenRecord("invalid/name")||
       SbStorageDeleteRecord(name))return 15;
    if(!SbStorageWriteRecord(record,payload,sizeof(payload)))return 3;
    if(SbStorageGetRecordSize(record)!=(int64_t)sizeof(payload))return 4;
    char immediate[sizeof(payload)];
    if(SbStorageReadRecord(record,immediate,sizeof(immediate))!=(int64_t)sizeof(immediate))return 16;
    for(size_t i=0;i<sizeof(payload);++i)if(immediate[i]!=payload[i])return 17;
    if(!SbStorageCloseRecord(record))return 5;
    print("ELF Starboard storage write and immediate read state PASS\n");return 43;
#elif STORAGE_MODE == 2
    char buffer[sizeof(payload)];
    if(size!=(int64_t)sizeof(payload))return 6;
    if(SbStorageReadRecord(record,buffer,sizeof(buffer))!=(int64_t)sizeof(buffer))return 7;
    for(size_t i=0;i<sizeof(payload);++i)if(buffer[i]!=payload[i])return 8;
    if(!SbStorageCloseRecord(record))return 9;
    print("ELF Starboard storage reboot persistence PASS\n");return 44;
#elif STORAGE_MODE == 3
    if(size!=(int64_t)sizeof(payload))return 10;
    if(!SbStorageCloseRecord(record))return 11;
    if(!SbStorageDeleteRecord(name))return 12;
    print("ELF Starboard storage delete committed PASS\n");return 45;
#else
    if(size!=0)return 13;
    if(!SbStorageCloseRecord(record))return 14;
    if(SbStorageDeleteRecord(name))return 18;
    record=SbStorageOpenRecord(0);if(!record||SbStorageGetRecordSize(record)!=0)return 19;
    if(!SbStorageCloseRecord(record))return 20;
    print("ELF Starboard storage tombstone survives reboot PASS\n");return 46;
#endif
}
