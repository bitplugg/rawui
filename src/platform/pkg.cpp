#include "platform/pkg.hpp"

#include <cctype>
#include <cstring>

#include <dirent.h>
#include <orbis/AppInstUtil.h>
#include <sys/stat.h>

#include "core/log.hpp"
#include "platform/fs.hpp"

namespace rawui {
namespace pkg {
namespace {

bool g_initialised = false;
int g_lastInitResult = 0;

void collect(const std::string& dir, int depth, int maxDepth, int maxFiles,
             std::vector<PkgCandidate>& out) {
    if (depth > maxDepth) return;
    if (static_cast<int>(out.size()) >= maxFiles) return;

    DIR* d = opendir(dir.c_str());
    if (!d) return;

    struct dirent* de = nullptr;
    while ((de = readdir(d)) != nullptr) {
        if (fs::isDotEntry(de->d_name)) continue;
        if (static_cast<int>(out.size()) >= maxFiles) break;

        const std::string path = joinPath(dir, de->d_name);

        struct stat st;
        if (lstat(path.c_str(), &st) != 0) continue;

        if (S_ISDIR(st.st_mode)) {
            collect(path, depth + 1, maxDepth, maxFiles, out);
        } else if (fileExtension(path) == "pkg") {
            PkgCandidate c;
            c.path = path;
            c.name = de->d_name;
            c.size = static_cast<uint64_t>(st.st_size);
            c.contentId = contentIdFromFileName(path);
            out.push_back(std::move(c));
        }
    }
    closedir(d);
}

}  // namespace

bool init() {
    if (g_initialised) return true;

    g_lastInitResult = sceAppInstUtilInitialize();
    RU_LOG("[pkg] sceAppInstUtilInitialize -> 0x%08X", g_lastInitResult);

    // A non-zero result can simply mean "already initialised", so treat the
    // service as usable either way and let individual calls report failures.
    g_initialised = true;
    return true;
}

bool initialised() { return g_initialised; }

std::vector<std::string> searchRoots() {
    std::vector<std::string> roots;

    // USB drives first: that is where people keep packages.
    for (int i = 0; i < 8; ++i) {
        const std::string p = "/mnt/usb" + std::to_string(i);
        if (fs::isDir(p)) roots.push_back(p);
    }
    for (const char* p : {"/mnt/ext0", "/mnt/ext1", "/data/pkg", "/data", "/user/data"}) {
        if (fs::isDir(p)) roots.emplace_back(p);
    }

    return roots;
}

std::vector<PkgCandidate> scan(const std::vector<std::string>& roots, int maxDepth,
                               int maxFiles) {
    std::vector<PkgCandidate> out;
    for (const std::string& root : roots) {
        collect(root, 0, maxDepth, maxFiles, out);
    }

    std::sort(out.begin(), out.end(),
              [](const PkgCandidate& a, const PkgCandidate& b) { return a.name < b.name; });
    return out;
}

std::string readTitleId(const std::string& pkgPath, bool* isApp) {
    char titleId[16] = {};
    int32_t appFlag = 0;

    const int rc = sceAppInstUtilGetTitleIdFromPkg(pkgPath.c_str(), titleId, &appFlag);
    if (rc != 0) {
        RU_LOG("[pkg] GetTitleIdFromPkg(%s) -> 0x%08X", pkgPath.c_str(), rc);
        if (isApp) *isApp = false;
        return {};
    }

    if (isApp) *isApp = (appFlag != 0);
    return std::string(titleId);
}

std::string contentIdFromFileName(const std::string& pkgPath) {
    std::string name = baseName(pkgPath);
    if (fileExtension(name) == "pkg") {
        name = name.substr(0, name.size() - 4);
    }

    // XX0000-XXXXXXXXX_00-XXXXXXXXXXXXXXXX
    if (name.size() != 36) return {};
    if (!std::isalpha(static_cast<unsigned char>(name[0])) ||
        !std::isalpha(static_cast<unsigned char>(name[1])))
        return {};
    for (int i = 2; i < 6; ++i) {
        if (!std::isdigit(static_cast<unsigned char>(name[i]))) return {};
    }
    if (name[6] != '-') return {};
    for (int i = 7; i < 16; ++i) {
        if (!std::isalnum(static_cast<unsigned char>(name[i]))) return {};
    }
    if (name[16] != '_') return {};
    for (int i = 17; i < 19; ++i) {
        if (!std::isdigit(static_cast<unsigned char>(name[i]))) return {};
    }
    if (name[19] != '-') return {};
    for (int i = 20; i < 36; ++i) {
        if (!std::isalnum(static_cast<unsigned char>(name[i]))) return {};
    }

    return name;
}

bool appExists(const std::string& titleId) {
    if (titleId.empty()) return false;

    int32_t exists = 0;
    if (sceAppInstUtilAppExists(titleId.c_str(), &exists) != 0) return false;
    return exists != 0;
}

int startInstall(const std::string& pkgPath) {
    init();

    if (!fs::exists(pkgPath)) {
        RU_LOG("[pkg] install: %s does not exist", pkgPath.c_str());
        return -1;
    }

    // Ask the system to accept an overwrite if the title is already installed.
    const int prepRc = sceAppInstUtilAppPrepareOverwritePkg(pkgPath.c_str());
    RU_LOG("[pkg] PrepareOverwritePkg -> 0x%08X", prepRc);

    const int rc = sceAppInstUtilAppInstallPkg(pkgPath.c_str(), nullptr);
    RU_LOG("[pkg] InstallPkg(%s) -> 0x%08X", pkgPath.c_str(), rc);
    return rc;
}

InstallProgress pollProgress(const std::string& contentId, const std::string& titleId) {
    InstallProgress p;
    p.contentId = contentId;
    p.titleId = titleId;
    p.active = true;

    if (!contentId.empty()) {
        uint32_t state = 0;
        uint32_t progress = 0;
        uint32_t progressSize = 0;
        uint32_t totalSize = 0;
        uint32_t restSec = 0;

        const int rc = sceAppInstUtilGetInstallProgressInfo(contentId.c_str(), &state,
                                                           &progress, &progressSize,
                                                           &totalSize, &restSec);
        if (rc == 0) {
            p.state = state;
            p.percent = progress;
            p.progressBytes = progressSize;
            p.totalBytes = totalSize;
            p.remainingSec = restSec;
            p.indeterminate = (totalSize == 0);

            if (progress >= 100) {
                p.active = false;
                p.finished = true;
                p.message = "Installed";
            } else {
                p.message = std::to_string(progress) + "%";
            }
            return p;
        }
    }

    // No content id (or the call failed): fall back to asking whether the title
    // exists yet.  That is enough to drive an indeterminate progress view.
    p.indeterminate = true;
    p.message = "Installing...";

    if (!titleId.empty() && appExists(titleId)) {
        p.active = false;
        p.finished = true;
        p.percent = 100;
        p.message = "Installed";
    }

    return p;
}

}  // namespace pkg
}  // namespace rawui
