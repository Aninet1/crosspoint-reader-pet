#include "Screens.h"

#include <cstdio>
#include <cstring>

namespace pp {

const char* const STATUS_LABELS[STATUS_COUNT] = {"BUSY", "MEETING", "ON CALL", "OPEN TO TALK", "REST", "CUSTOM"};

namespace {

constexpr int PAD = 24;
constexpr int T_SMALL = 17, T_BODY = 20, T_LABEL = 22, T_TITLE = 30;

void fmtDuration(uint32_t sec, char* buf, int n) {
  const uint32_t m = sec / 60;
  if (m >= 60) {
    snprintf(buf, n, "%luh %02lum", static_cast<unsigned long>(m / 60), static_cast<unsigned long>(m % 60));
  } else {
    snprintf(buf, n, "%lum", static_cast<unsigned long>(m));
  }
}

void fmtCost(int minutes, char* buf, int n) {
  if (minutes >= 60)
    snprintf(buf, n, "%dh", minutes / 60);
  else
    snprintf(buf, n, "%dm", minutes);
}

// Chunky card with a hard drop shadow. Locked cards get a dotted border.
void card(Canvas& c, const Box& b, bool focused, bool enabled = true, int r = 18) {
  if (enabled) c.roundRect(b.x + 5, b.y + 5, b.w, b.h, r, Ink::Black);
  c.frame(b.x, b.y, b.w, b.h, r, focused ? 6 : (enabled ? 3 : 2), Ink::Black);
  if (!enabled) {
    for (int x = b.x + 14; x < b.x + b.w - 14; x += 9) {
      c.rect(x, b.y + 8, 4, 2, Ink::Black);
      c.rect(x, b.y + b.h - 10, 4, 2, Ink::Black);
    }
  }
}

void pill(Canvas& c, const Box& b, const char* label, bool focused, bool dark = false, int size = T_LABEL) {
  const int r = b.h / 2 < 26 ? b.h / 2 : 26;
  c.roundRect(b.x + 4, b.y + 4, b.w, b.h, r, dark ? Ink::Gray : Ink::Black);
  if (dark) {
    c.roundRect(b.x, b.y, b.w, b.h, r, Ink::Black);
    if (focused) c.frame(b.x + 6, b.y + 6, b.w - 12, b.h - 12, r - 6, 3, Ink::White, Ink::Black);
  } else {
    c.frame(b.x, b.y, b.w, b.h, r, focused ? 6 : 3, Ink::Black);
  }
  c.textCentered(b.cx(), b.cy() - size * 62 / 100, label, size, true, dark ? Ink::White : Ink::Black);
}

void iconButton(Canvas& c, const Box& b, Icon icon, bool focused) {
  c.circle(b.cx() + 3, b.cy() + 3, b.w / 2, Ink::Black);
  c.circle(b.cx(), b.cy(), b.w / 2, Ink::Black);
  c.circle(b.cx(), b.cy(), b.w / 2 - (focused ? 6 : 3), Ink::White);
  drawIcon(c, icon, b.cx(), b.cy(), b.w * 46 / 100);
}

void focusMarks(Canvas& c, const Box& b) {
  const int l = 22;
  const Box k = b.inset(-6);
  c.line(k.x, k.y, k.x + l, k.y, 5, Ink::Black);
  c.line(k.x, k.y, k.x, k.y + l, 5, Ink::Black);
  c.line(k.x + k.w, k.y, k.x + k.w - l, k.y, 5, Ink::Black);
  c.line(k.x + k.w, k.y, k.x + k.w, k.y + l, 5, Ink::Black);
  c.line(k.x, k.y + k.h, k.x + l, k.y + k.h, 5, Ink::Black);
  c.line(k.x, k.y + k.h, k.x, k.y + k.h - l, 5, Ink::Black);
  c.line(k.x + k.w, k.y + k.h, k.x + k.w - l, k.y + k.h, 5, Ink::Black);
  c.line(k.x + k.w, k.y + k.h, k.x + k.w, k.y + k.h - l, 5, Ink::Black);
}

void bar(Canvas& c, int x, int y, int w, Icon icon, const char* label, int pct) {
  drawIcon(c, icon, x + 12, y + 22, 24);
  c.text(x + 34, y - 2, label, T_SMALL, true);
  const int bx = x + 34, by = y + 22, bw = w - 34, bh = 16;
  c.roundRect(bx, by, bw, bh, 8, Ink::Black);
  c.roundRect(bx + 3, by + 3, bw - 6, bh - 6, 5, Ink::White);
  const int p = pct < 0 ? 0 : (pct > 100 ? 100 : pct);
  const int fw = (bw - 6) * p / 100;
  if (fw > 2) c.roundRect(bx + 3, by + 3, fw, bh - 6, 5, Ink::Dark);
}

// Speech bubble with a tail pointing down at tailX.
void bubble(Canvas& c, const Box& b, int tailX, const char* text, int size = T_LABEL) {
  const int tx[3] = {tailX - 14, tailX + 14, tailX + 2};
  const int ty[3] = {b.y + b.h - 4, b.y + b.h - 4, b.y + b.h + 20};
  c.roundRect(b.x + 5, b.y + 5, b.w, b.h, 22, Ink::Gray);
  c.roundRect(b.x - 3, b.y - 3, b.w + 6, b.h + 6, 25, Ink::Black);
  const int ox[3] = {tx[0] - 5, tx[1] + 5, tx[2]};
  const int oy[3] = {ty[0], ty[1], ty[2] + 6};
  c.poly(ox, oy, 3, Ink::Black);
  c.roundRect(b.x, b.y, b.w, b.h, 22, Ink::White);
  c.poly(tx, ty, 3, Ink::White);
  const int n = c.countLines(b.w - 36, text, size, 2);
  const int y = b.cy() - (n * (size + 6) - 6) / 2 - size / 6;
  c.textWrapped(b.cx(), y, b.w - 36, text, size, 6, 2);
}

void header(Canvas& c, const char* title, bool focusBack) {
  iconButton(c, Box{16, 16, 52, 52}, Icon::Back, focusBack);
  c.textCentered(c.width() / 2, 24, title, T_TITLE - 2, true);
}

Button backButton() { return Button{Box{16, 16, 52, 52}, Act::Close, 0}; }

PetLook lookFor(const PetModel& pet, const Clock& clk) {
  PetLook l;
  l.species = pet.s.species;
  l.stage = pet.s.stage == Stage::Egg ? Stage::Baby : pet.s.stage;
  l.personality = pet.s.personality;
  l.pose = pet.lastPose;
  const Mode m = pet.mode(clk);
  l.sleeping = m == Mode::Sleeping || m == Mode::Napping;
  l.hungry = pet.s.fullness < 30 && !l.sleeping;
  if (l.hungry && l.pose != Pose::Eating) l.pose = Pose::Sad;
  return l;
}

void sceneBackdrop(Canvas& c, int w, int top, int ground, bool night) {
  c.rect(0, ground, w, 4, Ink::Black);
  c.rect(0, ground + 4, w, 22, Ink::Gray);
  for (int i = 0; i < 6; ++i) {
    const int x = 30 + i * (w - 60) / 5;
    c.line(x, ground, x - 6, ground - 10, 3, Ink::Black);
    c.line(x, ground, x + 5, ground - 13, 3, Ink::Black);
  }
  if (night) {
    drawIcon(c, Icon::Moon, w - 64, top + 120, 44);
    c.sparkle(54, top + 120, 9, Ink::Black);
    c.sparkle(96, top + 190, 6, Ink::Black);
    c.sparkle(w - 120, top + 200, 6, Ink::Black);
  } else {
    for (int k = 0; k < 2; ++k) {
      const int cx = k ? w - 70 : 66, cy = top + (k ? 130 : 200);
      c.ellipse(cx, cy, 34, 13, Ink::Gray);
      c.ellipse(cx - 14, cy - 8, 16, 12, Ink::Gray);
      c.ellipse(cx + 10, cy - 12, 18, 14, Ink::Gray);
    }
  }
}

// ---------------------------------------------------------------- Home ----

constexpr int HOME_GROUND = 604;
constexpr int HOME_PET_SIZE = 262;
constexpr int SCENE_TOP = 236;

int homeButtons(const PetModel& pet, const Clock& clk, int w, int h, Button* out) {
  int n = 0;
  out[n++] = {Box{w - 72, 18, 52, 52}, Act::OpenSettings, 0};
  out[n++] = {Box{w - 134, 18, 52, 52}, Act::OpenHelp, 0};
  if (pet.mode(clk) == Mode::Away) {
    out[n++] = {Box{w / 2 - 110, 500, 220, 62}, Act::CallBack, 0};
  } else {
    out[n++] = {Box{70, 330, w - 140, HOME_GROUND - 330}, Act::TapPet, 0};
  }
  const int tw = (w - 2 * PAD - 3 * 14) / 4;
  const Act acts[4] = {Act::OpenFood, Act::Play, Act::PetPet, Act::OpenApps};
  for (int i = 0; i < 4; ++i) out[n++] = {Box{PAD + i * (tw + 14), h - 150, tw, 112}, acts[i], 0};
  return n;
}

void drawHome(Canvas& c, const UiState& ui, const PetModel& pet, const Clock& clk, const Button* b, int nb) {
  const int w = c.width();
  const PetState& s = pet.s;
  const Mode m = pet.mode(clk);
  char buf[96];

  c.text(PAD, 14, s.name, T_TITLE + 4, true);
  if (s.stage == Stage::Egg) {
    snprintf(buf, sizeof(buf), "%s %s", PetModel::speciesName(s.species), PetModel::stageName(s.stage, s.species));
  } else {
    snprintf(
        buf, sizeof(buf), "%s %s  |  Day %d",
        s.personality != Personality::None ? PetModel::personalityName(s.personality)
                                           : PetModel::stageName(s.stage, s.species),
        s.personality != Personality::None ? PetModel::stageName(s.stage, s.species) : PetModel::speciesName(s.species),
        pet.ageDays(clk) + 1);
  }
  c.text(PAD, 60, buf, T_BODY);

  const int colW = (w - 2 * PAD - 24) / 2;
  bar(c, PAD, 98, colW, Icon::Heart, "Love", s.love);
  bar(c, PAD + colW + 24, 98, colW, Icon::Food, "Fullness", s.fullness);
  bar(c, PAD, 150, colW, Icon::Bolt, "Energy", s.energy);
  bar(c, PAD + colW + 24, 150, colW, Icon::Star, s.stage == Stage::Adult ? "All grown up" : "Growth",
      pet.stageProgressPct());

  char t1[24];
  fmtDuration(s.readCreditSec, t1, sizeof(t1));
  snprintf(buf, sizeof(buf), "Snack time  %s", t1);
  drawIcon(c, Icon::Book, PAD + 11, 213, 22);
  c.text(PAD + 32, 201, buf, T_SMALL, true);
  if (s.streak > 0) {
    snprintf(buf, sizeof(buf), "%u-day streak", s.streak);
    const int tw = c.textWidth(buf, T_SMALL, true);
    drawIcon(c, Icon::Leaf, w - PAD - tw - 18, 213, 22);
    c.text(w - PAD - tw, 201, buf, T_SMALL, true);
  }

  sceneBackdrop(c, w, SCENE_TOP, HOME_GROUND, pet.isNight(clk));
  const Box bub{PAD + 10, SCENE_TOP + 8, w - 2 * PAD - 20, 74};
  if (m == Mode::Away) {
    c.rect(w / 2 - 5, 420, 10, HOME_GROUND - 420, Ink::Black);
    const Box sign{w / 2 - 125, 350, 250, 92};
    card(c, sign, false);
    c.textCentered(w / 2, sign.y + 14, "Out exploring!", T_LABEL, true);
    if (clk.valid && s.awayUntil > clk.epoch) {
      const uint32_t left = static_cast<uint32_t>(s.awayUntil - clk.epoch);
      char t[24];
      fmtDuration(left < 60 ? 60 : left, t, sizeof(t));
      snprintf(buf, sizeof(buf), "Back in about %s", t);
    } else {
      snprintf(buf, sizeof(buf), "Back soon");
    }
    c.textCentered(w / 2, sign.y + 52, buf, T_SMALL);
    bubble(c, bub, w / 2, ui.bubble[0] ? ui.bubble : "Gone for a walk! Call me back anytime.");
  } else if (s.stage == Stage::Egg) {
    drawEgg(c, s.species, w / 2, HOME_GROUND - 6, 230, s.hatchTaps);
    const char* msg = s.species == Species::Plant
                          ? (s.hatchTaps == 0 ? "Tap the seed to water it!" : "A little more water...")
                          : (s.hatchTaps == 0 ? "Tap the egg three times!" : "Something is wiggling...");
    bubble(c, bub, w / 2, ui.bubble[0] ? ui.bubble : msg);
  } else {
    drawPet(c, lookFor(pet, clk), w / 2, HOME_GROUND - 4, HOME_PET_SIZE);
    bubble(c, bub, w / 2 + 12, ui.bubble[0] ? ui.bubble : "Hello there!");
  }

  const Icon icons[4] = {Icon::Food, Icon::Play, Icon::Pet, Icon::Apps};
  const char* labels[4] = {"Food", "Play", "Pat", "Apps"};
  for (int i = 0; i < nb; ++i) {
    const Box& t = b[i].box;
    const bool f = ui.focus == i;
    switch (b[i].act) {
      case Act::OpenSettings:
        iconButton(c, t, Icon::Gear, f);
        break;
      case Act::OpenHelp:
        iconButton(c, t, Icon::Help, f);
        break;
      case Act::CallBack:
        pill(c, t, "Call back", f, true);
        break;
      case Act::TapPet:
        if (f) focusMarks(c, t);
        break;
      default: {
        int k = -1;
        if (b[i].act == Act::OpenFood) k = 0;
        if (b[i].act == Act::Play) k = 1;
        if (b[i].act == Act::PetPet) k = 2;
        if (b[i].act == Act::OpenApps) k = 3;
        if (k < 0) break;
        card(c, t, f);
        drawIcon(c, icons[k], t.cx(), t.y + 42, 44);
        c.textCentered(t.cx(), t.y + 72, labels[k], T_LABEL, true);
      }
    }
  }
}

// ------------------------------------------------------------- Sheets -----

constexpr int SHEET_TOP = 300;

void sheet(Canvas& c, int top) {
  const int w = c.width(), h = c.height();
  c.roundRect(-4, top - 4, w + 8, h - top + 40, 30, Ink::Black);
  c.roundRect(0, top, w, h - top + 40, 28, Ink::White);
  c.roundRect(w / 2 - 34, top + 10, 68, 7, 3, Ink::Gray);
}

int foodButtons(int w, int h, Button* out) {
  int n = 0;
  out[n++] = {Box{w - 76, SHEET_TOP + 26, 52, 52}, Act::Close, 0};
  const int tw = (w - 2 * PAD - 2 * 16) / 3, th = 150;
  for (int i = 0; i < FOOD_COUNT; ++i) {
    out[n++] = {Box{PAD + (i % 3) * (tw + 16), SHEET_TOP + 128 + (i / 3) * (th + 18), tw, th}, Act::Feed, i};
  }
  (void)h;
  return n;
}

void drawFoodSheet(Canvas& c, const UiState& ui, const PetModel& pet, const Button* b, int nb) {
  const int w = c.width(), h = c.height();
  sheet(c, SHEET_TOP);
  drawIcon(c, Icon::Food, PAD + 20, SHEET_TOP + 50, 40);
  c.text(PAD + 52, SHEET_TOP + 26, "Food", T_TITLE, true);
  char t[24], buf[64];
  fmtDuration(pet.s.readCreditSec, t, sizeof(t));
  snprintf(buf, sizeof(buf), "Reading time: %s", t);
  c.text(PAD + 52, SHEET_TOP + 70, buf, T_BODY);
  for (int i = 0; i < nb; ++i) {
    const Box& x = b[i].box;
    if (b[i].act == Act::Close) {
      iconButton(c, x, Icon::Close, ui.focus == i);
      continue;
    }
    const bool ok = pet.canAfford(b[i].arg);
    card(c, x, ui.focus == i, ok);
    drawFood(c, b[i].arg, x.cx(), x.y + 56, 78);
    fmtCost(FOODS[b[i].arg].costMin, t, sizeof(t));
    c.textCentered(x.cx(), x.y + x.h - 40, t, T_LABEL, true);
  }
  c.textCentered(w / 2, h - 52, "Every minute you read = a minute of snacks", T_SMALL);
}

int launcherButtons(int w, int h, Button* out) {
  int n = 0;
  out[n++] = {Box{w - 76, SHEET_TOP + 26, 52, 52}, Act::Close, 0};
  const int cw = (w - 2 * PAD - 18) / 2, ch = 114;
  const Act acts[4] = {Act::AppPet, Act::AppPomodoro, Act::AppStatus, Act::AppAnswers};
  for (int i = 0; i < 4; ++i) {
    out[n++] = {Box{PAD + (i % 2) * (cw + 18), SHEET_TOP + 96 + (i / 2) * (ch + 16), cw, ch}, acts[i], 0};
  }
  out[n++] = {Box{PAD, h - 92, w - 2 * PAD, 58}, Act::ExitToReader, 0};
  return n;
}

void drawLauncher(Canvas& c, const UiState& ui, const Button* b, int nb) {
  const int w = c.width();
  sheet(c, SHEET_TOP);
  c.text(PAD + 6, SHEET_TOP + 30, "Apps", T_TITLE, true);
  const Icon icons[4] = {Icon::Pet, Icon::Timer, Icon::Status, Icon::Answers};
  const char* labels[4] = {"My pet", "Focus timer", "Status board", "Answers"};
  for (int i = 0; i < nb; ++i) {
    const Box& x = b[i].box;
    const bool f = ui.focus == i;
    if (b[i].act == Act::Close) {
      iconButton(c, x, Icon::Close, f);
    } else if (b[i].act == Act::ExitToReader) {
      pill(c, x, "Back to my books", f, true);
    } else {
      const int k = static_cast<int>(b[i].act) - static_cast<int>(Act::AppPet);
      card(c, x, f);
      drawIcon(c, icons[k], x.cx(), x.y + 42, 50);
      c.textCentered(x.cx(), x.y + 76, labels[k], T_LABEL, true);
    }
  }
  c.textCentered(w / 2, c.height() - 128, "Shake for Answers  |  Turn sideways for Status", T_SMALL);
}

// ----------------------------------------------------------- Pomodoro -----

void drawSegment(Canvas& c, int x0, int y0, int x1, int y1, int t) { c.capsule(x0, y0, x1, y1, t / 2, Ink::Black); }

}  // namespace

void drawBigDigits(Canvas& c, int cx, int y, int dh, const char* text) {
  // Rounded seven-segment digits: a b c d e f g.
  static const uint8_t SEG[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};
  const int dw = dh * 55 / 100, t = dh * 15 / 100, gap = dh * 18 / 100, colon = dh * 22 / 100;
  int total = 0;
  for (const char* p = text; *p; ++p) total += (*p == ':' ? colon : dw) + gap;
  total -= gap;
  int x = cx - total / 2;
  for (const char* p = text; *p; ++p) {
    if (*p == ':') {
      c.circle(x + colon / 2, y + dh * 30 / 100, t * 6 / 10, Ink::Black);
      c.circle(x + colon / 2, y + dh * 72 / 100, t * 6 / 10, Ink::Black);
      x += colon + gap;
      continue;
    }
    const int d = *p - '0';
    if (d < 0 || d > 9) continue;
    const uint8_t s = SEG[d];
    if (d == 1) {  // centre the lone stroke of a 1
      const int m = x + dw / 2;
      drawSegment(c, m, y + t / 2, m, y + dh - t / 2, t);
      c.capsule(m, y + t / 2, m - dw / 4, y + dh / 5, t / 2, Ink::Black);
      x += dw + gap;
      continue;
    }
    const int l = x + t / 2, r = x + dw - t / 2, top = y + t / 2, mid = y + dh / 2, bot = y + dh - t / 2;
    if (s & 0x01) drawSegment(c, l, top, r, top, t);
    if (s & 0x02) drawSegment(c, r, top, r, mid, t);
    if (s & 0x04) drawSegment(c, r, mid, r, bot, t);
    if (s & 0x08) drawSegment(c, l, bot, r, bot, t);
    if (s & 0x10) drawSegment(c, l, mid, l, bot, t);
    if (s & 0x20) drawSegment(c, l, top, l, mid, t);
    if (s & 0x40) drawSegment(c, l, mid, r, mid, t);
    x += dw + gap;
  }
}

namespace {

int pomoButtons(const UiState& ui, int w, int h, Button* out) {
  int n = 0;
  out[n++] = backButton();
  switch (ui.pomo) {
    case PomoPhase::Setup: {
      const int bw = (w - 2 * PAD - 2 * 16) / 3;
      const int presets[3] = {15, 25, 60};
      for (int i = 0; i < 3; ++i) out[n++] = {Box{PAD + i * (bw + 16), 430, bw, 124}, Act::PomoPreset, presets[i]};
      out[n++] = {Box{PAD, 586, w - 2 * PAD, 68}, Act::PomoCustom, 0};
      break;
    }
    case PomoPhase::Custom: {
      const int fw = (w - 2 * PAD - 2 * 16) / 3;
      for (int i = 0; i < 3; ++i) out[n++] = {Box{PAD + i * (fw + 16), 96, fw, 96}, Act::PomoField, i};
      const int kw = (w - 2 * PAD - 2 * 14) / 3, kh = 76;
      for (int i = 0; i < 12; ++i) {
        const Box k{PAD + (i % 3) * (kw + 14), 232 + (i / 3) * (kh + 14), kw, kh};
        if (i < 9)
          out[n++] = {k, Act::PomoKey, i + 1};
        else if (i == 9)
          out[n++] = {k, Act::PomoClear, 0};
        else if (i == 10)
          out[n++] = {k, Act::PomoKey, 0};
        else
          out[n++] = {k, Act::PomoDelete, 0};
      }
      out[n++] = {Box{PAD, h - 108, w - 2 * PAD, 70}, Act::PomoStart, 0};
      break;
    }
    case PomoPhase::Running:
    case PomoPhase::Paused: {
      const int bw = (w - 2 * PAD - 18) / 2;
      out[n++] = {Box{PAD, h - 110, bw, 72}, Act::PomoPause, 0};
      out[n++] = {Box{PAD + bw + 18, h - 110, bw, 72}, Act::PomoEnd, 0};
      break;
    }
    case PomoPhase::ConfirmEnd: {
      const int bw = (w - 2 * PAD - 18 - 40) / 2;
      out[n++] = {Box{PAD + 20, h - 130, bw, 66}, Act::PomoEndNo, 0};
      out[n++] = {Box{PAD + 38 + bw, h - 130, bw, 66}, Act::PomoEndYes, 0};
      break;
    }
    case PomoPhase::Alarm:
      out[n++] = {Box{PAD, h - 110, w - 2 * PAD, 72}, Act::PomoAlarmStop, 0};
      break;
  }
  return n;
}

void drawPomodoro(Canvas& c, const UiState& ui, const PetModel& pet, const Clock& clk, const Button* b, int nb) {
  const int w = c.width(), h = c.height();
  header(c, ui.pomo == PomoPhase::Custom ? "Custom time" : "Focus timer", ui.focus == 0);
  PetLook look = lookFor(pet, clk);
  look.sleeping = false;
  look.hungry = false;
  char buf[48];

  if (ui.pomo == PomoPhase::Setup) {
    look.pose = Pose::Idle;
    if (pet.s.stage == Stage::Egg)
      drawEgg(c, pet.s.species, w / 2, 368, 170, pet.s.hatchTaps);
    else
      drawPet(c, look, w / 2, 368, 190);
    c.rect(PAD + 40, 370, w - 2 * PAD - 80, 4, Ink::Black);
    bubble(c, Box{PAD + 40, 96, w - 2 * PAD - 80, 64}, w / 2, "How long shall we focus?");
    c.textCentered(w / 2, 392, "Pick a session", T_BODY);
    snprintf(buf, sizeof(buf), "Sessions finished: %u", pet.s.pomodoros);
    c.textCentered(w / 2, h - 74, buf, T_SMALL);
  }

  for (int i = 1; i < nb; ++i) {
    const Box& x = b[i].box;
    const bool f = ui.focus == i;
    switch (b[i].act) {
      case Act::PomoPreset:
        card(c, x, f);
        snprintf(buf, sizeof(buf), "%d", b[i].arg);
        c.textCentered(x.cx(), x.y + 18, buf, 52, true);
        c.textCentered(x.cx(), x.y + 82, "min", T_BODY);
        break;
      case Act::PomoCustom:
        pill(c, x, "Custom time", f);
        break;
      case Act::PomoField: {
        const bool active = ui.customField == b[i].arg;
        card(c, x, f || active);
        if (active) c.roundRect(x.x + 10, x.y + x.h - 16, x.w - 20, 6, 3, Ink::Black);
        snprintf(buf, sizeof(buf), "%02u", ui.custom[b[i].arg]);
        c.textCentered(x.cx(), x.y + 10, buf, 46, true);
        const char* unit[3] = {"hours", "minutes", "seconds"};
        c.textCentered(x.cx(), x.y + x.h + 6, unit[b[i].arg], T_SMALL);
        break;
      }
      case Act::PomoKey:
        card(c, x, f, true, 14);
        snprintf(buf, sizeof(buf), "%d", b[i].arg);
        c.textCentered(x.cx(), x.y + 16, buf, 34, true);
        break;
      case Act::PomoClear:
        card(c, x, f, true, 14);
        c.textCentered(x.cx(), x.y + 24, "CLEAR", T_LABEL, true);
        break;
      case Act::PomoDelete:
        card(c, x, f, true, 14);
        c.textCentered(x.cx(), x.y + 24, "DELETE", T_LABEL, true);
        break;
      case Act::PomoStart:
        pill(c, x, "Start", f, true);
        break;
      case Act::PomoPause:
        pill(c, x, ui.pomo == PomoPhase::Paused ? "Resume" : "Pause", f);
        break;
      case Act::PomoEnd:
        pill(c, x, "End", f, true);
        break;
      case Act::PomoEndNo:
        pill(c, x, "Keep going", f);
        break;
      case Act::PomoEndYes:
        pill(c, x, "End", f, true);
        break;
      case Act::PomoAlarmStop:
        pill(c, x, "END", f, true, 26);
        break;
      default:
        break;
    }
  }

  if (ui.pomo == PomoPhase::Running || ui.pomo == PomoPhase::Paused || ui.pomo == PomoPhase::ConfirmEnd) {
    const uint32_t left = ui.pomoLeftSec;
    if (left >= 3600) {
      snprintf(buf, sizeof(buf), "%lu:%02lu:%02lu", static_cast<unsigned long>(left / 3600),
               static_cast<unsigned long>(left / 60 % 60), static_cast<unsigned long>(left % 60));
    } else {
      snprintf(buf, sizeof(buf), "%02lu:%02lu", static_cast<unsigned long>(left / 60),
               static_cast<unsigned long>(left % 60));
    }
    drawBigDigits(c, w / 2, 110, left >= 3600 ? 96 : 120, buf);
    // Progress bar.
    const int bx = PAD + 20, bw = w - 2 * PAD - 40, by = 268;
    c.roundRect(bx, by, bw, 22, 11, Ink::Black);
    c.roundRect(bx + 4, by + 4, bw - 8, 14, 7, Ink::White);
    const uint32_t total = ui.pomoTotalSec ? ui.pomoTotalSec : 1;
    const int fw = static_cast<int>((bw - 8) * static_cast<uint64_t>(total - left) / total);
    if (fw > 4) c.roundRect(bx + 4, by + 4, fw, 14, 7, Ink::Dark);
    c.textCentered(w / 2, 300, ui.pomo == PomoPhase::Paused ? "Paused" : "Focusing...", T_BODY, true);

    if (ui.pomo == PomoPhase::ConfirmEnd) {
      const Box cardBox{PAD, h - 250, w - 2 * PAD, 200};
      c.roundRect(cardBox.x + 5, cardBox.y + 5, cardBox.w, cardBox.h, 24, Ink::Black);
      c.frame(cardBox.x, cardBox.y, cardBox.w, cardBox.h, 24, 3, Ink::Black);
      c.textCentered(w / 2, cardBox.y + 26, "End this session?", T_LABEL + 2, true);
      c.textCentered(w / 2, cardBox.y + 64, "You're doing great so far.", T_BODY);
      for (int i = 1; i < nb; ++i) {
        if (b[i].act == Act::PomoEndNo) pill(c, b[i].box, "Keep going", ui.focus == i);
        if (b[i].act == Act::PomoEndYes) pill(c, b[i].box, "End", ui.focus == i, true);
      }
    } else {
      look.pose = ui.pomo == PomoPhase::Paused ? Pose::Idle : Pose::Happy;
      if (pet.s.personality == Personality::None && pet.s.stage >= Stage::Teen)
        look.personality = Personality::Bookworm;
      if (pet.s.stage != Stage::Egg)
        drawPet(c, look, w / 2, h - 150, 230);
      else
        drawEgg(c, pet.s.species, w / 2, h - 150, 180, pet.s.hatchTaps);
      c.rect(PAD + 30, h - 148, w - 2 * PAD - 60, 4, Ink::Black);
    }
  } else if (ui.pomo == PomoPhase::Alarm) {
    c.textCentered(w / 2, 110, "Time's up!", 52, true);
    c.textCentered(w / 2, 184, "Great focus. Take a little break.", T_BODY);
    look.pose = Pose::Excited;
    if (pet.s.stage != Stage::Egg) drawPet(c, look, w / 2, h - 160, 280);
    c.sparkle(PAD + 70, 300, 18, Ink::Black);
    c.sparkle(w - PAD - 60, 360, 12, Ink::Black);
    c.sparkle(PAD + 40, 440, 9, Ink::Black);
    c.rect(PAD + 30, h - 158, w - 2 * PAD - 60, 4, Ink::Black);
  }
}

// ------------------------------------------------------- Status board -----

int statusMenuButtons(int w, int h, Button* out) {
  int n = 0;
  out[n++] = backButton();
  const int tw = (w - 2 * PAD - 2 * 18) / 3, th = (h - 112 - PAD - 18) / 2;
  for (int i = 0; i < STATUS_COUNT; ++i) {
    out[n++] = {Box{PAD + (i % 3) * (tw + 18), 100 + (i / 3) * (th + 18), tw, th}, Act::StatusPick, i};
  }
  return n;
}

void statusProp(Canvas& c, int idx, int cx, int cy) {
  switch (idx) {
    case 0:
      c.textCentered(cx, cy - 26, "!", 56, true);
      break;
    case 1:
      for (int i = -1; i <= 1; ++i) c.circle(cx + i * 18, cy, 7, Ink::Black);
      break;
    case 2: {  // mobile phone with sound waves
      c.roundRect(cx - 15, cy - 26, 30, 52, 8, Ink::Black);
      c.roundRect(cx - 10, cy - 19, 20, 32, 3, Ink::White);
      c.circle(cx, cy + 19, 3, Ink::White);
      c.arc(cx + 14, cy - 4, 14, -50, 50, 4, Ink::Black);
      c.arc(cx - 14, cy - 4, 14, 130, 230, 4, Ink::Black);
      break;
    }
    case 3:
      c.heart(cx, cy, 44, Ink::Black);
      break;
    case 4:
      drawIcon(c, Icon::Moon, cx, cy, 46);
      break;
    default:
      c.sparkle(cx, cy, 22, Ink::Black);
      break;
  }
}

void drawStatusMenu(Canvas& c, const UiState& ui, const Button* b, int nb) {
  header(c, "Status board", ui.focus == 0);
  for (int i = 1; i < nb; ++i) {
    const Box& x = b[i].box;
    card(c, x, ui.focus == i);
    statusProp(c, b[i].arg, x.cx(), x.y + 46);
    const char* label = b[i].arg == 5 ? ui.customStatus : STATUS_LABELS[b[i].arg];
    c.textWrapped(x.cx(), x.y + 82, x.w - 20, label, T_LABEL + 2, 4, 2, true);
    if (b[i].arg == 5) c.textCentered(x.cx(), x.y + x.h - 30, "hold to edit", T_SMALL - 2);
  }
}

void drawStatusFull(Canvas& c, const UiState& ui, const PetModel& pet, const Clock& clk) {
  const int w = c.width(), h = c.height();
  const char* label = ui.status == 5 ? ui.customStatus : STATUS_LABELS[ui.status];
  c.frame(10, 10, w - 20, h - 20, 30, 6, Ink::Black);
  const int textW = w * 58 / 100;
  const int size = c.textWidth(label, 80, true) > textW - 40 ? 64 : 80;
  const int lines = c.countLines(textW - 40, label, size, 3, true);
  c.textWrapped(40 + textW / 2, h / 2 - lines * (size + 10) / 2 - 10, textW - 40, label, size, 10, 3, true);
  c.textCentered(40 + textW / 2, h - 64, "Tap to change", T_SMALL);

  PetLook look = lookFor(pet, clk);
  look.hungry = false;
  look.sleeping = ui.status == 4;
  look.pose = ui.status == 3 || ui.status == 5 ? Pose::Happy : Pose::Idle;
  const int pcx = w - 160, ground = h - 70;
  c.rect(w - 300, ground, 260, 4, Ink::Black);
  if (pet.s.stage == Stage::Egg)
    drawEgg(c, pet.s.species, pcx, ground, 200, pet.s.hatchTaps);
  else
    drawPet(c, look, pcx, ground, 250);
  if (ui.status != 4) {
    c.roundRect(pcx + 34, 40, 96, 80, 26, Ink::Black);
    c.roundRect(pcx + 38, 44, 88, 72, 22, Ink::White);
    statusProp(c, ui.status, pcx + 82, 80);
  }
}

// ---------------------------------------------------- Book of Answers -----

int answersButtons(const UiState& ui, int w, int h, Button* out) {
  int n = 0;
  out[n++] = backButton();
  if (ui.ans == AnsPhase::Result) {
    out[n++] = {Box{PAD, h - 110, w - 2 * PAD, 70}, Act::AnsAgain, 0};
    return n;
  }
  const int sw = (w - 2 * PAD - 12) / 2;
  out[n++] = {Box{PAD, 92, sw, 56}, Act::AnsMode, 0};
  out[n++] = {Box{PAD + sw + 12, 92, sw, 56}, Act::AnsMode, 1};
  out[n++] = {Box{w / 2 - 150, 190, 300, 300}, Act::AnsHold, 0};
  return n;
}

void drawBook(Canvas& c, int cx, int cy, int s, bool open) {
  if (open) {
    const int x0 = cx - s / 2, y0 = cy - s / 3;
    c.roundRect(x0 + 6, y0 + 6, s, s * 2 / 3, 16, Ink::Black);
    c.frame(x0, y0, s / 2 + 3, s * 2 / 3, 16, 4, Ink::Black);
    c.frame(cx - 3, y0, s / 2 + 3, s * 2 / 3, 16, 4, Ink::Black);
    for (int i = 0; i < 4; ++i) {
      c.rect(x0 + 24, y0 + 30 + i * 26, s / 2 - 50, 4, Ink::Gray);
      c.rect(cx + 24, y0 + 30 + i * 26, s / 2 - 50, 4, Ink::Gray);
    }
    return;
  }
  const Box bk{cx - s * 38 / 100, cy - s / 2, s * 76 / 100, s};
  c.roundRect(bk.x + 8, bk.y + 8, bk.w, bk.h, 22, Ink::Black);
  c.frame(bk.x, bk.y, bk.w, bk.h, 22, 5, Ink::Black);
  c.rect(bk.x + 26, bk.y, 5, bk.h, Ink::Black);
  c.roundRect(bk.x + 50, bk.y + 40, bk.w - 76, 46, 12, Ink::Gray);
  c.sparkle(bk.cx() + 12, bk.cy() + 22, s / 7, Ink::Black);
  c.sparkle(bk.cx() + s / 6, bk.cy() - s / 20, s / 18, Ink::Black);
  c.sparkle(bk.cx() - s / 10, bk.cy() + s / 4, s / 20, Ink::Black);
}

void drawCrystal(Canvas& c, int cx, int cy, int s) {
  const int r = s * 38 / 100;
  c.roundRect(cx - r + 6, cy + r - 14, 2 * r - 12, s / 6, 14, Ink::Black);
  c.circle(cx + 5, cy - s / 10 + 5, r, Ink::Black);
  c.circle(cx, cy - s / 10, r, Ink::Black);
  c.circle(cx, cy - s / 10, r - 6, Ink::White);
  c.circle(cx - r / 3, cy - s / 10 - r / 3, r / 6, Ink::Gray);
  c.sparkle(cx + r / 4, cy - s / 10 + r / 6, s / 10, Ink::Black);
  c.roundRect(cx - r + 6, cy + r - 14, 2 * r - 12, s / 6, 14, Ink::Black);
  c.roundRect(cx - r + 12, cy + r - 8, 2 * r - 24, s / 6 - 12, 10, Ink::Gray);
}

void drawAnswers(Canvas& c, const UiState& ui, const Button* b, int nb) {
  const int w = c.width(), h = c.height();
  header(c, "Book of Answers", ui.focus == 0);
  if (ui.ans == AnsPhase::Result) {
    if (ui.yesNo) {
      drawCrystal(c, w / 2, 300, 300);
      c.textCentered(w / 2, 470, yesNoText(ui.answer), 80, true);
    } else {
      drawBook(c, w / 2, 230, 360, true);
      c.textWrapped(w / 2, 410, w - 2 * PAD - 20, answerText(ui.answer), 34, 10, 4, true);
    }
    c.textCentered(w / 2, h - 150, "Ask another question?", T_BODY);
    for (int i = 1; i < nb; ++i) pill(c, b[i].box, "Ask again", ui.focus == i, true);
    return;
  }
  for (int i = 1; i < nb; ++i) {
    const Box& x = b[i].box;
    if (b[i].act == Act::AnsMode) {
      const bool on = (b[i].arg == 1) == ui.yesNo;
      if (on) {
        c.roundRect(x.x, x.y, x.w, x.h, x.h / 2, Ink::Black);
        c.textCentered(x.cx(), x.y + 14, b[i].arg ? "YES OR NO" : "MESSAGE", T_BODY, true, Ink::White);
      } else {
        c.frame(x.x, x.y, x.w, x.h, x.h / 2, ui.focus == i ? 5 : 3, Ink::Black);
        c.textCentered(x.cx(), x.y + 14, b[i].arg ? "YES OR NO" : "MESSAGE", T_BODY, true);
      }
    } else if (b[i].act == Act::AnsHold) {
      if (ui.focus == i) focusMarks(c, x);
      if (ui.yesNo)
        drawCrystal(c, x.cx(), x.cy(), 280);
      else
        drawBook(c, x.cx(), x.cy(), 270, false);
    }
  }
  const char* msg;
  char buf[96];
  if (ui.ans == AnsPhase::Thinking) {
    snprintf(buf, sizeof(buf), "Thinking%s", ui.thinkFrame % 3 == 0 ? "." : ui.thinkFrame % 3 == 1 ? ".." : "...");
    msg = buf;
  } else if (ui.ans == AnsPhase::Retry) {
    msg = ui.hasImu ? "Shake a little longer! Keep going for three seconds." : "Hold a little longer!";
  } else {
    msg = ui.hasImu ? "Hold a question in your mind, then shake me for three seconds."
                    : "Hold a question in your mind, then press and hold the book.";
  }
  c.textWrapped(w / 2, 540, w - 2 * PAD - 40, msg, T_LABEL, 8, 3, ui.ans == AnsPhase::Thinking);
  if (ui.shakePct > 0 && ui.ans != AnsPhase::Thinking) {
    const int bx = PAD + 40, bw = w - 2 * PAD - 80, by = 660;
    c.roundRect(bx, by, bw, 20, 10, Ink::Black);
    c.roundRect(bx + 4, by + 4, bw - 8, 12, 6, Ink::White);
    const int fw = (bw - 8) * (ui.shakePct > 100 ? 100 : ui.shakePct) / 100;
    if (fw > 4) c.roundRect(bx + 4, by + 4, fw, 12, 6, Ink::Dark);
  }
  if (ui.ans == AnsPhase::Thinking) {
    for (int i = 0; i < 3; ++i) c.sparkle(w / 2 - 60 + i * 60, 680 - ((ui.thinkFrame + i) % 2) * 14, 10, Ink::Black);
  }
  (void)h;
}

// -------------------------------------------------------------- Guide -----

int guideButtons(const UiState& ui, int w, int h, Button* out) {
  int n = 0;
  out[n++] = backButton();
  const int bw = (w - 2 * PAD - 18) / 2;
  if (ui.guidePage > 0) out[n++] = {Box{PAD, h - 104, bw, 66}, Act::GuidePrev, 0};
  out[n++] = {Box{PAD + bw + 18, h - 104, bw, 66}, Act::GuideNext, 0};
  return n;
}

void drawGuide(Canvas& c, const UiState& ui, const PetModel& pet, const Button* b, int nb) {
  const int w = c.width(), h = c.height();
  static const char* const TITLES[GUIDE_PAGES] = {"Welcome!",   "Caring for your pet", "Reading is food",
                                                  "Growing up", "Little apps",         "Controls"};
  static const char* const BODY[GUIDE_PAGES] = {
      "A tiny friend lives in your reader now. Read, care for it, and watch it grow from an egg into a "
      "grown-up with its own personality.",
      "Love grows with pats and play. Fullness drops over time, so feed it. Energy comes back with naps and "
      "sleep (10pm to 7am). Daily care adds Growth, up to 10 a day.",
      "Every minute you spend reading a book earns one minute of snack time. Spend it in Food: bigger "
      "meals cost more reading but fill your pet up more.",
      "Egg, Baby, Child, Teen, Adult. As a teen it becomes a Foodie, Cuddly, Active or Bookworm, "
      "depending on how you cared for it. Care every day to build a streak.",
      "Open Apps for a focus timer, a status board for your desk, and the Book of Answers. Finished focus "
      "sessions count as care too.",
      "Tap to choose. Up and Down move, OK selects, swipe from the edge to go back. In Apps, shake for "
      "Answers or turn the Sticky sideways for the Status board.",
  };
  const int p = ui.guidePage;
  header(c, "How to play", ui.focus == 0);
  c.textCentered(w / 2, 96, TITLES[p], T_TITLE, true);

  const int artY = 370;
  c.rect(PAD + 20, artY, w - 2 * PAD - 40, 4, Ink::Black);
  PetLook l;
  l.species = pet.s.species;
  switch (p) {
    case 0:
      for (int i = 0; i < 4; ++i) {
        l.species = static_cast<Species>(i);
        l.stage = Stage::Baby;
        l.pose = i % 2 ? Pose::Happy : Pose::Idle;
        drawPet(c, l, PAD + 50 + i * (w - 2 * PAD - 100) / 3, artY, 190);
      }
      break;
    case 1:
      bar(c, PAD + 30, 170, w - 2 * PAD - 60, Icon::Heart, "Love", 80);
      bar(c, PAD + 30, 222, w - 2 * PAD - 60, Icon::Food, "Fullness", 55);
      bar(c, PAD + 30, 274, w - 2 * PAD - 60, Icon::Bolt, "Energy", 35);
      bar(c, PAD + 30, 326, w - 2 * PAD - 60, Icon::Star, "Growth", 65);
      break;
    case 2:
      drawBook(c, PAD + 80, 270, 130, false);
      c.textCentered(PAD + 175, 250, "=", 44, true);
      for (int i = 0; i < 3; ++i) drawFood(c, i * 2 + 1, w / 2 + 40 + i * 70, 270, 60);
      break;
    case 3:
      for (int i = 0; i < 5; ++i) {
        const int x = PAD + 34 + i * (w - 2 * PAD - 68) / 4;
        if (i == 0)
          drawEgg(c, pet.s.species, x, artY, 80, 0);
        else {
          l.stage = static_cast<Stage>(i);
          l.personality = i >= 3 ? Personality::Bookworm : Personality::None;
          drawPet(c, l, x, artY, 108);
        }
      }
      break;
    case 4: {
      const Icon icons[3] = {Icon::Timer, Icon::Status, Icon::Answers};
      for (int i = 0; i < 3; ++i) {
        const Box x{PAD + 20 + i * (w - 2 * PAD - 40) / 3, 200, (w - 2 * PAD - 40) / 3 - 16, 130};
        card(c, x, false);
        drawIcon(c, icons[i], x.cx(), x.cy(), 60);
      }
      break;
    }
    default:
      drawIcon(c, Icon::Apps, w / 2 - 110, 270, 70);
      drawIcon(c, Icon::Star, w / 2, 270, 60);
      drawIcon(c, Icon::Status, w / 2 + 110, 270, 70);
      break;
  }
  c.textWrapped(w / 2, 410, w - 2 * PAD - 30, BODY[p], T_BODY + 1, 9, 8);
  for (int i = 0; i < GUIDE_PAGES; ++i) {
    const int x = w / 2 - (GUIDE_PAGES - 1) * 13 + i * 26;
    if (i == p)
      c.circle(x, h - 136, 7, Ink::Black);
    else
      c.circle(x, h - 136, 5, Ink::Gray);
  }
  for (int i = 1; i < nb; ++i) {
    if (b[i].act == Act::GuidePrev) pill(c, b[i].box, "Back", ui.focus == i);
    if (b[i].act == Act::GuideNext) pill(c, b[i].box, p == GUIDE_PAGES - 1 ? "Let's go!" : "Next", ui.focus == i, true);
  }
}

// ------------------------------------------- Settings and collection -----

int settingsButtons(int w, Button* out) {
  int n = 0;
  out[n++] = backButton();
  const Act acts[5] = {Act::SetRename, Act::SetSound, Act::SetCollection, Act::SetGuide, Act::SetNewPet};
  for (int i = 0; i < 5; ++i) out[n++] = {Box{PAD, 100 + i * 94, w - 2 * PAD, 78}, acts[i], 0};
  return n;
}

void drawSettings(Canvas& c, const UiState& ui, const PetModel& pet, const Button* b, int nb) {
  header(c, "Settings", ui.focus == 0);
  const Icon icons[5] = {Icon::Pet, pet.s.soundOn ? Icon::Sound : Icon::Mute, Icon::Star, Icon::Help, Icon::Leaf};
  for (int i = 1; i < nb; ++i) {
    const Box& x = b[i].box;
    card(c, x, ui.focus == i);
    drawIcon(c, icons[i - 1], x.x + 40, x.cy(), 34);
    const char* label = "";
    const char* value = "";
    switch (b[i].act) {
      case Act::SetRename:
        label = "Name";
        value = pet.s.name;
        break;
      case Act::SetSound:
        label = "Sounds";
        value = pet.s.soundOn ? "On" : "Off";
        break;
      case Act::SetCollection:
        label = "Keepsakes & stats";
        break;
      case Act::SetGuide:
        label = "How to play";
        break;
      case Act::SetNewPet:
        label = "Adopt a new egg";
        break;
      default:
        break;
    }
    c.text(x.x + 80, x.cy() - 14, label, T_LABEL, true);
    if (*value) c.text(x.x + x.w - 24 - c.textWidth(value, T_LABEL), x.cy() - 14, value, T_LABEL);
  }
  c.textCentered(c.width() / 2, c.height() - 60, "Pocket Pets for CrossPoint", T_SMALL);
}

void drawCollection(Canvas& c, const UiState& ui, const PetModel& pet, const Clock& clk) {
  const int w = c.width();
  header(c, "Keepsakes", ui.focus == 0);
  c.textWrapped(w / 2, 90, w - 2 * PAD, "Things your pet brought back from its walks.", T_BODY, 6, 2);
  const int cw = (w - 2 * PAD - 3 * 14) / 4;
  for (int i = 0; i < KEEPSAKE_COUNT; ++i) {
    const Box x{PAD + (i % 4) * (cw + 14), 150 + (i / 4) * (cw + 50), cw, cw};
    const bool have = pet.s.keepsakes & (1u << i);
    card(c, x, false, have, 16);
    if (have) {
      drawKeepsake(c, i, x.cx(), x.cy(), cw * 6 / 10);
    } else {
      c.textCentered(x.cx(), x.cy() - 22, "?", 40, true);
    }
  }
  char t[24], buf[64];
  int y = 150 + 2 * (cw + 50) + 10;
  fmtDuration(pet.s.totalReadSec, t, sizeof(t));
  snprintf(buf, sizeof(buf), "Time spent reading:  %s", t);
  c.text(PAD + 10, y, buf, T_LABEL);
  snprintf(buf, sizeof(buf), "Focus sessions:  %u", pet.s.pomodoros);
  c.text(PAD + 10, y + 44, buf, T_LABEL);
  snprintf(buf, sizeof(buf), "Best care streak:  %u day%s", pet.s.bestStreak, pet.s.bestStreak == 1 ? "" : "s");
  c.text(PAD + 10, y + 88, buf, T_LABEL);
  snprintf(buf, sizeof(buf), "Age:  %d day%s", pet.ageDays(clk), pet.ageDays(clk) == 1 ? "" : "s");
  c.text(PAD + 10, y + 132, buf, T_LABEL);
}

int confirmButtons(int w, int h, Button* out) {
  const int bw = (w - 2 * PAD - 18 - 40) / 2;
  out[0] = {Box{PAD + 20, h / 2 + 60, bw, 66}, Act::ConfirmNo, 0};
  out[1] = {Box{PAD + 38 + bw, h / 2 + 60, bw, 66}, Act::ConfirmYes, 0};
  return 2;
}

void drawConfirm(Canvas& c, const UiState& ui, const PetModel& pet, const Clock& clk, const Button* b) {
  const int w = c.width(), h = c.height();
  PetLook l = lookFor(pet, clk);
  l.pose = Pose::Sad;
  l.sleeping = false;
  if (pet.s.stage != Stage::Egg) drawPet(c, l, w / 2, h / 2 - 150, 200);
  const Box x{PAD, h / 2 - 120, w - 2 * PAD, 280};
  c.roundRect(x.x + 5, x.y + 5, x.w, x.h, 26, Ink::Black);
  c.frame(x.x, x.y, x.w, x.h, 26, 3, Ink::Black);
  c.textCentered(w / 2, x.y + 30, "Adopt a new egg?", T_TITLE - 2, true);
  c.textWrapped(w / 2, x.y + 80, x.w - 50, "Your current pet will wave goodbye. Your snack time stays with you.",
                T_BODY, 6, 3);
  pill(c, b[0].box, "Keep my pet", ui.focus == 0);
  pill(c, b[1].box, "New egg", ui.focus == 1, true);
}

int adoptButtons(int w, int h, Button* out) {
  const int cw = (w - 2 * PAD - 18) / 2, ch = (h - 200 - PAD - 18) / 2;
  for (int i = 0; i < 4; ++i) {
    out[i] = {Box{PAD + (i % 2) * (cw + 18), 170 + (i / 2) * (ch + 18), cw, ch}, Act::Adopt, i};
  }
  return 4;
}

void drawAdopt(Canvas& c, const UiState& ui, const Button* b, int nb) {
  const int w = c.width();
  c.textCentered(w / 2, 40, "Choose your egg", T_TITLE + 6, true);
  c.textCentered(w / 2, 96, "Who will hatch? You can't peek!", T_BODY);
  const char* hints[4] = {"Spotty egg", "Stripy egg", "Fluffy egg", "Seed pot"};
  for (int i = 0; i < nb; ++i) {
    const Box& x = b[i].box;
    card(c, x, ui.focus == i);
    drawEgg(c, static_cast<Species>(b[i].arg), x.cx(), x.y + x.h - 70, 190, 0);
    c.textCentered(x.cx(), x.y + x.h - 52, hints[b[i].arg], T_LABEL, true);
  }
}

}  // namespace

const char* bannerTitle(Event e) {
  switch (e) {
    case Event::Hatched:
      return "Hello, world!";
    case Event::GrewUp:
      return "Growing up!";
    case Event::Personality:
      return "A personality blooms!";
    case Event::Milestone:
      return "Care streak!";
    case Event::Returned:
      return "Welcome home!";
    default:
      return "";
  }
}

int layout(const UiState& ui, const PetModel& pet, const Clock& clk, int w, int h, Button* out) {
  if (ui.banner != Event::None && bannerTitle(ui.banner)[0] && ui.screen == Screen::Home) {
    out[0] = {Box{0, 0, w, h}, Act::DismissBanner, 0};
    return 1;
  }
  switch (ui.screen) {
    case Screen::Adopt:
      return adoptButtons(w, h, out);
    case Screen::Home:
      return homeButtons(pet, clk, w, h, out);
    case Screen::Food:
      return foodButtons(w, h, out);
    case Screen::Launcher:
      return launcherButtons(w, h, out);
    case Screen::Pomodoro:
      return pomoButtons(ui, w, h, out);
    case Screen::StatusMenu:
      return statusMenuButtons(w, h, out);
    case Screen::StatusFull:
      out[0] = {Box{0, 0, w, h}, Act::StatusBack, 0};
      return 1;
    case Screen::Answers:
      return answersButtons(ui, w, h, out);
    case Screen::Guide:
      return guideButtons(ui, w, h, out);
    case Screen::Settings:
      return settingsButtons(w, out);
    case Screen::Collection:
      out[0] = backButton();
      return 1;
    case Screen::ConfirmNew:
      return confirmButtons(w, h, out);
  }
  return 0;
}

namespace {
void drawBanner(Canvas& c, const UiState& ui, const PetModel& pet, const Clock& clk) {
  const int w = c.width();
  const Box x{PAD + 6, 230, w - 2 * PAD - 12, 360};
  c.roundRect(x.x + 7, x.y + 7, x.w, x.h, 30, Ink::Black);
  c.frame(x.x, x.y, x.w, x.h, 30, 4, Ink::Black);
  c.sparkle(x.x + 40, x.y + 40, 14, Ink::Black);
  c.sparkle(x.x + x.w - 44, x.y + 56, 10, Ink::Black);
  c.sparkle(x.x + x.w - 70, x.y + 26, 6, Ink::Black);
  c.textCentered(w / 2, x.y + 28, bannerTitle(ui.banner), T_TITLE, true);
  char buf[120];
  const PetState& s = pet.s;
  switch (ui.banner) {
    case Event::Hatched:
      snprintf(buf, sizeof(buf), "%s %s! Say hi to %s.",
               s.species == Species::Plant ? "It sprouted. A tiny" : "It's a baby",
               s.species == Species::Plant ? "plant" : PetModel::speciesName(s.species), s.name);
      break;
    case Event::GrewUp:
      snprintf(buf, sizeof(buf), "%s is now a %s. All that care is paying off!", s.name,
               PetModel::stageName(s.stage, s.species));
      break;
    case Event::Personality:
      snprintf(buf, sizeof(buf), "%s became a teen and turned out to be a %s!", s.name,
               PetModel::personalityName(s.personality));
      break;
    case Event::Milestone:
      snprintf(buf, sizeof(buf), "%d days of care in a row. %s feels so loved.", ui.bannerValue, s.name);
      break;
    case Event::Returned:
      snprintf(buf, sizeof(buf), "%s came back with %s!", s.name,
               s.pendingKeepsake >= 0 ? KEEPSAKES[s.pendingKeepsake] : "a big smile");
      break;
    default:
      buf[0] = 0;
      break;
  }
  c.textWrapped(w / 2, x.y + 80, x.w - 50, buf, T_LABEL, 7, 3);
  if (ui.banner == Event::Returned && s.pendingKeepsake >= 0) {
    drawKeepsake(c, s.pendingKeepsake, w / 2, x.y + 230, 100);
  } else if (s.stage != Stage::Egg) {
    PetLook l = lookFor(pet, clk);
    l.pose = Pose::Excited;
    l.sleeping = false;
    l.hungry = false;
    drawPet(c, l, w / 2, x.y + x.h - 46, 150);
  }
  c.textCentered(w / 2, x.y + x.h - 38, "Tap to continue", T_SMALL, true);
}
}  // namespace

void drawScreen(Canvas& c, const UiState& ui, const PetModel& pet, const Clock& clk) {
  Button b[MAX_BUTTONS];
  c.clear();
  UiState base = ui;
  base.banner = Event::None;
  const int nb = layout(base, pet, clk, c.width(), c.height(), b);
  switch (ui.screen) {
    case Screen::Adopt:
      drawAdopt(c, ui, b, nb);
      break;
    case Screen::Home:
      drawHome(c, ui, pet, clk, b, nb);
      break;
    case Screen::Food: {
      Button hb[MAX_BUTTONS];
      UiState home = base;
      home.screen = Screen::Home;
      home.focus = -1;
      const int hn = layout(home, pet, clk, c.width(), c.height(), hb);
      drawHome(c, home, pet, clk, hb, hn);
      drawFoodSheet(c, ui, pet, b, nb);
      break;
    }
    case Screen::Launcher: {
      Button hb[MAX_BUTTONS];
      UiState home = base;
      home.screen = Screen::Home;
      home.focus = -1;
      const int hn = layout(home, pet, clk, c.width(), c.height(), hb);
      drawHome(c, home, pet, clk, hb, hn);
      drawLauncher(c, ui, b, nb);
      break;
    }
    case Screen::Pomodoro:
      drawPomodoro(c, ui, pet, clk, b, nb);
      break;
    case Screen::StatusMenu:
      drawStatusMenu(c, ui, b, nb);
      break;
    case Screen::StatusFull:
      drawStatusFull(c, ui, pet, clk);
      break;
    case Screen::Answers:
      drawAnswers(c, ui, b, nb);
      break;
    case Screen::Guide:
      drawGuide(c, ui, pet, b, nb);
      break;
    case Screen::Settings:
      drawSettings(c, ui, pet, b, nb);
      break;
    case Screen::Collection:
      drawCollection(c, ui, pet, clk);
      break;
    case Screen::ConfirmNew:
      drawConfirm(c, ui, pet, clk, b);
      break;
  }
  if (ui.screen == Screen::Home && ui.banner != Event::None && bannerTitle(ui.banner)[0]) {
    drawBanner(c, ui, pet, clk);
  }
}

}  // namespace pp
