#include "core/i18n.hpp"

#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace rawui {
namespace {

struct Entry {
    const char* en;
    const char* ru;
};

// Built straight from the X-macro, so the table can never drift out of sync
// with the enum above it.
const Entry kTable[] = {
#define RAWUI_UI_STR(name, en, ru) {en, ru},
    RAWUI_UI_STRINGS(RAWUI_UI_STR)
#undef RAWUI_UI_STR
};

static_assert(sizeof(kTable) / sizeof(kTable[0]) == static_cast<size_t>(Str::Count),
              "i18n table does not match Str enum");

Lang g_lang = Lang::English;

const Entry* entryFor(Str id) {
    const size_t i = static_cast<size_t>(id);
    if (i >= static_cast<size_t>(Str::Count)) return nullptr;
    return &kTable[i];
}

}  // namespace

const char* tr(Str id, Lang lang) {
    const Entry* e = entryFor(id);
    if (!e) return "";
    return lang == Lang::Russian ? e->ru : e->en;
}

const char* tr(Str id) { return tr(id, g_lang); }

const char* trf(Str id, ...) {
    // The template comes from the table, so both language columns must carry
    // the same conversion specifiers in the same order - enforced by eye at
    // review time, since C++ cannot check across an X-macro.
    //
    // A static buffer is fine here: every caller consumes the result before the
    // next trf() runs, and it keeps the UI free of per-frame allocations.
    static char buf[kTrBuf];

    const Entry* e = entryFor(id);
    if (!e) {
        buf[0] = '\0';
        return buf;
    }

    va_list args;
    va_start(args, id);
    std::vsnprintf(buf, sizeof(buf), g_lang == Lang::Russian ? e->ru : e->en, args);
    va_end(args);
    return buf;
}

const char* trn(Str one, Str few, Str many, int n) {
    if (g_lang == Lang::Russian) {
        // Standard Slavic selection.  The 11..19 band comes first because
        // "11 файлов" must win over the "ends in 1" rule that "21 файл"
        // satisfies.
        const int mod100 = n % 100;
        const int mod10 = n % 10;
        if (mod100 < 11 || mod100 > 19) {
            if (mod10 == 1) return tr(one);
            if (mod10 >= 2 && mod10 <= 4) return tr(few);
        }
        return tr(many);
    }
    return tr(n == 1 ? one : many);
}

Lang lang() { return g_lang; }

void setLang(Lang value) { g_lang = value; }

const char* langName(Lang value) {
    return value == Lang::Russian ? "Русский" : "English";
}

const char* langTag(Lang value) { return value == Lang::Russian ? "ru" : "en"; }

Lang langFromName(const char* tag) {
    if (tag && (std::strcmp(tag, "ru") == 0 || std::strcmp(tag, "RU") == 0)) {
        return Lang::Russian;
    }
    return Lang::English;
}

}  // namespace rawui
