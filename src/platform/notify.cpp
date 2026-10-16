#include "platform/notify.hpp"

#include <orbis/UserService.h>

#include "core/log.hpp"

// orbis/SysUtil.h declares this function with an empty parameter list, so the
// real prototype is provided here instead of including that header.
extern "C" int sceSysUtilSendSystemNotificationWithText(int userId, const char* message);

namespace rawui {
namespace notify {
namespace {

int g_userId = -1;

}  // namespace

void init() {
    if (g_userId >= 0) return;

    const int32_t rc = sceUserServiceInitialize(nullptr);
    if (rc != 0) {
        RU_LOG("[notify] sceUserServiceInitialize -> 0x%08X", rc);
    }

    int32_t userId = -1;
    if (sceUserServiceGetInitialUser(&userId) == 0) {
        g_userId = userId;
    } else {
        // Notification requests tolerate an invalid user, they just come from
        // "the system" instead of a player.
        g_userId = 0xFF;
    }

    RU_LOG("[notify] user id = 0x%X", g_userId);
}

void toast(const std::string& message) {
    if (g_userId < 0) init();

    const int rc = sceSysUtilSendSystemNotificationWithText(g_userId, message.c_str());
    if (rc < 0) {
        RU_LOG("[notify] toast failed -> 0x%08X (%s)", rc, message.c_str());
    }
}

}  // namespace notify
}  // namespace rawui
