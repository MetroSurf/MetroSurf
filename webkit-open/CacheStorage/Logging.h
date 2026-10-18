#pragma once
#include <wtf/Assertions.h>
#include <WebCore/SharedBuffer.h>

#undef RELEASE_LOG
#undef RELEASE_LOG_ERROR
#undef RELEASE_LOG_ERROR_IF
#define RELEASE_LOG(channel, ...) ((void)0)
#define RELEASE_LOG_ERROR(channel, ...) ((void)0)
#define RELEASE_LOG_ERROR_IF(isAllowed, channel, ...) ((void)0)
