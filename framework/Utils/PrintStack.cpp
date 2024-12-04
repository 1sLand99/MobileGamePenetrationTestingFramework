#include <inttypes.h>
#include <iomanip>
#include <vector>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/resource.h>
#include <Log.h>
#include <string.h>
#include "PrintStack.h"

inline _Unwind_Reason_Code unwindCallback(struct _Unwind_Context *context, void *arg)
{
    std::vector<_Unwind_Word> &stack = *(std::vector<_Unwind_Word> *)arg;
    _Unwind_Word ip = _Unwind_GetIP(context);
    stack.push_back(ip);
    return _URC_NO_REASON;
}

inline void callUnwindBacktrace(std::vector<_Unwind_Word> *stack)
{
    _Unwind_Backtrace(unwindCallback, (void *)stack);
}

void dumpStackUnwind()
{
    Dl_info info = {0};
    dladdr((void *)_Unwind_Backtrace, &info);
    std::vector<_Unwind_Word> stack;
    callUnwindBacktrace(&stack);
    char buff[256];
    for (size_t i = 0; i < stack.size(); i++)
    {
        Dl_info info;
        if (!dladdr((void *)stack[i], &info))
        {
            continue;
        }
        size_t addr = (char *)stack[i] - (char *)info.dli_fbase;
        if (info.dli_sname == NULL || strlen(info.dli_sname) == 0)
        {
            // sprintf(buff, "#%02x pc 0x%x %s 0x%x\n", i, addr, info.dli_fname, stack[i]);
            LOGI("#%02zx pc 0x%zx %s 0x%lx\n", i, addr, info.dli_fname, stack[i]);
        }
        else
        {
            // sprintf(buff, "#%02x pc 0x%x %s (%s+00) 0x%x\n", i, addr, info.dli_fname, info.dli_sname, stack[i]);
            LOGI("#%02zx pc 0x%zx %s (%s+00) 0x%lx\n", i, addr, info.dli_fname, info.dli_sname, stack[i]);
        }
    }
}