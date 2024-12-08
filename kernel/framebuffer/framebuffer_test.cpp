#include "Framebuffer.h"
#include <fstream>
#include <gtest/gtest.h>
#include <vector>
#include "VirtualConsole.h"

void dumpFramebufferToFile(const std::vector<uint32_t> &framebuffer, const std::string &filename) {
  if (std::ofstream file(filename, std::ios::binary); file.is_open()) {
    file.write(reinterpret_cast<const char *>(framebuffer.data()),
               static_cast<std::streamsize>(framebuffer.size() * sizeof(uint32_t)));
    file.close();
  } else {
    throw std::runtime_error("Unable to open file for writing");
  }
}

TEST(Framebuffer, draw_text) {
  constexpr auto width = 1024;
  constexpr auto height = 768;
  std::vector<uint32_t> framebuffer;
  framebuffer.resize(width * height);

  framebuffer::Framebuffer fb;

  fb.init(framebuffer.data(), width, height, width * 4);

  framebuffer::VirtualConsole vc;
  vc.init(&fb);

  char buf[257] = {};
  for (int i = 1; i < 256; i++) {
    buf[i - 1] = static_cast<char>(i);
  }

  vc.appendText(buf);
  vc.appendText("\n");
  for (int i = 0; i < 100; i++) {
    snprintf(buf, sizeof(buf), "\n%d hi all", i);
    vc.appendText(buf);
  }

  dumpFramebufferToFile(framebuffer, "framebuffer_dump.bin");

  // EXPECT_TRUE(false) << fb.textSize(" ").height << " " << fb.textSize(" ").width;
}
