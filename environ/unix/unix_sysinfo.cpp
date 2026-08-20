#include "tjsCommHead.h"
#include "Platform.h"
#include "PlatformFile.h"

#include <fstream>
#include <unistd.h>
#include <sys/sysinfo.h>

//---------------------------------------------------------------------------
void TVPGetMemoryInfo(TVPMemoryInfo& m)
{
    /* to read /proc/meminfo */
    FILE* meminfo;
    char buffer[100] = {0};
    char* end;
    int found = 0;

    /* Try to read /proc/meminfo, bail out if fails */
    meminfo = fopen("/proc/meminfo", "r");

    static const char pszMemFree[] = "MemFree:", pszMemTotal[] = "MemTotal:",
                      pszSwapTotal[] = "SwapTotal:", pszSwapFree[] = "SwapFree:",
                      pszVmallocTotal[] = "VmallocTotal:", pszVmallocUsed[] = "VmallocUsed:";

    /* Read each line untill we got all we ned */
    while (fgets(buffer, sizeof(buffer), meminfo))
    {
        if (strstr(buffer, pszMemFree) == buffer)
        {
            m.MemFree = strtol(buffer + sizeof(pszMemFree), &end, 10);
            found++;
        }
        else if (strstr(buffer, pszMemTotal) == buffer)
        {
            m.MemTotal = strtol(buffer + sizeof(pszMemTotal), &end, 10);
            found++;
        }
        else if (strstr(buffer, pszSwapTotal) == buffer)
        {
            m.SwapTotal = strtol(buffer + sizeof(pszSwapTotal), &end, 10);
            found++;
            break;
        }
        else if (strstr(buffer, pszSwapFree) == buffer)
        {
            m.SwapFree = strtol(buffer + sizeof(pszSwapFree), &end, 10);
            found++;
            break;
        }
        else if (strstr(buffer, pszVmallocTotal) == buffer)
        {
            m.VirtualTotal = strtol(buffer + sizeof(pszVmallocTotal), &end, 10);
            found++;
            break;
        }
        else if (strstr(buffer, pszVmallocUsed) == buffer)
        {
            m.VirtualUsed = strtol(buffer + sizeof(pszVmallocUsed), &end, 10);
            found++;
            break;
        }
    }
    fclose(meminfo);
}
tjs_int TVPGetSystemFreeMemory()
{
    TVPMemoryInfo m;
    m.MemFree = 0;
    TVPGetMemoryInfo(m);
    return m.MemFree / 1024;
}
tjs_int TVPGetSelfUsedMemory()
{
    std::ifstream statm{"/proc/self/statm"};
    tjs_int pages = 0;
    statm >> pages;
    return (pages * sysconf(_SC_PAGESIZE)) / (1024 * 1024);
}