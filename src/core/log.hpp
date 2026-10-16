// ============================================================================
//  RAW UI - logging.  Goes to stdout (GoldHEN can capture it) and to the PS4
//  kernel debug output, which shows up on a klog listener on port 3232.
// ============================================================================
#pragma once

namespace rawui {

// printf-style logging.  Appends a newline.
void logPrint(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

}  // namespace rawui

#define RU_LOG(...) ::rawui::logPrint(__VA_ARGS__)
