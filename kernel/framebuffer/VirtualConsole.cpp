#include <cstring>
#include <cstdarg>
#include <cstdio>
#include "Framebuffer.h"
#include "VirtualConsole.h"
#ifdef __KERNEL__
#include "serial/Serial.h"
#endif

namespace framebuffer {
#define MAX_RES_WIDTH 1280
#define MAX_RES_HEIGHT 1024
  constexpr auto bufferSize = (MAX_RES_WIDTH / 8) * (MAX_RES_HEIGHT / 16);
  char buffer[bufferSize];
  VirtualConsole defaultVirtualConsole;

#ifdef __KERNEL__
  void VirtualConsole::init() {
    defaultFramebuffer.init();
    init(&defaultFramebuffer);
    defaultFramebuffer.postInit();
    //
    // appendText("Before accessing UART base\n");
    //
    // // Memory barrier to ensure all previous memory operations are completed
    // asm volatile("dmb sy" ::: "memory");
    //
    // // Check if UART base address is accessible
    // if (uart_base == nullptr) {
    //     appendText("UART base address is null\n");
    //     return;
    // }
    //
    // appendText("UART base address is not null\n");
    //
    // // Add debug output before accessing uart_base[0]
    // appendText("About to access uart_base[0]\n");
    //
    // volatile uint32_t test = uart_base[0];
    //
    // // Add debug output after accessing uart_base[0]
    // appendText("Accessed uart_base[0]\n");
    //
    // if (test == 0xFFFFFFFF) {
    //     appendText("UART not found\n");
    // } else {
    //     appendText("UART found\n");
    // }
    //
    // // UART initialization
    // uart_base[1] = 0x00; // Disable all interrupts
    // uart_base[2] = 0x07; // Enable FIFO, clear them, with 1-byte threshold
    // uart_base[3] = 0x80; // Enable DLAB (set baud rate divisor)
    // uart_base[0] = 0x01; // Set divisor to 1 (lo byte) 115200 baud
    // uart_base[1] = 0x00; //                  (hi byte)
    // uart_base[3] = 0x03; // 8 bits, no parity, one stop bit
    // uart_base[4] = 0x03; // RTS/DSR set
  }
#endif

  void VirtualConsole::init(Framebuffer *framebuffer) {
    for (char &i: buffer) {
      i = ' ';
    }
    this->framebuffer = framebuffer;
    fontSize = framebuffer->textSize(" ");
    auto [resWidth, resHeight] = framebuffer->getResolution();
    if (resWidth > MAX_RES_WIDTH) {
      resWidth = MAX_RES_WIDTH;
    }
    if (resHeight > MAX_RES_HEIGHT) {
      resHeight = MAX_RES_HEIGHT;
    }
    lineLength = resWidth / fontSize.width;
    lineCount = resHeight / fontSize.height;
    initComplete = true;
    updateScreen();
  }

  void VirtualConsole::updateScreen() const {
    if (initComplete) {
      for (int x = 0; x < lineLength; x++) {
        for (int y = 0; y < lineCount; y++) {
          const auto i = (y * lineLength) + x;
          if (i < bufferSize) {
            framebuffer->drawCharAt(buffer[i], x * fontSize.width, y * fontSize.height);
          }
        }
      }
    }
  }

  void VirtualConsole::appendText(const char *text) {
    writeToSerial(text);
    while (*text) {
      if (*text == '\n') {
        for (; cursorX < lineLength; cursorX++) {
          buffer[(cursorY * lineLength) + cursorX] = ' ';
        }
      } else {
        buffer[(cursorY * lineLength) + cursorX] = *text;
      }
      cursorX++;
      text++;
      if (cursorX >= lineLength) {
        cursorX = 0;
        cursorY++;
        if (cursorY >= lineCount) {
          memmove(buffer, buffer + lineLength, lineLength * (lineCount - 1));
          memset(buffer + lineLength * (lineCount - 1), ' ', lineLength);
          cursorY--;
        }
      }
    }
    updateScreen();
  }

  void VirtualConsole::appendFormattedText(const char *format, ...) {
    char buf[1024];
    va_list args;
    va_start(args, format);
    kvsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    appendText(buf);
  }

  void VirtualConsole::writeToSerial(const char *text) {
#ifdef __KERNEL__
    serial::defaultSerial.write(text, strlen(text));
#else
    // noop
#endif
  }
}
