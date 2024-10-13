#include "../CLog.h"
#include "stypecast.h"

namespace detail_stypecast {

void LogEventWarnWrapper(const char* warn_str)
{
    g_Log.EventWarn(warn_str);
}

}
