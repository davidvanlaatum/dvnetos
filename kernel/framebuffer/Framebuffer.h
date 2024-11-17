#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <cstdint>

#include "font.h"

namespace framebuffer {
    struct Size {
        uint32_t width;
        uint32_t height;
    };

    class Framebuffer {
    public:
        [[nodiscard]] Framebuffer();
#ifdef __KERNEL__
        void init();
        void postInit() const;
        [[nodiscard]] uint64_t getFramebufferVirtualAddress() const;
#endif
        void init(volatile uint32_t *fb, uint32_t width, uint32_t height, uint32_t pitch);
        Size textSize(const char *text) const;

        void drawTextAt(const char *text, uint32_t x, uint32_t y) const;
        void drawCharAt(char c, uint32_t x, uint32_t y) const;
        [[nodiscard]] Size getResolution() const;

    protected:
        volatile uint32_t *fb = nullptr;
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t pitch = 0;
        const PSF2_t *font;
    };

    extern Framebuffer defaultFramebuffer;
}

#endif //FRAMEBUFFER_H
