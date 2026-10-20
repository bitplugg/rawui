// ============================================================================
//  RAW UI - filesystem helpers.
//
//  Thin wrappers over the POSIX-ish API the toolchain's libc provides, plus the
//  knowledge of where interesting things live on a jailbroken PS4.
// ============================================================================
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace rawui {

struct FileEntry {
    std::string name;   // file name without path
    std::string path;   // full path
    bool isDir = false;
    bool isSymlink = false;
    uint64_t size = 0;
    int64_t modified = 0;  // unix seconds
};

namespace fs {

// Returns entries sorted: directories first, then case-insensitive by name.
std::vector<FileEntry> listDir(const std::string& dir, bool& ok);

bool exists(const std::string& path);
bool isDir(const std::string& path);
uint64_t size(const std::string& path);
int64_t modifiedTime(const std::string& path);

bool removeAll(const std::string& path);
bool rename(const std::string& from, const std::string& to);
bool makeDir(const std::string& path);

struct SpaceInfo {
    uint64_t total = 0;
    uint64_t free = 0;
    uint64_t used = 0;
    bool valid = false;
};
SpaceInfo space(const std::string& mountPoint);

// Recursively totals the bytes below `path` (bounded by `maxEntries`).
uint64_t directorySize(const std::string& path, int maxEntries = 4096);

// Writable roots worth showing in the file manager, in a sensible order.
std::vector<std::string> mountPoints();

// USB volumes that are actually mounted, in probe order (/mnt/usb0..7, then
// the exFAT ports).  Used by theme import/export and settings backup.
std::vector<std::string> usbRoots();

// Whole-file helpers.  Reading is capped so a stray path (a /dev node, a
// virtual file that reports a huge size) cannot exhaust memory.
std::string readFile(const std::string& path, size_t maxBytes = 4u * 1024u * 1024u);
bool writeFile(const std::string& path, const std::string& data);

// True for "." and "..".
bool isDotEntry(const char* name);

}  // namespace fs

// ---------------------------------------------------------------- utilities --
std::string baseName(const std::string& path);
std::string dirName(const std::string& path);
std::string fileExtension(const std::string& path);  // lowercase, no dot
std::string humanSize(uint64_t bytes);
std::string humanTime(int64_t unixSeconds);
std::string trimTrailingSlash(const std::string& path);
std::string joinPath(const std::string& a, const std::string& b);

// Icon code point (see gfx/icons.hpp) for a file name / directory.
uint32_t iconForPath(const std::string& path, bool isDir);

}  // namespace rawui
