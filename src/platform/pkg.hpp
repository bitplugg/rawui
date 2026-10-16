// ============================================================================
//  RAW UI - package (.pkg) discovery and installation.
// ============================================================================
#pragma once

#include <string>
#include <vector>

namespace rawui {

struct PkgCandidate {
    std::string path;
    std::string name;
    uint64_t size = 0;
    std::string titleId;   // filled in lazily
    std::string contentId; // derived from the file name when it looks like one
    bool titleIdChecked = false;
};

struct InstallProgress {
    bool active = false;
    bool finished = false;
    bool failed = false;
    bool indeterminate = true;  // no numeric progress available
    int lastError = 0;
    uint32_t state = 0;
    uint32_t percent = 0;
    uint64_t progressBytes = 0;
    uint64_t totalBytes = 0;
    uint32_t remainingSec = 0;
    std::string titleId;
    std::string contentId;
    std::string message;
};

namespace pkg {

// Initialises libSceAppInstUtil once.
bool init();
bool initialised();

// Where do we look for .pkg files?
std::vector<std::string> searchRoots();

// Recursively finds *.pkg files (bounded so a huge drive cannot hang the UI).
std::vector<PkgCandidate> scan(const std::vector<std::string>& roots, int maxDepth = 3,
                               int maxFiles = 400);

// Reads the title id out of a package header, e.g. "CUSA12345".
std::string readTitleId(const std::string& pkgPath, bool* isApp = nullptr);

// The three-part content id (XX0000-XXXXXXXXX_00-XXXXXXXXXXXXXXXX) that the
// scene names packages after.  Returns an empty string when the file name does
// not follow that convention.
std::string contentIdFromFileName(const std::string& pkgPath);

// Is that title installed?
bool appExists(const std::string& titleId);

// Kicks off an install.  Returns the AppInstUtil result code.
int startInstall(const std::string& pkgPath);

// Polls the installer.  `titleId` is used to detect completion when no content
// id is known.
InstallProgress pollProgress(const std::string& contentId, const std::string& titleId);

}  // namespace pkg

}  // namespace rawui
