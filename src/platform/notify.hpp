// ============================================================================
//  RAW UI - system notifications (the little toast the PS4 shows top-left).
// ============================================================================
#pragma once

#include <string>

namespace rawui {

namespace notify {

// Resolves and caches the initial user id.  Safe to call more than once.
void init();

// Shows a system notification.  Missing: nothing fancy, this is the same toast
// the system itself uses for "downloaded" / "installed" messages.
void toast(const std::string& message);

}  // namespace notify

}  // namespace rawui
