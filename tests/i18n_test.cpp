// ============================================================================
//  RAW UI - host-side test for the UI string table.
//
//  The interesting failures here are not "is the translation right" (nobody can
//  check that mechanically) but the structural ones the compiler cannot see:
//  an empty column, a printf template whose conversions do not line up between
//  the two languages, or the plural selector getting Russian wrong.
// ============================================================================

#include <cstdio>
#include <cstring>

#include "core/i18n.hpp"

using rawui::langFromName;
using rawui::langTag;
using rawui::Lang;
using rawui::Str;
using rawui::setLang;
using rawui::tr;
using rawui::trf;
using rawui::trn;

namespace {

int failures = 0;

#define CHECK(cond)                                                        \
    do {                                                                   \
        if (!(cond)) {                                                     \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);    \
            ++failures;                                                    \
        }                                                                  \
    } while (0)

#define CHECK_STR(actual, expected)                                        \
    do {                                                                   \
        const char* a_ = (actual);                                         \
        const char* e_ = (expected);                                       \
        if (std::strcmp(a_, e_) != 0) {                                    \
            std::printf("FAIL %s:%d: got \"%s\", want \"%s\"\n", __FILE__, \
                        __LINE__, a_, e_);                                 \
            ++failures;                                                    \
        }                                                                  \
    } while (0)

void testBasics() {
    setLang(Lang::English);
    CHECK_STR(tr(Str::kSettings), "Settings");

    setLang(Lang::Russian);
    CHECK_STR(tr(Str::kSettings), "Настройки");

    // The explicit-language overload must ignore the global language, because
    // the header and the rail can legitimately want both in one frame.
    CHECK_STR(tr(Str::kSettings, Lang::English), "Settings");
    CHECK_STR(tr(Str::kSettings, Lang::Russian), "Настройки");

    setLang(Lang::English);
}

void testNoEmptyEntries() {
    for (int i = 0; i < static_cast<int>(Str::Count); ++i) {
        const Str id = static_cast<Str>(i);
        // Note the length check rather than a first-character check: a few
        // entries deliberately start with a space so they can be concatenated
        // (" free of ").
        if (std::strlen(tr(id, Lang::English)) == 0) {
            std::printf("FAIL: empty English string at index %d\n", i);
            ++failures;
        }
        if (std::strlen(tr(id, Lang::Russian)) == 0) {
            std::printf("FAIL: empty Russian string at index %d\n", i);
            ++failures;
        }
    }
}

// Counts printf conversions in a template, skipping "%%".  Both language
// columns are passed through the same vsnprintf call, so a mismatch here is
// undefined behaviour at every call site - and nothing in the compiler can see
// across the X-macro to catch it.
int conversions(const char* s) {
    int n = 0;
    for (; *s != '\0'; ++s) {
        if (*s != '%') continue;
        if (*(s + 1) == '%') {
            ++s;
            continue;
        }
        ++n;
    }
    return n;
}

void testTemplatesAgree() {    for (int i = 0; i < static_cast<int>(Str::Count); ++i) {
        const Str id = static_cast<Str>(i);
        const int en = conversions(tr(id, Lang::English));
        const int ru = conversions(tr(id, Lang::Russian));
        if (en != ru) {
            std::printf("FAIL: index %d has %d conversions in English but %d in Russian\n", i, en,
                        ru);
            ++failures;
        }
    }
}

void testPrintf() {
    setLang(Lang::English);
    CHECK_STR(trf(Str::kAboutLine, "RAWU00001", "v0.1.0", "Homebrew shell"),
              "Title ID RAWU00001 · v0.1.0 · Homebrew shell");
    setLang(Lang::Russian);
    CHECK_STR(trf(Str::kInstallingName, "game.pkg"), "Установка game.pkg");
    CHECK_STR(trf(Str::kTitleIsInstalled, "GAME01234"), "Титул GAME01234 уже установлен.");

    // The result must be self-contained: the next trf() overwrites the buffer.
    CHECK_STR(trf(Str::kInstallingName, "a"), "Установка a");

    setLang(Lang::English);
    CHECK_STR(trf(Str::kTitleIsInstalled, "GAME01234"), "Title GAME01234 is already installed.");
}

void testPlural() {
    const auto files = [](int n) {
        return trn(Str::kUnitFileOne, Str::kUnitFileFew, Str::kUnitFileMany, n);
    };

    setLang(Lang::English);
    CHECK_STR(files(0), "files");
    CHECK_STR(files(1), "file");
    CHECK_STR(files(2), "files");
    CHECK_STR(files(11), "files");

    setLang(Lang::Russian);
    CHECK_STR(files(1), "файл");
    CHECK_STR(files(2), "файла");
    CHECK_STR(files(4), "файла");
    CHECK_STR(files(5), "файлов");
    CHECK_STR(files(11), "файлов");  // the 11..19 band beats the "ends in 1" rule
    CHECK_STR(files(21), "файл");
    CHECK_STR(files(22), "файла");
    CHECK_STR(files(25), "файлов");
    CHECK_STR(files(101), "файл");
    CHECK_STR(files(111), "файлов");
    CHECK_STR(files(114), "файлов");
    CHECK_STR(files(122), "файла");

    setLang(Lang::English);
}

void testLangTags() {
    CHECK(langFromName("ru") == Lang::Russian);
    CHECK(langFromName("RU") == Lang::Russian);
    CHECK(langFromName("en") == Lang::English);
    CHECK(langFromName("de") == Lang::English);
    CHECK(langFromName("") == Lang::English);
    CHECK(langFromName(nullptr) == Lang::English);

    CHECK_STR(langTag(Lang::Russian), "ru");
    CHECK_STR(langTag(Lang::English), "en");
}

}  // namespace

int main() {
    testBasics();
    testNoEmptyEntries();
    testTemplatesAgree();
    testPrintf();
    testPlural();
    testLangTags();

    // Leave the global language where the rest of the toolchain expects it.
    setLang(Lang::English);

    if (failures != 0) {
        std::printf("i18n test FAILED: %d check(s)\n", failures);
        return 1;
    }
    std::printf("all i18n tests passed (%d strings, en + ru)\n",
                static_cast<int>(Str::Count));
    return 0;
}
