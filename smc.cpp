#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <IOKit/IOKitLib.h>
#include <libkern/OSAtomic.h>
#include "smc.h"

io_connect_t g_conn = 0;
int g_keyInfoCacheCount = 0;
OSSpinLock g_keyInfoSpinLock = 0;

UInt32 _strtoul(char *str, int size, int base)
{
    UInt32 total = 0;
    int i;

    for (i = 0; i < size; i++)
    {
        if (base == 16)
        {
            total += str[i] << (size - 1 - i) * 8;
        }
        else
        {
            total += ((unsigned char) (str[i]) << (size - 1 - i) * 8);
        }
    }

    return total;
}

float _strtof(unsigned char *str, int size, int e)
{
    float total = 0;
    int i;

    for (i = 0; i < size; i++)
    {
        if (i == (size - 1))
        {
            total += (str[i] & 0xff) >> e;
        }
        else
        {
            total += str[i] << (size - 1 - i) * (8 - e);
        }
    }

    total += (str[size - 1] & 0x03) * 0.25;

    return total;
}

void _ultostr(char *str, UInt32 val)
{
    str[0] = '\0';
    sprintf(str, "%c%c%c%c",
            (unsigned int) val >> 24,
            (unsigned int) val >> 16,
            (unsigned int) val >> 8,
            (unsigned int) val);
}

void smc_init()
{
    SMCOpen(&g_conn);
}

void smc_close()
{
    SMCClose(g_conn);
}

kern_return_t SMCReadKey(UInt32Char_t key, SMCVal_t *val)
{
    return SMCReadKey2(key, val, g_conn);
}

kern_return_t SMCWriteSimple(UInt32Char_t key, char *wvalue, io_connect_t conn)
{
    kern_return_t result;
    SMCVal_t val;
    int i;
    char c[3];

    for (i = 0; i < strlen(wvalue); i++)
    {
        sprintf(c, "%c%c", wvalue[i * 2], wvalue[(i * 2) + 1]);
        val.bytes[i] = (int) strtol(c, NULL, 16);
    }

    val.dataSize = i / 2;
    sprintf(val.key, key);
    result = SMCWriteKey2(val, conn);

    if (result != kIOReturnSuccess)
    {
        printf("Error: SMCWriteKey() = %08x\n", result);
    }

    return result;
}

kern_return_t SMCOpen(io_connect_t *conn)
{
    kern_return_t result;
    mach_port_t masterPort;
    io_iterator_t iterator;
    io_object_t device;

    IOMasterPort(MACH_PORT_NULL, &masterPort);

    CFMutableDictionaryRef matchingDictionary = IOServiceMatching("AppleSMC");
    result = IOServiceGetMatchingServices(masterPort, matchingDictionary,
                                          &iterator);

    if (result != kIOReturnSuccess)
    {
        printf("Error: IOServiceGetMatchingServices() = %08x\n", result);
        return 1;
    }

    device = IOIteratorNext(iterator);
    IOObjectRelease(iterator);

    if (device == 0)
    {
        printf("Error: no SMC found\n");
        return 1;
    }

    result = IOServiceOpen(device, mach_task_self(), 0, conn);
    IOObjectRelease(device);

    if (result != kIOReturnSuccess)
    {
        printf("Error: IOServiceOpen() = %08x\n", result);
        return 1;
    }

    return kIOReturnSuccess;
}

kern_return_t SMCClose(io_connect_t conn)
{
    return IOServiceClose(conn);
}

kern_return_t SMCReadKey2(UInt32Char_t key, SMCVal_t *val, io_connect_t conn)
{
    kern_return_t result;
    SMCKeyData_t inputStructure;
    SMCKeyData_t outputStructure;

    memset(&inputStructure, 0, sizeof(SMCKeyData_t));
    memset(&outputStructure, 0, sizeof(SMCKeyData_t));
    memset(val, 0, sizeof(SMCVal_t));

    inputStructure.key = _strtoul(key, 4, 16);
    sprintf(val->key, key);

    result = SMCGetKeyInfo(inputStructure.key, &outputStructure.keyInfo, conn);

    if (result != kIOReturnSuccess)
    {
        return result;
    }

    val->dataSize = outputStructure.keyInfo.dataSize;
    _ultostr(val->dataType, outputStructure.keyInfo.dataType);
    inputStructure.keyInfo.dataSize = val->dataSize;
    inputStructure.data8 = SMC_CMD_READ_BYTES;

    result = SMCCall2(KERNEL_INDEX_SMC, &inputStructure, &outputStructure, conn);

    if (result != kIOReturnSuccess)
    {
        return result;
    }

    memcpy(val->bytes, outputStructure.bytes, sizeof(outputStructure.bytes));

    return kIOReturnSuccess;
}

