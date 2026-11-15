#pragma once
// GfxRenderer as a Pocket Pets drawing surface (device only).

#include <GfxRenderer.h>

#include "fontIds.h"
#include "pocketpets/PetCanvas.h"

namespace pp {

// GfxRenderer as a Pocket Pets surface. Text sizes map to the closest built-in
// font; sizes past the largest font are drawn scaled.
class DeviceSurface : public Surface {
 public:
  explicit DeviceSurface(GfxRenderer& r) : r_(r) {
    const int ids[] = {SMALL_FONT_ID,       UI_10_FONT_ID,       UI_12_FONT_ID,      NOTOSANS_12_FONT_ID,
                       NOTOSANS_14_FONT_ID, NOTOSANS_16_FONT_ID, NOTOSANS_18_FONT_ID};
    for (int id : ids) {
      const int lh = r_.getLineHeight(id);
      if (lh <= 0) continue;
      int i = count_;
      while (i > 0 && fonts_[i - 1].lh > lh) {
        fonts_[i] = fonts_[i - 1];
        --i;
      }
      fonts_[i] = {id, lh};
      ++count_;
    }
  }

  int width() const override { return r_.getScreenWidth(); }
  int height() const override { return r_.getScreenHeight(); }

  void span(int x0, int x1, int y, Ink ink) override {
    const int w = x1 - x0 + 1;
    switch (ink) {
      case Ink::Black:
        r_.fillRect(x0, y, w, 1, true);
        break;
      case Ink::White:
        r_.fillRect(x0, y, w, 1, false);
        break;
      case Ink::Gray:
        r_.fillRectDither(x0, y, w, 1, Color::LightGray);
        break;
      case Ink::Dark:
        r_.fillRectDither(x0, y, w, 1, Color::DarkGray);
        break;
    }
  }

  void text(int x, int y, const char* s, int size, bool bold, Ink ink) override {
    int scale = 256;
    const int id = pick(size, scale);
    if (id == 0) return;
    const auto style = bold ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR;
    if (scale == 256) {
      r_.drawText(id, x, y, s, ink != Ink::White, style);
      return;
    }
    const int baseline = y + r_.getFontAscenderSize(id) * scale / 256;
    for (const char* p = s; *p; ++p) {
      x += r_.drawScaledCodepoint(id, static_cast<unsigned char>(*p), style, x, baseline, scale);
    }
  }

  int textWidth(const char* s, int size, bool bold) override {
    int scale = 256;
    const int id = pick(size, scale);
    if (id == 0) return 0;
    const auto style = bold ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR;
    if (scale == 256) return r_.getTextWidth(id, s, style);
    int64_t fp = 0;
    for (const char* p = s; *p; ++p) {
      int32_t adv = 0;
      int top = 0;
      if (r_.getCodepointMetrics(id, static_cast<unsigned char>(*p), style, adv, top)) fp += adv;
    }
    return static_cast<int>(fp * scale / 256 / 16);
  }

 private:
  struct Font {
    int id;
    int lh;
  };
  GfxRenderer& r_;
  Font fonts_[8] = {};
  int count_ = 0;

  int pick(int size, int& scale) const {
    if (count_ == 0) return 0;
    const Font& big = fonts_[count_ - 1];
    if (size > big.lh * 115 / 100) {
      scale = size * 256 / big.lh;
      return big.id;
    }
    int best = fonts_[0].id;
    for (int i = 0; i < count_; ++i) {
      if (fonts_[i].lh <= size * 110 / 100) best = fonts_[i].id;
    }
    scale = 256;
    return best;
  }
};

}  // namespace pp
