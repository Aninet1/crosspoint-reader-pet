#include "PetArt.h"

#include <cmath>

namespace pp {

namespace {

constexpr float kPi = 3.14159265f;
inline int iround(float v) { return static_cast<int>(v < 0 ? v - 0.5f : v + 0.5f); }

// Pen maps design units (x right, y UP from the ground line) to pixels.
struct Pen {
  Canvas& c;
  int cx, by;
  float u;
  int X(float x) const { return cx + iround(x * u); }
  int Y(float y) const { return by - iround(y * u); }
  int R(float r) const { return iround(r * u) > 0 ? iround(r * u) : 1; }
  int stroke() const { return R(3.2f) < 3 ? 3 : R(3.2f); }

  Shape ell(float x, float y, float rx, float ry) const { return Shape::ellipse(X(x), Y(y), R(rx), R(ry)); }
  Shape rot(float x, float y, float rx, float ry, float deg) const {
    return Shape::rotEllipse(X(x), Y(y), R(rx), R(ry), deg);
  }
  Shape cap(float x0, float y0, float x1, float y1, float r) const {
    return Shape::capsule(X(x0), Y(y0), X(x1), Y(y1), R(r));
  }
  Shape tri(float x0, float y0, float x1, float y1, float x2, float y2) const {
    const int xs[3] = {X(x0), X(x1), X(x2)};
    const int ys[3] = {Y(y0), Y(y1), Y(y2)};
    return Shape::poly(xs, ys, 3);
  }
  void fill(float x, float y, float rx, float ry, Ink ink) const { c.ellipse(X(x), Y(y), R(rx), R(ry), ink); }
  void arc(float x, float y, float r, float a0, float a1, float t, Ink ink = Ink::Black) const {
    c.arc(X(x), Y(y), R(r), a0, a1, R(t) < 2 ? 2 : R(t), ink);
  }
  void line(float x0, float y0, float x1, float y1, float t, Ink ink = Ink::Black) const {
    c.capsule(X(x0), Y(y0), X(x1), Y(y1), R(t / 2), ink);
  }
};

// Eyes, mouth and blush shared by every species. (fx, fy) is the face centre,
// gap the distance between the eyes.
void drawFace(const Pen& p, float fx, float fy, float gap, float eye, const PetLook& look, bool catMouth) {
  const float ex = gap / 2;
  // Blush first so the eyes sit on top.
  p.fill(fx - ex - eye * 1.2f, fy - eye * 1.6f, eye * 1.5f, eye * 0.85f, Ink::Gray);
  p.fill(fx + ex + eye * 1.2f, fy - eye * 1.6f, eye * 1.5f, eye * 0.85f, Ink::Gray);

  const bool closedHappy = look.pose == Pose::Happy || look.pose == Pose::Excited || look.pose == Pose::Eating;
  for (int side = -1; side <= 1; side += 2) {
    const float x = fx + side * ex;
    if (look.sleeping) {
      p.arc(x, fy + eye * 0.2f, eye * 0.9f, 20, 160, eye * 0.55f);
    } else if (closedHappy) {
      p.arc(x, fy - eye * 0.4f, eye * 0.95f, 200, 340, eye * 0.6f);
    } else {
      p.fill(x, fy, eye * 0.78f, eye, Ink::Black);
      p.fill(x + eye * 0.28f, fy + eye * 0.38f, eye * 0.3f, eye * 0.3f, Ink::White);
      if (look.pose == Pose::Sad || look.hungry) {
        // Worried brows.
        p.line(x - side * eye * 0.2f - eye * 0.6f * side, fy + eye * 1.9f + 0, x + side * eye * 0.9f, fy + eye * 1.45f,
               eye * 0.45f);
      }
    }
  }

  const float my = fy - eye * 2.0f;
  if (look.sleeping) {
    p.fill(fx, my, eye * 0.35f, eye * 0.3f, Ink::Black);
  } else if (look.pose == Pose::Eating) {
    p.fill(fx, my - eye * 0.1f, eye * 0.45f, eye * 0.42f, Ink::Black);
    p.fill(fx - eye * 2.2f, my + eye * 0.2f, eye * 0.5f, eye * 0.5f, Ink::Gray);  // crumbs
  } else if (look.pose == Pose::Excited) {
    // Open "D" smile: a half disc with a little tongue.
    p.fill(fx, my, eye * 0.62f, eye * 0.62f, Ink::Black);
    p.c.rect(p.X(fx - eye * 0.7f), p.Y(my + eye * 0.7f), p.R(eye * 1.4f), p.R(eye * 0.7f), Ink::White);
    p.fill(fx, my - eye * 0.34f, eye * 0.32f, eye * 0.2f, Ink::Gray);
  } else if (look.pose == Pose::Sad || look.hungry) {
    p.arc(fx, my - eye * 0.9f, eye * 0.6f, 200, 340, eye * 0.4f);
  } else if (catMouth) {
    p.arc(fx - eye * 0.45f, my + eye * 0.1f, eye * 0.45f, 0, 180, eye * 0.38f);
    p.arc(fx + eye * 0.45f, my + eye * 0.1f, eye * 0.45f, 0, 180, eye * 0.38f);
  } else {
    p.arc(fx, my + eye * 0.25f, eye * 0.6f, 20, 160, eye * 0.42f);
  }
}

// Teen and adult personality touches.
void drawAccessory(const Pen& p, const PetLook& look, float fx, float fy, float gap, float eye, float topY,
                   float chestY) {
  if (look.stage < Stage::Teen) return;
  switch (look.personality) {
    case Personality::Bookworm: {  // round glasses
      const float ex = gap / 2;
      const float r = eye * 1.75f;
      for (int s = -1; s <= 1; s += 2) p.arc(fx + s * ex, fy, r, 0, 360, eye * 0.38f);
      p.arc(fx, fy + r * 0.2f, ex - r, 200, 340, eye * 0.38f);
      break;
    }
    case Personality::Active: {                                                // sweatband
      if (look.species == Species::Plant || look.species == Species::Cloud) {  // speed lines instead
        for (int i = 0; i < 3; ++i)
          p.line(-gap * 1.55f - i * 3, fy - 8 + i * 9, -gap * 2.3f - i * 3, fy - 8 + i * 9, 2.2f);
        break;
      }
      const float y = topY - eye * 2.2f;
      const Shape band = p.cap(fx - gap * 1.05f, y, fx + gap * 1.05f, y, eye * 0.9f);
      p.c.outlined(band, p.R(1.6f), Ink::Gray);
      break;
    }
    case Personality::Foodie: {  // tiny bib with a heart (a floating snack for clouds and plants)
      if (look.species == Species::Cloud || look.species == Species::Plant) {
        drawFood(p.c, 1, p.X(fx + gap * 1.6f), p.Y(topY + eye * 1.5f), p.R(eye * 4.5f));
        break;
      }
      Shape bib[2] = {p.ell(fx, chestY - eye * 0.2f, gap * 0.5f, eye * 2.0f),
                      p.rot(fx, chestY + eye * 1.4f, gap * 0.58f, eye * 0.7f, 0)};
      p.c.outlined(bib, 2, p.R(1.4f));
      p.c.heart(p.X(fx), p.Y(chestY - eye * 0.2f), p.R(eye * 1.7f), Ink::Black);
      p.c.heart(p.X(fx), p.Y(chestY - eye * 0.6f), p.R(eye * 1.5f), Ink::Gray);
      break;
    }
    case Personality::Cuddly: {  // floating hearts
      p.c.heart(p.X(fx + gap * 1.5f), p.Y(topY + eye * 2.5f), p.R(eye * 2.6f), Ink::Black);
      p.c.heart(p.X(fx + gap * 2.1f), p.Y(topY + eye * 0.6f), p.R(eye * 1.5f), Ink::Gray);
      break;
    }
    default:
      break;
  }
}

// Headphones or a nightcap on a head centred at (hx, hy) with radius hr.
void drawHeadwear(const Pen& p, const PetLook& look, float hx, float hy, float hr) {
  if (look.wear == PetLook::Headphones) {
    p.arc(hx, hy + hr * 0.05f, hr * 1.04f, 200, 340, hr * 0.11f);
    for (int s = -1; s <= 1; s += 2) {
      Shape cup[2] = {p.ell(hx + s * hr * 0.98f, hy - hr * 0.05f, hr * 0.2f, hr * 0.3f),
                      p.ell(hx + s * hr * 0.9f, hy - hr * 0.05f, hr * 0.12f, hr * 0.22f)};
      p.c.outlined(cup, 2, p.R(1.6f), Ink::Dark);
    }
  } else if (look.wear == PetLook::Nightcap) {
    // Floppy cap flopping to one side, with a pompom.
    const float top = hy + hr * 0.82f;
    Shape cap[3] = {p.tri(hx - hr * 0.62f, top - hr * 0.1f, hx + hr * 0.5f, top, hx + hr * 1.05f, top - hr * 0.32f),
                    p.ell(hx - hr * 0.05f, top + hr * 0.12f, hr * 0.56f, hr * 0.3f),
                    p.ell(hx + hr * 0.62f, top + hr * 0.02f, hr * 0.3f, hr * 0.2f)};
    p.c.outlined(cap, 3, p.R(1.6f), Ink::Gray);
    p.c.outlined(p.ell(hx + hr * 1.08f, top - hr * 0.34f, hr * 0.13f, hr * 0.13f), p.R(1.4f));
    const Shape brim = p.cap(hx - hr * 0.62f, top - hr * 0.12f, hx + hr * 0.52f, top - hr * 0.04f, hr * 0.08f);
    p.c.outlined(brim, p.R(1.4f));
  }
}

struct Body {
  float bodyRx, bodyRy, bodyY, headRx, headRy, headY;
};
// Index by Stage (Egg unused). Younger pets have bigger heads.
constexpr Body DOG_BODY[] = {{0, 0, 0, 0, 0, 0},
                             {22, 14, 14, 40, 33, 46},
                             {26, 18, 18, 38, 31, 52},
                             {29, 21, 21, 36, 30, 57},
                             {31, 24, 24, 36, 30, 62}};
constexpr float STAGE_SCALE[] = {0.6f, 0.58f, 0.72f, 0.86f, 1.0f};

void drawDog(const Pen& p, const PetLook& look) {
  const Body& b = DOG_BODY[static_cast<int>(look.stage)];
  const int st = p.stroke();
  const float droop = look.hungry || look.pose == Pose::Sad ? 6.0f : 0.0f;

  // Tail, then floppy ears behind the head.
  const float wag = look.pose == Pose::Excited || look.pose == Pose::Happy ? 14.0f : 4.0f;
  Shape tail[2] = {p.cap(b.bodyRx * 0.75f, b.bodyY * 0.7f, b.bodyRx + 9, b.bodyY + wag, 5.5f),
                   p.ell(b.bodyRx + 9, b.bodyY + wag + 2, 5.5f, 5.5f)};
  p.c.outlined(tail, 2, st);
  // Head and body share one outline.
  Shape hb[2] = {p.ell(0, b.bodyY, b.bodyRx, b.bodyRy), p.ell(0, b.headY, b.headRx, b.headRy)};
  p.c.outlined(hb, 2, st);
  // Floppy ears hang from the top of the head.
  for (int s = -1; s <= 1; s += 2) {
    Shape ear[2] = {p.cap(s * b.headRx * 0.74f, b.headY + b.headRy * 0.78f, s * b.headRx * (1.1f + droop * 0.01f),
                          b.headY - b.headRy * 0.12f - droop, b.headRx * 0.16f),
                    p.ell(s * b.headRx * 0.76f, b.headY + b.headRy * 0.76f, b.headRx * 0.2f, b.headRx * 0.15f)};
    p.c.outlined(ear, 2, st, Ink::Dark);
  }
  Shape paws[2] = {p.ell(-b.bodyRx * 0.42f, 4, b.bodyRx * 0.3f, 5.5f),
                   p.ell(b.bodyRx * 0.42f, 4, b.bodyRx * 0.3f, 5.5f)};
  p.c.outlined(paws, 2, st);

  const float eye = b.headRx * 0.125f;
  const float gap = b.headRx * 0.82f;
  const float fy = b.headY - b.headRy * 0.05f;
  // Patch around one eye: this pup's mark.
  p.fill(gap / 2, fy + eye * 0.2f, eye * 2.3f, eye * 2.1f, Ink::Gray);
  drawFace(p, 0, fy, gap, eye, look, false);
  // Little nose above the mouth.
  p.fill(0, fy - eye * 0.9f, eye * 0.52f, eye * 0.38f, Ink::Black);
  if (look.stage == Stage::Adult && look.personality != Personality::Foodie) {
    // Collar with a round tag.
    const Shape collar =
        p.cap(-b.bodyRx * 0.62f, b.bodyY + b.bodyRy * 0.62f, b.bodyRx * 0.62f, b.bodyY + b.bodyRy * 0.62f, 1.8f);
    p.c.outlined(collar, p.R(1.3f), Ink::Gray);
    p.c.outlined(p.ell(0, b.bodyY + b.bodyRy * 0.42f, 4, 4), p.R(1.4f));
  }
  drawAccessory(p, look, 0, fy, gap, eye, b.headY + b.headRy, b.bodyY + b.bodyRy * 0.55f);
  drawHeadwear(p, look, 0, b.headY, b.headRx);
}

void drawCat(const Pen& p, const PetLook& look) {
  const Body& b = DOG_BODY[static_cast<int>(look.stage)];
  const int st = p.stroke();
  const float hrx = b.headRx * 1.06f, hry = b.headRy * 0.92f;
  const float flat = look.hungry || look.pose == Pose::Sad ? 0.7f : 1.0f;

  // Tail curling up behind the body.
  const float sway = look.pose == Pose::Excited ? 8.0f : 0.0f;
  Shape tail[11];
  {
    const float tx = b.bodyRx + 2 + sway * 0.4f, ty = b.bodyY * 0.9f + 6, tr = b.bodyRy * 0.55f + 6;
    float px = b.bodyRx * 0.5f, py = 5;
    for (int i = 0; i < 10; ++i) {
      const float a = (-100.0f + i * 24.0f) * kPi / 180.0f;
      const float x = tx + std::cos(a) * tr, y = ty + std::sin(a) * tr;
      tail[i] = p.cap(px, py, x, y, 4.2f);
      px = x;
      py = y;
    }
    tail[10] = p.ell(px, py, 4.6f, 4.6f);
  }
  p.c.outlined(tail, 11, st);
  // Pointy-but-soft ears.
  for (int s = -1; s <= 1; s += 2) {
    const float bx = s * hrx * 0.62f;
    const Shape ear = p.tri(bx - s * 13, b.headY + hry * 0.55f, bx + s * 12, b.headY + hry * 0.72f, bx + s * 3,
                            b.headY + hry + 16 * flat);
    p.c.outlined(ear, st);
    const Shape inner = p.tri(bx - s * 6, b.headY + hry * 0.72f, bx + s * 6, b.headY + hry * 0.8f, bx + s * 2.5f,
                              b.headY + hry + 8 * flat);
    p.c.shape(inner, 0, Ink::Gray);
  }
  Shape hb[2] = {p.ell(0, b.bodyY, b.bodyRx, b.bodyRy), p.ell(0, b.headY, hrx, hry)};
  p.c.outlined(hb, 2, st);
  // Forehead stripes.
  for (int i = -1; i <= 1; ++i) {
    p.line(i * hrx * 0.17f, b.headY + hry * 0.92f, i * hrx * 0.2f, b.headY + hry * 0.62f, 3.4f, Ink::Dark);
  }
  Shape paws[2] = {p.ell(-b.bodyRx * 0.4f, 4, b.bodyRx * 0.28f, 5.5f),
                   p.ell(b.bodyRx * 0.4f, 4, b.bodyRx * 0.28f, 5.5f)};
  p.c.outlined(paws, 2, st);
  // Belly patch.
  p.fill(0, b.bodyY - b.bodyRy * 0.15f, b.bodyRx * 0.45f, b.bodyRy * 0.55f, Ink::Gray);

  const float eye = hrx * 0.12f;
  const float gap = hrx * 0.84f;
  const float fy = b.headY - hry * 0.08f;
  drawFace(p, 0, fy, gap, eye, look, true);
  p.fill(0, fy - eye * 1.05f, eye * 0.5f, eye * 0.35f, Ink::Black);
  // Whiskers.
  for (int s = -1; s <= 1; s += 2) {
    p.line(s * hrx * 0.62f, fy - eye * 1.6f, s * hrx * 1.12f, fy - eye * 0.9f, 1.4f);
    p.line(s * hrx * 0.62f, fy - eye * 2.4f, s * hrx * 1.12f, fy - eye * 2.9f, 1.4f);
  }
  if (look.stage == Stage::Adult && look.personality != Personality::Foodie) {
    // A bow on one ear.
    const float x = -hrx * 0.62f, y = b.headY + hry * 0.95f;
    Shape bow[3] = {p.rot(x - 6, y, 6, 4, -20), p.rot(x + 6, y, 6, 4, 20), p.ell(x, y, 2.6f, 2.6f)};
    p.c.outlined(bow, 3, p.R(1.5f), Ink::Gray);
  }
  drawAccessory(p, look, 0, fy, gap, eye, b.headY + hry, b.bodyY + b.bodyRy * 0.55f);
  drawHeadwear(p, look, 0, b.headY + hry * 0.1f, hrx);
}

void drawCloud(const Pen& p, const PetLook& look) {
  const int st = p.stroke();
  const int stage = static_cast<int>(look.stage);
  const float k = 0.8f + stage * 0.07f;  // puffs widen as it grows
  const float lift = 10;

  // Shadow on the ground.
  p.fill(0, 2, 30 * k, 3.5f, Ink::Gray);

  const bool sunny = look.stage == Stage::Adult && !look.hungry && !look.sleeping;
  if (sunny) {  // rainbow behind the adult cloud on good days
    for (int i = 0; i < 3; ++i) {
      p.arc(0, lift + 18, 44 * k + i * 5.5f, 190, 350, 4.2f, i == 1 ? Ink::Gray : Ink::Black);
    }
  }
  Shape puffs[7];
  int n = 0;
  puffs[n++] = Shape::roundRect(p.X(-38 * k), p.Y(lift + 26), p.R(76 * k), p.R(26), p.R(13));
  puffs[n++] = p.ell(0, lift + 34, 24 * k, 22);
  puffs[n++] = p.ell(-24 * k, lift + 24, 17 * k, 15);
  puffs[n++] = p.ell(24 * k, lift + 26, 17 * k, 16);
  if (stage >= 2) puffs[n++] = p.ell(-38 * k, lift + 13, 12, 12);
  if (stage >= 3) puffs[n++] = p.ell(38 * k, lift + 13, 12, 12);
  if (stage >= 4) puffs[n++] = p.ell(12 * k, lift + 46, 13, 12);
  p.c.outlined(puffs, n, st);
  // Fluff curls on top.
  p.arc(-6 * k, lift + 38, 9, 200, 300, 2.0f);
  if (stage >= 3) p.arc(-26 * k, lift + 28, 7, 200, 290, 2.0f);

  const float eye = 4.0f;
  drawFace(p, 0, lift + 22, 26 * k, eye, look, false);

  if (look.hungry || look.pose == Pose::Sad) {  // raindrops
    for (int i = 0; i < 3; ++i) {
      const float x = -18 + i * 18, y = lift - 4 - (i % 2) * 8;
      Shape drop[2] = {p.ell(x, y, 3.2f, 3.2f), p.tri(x - 3.2f, y + 0.5f, x + 3.2f, y + 0.5f, x, y + 8)};
      p.c.outlined(drop, 2, p.R(1.4f), Ink::Gray);
    }
  }
  drawAccessory(p, look, 0, lift + 22, 26 * k, eye, lift + 36, lift + 6);
  drawHeadwear(p, look, 0, lift + 28, 30 * k);
}

void drawLeaf(const Pen& p, float x, float y, float len, float deg) {
  const float a = deg * kPi / 180.0f;
  const float mx = x + std::cos(a) * len * 0.5f, my = y + std::sin(a) * len * 0.5f;
  const Shape leaf = p.rot(mx, my, len * 0.5f, len * 0.24f, -deg);
  p.c.outlined(leaf, p.stroke() - 1);
  p.line(x, y, x + std::cos(a) * len * 0.8f, y + std::sin(a) * len * 0.8f, 1.5f);
}

void drawPlant(const Pen& p, const PetLook& look) {
  const int st = p.stroke();
  const int stage = static_cast<int>(look.stage);
  const float stemTop[] = {40, 54, 66, 78, 86};
  const float top = stemTop[stage];
  const float droop = look.hungry || look.pose == Pose::Sad ? -22.0f : 0.0f;
  const float sway = look.pose == Pose::Excited ? 6.0f : 0.0f;

  // Stem and leaves sit behind the pot rim.
  const Shape stem = p.cap(0, 34, sway, top - 4, 2.6f);
  p.c.outlined(stem, p.R(1.5f), Ink::White);
  drawLeaf(p, sway * 0.6f, top - 8, 20, 30 + droop);
  drawLeaf(p, sway * 0.6f, top - 8, 20, 150 - droop);
  if (stage >= 2) {
    drawLeaf(p, sway * 0.3f, top - 26, 17, 18 + droop);
    drawLeaf(p, sway * 0.3f, top - 22, 15, 162 - droop);
  }
  if (stage == 3) {  // bud
    Shape bud[2] = {p.ell(sway, top + 2, 7, 9), p.tri(sway - 7, top + 4, sway + 7, top + 4, sway, top + 16)};
    p.c.outlined(bud, 2, st, Ink::Gray);
  } else if (stage == 4) {  // bloom
    Shape petals[6];
    for (int i = 0; i < 6; ++i) {
      const float a = i * 60.0f * kPi / 180.0f;
      petals[i] = p.ell(sway + std::cos(a) * 10, top + 6 + std::sin(a) * 10, 7.5f, 7.5f);
    }
    p.c.outlined(petals, 6, st);
    p.c.outlined(p.ell(sway, top + 6, 6.5f, 6.5f), p.R(1.6f), Ink::Gray);
    if (look.personality != Personality::Bookworm) p.c.sparkle(p.X(sway + 22), p.Y(top + 20), p.R(5), Ink::Black);
  }

  // Pot with the face on it.
  Shape pot[2] = {p.tri(-27, 34, 27, 34, 0, 34), Shape{}};
  {
    const int xs[4] = {p.X(-27), p.X(27), p.X(21), p.X(-21)};
    const int ys[4] = {p.Y(34), p.Y(34), p.Y(0), p.Y(0)};
    pot[0] = Shape::poly(xs, ys, 4);
    pot[1] = Shape::roundRect(p.X(-31), p.Y(42), p.R(62), p.R(10), p.R(4));
  }
  p.c.outlined(pot, 2, st);
  p.c.rect(p.X(-27), p.Y(34) - p.R(1), p.R(54), p.R(2), Ink::Black);
  // Soil peeking over the rim.
  p.fill(0, 42, 26, 3, Ink::Dark);

  const float eye = 3.7f;
  drawFace(p, 0, 19, 24, eye, look, false);
  if (look.stage == Stage::Adult && look.personality == Personality::None) {
    p.c.heart(p.X(14), p.Y(7), p.R(6), Ink::Gray);
  }
  drawAccessory(p, look, 0, 19, 24, eye, 30, 8);
  if (look.wear == PetLook::Headphones) drawHeadwear(p, look, 0, 22, 28);
}

}  // namespace

void drawPet(Canvas& c, const PetLook& look, int cx, int baseY, int size) {
  float scale = STAGE_SCALE[static_cast<int>(look.stage)];
  if (look.species == Species::Plant && look.stage >= Stage::Teen) scale *= 0.9f;  // flowers grow tall
  Pen p{c, cx, baseY, size * scale / 100.0f};
  switch (look.species) {
    case Species::Cat:
      drawCat(p, look);
      break;
    case Species::Cloud:
      drawCloud(p, look);
      break;
    case Species::Plant:
      drawPlant(p, look);
      break;
    default:
      drawDog(p, look);
      break;
  }
  if (look.sleeping) {
    drawZzz(c, cx + iround(size * scale * 0.34f), baseY - iround(size * scale * 0.95f), iround(size * 0.12f));
  }
}

void drawZzz(Canvas& c, int x, int y, int size) {
  for (int i = 0; i < 3; ++i) {
    const int s = size - i * size / 4;
    const int ox = x + i * size * 7 / 10, oy = y - i * size * 9 / 10;
    const int t = s / 6 > 2 ? s / 6 : 2;
    c.line(ox, oy, ox + s, oy, t, Ink::Black);
    c.line(ox + s, oy, ox, oy + s, t, Ink::Black);
    c.line(ox, oy + s, ox + s, oy + s, t, Ink::Black);
  }
}

void drawEgg(Canvas& c, Species sp, int cx, int baseY, int size, int cracks) {
  Pen p{c, cx, baseY, size / 100.0f};
  const int st = p.stroke();
  if (sp == Species::Plant) {
    // A seed waiting in its pot.
    const int xs[4] = {p.X(-30), p.X(30), p.X(23), p.X(-23)};
    const int ys[4] = {p.Y(38), p.Y(38), p.Y(0), p.Y(0)};
    Shape pot[2] = {Shape::poly(xs, ys, 4), Shape::roundRect(p.X(-34), p.Y(47), p.R(68), p.R(11), p.R(4))};
    const Shape seed = p.rot(0, 50 + cracks * 3.0f, 9, 12, 20.0f + cracks * 10);
    p.c.outlined(seed, st, Ink::Gray);
    if (cracks >= 1) p.line(-1, 60 + cracks * 3.0f, -1, 66 + cracks * 5.0f, 2.2f);
    if (cracks >= 2) drawLeaf(p, -1, 66 + cracks * 5.0f, 9, 40);
    p.c.outlined(pot, 2, st);
    p.fill(0, 47, 30, 3, Ink::Dark);
    p.c.sparkle(p.X(-30), p.Y(70), p.R(5), Ink::Black);
    return;
  }
  p.fill(0, 2, 30, 4, Ink::Gray);
  const Shape egg = p.ell(0, 44, 33, 42);
  p.c.outlined(egg, st);
  switch (sp) {
    case Species::Dog:  // paw-print spots
      for (int i = 0; i < 2; ++i) {
        const float x = i ? 14 : -12, y = i ? 30 : 56;
        p.fill(x, y, 6, 5, Ink::Gray);
        for (int t = -1; t <= 1; ++t) p.fill(x + t * 5.5f, y + 7 - (t == 0 ? 1.5f : 0), 2.4f, 2.4f, Ink::Gray);
      }
      break;
    case Species::Cat: {  // zigzag band
      const int n = 6;
      for (int i = 0; i < n; ++i) {
        const float x0 = -30 + i * 10.0f, x1 = x0 + 10;
        p.line(x0, i % 2 ? 46.0f : 38.0f, x1, i % 2 ? 38.0f : 46.0f, 3.6f, Ink::Dark);
      }
      p.fill(-12, 64, 3, 3, Ink::Gray);
      p.fill(12, 66, 3, 3, Ink::Gray);
      p.fill(0, 20, 3, 3, Ink::Gray);
      break;
    }
    default: {  // little clouds
      Shape cl[3] = {p.ell(-10, 58, 7, 6), p.ell(-3, 61, 7, 7), p.ell(4, 58, 6, 5)};
      p.c.outlined(cl, 3, p.R(1.4f), Ink::Gray);
      Shape cl2[3] = {p.ell(8, 28, 6, 5), p.ell(14, 31, 6, 6), p.ell(20, 28, 5, 5)};
      p.c.outlined(cl2, 3, p.R(1.4f), Ink::Gray);
      break;
    }
  }
  if (cracks >= 1) {
    const float xs[] = {-18, -10, -4, 4, 10, 18};
    const float ys[] = {50, 44, 52, 44, 52, 46};
    for (int i = 0; i + 1 < 6; ++i) p.line(xs[i], ys[i], xs[i + 1], ys[i + 1], 2.4f);
  }
  if (cracks >= 2) {
    p.line(-4, 52, -8, 62, 2.4f);
    p.line(10, 52, 13, 60, 2.4f);
    p.c.sparkle(p.X(34), p.Y(80), p.R(5), Ink::Black);
    p.c.sparkle(p.X(-36), p.Y(70), p.R(3.5f), Ink::Black);
  }
}

void drawFood(Canvas& c, int item, int cx, int cy, int size) {
  // Design space: 100 units centred on (cx, cy); y up.
  Pen p{c, cx, cy + size / 2, size / 100.0f};
  const int st = p.R(4) < 2 ? 2 : p.R(4);
  switch (item) {
    case 0: {  // berry
      Shape b[2] = {p.ell(0, 44, 26, 24), p.tri(-24, 50, 24, 50, 0, 10)};
      p.c.outlined(b, 2, st, Ink::Gray);
      for (int i = 0; i < 5; ++i) p.fill(-12 + (i % 3) * 12, 44 - (i / 3) * 14 + (i % 2) * 4, 2, 2.5f, Ink::Black);
      Shape leaf[3] = {p.rot(-10, 70, 10, 5, 20), p.rot(10, 70, 10, 5, -20), p.cap(0, 70, 2, 84, 2.5f)};
      p.c.outlined(leaf, 3, st);
      break;
    }
    case 1: {  // cookie
      p.c.outlined(p.ell(0, 50, 34, 32), st);
      const float cx2[] = {-14, 10, -2, 16, -16, 4};
      const float cy2[] = {60, 64, 46, 44, 38, 30};
      for (int i = 0; i < 6; ++i) p.fill(cx2[i], cy2[i], 4, 3.5f, Ink::Black);
      break;
    }
    case 2: {  // juice with a straw
      p.line(6, 70, 20, 92, 4, Ink::Black);
      Shape cup[1] = {Shape{}};
      const int xs[4] = {p.X(-22), p.X(22), p.X(17), p.X(-17)};
      const int ys[4] = {p.Y(74), p.Y(74), p.Y(10), p.Y(10)};
      cup[0] = Shape::poly(xs, ys, 4);
      p.c.outlined(cup, 1, st);
      const int fx[4] = {p.X(-20), p.X(20), p.X(17), p.X(-17)};
      const int fy[4] = {p.Y(52), p.Y(52), p.Y(12), p.Y(12)};
      p.c.poly(fx, fy, 4, Ink::Gray);
      p.c.outlined(p.ell(-20, 72, 11, 11), p.R(3));
      p.line(-20, 63, -20, 81, 2);
      p.line(-29, 72, -11, 72, 2);
      break;
    }
    case 3: {  // rice ball
      const Shape ball = p.tri(-26, 22, 26, 22, 0, 70);
      p.c.outlined(ball, st + p.R(10));
      p.c.shape(ball, p.R(10), Ink::White);
      p.c.rect(p.X(-13), p.Y(30), p.R(26), p.R(26), Ink::Black);
      p.fill(-9, 46, 2.2f, 2.2f, Ink::Black);
      p.fill(9, 46, 2.2f, 2.2f, Ink::Black);
      break;
    }
    case 4: {  // noodle bowl
      for (int i = 0; i < 3; ++i) p.arc(-12 + i * 12, 70, 5, 180, 360, 2.2f);
      p.line(-6, 52, 34, 96, 3.4f);
      p.line(2, 52, 40, 92, 3.4f);
      Shape bowl[2] = {p.ell(0, 46, 40, 30), Shape::roundRect(p.X(-44), p.Y(56), p.R(88), p.R(10), p.R(4))};
      p.c.rect(p.X(-44), p.Y(70), p.R(88), p.R(24), Ink::White);
      p.c.outlined(bowl, 2, st);
      p.c.rect(p.X(-44), p.Y(76), p.R(88), p.R(20), Ink::White);
      p.c.outlined(Shape::roundRect(p.X(-44), p.Y(56), p.R(88), p.R(10), p.R(4)), st, Ink::Gray);
      p.c.outlined(p.ell(14, 60, 9, 6), p.R(2));
      p.fill(14, 60, 4, 3, Ink::Dark);
      break;
    }
    default: {  // bento box
      p.c.outlined(Shape::roundRect(p.X(-42), p.Y(78), p.R(84), p.R(62), p.R(8)), st);
      p.line(0, 76, 0, 18, 3);
      p.line(0, 47, 40, 47, 3);
      for (int i = 0; i < 9; ++i) p.fill(-30 + (i % 3) * 10, 64 - (i / 3) * 14, 2.6f, 2, Ink::Gray);
      p.fill(-20, 50, 6, 6, Ink::Black);
      p.c.heart(p.X(20), p.Y(64), p.R(16), Ink::Gray);
      p.fill(20, 32, 10, 7, Ink::Gray);
      break;
    }
  }
}

void drawIcon(Canvas& c, Icon icon, int cx, int cy, int size) {
  Pen p{c, cx, cy + size / 2, size / 100.0f};
  const float t = 9;
  switch (icon) {
    case Icon::Food:
    case Icon::Bowl: {  // apple
      Shape apple[2] = {p.ell(-14, 44, 28, 34), p.ell(14, 44, 28, 34)};
      p.c.outlined(apple, 2, p.R(9));
      p.line(0, 72, 6, 94, 9);
      const Shape leaf = p.rot(18, 88, 14, 7, -25);
      c.shape(leaf, 0, Ink::Black);
      p.arc(-16, 52, 14, 200, 260, 7);
      break;
    }
    case Icon::Play:
      p.c.ellipse(p.X(0), p.Y(50), p.R(40), p.R(40), Ink::Black);
      p.c.ellipse(p.X(0), p.Y(50), p.R(31), p.R(31), Ink::White);
      p.arc(-60, 50, 52, -40, 40, t);
      p.arc(60, 50, 52, 140, 220, t);
      break;
    case Icon::Pet:
    case Icon::Heart:
      c.heart(cx, cy, size * 8 / 10, Ink::Black);
      if (icon == Icon::Pet) c.heart(cx, cy, size * 5 / 10, Ink::White);
      break;
    case Icon::Apps:
      for (int i = 0; i < 4; ++i) {
        const int x = cx - size * 4 / 10 + (i % 2) * size * 45 / 100;
        const int y = cy - size * 4 / 10 + (i / 2) * size * 45 / 100;
        c.frame(x, y, size * 35 / 100, size * 35 / 100, size / 10, size / 11 + 1, Ink::Black);
      }
      break;
    case Icon::Help:
      p.arc(0, 66, 18, 180, 405, t);
      p.line(13, 54, 0, 42, t);
      p.line(0, 42, 0, 32, t);
      p.fill(0, 14, 6, 6, Ink::Black);
      break;
    case Icon::Gear:
      for (int i = 0; i < 8; ++i) {
        const float a = i * 45 * kPi / 180;
        p.line(std::cos(a) * 30, 50 + std::sin(a) * 30, std::cos(a) * 44, 50 + std::sin(a) * 44, 15);
      }
      p.c.ellipse(p.X(0), p.Y(50), p.R(34), p.R(34), Ink::Black);
      p.c.ellipse(p.X(0), p.Y(50), p.R(14), p.R(14), Ink::White);
      break;
    case Icon::Close:
      p.line(-30, 80, 30, 20, t * 1.2f);
      p.line(-30, 20, 30, 80, t * 1.2f);
      break;
    case Icon::Back:
      p.line(-24, 50, 20, 88, t * 1.2f);
      p.line(-24, 50, 20, 12, t * 1.2f);
      break;
    case Icon::Timer:
      p.c.ellipse(p.X(0), p.Y(44), p.R(40), p.R(40), Ink::Black);
      p.c.ellipse(p.X(0), p.Y(44), p.R(31), p.R(31), Ink::White);
      p.line(0, 44, 0, 68, t);
      p.line(0, 44, 16, 34, t);
      p.c.rect(p.X(-10), p.Y(98), p.R(20), p.R(10), Ink::Black);
      break;
    case Icon::Status:
      c.frame(cx - size * 45 / 100, cy - size * 30 / 100, size * 9 / 10, size * 6 / 10, size / 8, size / 11 + 1,
              Ink::Black);
      p.line(-24, 56, 24, 56, t);
      p.line(-24, 40, 10, 40, t);
      break;
    case Icon::Answers:
    case Icon::Book:
      c.frame(cx - size * 40 / 100, cy - size * 42 / 100, size * 80 / 100, size * 84 / 100, size / 12, size / 11 + 1,
              Ink::Black);
      p.line(-22, 92, -22, 8, t);
      if (icon == Icon::Answers) {
        c.sparkle(p.X(10), p.Y(52), p.R(18), Ink::Black);
      } else {
        p.line(-8, 68, 24, 68, t * 0.7f);
        p.line(-8, 50, 24, 50, t * 0.7f);
      }
      break;
    case Icon::Bolt: {
      const int xs[6] = {p.X(8), p.X(-26), p.X(-2), p.X(-10), p.X(26), p.X(2)};
      const int ys[6] = {p.Y(98), p.Y(42), p.Y(42), p.Y(2), p.Y(60), p.Y(60)};
      c.poly(xs, ys, 6, Ink::Black);
      break;
    }
    case Icon::Star:
      c.sparkle(cx, cy, size / 2, Ink::Black);
      break;
    case Icon::Leaf: {
      const Shape leaf = p.rot(0, 50, 44, 22, -40);
      c.shape(leaf, 0, Ink::Black);
      p.line(-26, 24, 18, 70, 6, Ink::White);
      break;
    }
    case Icon::Moon:
      p.c.ellipse(p.X(0), p.Y(50), p.R(40), p.R(40), Ink::Black);
      p.c.ellipse(p.X(18), p.Y(62), p.R(34), p.R(34), Ink::White);
      break;
    case Icon::Sound:
    case Icon::Mute: {
      const int xs[6] = {p.X(-40), p.X(-20), p.X(4), p.X(4), p.X(-20), p.X(-40)};
      const int ys[6] = {p.Y(64), p.Y(64), p.Y(88), p.Y(12), p.Y(36), p.Y(36)};
      c.poly(xs, ys, 6, Ink::Black);
      if (icon == Icon::Sound) {
        p.arc(4, 50, 18, -45, 45, 7);
        p.arc(4, 50, 34, -45, 45, 7);
      } else {
        p.line(18, 66, 42, 34, 8);
        p.line(18, 34, 42, 66, 8);
      }
      break;
    }
    case Icon::Crystal:
      p.c.ellipse(p.X(0), p.Y(56), p.R(36), p.R(36), Ink::Black);
      p.c.ellipse(p.X(0), p.Y(56), p.R(28), p.R(28), Ink::White);
      c.sparkle(p.X(-8), p.Y(64), p.R(10), Ink::Black);
      c.roundRect(p.X(-30), p.Y(18), p.R(60), p.R(14), p.R(5), Ink::Black);
      break;
    case Icon::Pause:
      c.roundRect(p.X(-30), p.Y(88), p.R(22), p.R(76), p.R(6), Ink::Black);
      c.roundRect(p.X(8), p.Y(88), p.R(22), p.R(76), p.R(6), Ink::Black);
      break;
    case Icon::PlayArrow: {
      const int xs[3] = {p.X(-26), p.X(36), p.X(-26)};
      const int ys[3] = {p.Y(90), p.Y(50), p.Y(10)};
      c.poly(xs, ys, 3, Ink::Black);
      break;
    }
    case Icon::None:
      break;
    case Icon::Stop:
      c.roundRect(p.X(-34), p.Y(84), p.R(68), p.R(68), p.R(10), Ink::Black);
      break;
  }
}

void drawKeepsake(Canvas& c, int index, int cx, int cy, int size) {
  Pen p{c, cx, cy + size / 2, size / 100.0f};
  const int st = p.R(5) < 2 ? 2 : p.R(5);
  switch (index) {
    case 0:
      p.c.outlined(p.rot(0, 46, 38, 28, -10), st, Ink::Gray);
      p.fill(-14, 56, 6, 4, Ink::White);
      break;
    case 1: {
      Shape cl[4] = {p.ell(-14, 58, 15, 15), p.ell(14, 58, 15, 15), p.ell(0, 72, 15, 15), p.cap(0, 50, 6, 10, 3)};
      p.c.outlined(cl, 4, st);
      break;
    }
    case 2:
      p.c.outlined(p.ell(0, 46, 36, 30), st);
      for (int i = -2; i <= 2; ++i) p.line(0, 18, i * 14, 72, 3);
      break;
    case 3:
      p.c.outlined(p.ell(0, 50, 36, 36), st, Ink::Gray);
      p.c.outlined(p.ell(0, 50, 22, 22), p.R(3));
      break;
    case 4:
      p.c.outlined(p.ell(0, 50, 34, 34), st);
      for (int i = 0; i < 4; ++i) p.fill(i % 2 ? 8 : -8, i / 2 ? 42 : 58, 5, 5, Ink::Black);
      break;
    case 5: {
      p.c.outlined(p.ell(0, 44, 26, 36), st, Ink::Gray);
      for (int i = 0; i < 3; ++i) p.line(-22, 24 + i * 18, 22, 32 + i * 18, 2.5f);
      break;
    }
    case 6: {
      int xs[10], ys[10];
      for (int i = 0; i < 10; ++i) {
        const float a = (-90 + i * 36) * kPi / 180;
        const float r = i % 2 ? 18 : 42;
        xs[i] = p.X(std::cos(a) * r);
        ys[i] = p.Y(50 - std::sin(a) * r);
      }
      // Two halves keep within the 8-point shape limit.
      p.c.poly(xs, ys, 10, Ink::Black);
      p.c.sparkle(p.X(0), p.Y(50), p.R(12), Ink::White);
      break;
    }
    default: {
      const Shape f = p.rot(0, 52, 44, 14, -50);
      p.c.outlined(f, st, Ink::Gray);
      p.line(-30, 16, 28, 86, 3);
      break;
    }
  }
}

}  // namespace pp
