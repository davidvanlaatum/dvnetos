#ifndef VIRTUALCONSOLE_H
#define VIRTUALCONSOLE_H

#include <cstddef>

#include "Framebuffer.h"

#define kprintf(msg, ...) framebuffer::defaultVirtualConsole.appendFormattedText(msg, __VA_ARGS__)
#define kprint(msg) framebuffer::defaultVirtualConsole.appendText(msg)

namespace framebuffer {
  class VirtualConsole {
  public:
#ifdef __KERNEL__
    void init();
#endif
    void init(Framebuffer *framebuffer);

    void appendText(const char *text);
    void appendFormattedText(const char *format, ...) __attribute__((format(printf, 2, 3)));

  protected:
    size_t cursorX = 0;
    size_t cursorY = 0;

    Framebuffer *framebuffer = nullptr;
    size_t lineLength = 80;
    size_t lineCount = 10;
    Size fontSize{};
    bool initComplete = false;

    void updateScreen() const;
    void writeToSerial(const char *text);
  };
#ifdef __KERNEL__
  extern VirtualConsole defaultVirtualConsole;
#endif
} // namespace framebuffer

#endif // VIRTUALCONSOLE_H
