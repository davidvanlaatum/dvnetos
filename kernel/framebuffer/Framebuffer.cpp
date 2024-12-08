#include <cstdio>
#include <cstring>

#include "Framebuffer.h"
#include "font.h"
#include "VirtualConsole.h"
#include "utils/panic.h"

#ifdef __KERNEL__
#include <cstdint>
#include <limine.h>

namespace {
    __attribute__((used, section(".limine_requests")))
    volatile limine_framebuffer_request framebuffer_request = {
        .id = LIMINE_FRAMEBUFFER_REQUEST,
        .revision = 0,
        .response = nullptr
    };
}
#endif

namespace framebuffer {
#include "font_data.h"
  Framebuffer defaultFramebuffer;

  Framebuffer::Framebuffer() {
    this->font = reinterpret_cast<const PSF2_t *>(font_data);
  }

#ifdef __KERNEL__
    void Framebuffer::init() {
        const limine_framebuffer *framebuffer = framebuffer_request.response->framebuffers[0];
        if (framebuffer_request.response == nullptr || framebuffer_request.response->framebuffer_count < 1) {
            kpanic("No framebuffer found");
        }
        volatile auto *fb_ptr = static_cast<volatile uint32_t *>(framebuffer->address);
        init(fb_ptr, framebuffer->width, framebuffer->height, framebuffer->pitch);
    }

    void Framebuffer::postInit() const {
        const limine_framebuffer *framebuffer = framebuffer_request.response->framebuffers[0];
        kprintf("Framebuffer found at %p\n", framebuffer->address);
        kprintf("fb: %dx%d %d bpp, %lu modes\n", width, height, framebuffer->bpp, framebuffer->mode_count);
        for (int i = 0; i < framebuffer->mode_count; i++) {
            const auto mode = framebuffer->modes[i];
            kprintf("fb mode %d: %lux%lu %d bpp\n", i, mode->width, mode->height, mode->bpp);
        }
    }

    uint64_t Framebuffer::getFramebufferVirtualAddress() const {
        return reinterpret_cast<uint64_t>(fb);
    }
#endif

  void Framebuffer::init(volatile uint32_t *fb, const uint32_t width, const uint32_t height, const uint32_t pitch) {
    this->fb = fb;
    this->width = width;
    this->height = height;
    this->pitch = pitch;
    for (uint32_t i = 0; i < width * height; i++) {
      fb[i] = 0xFF000000;
    }
  }

  Size Framebuffer::textSize(const char *text) const {
    Size s{};
    s.width = strlen(text) * this->font->width;
    s.height = this->font->height;
    return s;
  }

  void Framebuffer::drawTextAt(const char *text, uint32_t x, uint32_t y) const {
    while (*text) {
      drawCharAt(*text, x, y);
      x += font->width;
      if (x > width - font->width) {
        x = 0;
        y += font->height;
      }
      text++;
    }
  }

  void Framebuffer::drawCharAt(const char c, const uint32_t x, uint32_t y) const {
    const auto bpl = (font->width + 7) / 8;
    const unsigned char *glyph = font_data + font->headersize + (c > 0 && c < font->numglyph ? c : 0) *
                                 font->bytesperglyph;
    for (int i = 0; i < font->height; i++) {
      auto line = y * pitch / 4 + x;
      uint32_t mask = 1 << (font->width - 1);
      for (int j = 0; j < font->width; j++) {
        fb[line] = (*glyph & mask) ? 0xFFFFFFFF : 0xFF000000;
        mask >>= 1;
        line++;
      }
      glyph += bpl;
      y++;
    }
  }

  Size Framebuffer::getResolution() const {
    return {width, height};
  }
}