kern_return_t SMCCall(int index, SMCKeyData_t *inputStructure, SMCKeyData_t *outputStructure)
{
    return SMCCall2(index, inputStructure, outputStructure, g_conn);
}

kern_return_t SMCCall2(int index, SMCKeyData_t *inputStructure, SMCKeyData_t *outputStructure, io_connect_t conn)
{
    size_t structureInputSize;
    size_t structureOutputSize;
    structureInputSize = sizeof(SMCKeyData_t);
    structureOutputSize = sizeof(SMCKeyData_t);

    return IOConnectCallStructMethod(conn, index, inputStructure,
                                     structureInputSize, outputStructure,
                                     &structureOutputSize);
}

kern_return_t SMCWriteKey(SMCVal_t writeVal)
{
    return SMCWriteKey2(writeVal, g_conn);
}

kern_return_t SMCWriteKey2(SMCVal_t writeVal, io_connect_t conn)
{
    kern_return_t result;
    SMCKeyData_t inputStructure;
    SMCKeyData_t outputStructure;

    SMCVal_t readVal;

    result = SMCReadKey2(writeVal.key, &readVal, conn);

    if (result != kIOReturnSuccess)
    {
        return result;
    }

    if (readVal.dataSize != writeVal.dataSize)
    {
        return kIOReturnError;
    }

    memset(&inputStructure, 0, sizeof(SMCKeyData_t));
    memset(&outputStructure, 0, sizeof(SMCKeyData_t));

    inputStructure.key = _strtoul(writeVal.key, 4, 16);
    inputStructure.data8 = SMC_CMD_WRITE_BYTES;
    inputStructure.keyInfo.dataSize = writeVal.dataSize;
    memcpy(inputStructure.bytes, writeVal.bytes, sizeof(writeVal.bytes));
    result = SMCCall2(KERNEL_INDEX_SMC, &inputStructure, &outputStructure, conn);

    if (result != kIOReturnSuccess)
    {
        return result;
    }

    return kIOReturnSuccess;
}

// Provides key info, using a cache to dramatically improve the energy impact of smcFanControl
kern_return_t SMCGetKeyInfo(UInt32 key, SMCKeyData_keyInfo_t *keyInfo,
                            io_connect_t conn)
{
    SMCKeyData_t inputStructure;
    SMCKeyData_t outputStructure;
    kern_return_t result = kIOReturnSuccess;
    int i = 0;

    OSSpinLockLock(&g_keyInfoSpinLock);

    for (; i < g_keyInfoCacheCount; ++i)
    {
        if (key == g_keyInfoCache[i].key)
        {
            *keyInfo = g_keyInfoCache[i].keyInfo;
            break;
        }
    }

    if (i == g_keyInfoCacheCount)
    {
        // Not in cache, must look it up.
        memset(&inputStructure, 0, sizeof(inputStructure));
        memset(&outputStructure, 0, sizeof(outputStructure));

        inputStructure.key = key;
        inputStructure.data8 = SMC_CMD_READ_KEYINFO;

        result = SMCCall2(KERNEL_INDEX_SMC, &inputStructure, &outputStructure, conn);

        if (result == kIOReturnSuccess)
        {
            *keyInfo = outputStructure.keyInfo;

            if (g_keyInfoCacheCount < KEY_INFO_CACHE_SIZE)
            {
                g_keyInfoCache[g_keyInfoCacheCount].key = key;
                g_keyInfoCache[g_keyInfoCacheCount].keyInfo = outputStructure.keyInfo;
                ++g_keyInfoCacheCount;
            }
        }
    }

    OSSpinLockUnlock(&g_keyInfoSpinLock);

    return result;
}