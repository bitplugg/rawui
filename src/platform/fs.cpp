#include "platform/fs.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <ctime>

#include <dirent.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>

#include "core/log.hpp"
#include "gfx/icons.hpp"

namespace rawui {
namespace fs {
namespace {

inline std::string lowerCase(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

}  // namespace

bool isDotEntry(const char* name) {
    return name[0] == '.' && (name[1] == '\0' || (name[1] == '.' && name[2] == '\0'));
}

std::vector<FileEntry> listDir(const std::string& dir, bool& ok) {
    std::vector<FileEntry> out;
    ok = false;

    DIR* d = opendir(dir.c_str());
    if (!d) return out;

    ok = true;
    struct dirent* de = nullptr;
    while ((de = readdir(d)) != nullptr) {
        if (isDotEntry(de->d_name)) continue;

        FileEntry e;
        e.name = de->d_name;
        e.path = joinPath(dir, e.name);

        struct stat st;
        if (lstat(e.path.c_str(), &st) == 0) {
            e.isDir = S_ISDIR(st.st_mode);
            e.isSymlink = S_ISLNK(st.st_mode);
            e.size = static_cast<uint64_t>(st.st_size);
            e.modified = static_cast<int64_t>(st.st_mtime);
        } else {
            // Some virtual filesystems refuse lstat; fall back to the dirent.
            e.isDir = (de->d_type == DT_DIR);
        }

        out.push_back(std::move(e));
    }
    closedir(d);

    std::sort(out.begin(), out.end(), [](const FileEntry& a, const FileEntry& b) {
        if (a.isDir != b.isDir) return a.isDir;
        return lowerCase(a.name) < lowerCase(b.name);
    });

    return out;
}

bool exists(const std::string& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0;
}

bool isDir(const std::string& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

uint64_t size(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return 0;
    return static_cast<uint64_t>(st.st_size);
}

int64_t modifiedTime(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return 0;
    return static_cast<int64_t>(st.st_mtime);
}

bool removeAll(const std::string& path) {
    struct stat st;
    if (lstat(path.c_str(), &st) != 0) return false;

    if (!S_ISDIR(st.st_mode)) {
        return std::remove(path.c_str()) == 0;
    }

    DIR* d = opendir(path.c_str());
    if (d) {
        struct dirent* de = nullptr;
        while ((de = readdir(d)) != nullptr) {
            if (isDotEntry(de->d_name)) continue;
            removeAll(joinPath(path, de->d_name));
        }
        closedir(d);
    }

    return rmdir(path.c_str()) == 0;
}

bool rename(const std::string& from, const std::string& to) {
    return std::rename(from.c_str(), to.c_str()) == 0;
}

bool makeDir(const std::string& path) {
    if (mkdir(path.c_str(), 0777) == 0) return true;

    // Create intermediate directories.
    size_t pos = path.find('/', 1);
    while (pos != std::string::npos) {
        const std::string sub = path.substr(0, pos);
        mkdir(sub.c_str(), 0777);
        pos = path.find('/', pos + 1);
    }
    return mkdir(path.c_str(), 0777) == 0 || isDir(path);
}

SpaceInfo space(const std::string& mountPoint) {
    SpaceInfo info;

    struct statvfs vfs;
    if (statvfs(mountPoint.c_str(), &vfs) != 0) return info;

    const uint64_t blockSize = vfs.f_frsize ? vfs.f_frsize : vfs.f_bsize;
    info.total = static_cast<uint64_t>(vfs.f_blocks) * blockSize;
    info.free = static_cast<uint64_t>(vfs.f_bavail) * blockSize;
    info.used = info.total - info.free;
    info.valid = info.total > 0;
    return info;
}

uint64_t directorySize(const std::string& path, int maxEntries) {
    uint64_t total = 0;
    int visited = 0;

    DIR* d = opendir(path.c_str());
    if (!d) return 0;

    struct dirent* de = nullptr;
    while ((de = readdir(d)) != nullptr && visited < maxEntries) {
        if (isDotEntry(de->d_name)) continue;
        ++visited;

        const std::string child = joinPath(path, de->d_name);
        struct stat st;
        if (lstat(child.c_str(), &st) != 0) continue;

        if (S_ISDIR(st.st_mode)) {
            total += directorySize(child, maxEntries - visited);
        } else {
            total += static_cast<uint64_t>(st.st_size);
        }
    }
    closedir(d);

    return total;
}

std::vector<std::string> mountPoints() {
    // Order matters: this is what the file manager shows first.
    static const char* kCandidates[] = {
        "/data",     "/user",   "/mnt/usb0", "/mnt/usb1", "/mnt/usb2", "/mnt/usb3",
        "/mnt/usb4", "/mnt/usb5", "/mnt/usb6", "/mnt/usb7", "/mnt/ext0", "/mnt/ext1",
        "/app0",     "/tmp",
    };

    std::vector<std::string> out;
    for (const char* c : kCandidates) {
        if (isDir(c)) out.emplace_back(c);
    }
    return out;
}

std::vector<std::string> usbRoots() {
    static const char* kExtra[] = {"/mnt/ext0", "/mnt/ext1"};

    std::vector<std::string> out;
    for (int i = 0; i < 8; ++i) {
        std::string p = "/mnt/usb";
        p += static_cast<char>('0' + i);
        if (isDir(p)) out.push_back(p);
    }
    for (const char* c : kExtra) {
        if (isDir(c)) out.emplace_back(c);
    }
    return out;
}

std::string readFile(const std::string& path, size_t maxBytes) {
    std::FILE* fh = std::fopen(path.c_str(), "rb");
    if (!fh) return {};

    std::string out;
    char buf[1024];
    size_t n = 0;
    while ((n = std::fread(buf, 1, sizeof(buf), fh)) > 0) {
        // A capped read still returns the prefix rather than nothing, so a
        // truncated theme file is reported as malformed rather than missing.
        if (out.size() + n > maxBytes) {
            out.append(buf, maxBytes - out.size());
            break;
        }
        out.append(buf, n);
    }
    std::fclose(fh);
    return out;
}

bool writeFile(const std::string& path, const std::string& data) {
    std::FILE* fh = std::fopen(path.c_str(), "wb");
    if (!fh) return false;

    const bool ok = data.empty() ||
                    std::fwrite(data.data(), 1, data.size(), fh) == data.size();
    std::fclose(fh);
    return ok;
}

}  // namespace fs

// ---------------------------------------------------------------------------
std::string baseName(const std::string& path) {
    if (path.empty()) return path;
    const size_t pos = path.find_last_of('/');
    return (pos == std::string::npos) ? path : path.substr(pos + 1);
}

std::string dirName(const std::string& path) {
    if (path.size() <= 1) return "/";
    const size_t pos = path.find_last_of('/');
    if (pos == std::string::npos) return ".";
    if (pos == 0) return "/";
    return path.substr(0, pos);
}

std::string fileExtension(const std::string& path) {
    const std::string name = baseName(path);
    const size_t pos = name.find_last_of('.');
    if (pos == std::string::npos || pos == 0) return {};

    std::string ext = name.substr(pos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext;
}

std::string humanSize(uint64_t bytes) {
    static const char* kUnits[] = {"B", "KB", "MB", "GB", "TB"};
    double v = static_cast<double>(bytes);
    int unit = 0;
    while (v >= 1024.0 && unit < 4) {
        v /= 1024.0;
        ++unit;
    }

    char buf[64];
    if (unit == 0) {
        std::snprintf(buf, sizeof(buf), "%llu %s",
                      static_cast<unsigned long long>(bytes), kUnits[unit]);
    } else {
        std::snprintf(buf, sizeof(buf), "%.1f %s", v, kUnits[unit]);
    }
    return buf;
}

std::string humanTime(int64_t unixSeconds) {
    if (unixSeconds <= 0) return "-";

    std::time_t t = static_cast<std::time_t>(unixSeconds);
    struct tm tmv;
    if (!localtime_r(&t, &tmv)) return "-";

    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &tmv);
    return buf;
}

std::string trimTrailingSlash(const std::string& path) {
    if (path.size() <= 1) return path;
    std::string out = path;
    while (out.size() > 1 && out.back() == '/') out.pop_back();
    return out;
}

std::string joinPath(const std::string& a, const std::string& b) {
    if (a.empty()) return b;
    if (b.empty()) return a;
    if (a.back() == '/') return a + b;
    return a + "/" + b;
}

uint32_t iconForPath(const std::string& path, bool isDir) {
    if (isDir) return icons::kFolder;

    std::string ext = fileExtension(path);
    // Ensure lowercase for case-insensitive extension checks
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (ext == "pkg") return icons::kPackage;
    if (ext == "json") return icons::kDataObject;
    if (ext == "xml" || ext == "html" || ext == "htm") return icons::kCode;
    if (ext == "js" || ext == "lua") return icons::kJavascript;
    if (ext == "c" || ext == "h" || ext == "cpp" || ext == "hpp" || ext == "py")
        return icons::kCode;
    if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp" || ext == "webp")
        return icons::kImage;
    if (ext == "mp4" || ext == "mkv" || ext == "avi" || ext == "mov") return icons::kMovie;
    if (ext == "mp3" || ext == "wav" || ext == "flac" || ext == "ogg") return icons::kAudioFile;
    if (ext == "zip" || ext == "7z" || ext == "rar" || ext == "tar" || ext == "gz")
        return icons::kArchive;
    if (ext == "pdf") return icons::kPdf;
    if (ext == "txt" || ext == "md" || ext == "log") return icons::kDescription;
    if (ext == "sprx" || ext == "prx" || ext == "bin" || ext == "elf" || ext == "self" ||
        ext == "oelf")
        return icons::kDeployedCode;
    if (ext == "ttf" || ext == "otf") return icons::kTextFields;

    return icons::kFile;
}

}  // namespace rawui
