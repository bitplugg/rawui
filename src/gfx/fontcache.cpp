#include "gfx/fontcache.hpp"

#include <cstdio>

#include "core/log.hpp"

namespace rawui {

std::vector<uint8_t> FontCache::readFile(const std::string& path, std::string& errors) {
    std::vector<uint8_t> data;

    std::FILE* fh = std::fopen(path.c_str(), "rb");
    if (!fh) {
        errors += "cannot open " + path + "\n";
        return data;
    }

    std::fseek(fh, 0, SEEK_END);
    const long size = std::ftell(fh);
    std::fseek(fh, 0, SEEK_SET);

    if (size > 0) {
        data.resize(static_cast<size_t>(size));
        const size_t got = std::fread(data.data(), 1, data.size(), fh);
        data.resize(got);
    }
    std::fclose(fh);

    RU_LOG("[fontcache] %s: %zu bytes", path.c_str(), data.size());
    return data;
}

bool FontCache::init(const std::string& assetDir) {
    textData_ = readFile(assetDir + "/RobotoFlex.ttf", errors_);
    iconData_ = readFile(assetDir + "/MaterialSymbolsRounded.ttf", errors_);

    if (textData_.empty()) errors_ += "RobotoFlex.ttf missing\n";
    if (iconData_.empty()) errors_ += "MaterialSymbolsRounded.ttf missing\n";
    return ready();
}

void FontCache::shutdown() {
    entries_.clear();
    textData_.clear();
    iconData_.clear();
}

Font& FontCache::text(float size, int weight) {
    // Quantise the size so tiny layout changes do not explode the cache.
    const float q = std::round(size * 2.0f) / 2.0f;
    const int w = clampi((weight / 50) * 50, 100, 900);

    for (Entry& e : entries_) {
        if (e.kind == 0 && e.size == q && e.weight == w && e.font) return *e.font;
    }

    Entry e;
    e.kind = 0;
    e.size = q;
    e.weight = w;
    e.font = std::make_unique<Font>();

    if (!e.font->load(textData_.data(), textData_.size(), q)) {
        RU_LOG("[fontcache] text font load failed (%.1f / %d)", q, w);
    } else {
        e.font->setVariation("wght", static_cast<float>(w));
    }

    entries_.push_back(std::move(e));
    return *entries_.back().font;
}

Font& FontCache::icon(float size, bool filled) {
    const float q = std::round(size);

    for (Entry& e : entries_) {
        if (e.kind == 1 && e.size == q && e.filled == filled && e.font) return *e.font;
    }

    Entry e;
    e.kind = 1;
    e.size = q;
    e.filled = filled;
    e.font = std::make_unique<Font>();

    if (!e.font->load(iconData_.data(), iconData_.size(), q)) {
        RU_LOG("[fontcache] icon font load failed (%.0f)", q);
    } else {
        e.font->setVariation("wght", 400.0f);
        e.font->setVariation("opsz", 24.0f);
        e.font->setVariation("GRAD", 0.0f);
        e.font->setVariation("FILL", filled ? 1.0f : 0.0f);
    }

    entries_.push_back(std::move(e));
    return *entries_.back().font;
}

}  // namespace rawui
