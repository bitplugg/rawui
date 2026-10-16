#include "platform/goldhen.hpp"

#include <cstring>

#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include "core/log.hpp"
#include "platform/fs.hpp"

namespace rawui {
namespace {

int countFilesIn(const std::string& dir, const char* suffix) {
    DIR* d = opendir(dir.c_str());
    if (!d) return 0;

    int count = 0;
    struct dirent* de = nullptr;
    while ((de = readdir(d)) != nullptr) {
        if (fs::isDotEntry(de->d_name)) continue;
        if (suffix) {
            const std::string name = de->d_name;
            if (name.size() < std::strlen(suffix) ||
                name.compare(name.size() - std::strlen(suffix), std::strlen(suffix),
                             suffix) != 0) {
                continue;
            }
        }
        ++count;
    }
    closedir(d);
    return count;
}

std::string readFirstLine(const std::string& path) {
    std::FILE* fh = std::fopen(path.c_str(), "rb");
    if (!fh) return {};

    char buf[256] = {};
    const size_t n = std::fread(buf, 1, sizeof(buf) - 1, fh);
    std::fclose(fh);
    if (n == 0) return {};

    buf[n] = '\0';
    std::string out = buf;
    while (!out.empty() && (out.back() == '\n' || out.back() == '\r')) out.pop_back();
    return out;
}

}  // namespace

bool tcpProbe(const char* host, int port, int timeoutMs) {
    const int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return false;

    struct timeval tv;
    tv.tv_sec = timeoutMs / 1000;
    tv.tv_usec = (timeoutMs % 1000) * 1000;
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port));
    addr.sin_addr.s_addr = inet_addr(host);

    const bool ok = connect(fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == 0;
    close(fd);
    return ok;
}

HenStatus probeHen() {
    HenStatus status;

    status.payloadDirPresent = fs::isDir("/data/GoldHEN");
    status.payloadPath = "/data/GoldHEN";

    // The payload writes its build info next to the binary when one is present.
    for (const char* candidate : {"/data/GoldHEN/version.txt", "/data/GoldHEN/version",
                                  "/data/goldhen/version.txt"}) {
        if (!fs::exists(candidate)) continue;
        status.version = readFirstLine(candidate);
        if (!status.version.empty()) break;
    }

    const HenServer defaults[] = {
        {"ftp", "FTP server", "File transfers over port 2121", "127.0.0.1", 2121, false},
        {"binloader", "BinLoader", "Accepts payloads on port 9090", "127.0.0.1", 9090, false},
        {"klog", "Kernel log", "Streams klog over port 3232", "127.0.0.1", 3232, false},
        {"http", "Web server", "Serves the payload host on port 8080", "127.0.0.1", 8080, false},
    };

    for (const HenServer& s : defaults) {
        HenServer copy = s;
        copy.listening = tcpProbe(copy.host, copy.port, 60);
        if (copy.listening) {
            ++status.portCount;
        }
        status.servers.push_back(copy);
    }

    status.running = status.portCount > 0;

    const std::vector<std::string> cheatDirs = cheatDirectories();
    status.cheatDir = cheatDirs.empty() ? "/data/GoldHEN/cheats/json" : cheatDirs.front();
    for (const std::string& dir : cheatDirs) {
        status.cheatCount += countFilesIn(dir, ".json");
    }

    const std::vector<std::string> patchDirs = patchDirectories();
    status.patchDir = patchDirs.empty() ? "/data/GoldHEN/patches/xml" : patchDirs.front();
    for (const std::string& dir : patchDirs) {
        status.patchCount += countFilesIn(dir, nullptr);
    }

    return status;
}

std::vector<std::string> cheatDirectories() {
    static const char* kDirs[] = {
        "/data/GoldHEN/cheats/json",
        "/data/GoldHEN/cheats",
    };

    std::vector<std::string> out;
    for (const char* d : kDirs) {
        if (fs::isDir(d)) out.emplace_back(d);
    }
    return out;
}

std::vector<std::string> patchDirectories() {
    static const char* kDirs[] = {
        "/data/GoldHEN/patches/xml",
        "/data/GoldHEN/patches",
    };

    std::vector<std::string> out;
    for (const char* d : kDirs) {
        if (fs::isDir(d)) out.emplace_back(d);
    }
    return out;
}

}  // namespace rawui
