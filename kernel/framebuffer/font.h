#ifndef FONT_H
#define FONT_H

#include <cstdint>

namespace framebuffer {
    typedef struct {
        uint32_t magic;
        uint32_t version;
        uint32_t headersize;
        uint32_t flags;
        uint32_t numglyph;
        uint32_t bytesperglyph;
        uint32_t height;
        uint32_t width;
        uint8_t glyphs;
    } __attribute__((packed)) PSF2_t;
}

#endif //FONT_H
