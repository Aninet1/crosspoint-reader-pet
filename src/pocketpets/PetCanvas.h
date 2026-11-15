#pragma once
// Small vector canvas for Pocket Pets. Every shape is filled as horizontal
// spans, so the same drawing code runs on the device (GfxRenderer spans) and in
// the host preview tool (an in-memory bitmap).

#include <cstdint>

namespace pp {

enum class Ink : uint8_t { White, Black, Gray, Dark };

struct Box {
  int x = 0, y = 0, w = 0, h = 0;
  bool contains(int px, int py) const { return px >= x && py >= y && px < x + w && py < y + h; }
  int cx() const { return x + w / 2; }
  int cy() const { return y + h / 2; }
  Box inset(int d) const { return Box{x + d, y + d, w - 2 * d, h - 2 * d}; }
};

class Surface {
 public:
  virtual ~Surface() = default;
  virtual int width() const = 0;
  virtual int height() const = 0;
  // Fill pixels x0..x1 (inclusive) on row y.
  virtual void span(int x0, int x1, int y, Ink ink) = 0;
  // Text with its top-left at (x, y). size is the cap-to-descender height in px.
  virtual void text(int x, int y, const char* s, int size, bool bold, Ink ink) = 0;
  virtual int textWidth(const char* s, int size, bool bold) = 0;
};

// A shape used by outlined groups.
struct Shape {
  enum Kind : uint8_t { Ellipse, RotEllipse, RoundRect, Capsule, Poly };
  Kind kind = Ellipse;
  int a = 0, b = 0, c = 0, d = 0, e = 0;  // meaning depends on kind
  float angle = 0;                        // RotEllipse, degrees
  int n = 0;
  int px[8] = {}, py[8] = {};

  static Shape ellipse(int cx, int cy, int rx, int ry);
  static Shape circle(int cx, int cy, int r) { return ellipse(cx, cy, r, r); }
  static Shape rotEllipse(int cx, int cy, int rx, int ry, float deg);
  static Shape roundRect(int x, int y, int w, int h, int r);
  static Shape capsule(int x0, int y0, int x1, int y1, int r);
  static Shape poly(const int* xs, const int* ys, int count);
};

class Canvas {
 public:
  explicit Canvas(Surface& s) : s_(s) {}
  Surface& surface() { return s_; }
  int width() const { return s_.width(); }
  int height() const { return s_.height(); }

  void clear(Ink ink = Ink::White);
  void rect(int x, int y, int w, int h, Ink ink);
  void ellipse(int cx, int cy, int rx, int ry, Ink ink);
  void circle(int cx, int cy, int r, Ink ink) { ellipse(cx, cy, r, r, ink); }
  void rotEllipse(int cx, int cy, int rx, int ry, float deg, Ink ink);
  void roundRect(int x, int y, int w, int h, int r, Ink ink);
  void capsule(int x0, int y0, int x1, int y1, int r, Ink ink);
  void poly(const int* xs, const int* ys, int n, Ink ink);
  void shape(const Shape& sh, int grow, Ink ink);

  // Outlined union: every shape grown by `stroke` in black, then every shape
  // filled. Overlapping shapes merge into one silhouette with one outline.
  void outlined(const Shape* shapes, int n, int stroke, Ink fill = Ink::White);
  void outlined(const Shape& sh, int stroke, Ink fill = Ink::White) { outlined(&sh, 1, stroke, fill); }

  void frame(int x, int y, int w, int h, int r, int t, Ink ink, Ink fill = Ink::White);
  // Stroked arc from a0 to a1 degrees (0 = right, 90 = down).
  void arc(int cx, int cy, int r, float a0, float a1, int t, Ink ink);
  void line(int x0, int y0, int x1, int y1, int t, Ink ink) { capsule(x0, y0, x1, y1, t / 2 > 0 ? t / 2 : 1, ink); }
  void heart(int cx, int cy, int size, Ink ink);
  void sparkle(int cx, int cy, int r, Ink ink);

  // Text helpers.
  int textWidth(const char* t, int size, bool bold = false) { return s_.textWidth(t, size, bold); }
  void text(int x, int y, const char* t, int size, bool bold = false, Ink ink = Ink::Black) {
    s_.text(x, y, t, size, bold, ink);
  }
  void textCentered(int cx, int y, const char* t, int size, bool bold = false, Ink ink = Ink::Black);
  // Word-wraps into at most maxLines lines centred on cx. Returns lines drawn.
  int textWrapped(int cx, int y, int maxW, const char* t, int size, int lineGap, int maxLines, bool bold = false,
                  Ink ink = Ink::Black) {
    return wrap(cx, y, maxW, t, size, lineGap, maxLines, bold, ink, true);
  }
  int countLines(int maxW, const char* t, int size, int maxLines, bool bold = false) {
    return wrap(0, 0, maxW, t, size, 0, maxLines, bold, Ink::Black, false);
  }

 private:
  Surface& s_;
  void hspan(int x0, int x1, int y, Ink ink);
  int wrap(int cx, int y, int maxW, const char* t, int size, int lineGap, int maxLines, bool bold, Ink ink, bool draw);
};

}  // namespace pp
