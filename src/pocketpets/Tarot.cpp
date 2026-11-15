// Tarot mode for the Book of Answers: the 22 Major Arcana with original
// symbols and short readings.
#include <cmath>
#include <cstdio>

#include "Screens.h"

namespace pp {

namespace {

constexpr float kPi = 3.14159265f;
inline int iround(float v) { return static_cast<int>(v < 0 ? v - 0.5f : v + 0.5f); }

struct Card {
  const char* numeral;
  const char* name;
  const char* upright;
  const char* reversed;
};

const Card CARDS[TAROT_COUNT] = {
    {"0", "The Fool", "A fresh start. Leap, gently.", "Look before you leap."},
    {"I", "The Magician", "You already have what you need.", "Don't scatter your energy."},
    {"II", "The High Priestess", "Trust what you quietly know.", "Listen to your inner voice again."},
    {"III", "The Empress", "Something is growing. Nurture it.", "Care for yourself first."},
    {"IV", "The Emperor", "Make a plan and stick to it.", "Loosen your grip a little."},
    {"V", "The Hierophant", "Learn from someone wise.", "Your own path is allowed."},
    {"VI", "The Lovers", "A heartfelt choice is near.", "Make sure your choice matches your heart."},
    {"VII", "The Chariot", "Full speed ahead!", "Steer before you speed up."},
    {"VIII", "Strength", "Gentle courage wins today.", "Be kind to your nervous heart."},
    {"IX", "The Hermit", "Take some quiet time to think.", "Don't hide away for too long."},
    {"X", "Wheel of Fortune", "Luck is turning your way.", "This bump won't last."},
    {"XI", "Justice", "Fair and honest wins.", "Double-check the details."},
    {"XII", "The Hanged Man", "Pause and see it differently.", "Stop waiting. Start moving."},
    {"XIII", "Death", "One chapter ends, a new one begins.", "Let go of what's finished."},
    {"XIV", "Temperance", "Balance and patience.", "Too much of one thing. Mix it up."},
    {"XV", "The Devil", "Notice what's holding you back.", "You're breaking free."},
    {"XVI", "The Tower", "A surprise shakes things up.", "A small wobble, not a fall."},
    {"XVII", "The Star", "Hope and healing are on the way.", "Believe in yourself again."},
    {"XVIII", "The Moon", "Not everything is clear yet.", "The fog is lifting."},
    {"XIX", "The Sun", "Joy, warmth and a very good day!", "The sun is behind a cloud. It'll pass."},
    {"XX", "Judgement", "Wake up to something new.", "Forgive yourself and move on."},
    {"XXI", "The World", "You did it! Something comes full circle.", "Almost there. Finish the last bit."},
};

// Symbol pen: design units centred on (cx, cy), y up, optionally turned 180°.
struct SPen {
  Canvas& c;
  int cx, cy;
  float u;
  bool flip;
  int X(float x) const { return cx + iround((flip ? -x : x) * u); }
  int Y(float y) const { return cy - iround((flip ? -y : y) * u); }
  int R(float r) const { return iround(r * u) > 0 ? iround(r * u) : 1; }
  int st() const { return R(3) < 2 ? 2 : R(3); }
  Shape ell(float x, float y, float rx, float ry) const { return Shape::ellipse(X(x), Y(y), R(rx), R(ry)); }
  Shape rot(float x, float y, float rx, float ry, float deg) const {
    return Shape::rotEllipse(X(x), Y(y), R(rx), R(ry), deg);
  }
  Shape cap(float x0, float y0, float x1, float y1, float r) const {
    return Shape::capsule(X(x0), Y(y0), X(x1), Y(y1), R(r));
  }
  Shape box(float x0, float y0, float x1, float y1) const {
    const int xs[4] = {X(x0), X(x1), X(x1), X(x0)};
    const int ys[4] = {Y(y0), Y(y0), Y(y1), Y(y1)};
    return Shape::poly(xs, ys, 4);
  }
  Shape tri(float x0, float y0, float x1, float y1, float x2, float y2) const {
    const int xs[3] = {X(x0), X(x1), X(x2)};
    const int ys[3] = {Y(y0), Y(y1), Y(y2)};
    return Shape::poly(xs, ys, 3);
  }
  void line(float x0, float y0, float x1, float y1, float t, Ink ink = Ink::Black) const {
    c.capsule(X(x0), Y(y0), X(x1), Y(y1), R(t / 2), ink);
  }
  void ring(float x, float y, float r, float t, Ink ink = Ink::Black) const {
    c.arc(X(x), Y(y), R(r), 0, 360, R(t) < 2 ? 2 : R(t), ink);
  }
};

void drawSymbol(const SPen& p, int card) {
  const int st = p.st();
  switch (card) {
    case 0:     // leaf on the wind
    case 13: {  // butterfly for change
      if (card == 0) {
        p.c.outlined(p.rot(0, 0, 30, 15, 35), st, Ink::Gray);
        p.line(-20, -18, 18, 16, 3);
        p.c.sparkle(p.X(26), p.Y(26), p.R(7), Ink::Black);
        p.c.sparkle(p.X(-28), p.Y(18), p.R(4), Ink::Black);
      } else {
        Shape wings[4] = {p.rot(-16, 10, 18, 13, -30), p.rot(16, 10, 18, 13, 30), p.rot(-12, -14, 12, 9, 25),
                          p.rot(12, -14, 12, 9, -25)};
        p.c.outlined(wings, 4, st, Ink::Gray);
        p.c.outlined(p.cap(0, 18, 0, -22, 3.5f), p.R(1.5f));
        p.line(0, 20, -8, 32, 2);
        p.line(0, 20, 8, 32, 2);
      }
      break;
    }
    case 1:  // infinity
      p.ring(-14, 0, 13, 5);
      p.ring(14, 0, 13, 5);
      p.c.sparkle(p.X(0), p.Y(30), p.R(7), Ink::Black);
      break;
    case 2: {  // closed book with a moon
      p.c.outlined(Shape::roundRect(p.X(-24) < p.X(24) ? p.X(-24) : p.X(24), p.Y(30) < p.Y(-30) ? p.Y(30) : p.Y(-30),
                                    p.R(48), p.R(60), p.R(6)),
                   st);
      p.line(-16, 30, -16, -30, 3);
      drawIcon(p.c, Icon::Moon, p.X(6), p.Y(0), p.R(26));
      break;
    }
    case 3: {  // flower
      Shape petals[6];
      for (int i = 0; i < 6; ++i) {
        const float a = i * 60 * kPi / 180;
        petals[i] = p.ell(std::cos(a) * 15, 6 + std::sin(a) * 15, 10, 10);
      }
      p.line(0, -6, 0, -36, 4);
      p.c.outlined(p.rot(-10, -26, 9, 4, -30), p.R(2));
      p.c.outlined(petals, 6, st);
      p.c.outlined(p.ell(0, 6, 8, 8), p.R(2), Ink::Gray);
      break;
    }
    case 4: {  // crown
      int xs[7] = {p.X(-30), p.X(-30), p.X(-15), p.X(0), p.X(15), p.X(30), p.X(30)};
      int ys[7] = {p.Y(-18), p.Y(20), p.Y(2), p.Y(26), p.Y(2), p.Y(20), p.Y(-18)};
      const Shape crown = Shape::poly(xs, ys, 7);
      p.c.outlined(crown, st, Ink::Gray);
      p.c.outlined(p.box(-30, -18, 30, -8), p.R(1.5f));
      for (int i = -1; i <= 1; ++i) p.c.circle(p.X(i * 15), p.Y(-13), p.R(3), Ink::Black);
      break;
    }
    case 5:  // key
      p.ring(0, 18, 13, 5);
      p.line(0, 5, 0, -34, 6);
      p.line(0, -24, 10, -24, 5);
      p.line(0, -33, 12, -33, 5);
      break;
    case 6:  // two hearts
      p.c.heart(p.X(-12), p.Y(6), p.R(34), Ink::Black);
      p.c.heart(p.X(14), p.Y(-6), p.R(30), Ink::Gray);
      p.c.sparkle(p.X(24), p.Y(26), p.R(6), Ink::Black);
      break;
    case 7:  // moving forward
      for (int i = 0; i < 3; ++i) {
        const float x = -20 + i * 18;
        p.line(x, 20, x + 14, 0, 6);
        p.line(x + 14, 0, x, -20, 6);
      }
      break;
    case 8: {  // paw
      p.c.outlined(p.ell(0, -10, 18, 15), st, Ink::Gray);
      const float tx[4] = {-22, -8, 8, 22}, ty[4] = {8, 20, 20, 8};
      for (int i = 0; i < 4; ++i) p.c.outlined(p.ell(tx[i], ty[i], 7, 8), p.R(2), Ink::Gray);
      break;
    }
    case 9: {  // lantern
      p.ring(0, 34, 6, 3);
      p.c.outlined(p.box(-12, 28, 12, 22), p.R(2));
      p.c.outlined(Shape::roundRect(p.X(-18) < p.X(18) ? p.X(-18) : p.X(18), p.Y(22) < p.Y(-26) ? p.Y(22) : p.Y(-26),
                                    p.R(36), p.R(48), p.R(8)),
                   st);
      Shape flame[2] = {p.ell(0, -6, 8, 9), p.tri(-8, -4, 8, -4, 0, 12)};
      p.c.outlined(flame, 2, p.R(1.5f), Ink::Gray);
      p.c.outlined(p.box(-14, -26, 14, -32), p.R(2));
      break;
    }
    case 10:  // wheel
      p.ring(0, 0, 30, 5);
      for (int i = 0; i < 8; ++i) {
        const float a = i * 45 * kPi / 180;
        p.line(std::cos(a) * 6, std::sin(a) * 6, std::cos(a) * 28, std::sin(a) * 28, 3);
      }
      p.c.circle(p.X(0), p.Y(0), p.R(7), Ink::Black);
      break;
    case 11:  // scales
      p.line(0, -34, 0, 26, 4);
      p.line(-28, 20, 28, 20, 4);
      p.line(-14, -34, 14, -34, 5);
      for (int s = -1; s <= 1; s += 2) {
        p.line(s * 28, 20, s * 36, -2, 2);
        p.line(s * 28, 20, s * 20, -2, 2);
        p.c.outlined(p.ell(s * 28, -4, 12, 5), p.R(1.5f), Ink::Gray);
      }
      p.c.circle(p.X(0), p.Y(26), p.R(4), Ink::Black);
      break;
    case 12: {  // hourglass
      Shape glass[2] = {p.tri(-20, 28, 20, 28, 0, 0), p.tri(-20, -28, 20, -28, 0, 0)};
      p.c.outlined(glass, 2, st);
      p.c.shape(p.tri(-10, -26, 10, -26, 0, -14), 0, Ink::Gray);
      p.line(-26, 32, 26, 32, 6);
      p.line(-26, -32, 26, -32, 6);
      break;
    }
    case 14: {  // cup
      Shape cup[3] = {p.ell(0, 14, 24, 18), p.cap(0, 0, 0, -22, 3), p.ell(0, -26, 16, 5)};
      p.c.outlined(cup, 3, st);
      p.c.rect(p.X(-27) < p.X(27) ? p.X(-27) : p.X(27), p.Y(34) < p.Y(18) ? p.Y(34) : p.Y(18), p.R(54), p.R(15),
               Ink::White);
      p.line(-24, 16, 24, 16, 4);
      p.c.ellipse(p.X(0), p.Y(12), p.R(16), p.R(6), Ink::Gray);
      break;
    }
    case 15:  // linked rings
      p.ring(-10, 6, 15, 5);
      p.ring(10, -6, 15, 5);
      break;
    case 16: {  // tower
      p.c.outlined(p.box(-16, -34, 16, 22), st);
      for (int i = -1; i <= 1; ++i) p.c.outlined(p.box(i * 11 - 5, 22, i * 11 + 5, 30), p.R(1.5f));
      p.c.outlined(p.box(-6, -34, 6, -18), p.R(1.5f), Ink::Gray);
      const int xs[6] = {p.X(30), p.X(18), p.X(26), p.X(16), p.X(34), p.X(26)};
      const int ys[6] = {p.Y(38), p.Y(16), p.Y(16), p.Y(-4), p.Y(22), p.Y(22)};
      p.c.poly(xs, ys, 6, Ink::Black);
      break;
    }
    case 17:
      p.c.sparkle(p.X(0), p.Y(0), p.R(26), Ink::Black);
      p.c.sparkle(p.X(24), p.Y(24), p.R(8), Ink::Black);
      p.c.sparkle(p.X(-26), p.Y(20), p.R(6), Ink::Black);
      p.c.sparkle(p.X(-22), p.Y(-26), p.R(5), Ink::Black);
      break;
    case 18:
      drawIcon(p.c, Icon::Moon, p.X(0), p.Y(0), p.R(58));
      p.c.sparkle(p.X(26), p.Y(-24), p.R(6), Ink::Black);
      break;
    case 19: {  // sun with a face
      for (int i = 0; i < 8; ++i) {
        const float a = i * 45 * kPi / 180;
        p.line(std::cos(a) * 26, std::sin(a) * 26, std::cos(a) * 36, std::sin(a) * 36, 5);
      }
      p.c.outlined(p.ell(0, 0, 19, 19), st, Ink::White);
      p.c.circle(p.X(-6), p.Y(3), p.R(2.5f), Ink::Black);
      p.c.circle(p.X(6), p.Y(3), p.R(2.5f), Ink::Black);
      p.c.arc(p.X(0), p.Y(-2), p.R(6), p.flip ? 200 : 20, p.flip ? 340 : 160, 2, Ink::Black);
      break;
    }
    case 20:  // a bright wake-up burst
      for (int i = 0; i < 12; ++i) {
        const float a = i * 30 * kPi / 180;
        const float r0 = 12, r1 = i % 2 ? 26 : 34;
        p.line(std::cos(a) * r0, std::sin(a) * r0, std::cos(a) * r1, std::sin(a) * r1, 4);
      }
      p.c.circle(p.X(0), p.Y(0), p.R(8), Ink::Black);
      break;
    default: {  // the world
      p.c.outlined(p.ell(0, 0, 26, 26), st, Ink::Gray);
      p.c.ellipse(p.X(-6), p.Y(6), p.R(9), p.R(12), Ink::White);
      p.c.ellipse(p.X(10), p.Y(-8), p.R(7), p.R(6), Ink::White);
      p.c.arc(p.X(0), p.Y(0), p.R(36), 0, 360, 3, Ink::Black);
      p.c.sparkle(p.X(30), p.Y(30), p.R(6), Ink::Black);
      break;
    }
  }
}

}  // namespace

const char* tarotName(int card) { return CARDS[(card % TAROT_COUNT + TAROT_COUNT) % TAROT_COUNT].name; }
const char* tarotMeaning(int card, bool reversed) {
  const Card& k = CARDS[(card % TAROT_COUNT + TAROT_COUNT) % TAROT_COUNT];
  return reversed ? k.reversed : k.upright;
}

void drawTarotCard(Canvas& c, int card, bool reversed, int cx, int cy, int h) {
  const int w = h * 62 / 100;
  const Box b{cx - w / 2, cy - h / 2, w, h};
  c.roundRect(b.x + 7, b.y + 7, b.w, b.h, h / 14, Ink::Black);
  c.frame(b.x, b.y, b.w, b.h, h / 14, 5, Ink::Black);
  c.frame(b.x + 12, b.y + 12, b.w - 24, b.h - 24, h / 22, 2, Ink::Black);
  // Little corner stars.
  c.sparkle(b.x + 26, b.y + 26, 6, Ink::Black);
  c.sparkle(b.x + b.w - 26, b.y + 26, 6, Ink::Black);
  c.sparkle(b.x + 26, b.y + b.h - 26, 6, Ink::Black);
  c.sparkle(b.x + b.w - 26, b.y + b.h - 26, 6, Ink::Black);
  const Card& k = CARDS[(card % TAROT_COUNT + TAROT_COUNT) % TAROT_COUNT];
  // The numeral sits at the top; a reversed card shows it at the bottom.
  const int ny = reversed ? b.y + b.h - 54 : b.y + 22;
  c.textCentered(cx, ny, k.numeral, 26, true);
  const SPen p{c, cx, cy, h / 170.0f, reversed};
  // A soft halo behind the symbol.
  c.circle(cx, cy, h * 26 / 100, Ink::Gray);
  c.circle(cx, cy, h * 26 / 100 - 6, Ink::White);
  drawSymbol(p, card);
}

void drawTarotBack(Canvas& c, int cx, int cy, int h) {
  const int w = h * 62 / 100;
  const Box b{cx - w / 2, cy - h / 2, w, h};
  c.roundRect(b.x + 7, b.y + 7, b.w, b.h, h / 14, Ink::Black);
  c.frame(b.x, b.y, b.w, b.h, h / 14, 5, Ink::Black, Ink::Dark);
  c.frame(b.x + 14, b.y + 14, b.w - 28, b.h - 28, h / 22, 4, Ink::Black);
  c.circle(cx, cy, w / 4, Ink::Black);
  c.circle(cx, cy, w / 4 - 5, Ink::White);
  drawIcon(c, Icon::Moon, cx - w / 24, cy, w / 4);
  c.sparkle(cx + w / 8, cy - w / 10, w / 14, Ink::Black);
  c.sparkle(cx, b.y + 46, 10, Ink::Black);
  c.sparkle(cx, b.y + b.h - 46, 10, Ink::Black);
}

}  // namespace pp
