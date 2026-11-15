#pragma once
// Host-side Surface for Pocket Pets previews: a 1-bit bitmap plus Noto Sans
// text through stb_truetype (the fonts the device firmware is built from).

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "pocketpets/PetCanvas.h"
#include "stb_truetype.h"

class HostSurface : public pp::Surface {
 public:
  HostSurface(int w, int h) : w_(w), h_(h), px_(w * h, 0) {}

  static bool loadFonts(const char* regular, const char* bold) {
    return load(regular, regData(), regFont()) && load(bold, boldData(), boldFont());
  }

  int width() const override { return w_; }
  int height() const override { return h_; }

  void span(int x0, int x1, int y, pp::Ink ink) override {
    for (int x = x0; x <= x1; ++x) px_[y * w_ + x] = on(x, y, ink);
  }

  int textWidth(const char* s, int size, bool bold) override {
    stbtt_fontinfo& f = bold ? boldFont() : regFont();
    const float sc = stbtt_ScaleForPixelHeight(&f, static_cast<float>(size));
    float x = 0;
    for (const char* p = s; *p; ++p) {
      int adv, lsb;
      stbtt_GetCodepointHMetrics(&f, static_cast<unsigned char>(*p), &adv, &lsb);
      x += adv * sc;
      if (p[1])
        x += sc * stbtt_GetCodepointKernAdvance(&f, static_cast<unsigned char>(*p), static_cast<unsigned char>(p[1]));
    }
    return static_cast<int>(x + 0.5f);
  }

  void text(int x, int y, const char* s, int size, bool bold, pp::Ink ink) override {
    stbtt_fontinfo& f = bold ? boldFont() : regFont();
    const float sc = stbtt_ScaleForPixelHeight(&f, static_cast<float>(size));
    int asc, desc, gap;
    stbtt_GetFontVMetrics(&f, &asc, &desc, &gap);
    const int baseline = y + static_cast<int>(asc * sc + 0.5f);
    float pen = static_cast<float>(x);
    for (const char* p = s; *p; ++p) {
      const int cp = static_cast<unsigned char>(*p);
      int adv, lsb;
      stbtt_GetCodepointHMetrics(&f, cp, &adv, &lsb);
      int gw, gh, xo, yo;
      unsigned char* bmp = stbtt_GetCodepointBitmap(&f, sc, sc, cp, &gw, &gh, &xo, &yo);
      for (int j = 0; j < gh; ++j)
        for (int i = 0; i < gw; ++i) {
          if (bmp[j * gw + i] < 110) continue;
          const int X = static_cast<int>(pen) + xo + i, Y = baseline + yo + j;
          if (X < 0 || Y < 0 || X >= w_ || Y >= h_) continue;
          px_[Y * w_ + X] = on(X, Y, ink);
        }
      stbtt_FreeBitmap(bmp, nullptr);
      pen += adv * sc;
      if (p[1]) pen += sc * stbtt_GetCodepointKernAdvance(&f, cp, static_cast<unsigned char>(p[1]));
    }
  }

  bool writePgm(const std::string& path) const {
    FILE* fp = fopen(path.c_str(), "wb");
    if (!fp) return false;
    fprintf(fp, "P5\n%d %d\n255\n", w_, h_);
    for (uint8_t v : px_) fputc(v ? 0 : 255, fp);
    fclose(fp);
    return true;
  }

 private:
  int w_, h_;
  std::vector<uint8_t> px_;

  // Approximates the panel's dither patterns for the gray inks.
  static uint8_t on(int x, int y, pp::Ink ink) {
    switch (ink) {
      case pp::Ink::Black:
        return 1;
      case pp::Ink::White:
        return 0;
      case pp::Ink::Gray:
        return ((x & 1) == 0 && (y & 1) == 0) ? 1 : 0;  // 25%
      case pp::Ink::Dark:
        return ((x + y) & 1) == 0 ? 1 : 0;  // 50%
    }
    return 0;
  }

  static std::vector<unsigned char>& regData() {
    static std::vector<unsigned char> d;
    return d;
  }
  static std::vector<unsigned char>& boldData() {
    static std::vector<unsigned char> d;
    return d;
  }
  static stbtt_fontinfo& regFont() {
    static stbtt_fontinfo f;
    return f;
  }
  static stbtt_fontinfo& boldFont() {
    static stbtt_fontinfo f;
    return f;
  }
  static bool load(const char* path, std::vector<unsigned char>& data, stbtt_fontinfo& info) {
    FILE* fp = fopen(path, "rb");
    if (!fp) return false;
    fseek(fp, 0, SEEK_END);
    data.resize(ftell(fp));
    fseek(fp, 0, SEEK_SET);
    const size_t n = fread(data.data(), 1, data.size(), fp);
    fclose(fp);
    return n == data.size() && stbtt_InitFont(&info, data.data(), 0);
  }
};
