#include "PetCanvas.h"

#include <cmath>
#include <cstring>

namespace pp {

namespace {
constexpr float kPi = 3.14159265f;
inline int iround(float v) { return static_cast<int>(v < 0 ? v - 0.5f : v + 0.5f); }
}  // namespace

Shape Shape::ellipse(int cx, int cy, int rx, int ry) {
  Shape s;
  s.kind = Ellipse;
  s.a = cx;
  s.b = cy;
  s.c = rx;
  s.d = ry;
  return s;
}

Shape Shape::rotEllipse(int cx, int cy, int rx, int ry, float deg) {
  Shape s = ellipse(cx, cy, rx, ry);
  s.kind = RotEllipse;
  s.angle = deg;
  return s;
}

Shape Shape::roundRect(int x, int y, int w, int h, int r) {
  Shape s;
  s.kind = RoundRect;
  s.a = x;
  s.b = y;
  s.c = w;
  s.d = h;
  s.e = r;
  return s;
}

Shape Shape::capsule(int x0, int y0, int x1, int y1, int r) {
  Shape s;
  s.kind = Capsule;
  s.a = x0;
  s.b = y0;
  s.c = x1;
  s.d = y1;
  s.e = r;
  return s;
}

Shape Shape::poly(const int* xs, const int* ys, int count) {
  Shape s;
  s.kind = Poly;
  s.n = count > 8 ? 8 : count;
  for (int i = 0; i < s.n; ++i) {
    s.px[i] = xs[i];
    s.py[i] = ys[i];
  }
  return s;
}

void Canvas::hspan(int x0, int x1, int y, Ink ink) {
  if (y < 0 || y >= s_.height()) return;
  if (x0 > x1) {
    const int t = x0;
    x0 = x1;
    x1 = t;
  }
  if (x1 < 0 || x0 >= s_.width()) return;
  if (x0 < 0) x0 = 0;
  if (x1 >= s_.width()) x1 = s_.width() - 1;
  s_.span(x0, x1, y, ink);
}

void Canvas::clear(Ink ink) { rect(0, 0, s_.width(), s_.height(), ink); }

void Canvas::rect(int x, int y, int w, int h, Ink ink) {
  for (int yy = y; yy < y + h; ++yy) hspan(x, x + w - 1, yy, ink);
}

void Canvas::ellipse(int cx, int cy, int rx, int ry, Ink ink) {
  if (rx <= 0 || ry <= 0) return;
  for (int dy = -ry; dy <= ry; ++dy) {
    const float t = 1.0f - (dy + 0.0f) * dy / (static_cast<float>(ry) * ry);
    if (t < 0) continue;
    const int dx = iround(rx * std::sqrt(t));
    hspan(cx - dx, cx + dx, cy + dy, ink);
  }
}

void Canvas::rotEllipse(int cx, int cy, int rx, int ry, float deg, Ink ink) {
  if (rx <= 0 || ry <= 0) return;
  const float th = deg * kPi / 180.0f;
  const float co = std::cos(th), si = std::sin(th);
  const float irx2 = 1.0f / (static_cast<float>(rx) * rx), iry2 = 1.0f / (static_cast<float>(ry) * ry);
  const float A = co * co * irx2 + si * si * iry2;
  const int ext = rx > ry ? rx : ry;
  for (int dy = -ext; dy <= ext; ++dy) {
    const float y = static_cast<float>(dy);
    const float B = 2.0f * y * co * si * (irx2 - iry2);
    const float C = y * y * (si * si * irx2 + co * co * iry2) - 1.0f;
    const float disc = B * B - 4 * A * C;
    if (disc < 0) continue;
    const float sq = std::sqrt(disc);
    hspan(cx + iround((-B - sq) / (2 * A)), cx + iround((-B + sq) / (2 * A)), cy + dy, ink);
  }
}

void Canvas::roundRect(int x, int y, int w, int h, int r, Ink ink) {
  if (w <= 0 || h <= 0) return;
  if (r * 2 > w) r = w / 2;
  if (r * 2 > h) r = h / 2;
  for (int yy = 0; yy < h; ++yy) {
    int inset = 0;
    int dy = -1;
    if (yy < r)
      dy = r - yy;
    else if (yy >= h - r)
      dy = yy - (h - r - 1);
    if (dy > 0) {
      const float t = static_cast<float>(r * r - (dy - 0.5f) * (dy - 0.5f));
      inset = r - (t > 0 ? iround(std::sqrt(t)) : 0);
    }
    hspan(x + inset, x + w - 1 - inset, y + yy, ink);
  }
}

void Canvas::poly(const int* xs, const int* ys, int n, Ink ink) {
  if (n < 3) return;
  int minY = ys[0], maxY = ys[0];
  for (int i = 1; i < n; ++i) {
    if (ys[i] < minY) minY = ys[i];
    if (ys[i] > maxY) maxY = ys[i];
  }
  float nodes[16];
  for (int y = minY; y <= maxY; ++y) {
    const float sy = y + 0.5f;
    int k = 0;
    for (int i = 0, j = n - 1; i < n && k < 16; j = i++) {
      const float yi = static_cast<float>(ys[i]), yj = static_cast<float>(ys[j]);
      if ((yi < sy && yj >= sy) || (yj < sy && yi >= sy)) {
        nodes[k++] = xs[i] + (sy - yi) / (yj - yi) * (xs[j] - xs[i]);
      }
    }
    for (int a = 1; a < k; ++a) {  // insertion sort, k is tiny
      const float v = nodes[a];
      int b = a - 1;
      while (b >= 0 && nodes[b] > v) {
        nodes[b + 1] = nodes[b];
        --b;
      }
      nodes[b + 1] = v;
    }
    for (int a = 0; a + 1 < k; a += 2) hspan(iround(nodes[a]), iround(nodes[a + 1]) - 1, y, ink);
  }
}

void Canvas::capsule(int x0, int y0, int x1, int y1, int r, Ink ink) {
  if (r < 1) r = 1;
  circle(x0, y0, r, ink);
  circle(x1, y1, r, ink);
  const float dx = static_cast<float>(x1 - x0), dy = static_cast<float>(y1 - y0);
  const float len = std::sqrt(dx * dx + dy * dy);
  if (len < 0.5f) return;
  const float nx = -dy / len * r, ny = dx / len * r;
  const int xs[4] = {iround(x0 + nx), iround(x1 + nx), iround(x1 - nx), iround(x0 - nx)};
  const int ys[4] = {iround(y0 + ny), iround(y1 + ny), iround(y1 - ny), iround(y0 - ny)};
  poly(xs, ys, 4, ink);
}

void Canvas::shape(const Shape& sh, int g, Ink ink) {
  switch (sh.kind) {
    case Shape::Ellipse:
      ellipse(sh.a, sh.b, sh.c + g, sh.d + g, ink);
      break;
    case Shape::RotEllipse:
      rotEllipse(sh.a, sh.b, sh.c + g, sh.d + g, sh.angle, ink);
      break;
    case Shape::RoundRect:
      roundRect(sh.a - g, sh.b - g, sh.c + 2 * g, sh.d + 2 * g, sh.e + g, ink);
      break;
    case Shape::Capsule:
      capsule(sh.a, sh.b, sh.c, sh.d, sh.e + g, ink);
      break;
    case Shape::Poly:
      poly(sh.px, sh.py, sh.n, ink);
      if (g > 0) {
        for (int i = 0, j = sh.n - 1; i < sh.n; j = i++) capsule(sh.px[j], sh.py[j], sh.px[i], sh.py[i], g, ink);
      }
      break;
  }
}

void Canvas::outlined(const Shape* shapes, int n, int stroke, Ink fill) {
  for (int i = 0; i < n; ++i) shape(shapes[i], stroke, Ink::Black);
  for (int i = 0; i < n; ++i) shape(shapes[i], 0, fill);
}

void Canvas::frame(int x, int y, int w, int h, int r, int t, Ink ink, Ink fill) {
  roundRect(x, y, w, h, r, ink);
  roundRect(x + t, y + t, w - 2 * t, h - 2 * t, r > t ? r - t : 0, fill);
}

void Canvas::arc(int cx, int cy, int r, float a0, float a1, int t, Ink ink) {
  const int steps = 10 + r / 3;
  const int rad = t / 2 > 0 ? t / 2 : 1;
  int px = 0, py = 0;
  for (int i = 0; i <= steps; ++i) {
    const float a = (a0 + (a1 - a0) * i / steps) * kPi / 180.0f;
    const int x = cx + iround(r * std::cos(a));
    const int y = cy + iround(r * std::sin(a));
    if (i) capsule(px, py, x, y, rad, ink);
    px = x;
    py = y;
  }
}

void Canvas::heart(int cx, int cy, int size, Ink ink) {
  const int r = size * 27 / 100;
  circle(cx - r + 1, cy - r / 2, r, ink);
  circle(cx + r - 1, cy - r / 2, r, ink);
  const int xs[3] = {cx - 2 * r + 1, cx + 2 * r - 1, cx};
  const int ys[3] = {cy - r / 3, cy - r / 3, cy + size / 2};
  poly(xs, ys, 3, ink);
}

void Canvas::sparkle(int cx, int cy, int r, Ink ink) {
  const int w = r / 4 > 1 ? r / 4 : 1;
  const int xs[8] = {cx, cx + w, cx + r, cx + w, cx, cx - w, cx - r, cx - w};
  const int ys[8] = {cy - r, cy - w, cy, cy + w, cy + r, cy + w, cy, cy - w};
  poly(xs, ys, 8, ink);
}

void Canvas::textCentered(int cx, int y, const char* t, int size, bool bold, Ink ink) {
  s_.text(cx - s_.textWidth(t, size, bold) / 2, y, t, size, bold, ink);
}

int Canvas::wrap(int cx, int y, int maxW, const char* t, int size, int lineGap, int maxLines, bool bold, Ink ink,
                 bool draw) {
  char line[96];
  char trial[96];
  int lines = 0;
  const char* p = t;
  while (*p && lines < maxLines) {
    while (*p == ' ') ++p;
    line[0] = 0;
    const char* consumed = p;
    while (*consumed) {
      const char* we = consumed;
      while (*we == ' ') ++we;
      while (*we && *we != ' ') ++we;
      const int addLen = static_cast<int>(we - consumed);
      const int curLen = static_cast<int>(strlen(line));
      if (curLen + addLen >= static_cast<int>(sizeof(trial))) break;
      memcpy(trial, line, curLen);
      memcpy(trial + curLen, consumed, addLen);
      trial[curLen + addLen] = 0;
      const char* tt = trial;
      while (*tt == ' ') ++tt;
      if (line[0] && s_.textWidth(tt, size, bold) > maxW) break;
      memcpy(line, tt, strlen(tt) + 1);
      consumed = we;
    }
    if (consumed == p) {  // one word too long for the line: take it anyway
      const char* we = p;
      while (*we && *we != ' ') ++we;
      int len = static_cast<int>(we - p);
      if (len > 95) len = 95;
      memcpy(line, p, len);
      line[len] = 0;
      consumed = we;
    }
    if (draw) textCentered(cx, y + lines * (size + lineGap), line, size, bold, ink);
    ++lines;
    p = consumed;
  }
  return lines;
}

}  // namespace pp
