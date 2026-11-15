#include "PocketPetsActivity.h"

#include <GfxRenderer.h>
#include <HalTiltSensor.h>
#include <Logging.h>

#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "MappedInputManager.h"
#include "activities/util/KeyboardEntryActivity.h"
#include "fontIds.h"
#include "pocketpets/PetStore.h"
#include "pocketpets/TodoSync.h"

using namespace pp;

namespace {

constexpr uint32_t TICK_MS = 60 * 1000;
constexpr uint32_t HOME_REFRESH_MS = 10 * 60 * 1000;
constexpr uint32_t POSE_HOLD_MS = 20 * 1000;
constexpr uint32_t MOTION_POLL_MS = 40;
constexpr uint32_t SHAKE_NEEDED_MS = 3000;
constexpr uint32_t LAUNCHER_SHAKE_MS = 800;
constexpr uint32_t SHAKE_GAP_MS = 450;
constexpr uint32_t ALARM_REPEAT_MS = 2500;
constexpr uint32_t ALARM_MAX_MS = 2 * 60 * 1000;
constexpr int FAST_REFRESHES_BEFORE_CLEAN = 14;

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

const char* reactionFor(Species sp, uint32_t r) {
  static const char* const COMMON[] = {"Hehe, that tickles!", "More pats, please!", "*happy wiggle*", "I like you!"};
  static const char* const DOG[] = {"*tail wag*", "Belly rubs next?"};
  static const char* const CAT[] = {"*purr purr*", "...fine, one more."};
  static const char* const CLOUD[] = {"*fluffs up*", "So cosy!"};
  static const char* const PLANT[] = {"*leaves sway*", "I feel greener already!"};
  if (r % 3 == 0) {
    const char* const* t = sp == Species::Dog ? DOG : sp == Species::Cat ? CAT : sp == Species::Cloud ? CLOUD : PLANT;
    return t[(r / 3) % 2];
  }
  return COMMON[r % 4];
}

}  // namespace

// ------------------------------------------------------------- lifecycle --

void PocketPetsActivity::onEnter() {
  Activity::onEnter();
  savedOrientation = renderer.getOrientation();
  buzzerReady = buzzer.begin();
  ui.hasImu = halTiltSensor.isAvailable();
  ui.todo = &TODO_STORE.list;
  strncpy(ui.customStatus, PET_STORE.customStatus, sizeof(ui.customStatus) - 1);

  PetModel& pet = PET_STORE.model;
  const Clock clk = PetStore::now();
  {
    RenderLock lock;
    renderer.setOrientation(GfxRenderer::Portrait);
    pet.tick(clk);
  }
  lastTickMs = lastHomeRefreshMs = millis();
  if (!pet.s.adopted) {
    go(Screen::Adopt);
  } else if (!pet.s.guideSeen) {
    go(Screen::Guide);
  } else {
    sayLine();
    checkBanners();
    go(Screen::Home);
  }
}

void PocketPetsActivity::onExit() {
  halTiltSensor.setMotionHold(false);
  if (buzzerReady) {
    buzzer.noTone();
    buzzer.end();
  }
  {
    RenderLock lock;
    renderer.setOrientation(savedOrientation);
  }
  PET_STORE.flush();
  Activity::onExit();
}

bool PocketPetsActivity::preventAutoSleep() {
  if (ui.screen == Screen::Pomodoro &&
      (ui.pomo == PomoPhase::Running || ui.pomo == PomoPhase::Alarm || ui.pomo == PomoPhase::ConfirmEnd)) {
    return true;
  }
  return ui.screen == Screen::Answers && ui.ans == AnsPhase::Thinking;
}

bool PocketPetsActivity::motionActive() const {
  if (!ui.hasImu) return false;
  if (ui.screen == Screen::Launcher) return true;
  return ui.screen == Screen::Answers && (ui.ans == AnsPhase::Ask || ui.ans == AnsPhase::Retry);
}

void PocketPetsActivity::go(Screen screen) {
  const bool wasLandscape = isLandscape(ui.screen);
  {
    RenderLock lock;
    ui.screen = screen;
    ui.focus = -1;
    if (isLandscape(screen) != wasLandscape || screen == Screen::Adopt) fullRefreshNext = true;
    renderer.setOrientation(isLandscape(screen) ? GfxRenderer::LandscapeCounterClockwise : GfxRenderer::Portrait);
    if (screen == Screen::Answers) {
      ui.ans = AnsPhase::Ask;
      ui.shakePct = 0;
    }
    if (screen == Screen::Todo && ui.todoPage * TODO_PER_PAGE >= TODO_STORE.list.count) ui.todoPage = 0;
  }
  if (screen == Screen::Todo) {
    TODO_STORE.importTokenFromSd();
    if (TODO_STORE.list.syncDue(PetStore::now())) requestTodoSync();
  }
  shakeMs = 0;
  baseAxis = -1;
  rotateStableCount = 0;
  lastShownShakePct = 0;
  halTiltSensor.setMotionHold(motionActive());
  relayout();
  requestUpdate();
}

void PocketPetsActivity::relayout() {
  const Clock clk = PetStore::now();
  buttonCount = layout(ui, PET_STORE.model, clk, renderer.getScreenWidth(), renderer.getScreenHeight(), buttons);
  if (ui.focus >= buttonCount) ui.focus = buttonCount - 1;
}

void PocketPetsActivity::render(RenderLock&&) {
  const Clock clk = PetStore::now();
  DeviceSurface surface(renderer);
  Canvas canvas(surface);
  drawScreen(canvas, ui, PET_STORE.model, clk);

  HalDisplay::RefreshMode mode = HalDisplay::FAST_REFRESH;
  if (fullRefreshNext || ++fastRefreshes >= FAST_REFRESHES_BEFORE_CLEAN) {
    mode = HalDisplay::HALF_REFRESH;
    fastRefreshes = 0;
    fullRefreshNext = false;
  }
  renderer.displayBuffer(mode);
}

// -------------------------------------------------------------- the loop --

void PocketPetsActivity::loop() {
  const uint32_t now = millis();
  PetModel& pet = PET_STORE.model;

  // Time passes for the pet.
  if (now - lastTickMs >= TICK_MS) {
    lastTickMs = now;
    const Clock clk = PetStore::now();
    const Mode before = pet.mode(clk);
    {
      RenderLock lock;
      pet.tick(clk);
    }
    PET_STORE.markDirty();
    checkBanners();
    if (ui.screen == Screen::Home &&
        (pet.mode(clk) != before || ui.banner != Event::None || now - lastHomeRefreshMs >= HOME_REFRESH_MS)) {
      lastHomeRefreshMs = now;
      sayLine();
      relayout();
      requestUpdate();
    }
  }
  if (poseResetAtMs && now >= poseResetAtMs && ui.screen == Screen::Home) {
    poseResetAtMs = 0;
    {
      RenderLock lock;
      pet.lastPose = Pose::Idle;
    }
    sayLine();
    requestUpdate();
  }

  if (todoSyncAtMs && now >= todoSyncAtMs) {
    todoSyncAtMs = 0;
    TODO_STORE.sync();
    {
      RenderLock lock;
      ui.syncing = false;
      if (ui.todoPage * TODO_PER_PAGE >= TODO_STORE.list.count) ui.todoPage = 0;
    }
    relayout();
    requestUpdate();
  }

  updatePomodoro(now);
  updateMotion(now);

  // Taps and long presses.
  relayout();
  int x = 0, y = 0;
  if (mappedInput.wasScreenLongPress(x, y)) {
    for (int i = 0; i < buttonCount; ++i) {
      if (buttons[i].box.contains(x, y) && buttons[i].act == Act::StatusPick && buttons[i].arg == STATUS_CUSTOM) {
        editText(false);
        return;
      }
    }
  }
  if (mappedInput.wasScreenTapped(x, y)) {
    for (int i = 0; i < buttonCount; ++i) {
      if (buttons[i].box.contains(x, y)) {
        act(buttons[i]);
        return;
      }
    }
    return;
  }

  // Physical keys: Up/Down move the focus, OK activates, Back goes back.
  using B = MappedInputManager::Button;
  if (mappedInput.wasReleased(B::Back)) {
    back();
    return;
  }
  const bool prev =
      mappedInput.wasReleased(B::Up) || mappedInput.wasReleased(B::Left) || mappedInput.wasReleased(B::PageBack);
  const bool next =
      mappedInput.wasReleased(B::Down) || mappedInput.wasReleased(B::Right) || mappedInput.wasReleased(B::PageForward);
  if ((prev || next) && buttonCount > 0) {
    if (ui.focus < 0)
      ui.focus = next ? 0 : buttonCount - 1;
    else
      ui.focus = (ui.focus + (next ? 1 : buttonCount - 1)) % buttonCount;
    requestUpdate();
    return;
  }
  if (mappedInput.wasReleased(B::Confirm) && buttonCount > 0) {
    if (ui.focus < 0) {
      ui.focus = 0;
      requestUpdate();
    } else {
      act(buttons[ui.focus]);
    }
  }
}

// ---------------------------------------------------------------- actions --

void PocketPetsActivity::say(const char* text) {
  RenderLock lock;
  snprintf(ui.bubble, sizeof(ui.bubble), "%s", text);
}

void PocketPetsActivity::sayLine() {
  char buf[96];
  RenderLock lock;
  PET_STORE.speak(PetStore::now(), buf, sizeof(buf));
  snprintf(ui.bubble, sizeof(ui.bubble), "%s", buf);
}

void PocketPetsActivity::checkBanners() {
  PetModel& pet = PET_STORE.model;
  RenderLock lock;
  if (ui.banner != Event::None) return;
  const Event e = pet.lastEvent;
  if (e == Event::Hatched || e == Event::GrewUp || e == Event::Personality || e == Event::Returned) {
    ui.banner = e;
    pet.lastEvent = Event::None;
    return;
  }
  const int m = pet.pendingMilestone();
  if (m > 0) {
    ui.banner = Event::Milestone;
    ui.bannerValue = m;
  }
}

void PocketPetsActivity::afterCare(Event e) {
  PET_STORE.markDirty();
  poseResetAtMs = millis() + POSE_HOLD_MS;
  (void)e;
  checkBanners();
  if (ui.banner != Event::None) chirp(2);
}

void PocketPetsActivity::chirp(int kind) {
  if (!buzzerReady || !PET_STORE.model.s.soundOn) return;
  switch (kind) {
    case 0:  // pat
      buzzer.tone(1900, 35);
      buzzer.tone(2500, 45);
      break;
    case 1:  // munch
      for (int i = 0; i < 3; ++i) {
        buzzer.tone(900, 25);
        delay(30);
      }
      break;
    case 2:  // celebration
      buzzer.tone(1568, 70);
      buzzer.tone(1976, 70);
      buzzer.tone(2349, 120);
      break;
    case 3:  // gentle alarm
      buzzer.tone(1319, 120);
      delay(60);
      buzzer.tone(1760, 160);
      break;
    default:  // tap
      buzzer.tone(2200, 20);
      break;
  }
}

void PocketPetsActivity::adoptKeepingCredit(Species sp) {
  PetModel& pet = PET_STORE.model;
  RenderLock lock;
  const uint32_t credit = pet.s.adopted ? pet.s.readCreditSec : 0;
  const uint32_t total = pet.s.adopted ? pet.s.totalReadSec : 0;
  const uint16_t pomos = pet.s.pomodoros;
  pet.adopt(sp, nullptr, PetStore::now());
  pet.s.readCreditSec = credit;
  pet.s.totalReadSec = total;
  pet.s.pomodoros = pomos;
  ui.bubble[0] = 0;
}

void PocketPetsActivity::act(const Button& b) {
  PetModel& pet = PET_STORE.model;
  const Clock clk = PetStore::now();
  const bool egg = pet.s.stage == Stage::Egg;
  ui.focus = -1;

  switch (b.act) {
    case Act::DismissBanner: {
      RenderLock lock;
      if (ui.banner == Event::Milestone) pet.ackMilestone();
      if (ui.banner == Event::Returned) pet.s.pendingKeepsake = -1;
      ui.banner = Event::None;
      PET_STORE.markDirty();
    }
      checkBanners();
      break;

    case Act::Adopt:
      adoptKeepingCredit(static_cast<Species>(b.arg));
      PET_STORE.markDirty();
      PET_STORE.flush();
      chirp(2);
      go(pet.s.guideSeen ? Screen::Home : Screen::Guide);
      return;

    case Act::TapPet:
      if (egg) {
        Event e;
        {
          RenderLock lock;
          e = pet.tapEgg(clk);
          ui.bubble[0] = 0;
        }
        chirp(e == Event::Hatched ? 2 : 4);
        afterCare(e);
        if (e == Event::Hatched) say("Hi! I'm new here.");
        break;
      }
      [[fallthrough]];
    case Act::PetPet: {
      if (egg) {
        say("Hatch me first! Tap the egg.");
        break;
      }
      Event e;
      {
        RenderLock lock;
        e = pet.pet(clk);
      }
      if (e == Event::Away) {
        say("I'm out exploring!");
        break;
      }
      const Mode m = pet.mode(clk);
      say(m == Mode::Sleeping || m == Mode::Napping ? "Zzz... *smiles in its sleep*"
                                                    : reactionFor(pet.s.species, pet.s.rng));
      chirp(0);
      afterCare(e);
      break;
    }

    case Act::Play: {
      if (egg) {
        say("Hatch me first! Tap the egg.");
        break;
      }
      Event e;
      {
        RenderLock lock;
        e = pet.play(clk);
      }
      switch (e) {
        case Event::Away:
          say("I'm out exploring!");
          break;
        case Event::Sleepy:
          say("Shh... sleeping.");
          break;
        case Event::TooTired:
          say("Too tired to play... nap first?");
          break;
        default:
          say(pet.s.species == Species::Cloud ? "Wheee! Loop-de-loop!" : "Wheee! Again, again!");
          chirp(0);
          afterCare(e);
      }
      break;
    }

    case Act::OpenFood:
      if (egg)
        say("Eggs don't eat. Tap it to hatch!");
      else if (pet.mode(clk) == Mode::Away)
        say("I'm out exploring! Feed me later.");
      else {
        go(Screen::Food);
        return;
      }
      break;

    case Act::Feed: {
      Event e;
      {
        RenderLock lock;
        e = pet.feed(b.arg, clk);
      }
      char buf[64];
      switch (e) {
        case Event::NotEnoughReading: {
          const uint32_t need = FOODS[b.arg].costMin * 60u - pet.s.readCreditSec;
          snprintf(buf, sizeof(buf), "Read %lu more min for that!", static_cast<unsigned long>((need + 59) / 60));
          say(buf);
          break;
        }
        case Event::Full:
          say("I'm stuffed! Maybe later.");
          break;
        case Event::Sleepy:
          say("Zzz... too sleepy to eat.");
          break;
        case Event::Away:
          say("I'm out exploring!");
          break;
        default:
          snprintf(buf, sizeof(buf), "Yum, %s! Thank you!", FOODS[b.arg].name);
          say(buf);
          chirp(1);
          afterCare(e);
      }
      go(Screen::Home);
      return;
    }

    case Act::CallBack: {
      {
        RenderLock lock;
        pet.callBack(clk);
      }
      afterCare(Event::Returned);
      say("I'm back! Look what I found.");
      break;
    }

    case Act::OpenApps:
      go(Screen::Launcher);
      return;
    case Act::OpenHelp:
      ui.guidePage = 0;
      go(Screen::Guide);
      return;
    case Act::OpenSettings:
      ui.quoteCount = PET_STORE.quoteCount();
      go(Screen::Settings);
      return;
    case Act::Close:
      back();
      return;

    case Act::AppPet:
      go(Screen::Home);
      return;
    case Act::AppPomodoro:
      ui.pomo = PomoPhase::Setup;
      go(Screen::Pomodoro);
      return;
    case Act::AppStatus:
      go(Screen::StatusMenu);
      return;
    case Act::AppAnswers:
      go(Screen::Answers);
      return;
    case Act::ExitToReader:
      onGoHome(HomeMenuItem::POCKET_PETS);
      return;

    case Act::PomoPreset:
      startPomodoro(static_cast<uint32_t>(b.arg) * 60);
      return;
    case Act::PomoCustom:
      ui.pomo = PomoPhase::Custom;
      ui.customField = 1;
      break;
    case Act::PomoField:
      ui.customField = static_cast<uint8_t>(b.arg);
      break;
    case Act::PomoKey: {
      uint8_t& v = ui.custom[ui.customField];
      const int limit = ui.customField == 0 ? 23 : 59;
      int nv = (v % 10) * 10 + b.arg;
      if (nv > limit) nv = b.arg;
      v = static_cast<uint8_t>(nv);
      break;
    }
    case Act::PomoClear:
      ui.custom[ui.customField] = 0;
      break;
    case Act::PomoDelete:
      ui.custom[ui.customField] /= 10;
      break;
    case Act::PomoStart: {
      const uint32_t total = ui.custom[0] * 3600u + ui.custom[1] * 60u + ui.custom[2];
      if (total > 0) {
        startPomodoro(total);
        return;
      }
      break;
    }
    case Act::PomoPause:
      if (ui.pomo == PomoPhase::Running) {
        ui.pomo = PomoPhase::Paused;
      } else {
        ui.pomo = PomoPhase::Running;
        pomoStartMs = millis();
        pomoStartLeft = ui.pomoLeftSec;
      }
      break;
    case Act::PomoEnd:
      pomoBeforeConfirm = ui.pomo;
      ui.pomo = PomoPhase::ConfirmEnd;
      break;
    case Act::PomoEndNo:
      ui.pomo = pomoBeforeConfirm;
      if (ui.pomo == PomoPhase::Running) {
        pomoStartMs = millis();
        pomoStartLeft = ui.pomoLeftSec;
      }
      break;
    case Act::PomoEndYes:
      ui.pomo = PomoPhase::Setup;
      break;
    case Act::PomoAlarmStop:
      if (buzzerReady) buzzer.noTone();
      alarmUntilMs = 0;
      ui.pomo = PomoPhase::Setup;
      break;

    case Act::StatusPick: {
      RenderLock lock;
      ui.status = b.arg;
      ui.sinceValid = clk.valid;
      ui.sinceHour = clk.hour;
      ui.sinceMinute = clk.minute;
    }
      go(Screen::StatusFull);
      return;
    case Act::StatusBack:
      go(Screen::StatusMenu);
      return;

    case Act::AnsMode:
      ui.ansMode = static_cast<uint8_t>(b.arg);
      break;
    case Act::AnsHold:
      if (ui.hasImu) {
        ui.ans = AnsPhase::Retry;  // a tap isn't a shake: show the reminder
      }
      break;
    case Act::AnsAgain:
      go(Screen::Answers);
      return;

    case Act::GuidePrev:
      if (ui.guidePage > 0) --ui.guidePage;
      break;
    case Act::GuideNext:
      if (ui.guidePage < GUIDE_PAGES - 1) {
        ++ui.guidePage;
      } else {
        pet.s.guideSeen = true;
        PET_STORE.markDirty();
        sayLine();
        go(Screen::Home);
        return;
      }
      break;

    case Act::SetRename:
      editText(true);
      return;
    case Act::SetSound:
      pet.s.soundOn = !pet.s.soundOn;
      PET_STORE.markDirty();
      chirp(4);
      break;
    case Act::AppTodo:
      go(Screen::Todo);
      return;
    case Act::TodoToggle: {
      bool nowDone;
      {
        RenderLock lock;
        nowDone = TODO_STORE.list.toggle(b.arg);
        if (nowDone) pet.taskDone(clk);
      }
      TODO_STORE.saveToFile();
      if (nowDone) {
        PET_STORE.markDirty();
        chirp(TODO_STORE.list.remaining() == 0 ? 2 : 0);
        checkBanners();
      }
      break;
    }
    case Act::TodoSync:
      requestTodoSync();
      break;
    case Act::TodoOpenSettings:
      go(Screen::TodoSettings);
      return;
    case Act::TodoPrev:
      if (ui.todoPage > 0) --ui.todoPage;
      break;
    case Act::TodoNext:
      if ((ui.todoPage + 1) * TODO_PER_PAGE < TODO_STORE.list.count) ++ui.todoPage;
      break;
    case Act::TodoSetToken:
      editTodoText(true);
      return;
    case Act::TodoSetFilter:
      editTodoText(false);
      return;
    case Act::TodoSetSchedule: {
      RenderLock lock;
      pp::TodoList& l = TODO_STORE.list;
      l.schedule =
          static_cast<SyncSchedule>((static_cast<int>(l.schedule) + 1) % static_cast<int>(SyncSchedule::Count));
    }
      TODO_STORE.saveToFile();
      break;
    case Act::TodoHourDown:
    case Act::TodoHourUp: {
      RenderLock lock;
      pp::TodoList& l = TODO_STORE.list;
      l.schedule = SyncSchedule::Daily;
      l.dailyHour = static_cast<uint8_t>((l.dailyHour + (b.act == Act::TodoHourUp ? 1 : 23)) % 24);
    }
      TODO_STORE.saveToFile();
      break;

    case Act::SetChatter: {
      // Pet only -> + quotes -> + book chatter -> all three.
      RenderLock lock;
      const int mode = (pet.s.quotesOn ? 1 : 0) + (pet.s.chatterOn ? 2 : 0);
      const int next = mode == 0 ? 1 : mode == 1 ? 2 : mode == 2 ? 3 : 0;
      pet.s.quotesOn = next & 1;
      pet.s.chatterOn = next & 2;
      PET_STORE.markDirty();
      break;
    }
    case Act::SetAddQuote:
      startActivityForResult(
          std::make_unique<KeyboardEntryActivity>(renderer, mappedInput, "A quote for your pet", "", 90),
          [this](const ActivityResult& result) {
            if (result.isCancelled) return;
            const auto* k = std::get_if<KeyboardResult>(&result.data);
            if (k && !k->text.empty() && PET_STORE.addQuote(k->text.c_str())) {
              RenderLock lock;
              PET_STORE.model.s.quotesOn = true;
              ui.quoteCount = PET_STORE.quoteCount();
              PET_STORE.markDirty();
            }
            fullRefreshNext = true;
          });
      return;
    case Act::SetCollection:
      go(Screen::Collection);
      return;
    case Act::SetGuide:
      ui.guidePage = 0;
      go(Screen::Guide);
      return;
    case Act::SetNewPet:
      go(Screen::ConfirmNew);
      return;
    case Act::ConfirmNo:
      go(Screen::Settings);
      return;
    case Act::ConfirmYes:
      go(Screen::Adopt);
      return;

    default:
      break;
  }
  relayout();
  requestUpdate();
}

void PocketPetsActivity::back() {
  switch (ui.screen) {
    case Screen::Home:
    case Screen::Adopt:
      onGoHome(HomeMenuItem::POCKET_PETS);
      return;
    case Screen::Pomodoro:
      if (ui.pomo == PomoPhase::Custom) {
        ui.pomo = PomoPhase::Setup;
        relayout();
        requestUpdate();
      } else if (ui.pomo == PomoPhase::Running || ui.pomo == PomoPhase::Paused) {
        pomoBeforeConfirm = ui.pomo;
        ui.pomo = PomoPhase::ConfirmEnd;
        relayout();
        requestUpdate();
      } else if (ui.pomo == PomoPhase::Alarm) {
        Button stop;
        stop.act = Act::PomoAlarmStop;
        act(stop);
      } else if (ui.pomo == PomoPhase::ConfirmEnd) {
        Button no;
        no.act = Act::PomoEndNo;
        act(no);
      } else {
        go(Screen::Home);
      }
      return;
    case Screen::StatusFull:
      go(Screen::StatusMenu);
      return;
    case Screen::TodoSettings:
      go(Screen::Todo);
      return;
    case Screen::Collection:
    case Screen::ConfirmNew:
      go(Screen::Settings);
      return;
    case Screen::Guide:
      PET_STORE.model.s.guideSeen = true;
      PET_STORE.markDirty();
      go(PET_STORE.model.s.adopted ? Screen::Home : Screen::Adopt);
      return;
    default:
      go(Screen::Home);
      return;
  }
}

void PocketPetsActivity::editText(bool petName) {
  {
    RenderLock lock;
    renderer.setOrientation(GfxRenderer::Portrait);
  }
  const std::string initial = petName ? PET_STORE.model.s.name : ui.customStatus;
  startActivityForResult(std::make_unique<KeyboardEntryActivity>(
                             renderer, mappedInput, petName ? "Pet name" : "Custom status", initial, petName ? 12 : 20),
                         [this, petName](const ActivityResult& result) {
                           RenderLock lock;
                           if (!result.isCancelled) {
                             if (const auto* k = std::get_if<KeyboardResult>(&result.data)) {
                               if (!k->text.empty()) {
                                 if (petName) {
                                   snprintf(PET_STORE.model.s.name, sizeof(PET_STORE.model.s.name), "%s",
                                            k->text.c_str());
                                 } else {
                                   snprintf(ui.customStatus, sizeof(ui.customStatus), "%s", k->text.c_str());
                                   for (char* p = ui.customStatus; *p; ++p)
                                     *p = static_cast<char>(toupper(static_cast<unsigned char>(*p)));
                                   memcpy(PET_STORE.customStatus, ui.customStatus, sizeof(ui.customStatus));
                                   ui.status = STATUS_CUSTOM;
                                 }
                                 PET_STORE.markDirty();
                               }
                             }
                           }
                           renderer.setOrientation(isLandscape(ui.screen) ? GfxRenderer::LandscapeCounterClockwise
                                                                          : GfxRenderer::Portrait);
                           fullRefreshNext = true;
                         });
}

void PocketPetsActivity::requestTodoSync() {
  {
    RenderLock lock;
    ui.syncing = true;
  }
  todoSyncAtMs = millis() + 600;  // let the "Syncing" screen draw first
  relayout();
  requestUpdate();
}

void PocketPetsActivity::editTodoText(bool token) {
  const std::string initial = token ? TODO_STORE.list.token : TODO_STORE.list.filter;
  startActivityForResult(std::make_unique<KeyboardEntryActivity>(
                             renderer, mappedInput, token ? "Todoist API token" : "Todoist filter", initial, 63),
                         [this, token](const ActivityResult& result) {
                           if (result.isCancelled) return;
                           const auto* k = std::get_if<KeyboardResult>(&result.data);
                           if (!k) return;
                           {
                             RenderLock lock;
                             pp::TodoList& l = TODO_STORE.list;
                             if (token) {
                               snprintf(l.token, sizeof(l.token), "%s", k->text.c_str());
                             } else {
                               snprintf(l.filter, sizeof(l.filter), "%s",
                                        k->text.empty() ? "today | overdue" : k->text.c_str());
                             }
                             l.lastSync = 0;  // fetch again with the new settings
                             l.lastError[0] = 0;
                           }
                           TODO_STORE.saveToFile();
                           fullRefreshNext = true;
                         });
}

// ------------------------------------------------------------ focus timer --

void PocketPetsActivity::startPomodoro(uint32_t seconds) {
  {
    RenderLock lock;
    ui.pomoTotalSec = seconds;
    ui.pomoLeftSec = seconds;
    ui.pomo = PomoPhase::Running;
  }
  pomoStartMs = lastPomoDrawMs = millis();
  pomoStartLeft = seconds;
  chirp(4);
  relayout();
  requestUpdate();
}

void PocketPetsActivity::updatePomodoro(uint32_t now) {
  if (ui.screen != Screen::Pomodoro && ui.pomo != PomoPhase::Running) return;

  if (ui.pomo == PomoPhase::Running) {
    const uint32_t elapsed = (now - pomoStartMs) / 1000;
    const uint32_t left = elapsed >= pomoStartLeft ? 0 : pomoStartLeft - elapsed;
    if (left != ui.pomoLeftSec) {
      {
        RenderLock lock;
        ui.pomoLeftSec = left;
      }
      if (left == 0) {
        {
          RenderLock lock;
          ui.pomo = PomoPhase::Alarm;
          PET_STORE.model.pomodoroDone(PetStore::now());
        }
        PET_STORE.markDirty();
        PET_STORE.flush();
        checkBanners();
        alarmNextMs = now;
        alarmUntilMs = now + ALARM_MAX_MS;
        if (ui.screen != Screen::Pomodoro) go(Screen::Pomodoro);
        relayout();
        requestUpdate();
      } else if (ui.screen == Screen::Pomodoro && (now - lastPomoDrawMs >= (left <= 10 ? 1000u : 5000u))) {
        lastPomoDrawMs = now;
        requestUpdate();
      }
    }
  }

  if (ui.pomo == PomoPhase::Alarm && alarmUntilMs) {
    if (now >= alarmUntilMs) {
      alarmUntilMs = 0;
    } else if (now >= alarmNextMs) {
      alarmNextMs = now + ALARM_REPEAT_MS;
      chirp(3);
    }
  }
}

// ----------------------------------------------------------------- motion --

void PocketPetsActivity::finishShake() {
  if (ui.screen == Screen::Answers) {
    RenderLock lock;
    ui.ans = shakeMs > 200 ? AnsPhase::Retry : ui.ans;
    ui.shakePct = 0;
  }
  shakeMs = 0;
  lastShownShakePct = 0;
  requestUpdate();
}

void PocketPetsActivity::updateMotion(uint32_t now) {
  // Book of Answers: thinking animation, then the reveal.
  if (ui.screen == Screen::Answers && ui.ans == AnsPhase::Thinking) {
    if (now >= thinkNextMs) {
      RenderLock lock;
      if (++ui.thinkFrame >= 4) {
        ui.ans = AnsPhase::Result;
        const uint32_t r = PET_STORE.model.s.rng = PET_STORE.model.s.rng * 1664525u + 1013904223u + now;
        const int pool = ui.ansMode == 1 ? 3 : ui.ansMode == 2 ? TAROT_COUNT : answerCount();
        ui.answer = static_cast<int>((r >> 8) % pool);
        ui.reversed = ui.ansMode == 2 && ((r >> 3) & 3) == 0;  // about one card in four
        lock.unlock();
        chirp(2);
        relayout();
      }
      thinkNextMs = now + 700;
      requestUpdate();
    }
    return;
  }

  // Without an IMU the book answers to a three-second press instead.
  if (!ui.hasImu && ui.screen == Screen::Answers && (ui.ans == AnsPhase::Ask || ui.ans == AnsPhase::Retry)) {
    int x = 0, y = 0;
    const bool held = mappedInput.isScreenTouchHeld(x, y) && buttons[buttonCount - 1].act == Act::AnsHold &&
                      buttons[buttonCount - 1].box.contains(x, y);
    if (held) {
      if (!holdStartMs) holdStartMs = now;
      if (now - holdStartMs >= SHAKE_NEEDED_MS) {
        holdStartMs = 0;
        RenderLock lock;
        ui.ans = AnsPhase::Thinking;
        ui.thinkFrame = 0;
        thinkNextMs = now;
      }
    } else if (holdStartMs) {
      holdStartMs = 0;
      RenderLock lock;
      ui.ans = AnsPhase::Retry;
      requestUpdate();
    }
    return;
  }

  if (!motionActive() || now - lastMotionMs < MOTION_POLL_MS) return;
  const uint32_t dt = lastMotionMs ? (now - lastMotionMs > 120 ? 120 : now - lastMotionMs) : 0;
  lastMotionMs = now;
  Imu::Sample s;
  if (!halTiltSensor.readSample(s)) return;

  const float mag = std::sqrt(s.ax * s.ax + s.ay * s.ay + s.az * s.az);
  const float spin = std::sqrt(s.gx * s.gx + s.gy * s.gy + s.gz * s.gz);
  const bool shaking = std::fabs(mag - 1.0f) > 0.45f || spin > 260.0f;
  if (shaking) {
    shakeMs += dt;
    lastShakeMs = now;
  } else if (shakeMs > 0 && now - lastShakeMs > SHAKE_GAP_MS) {
    if (ui.screen == Screen::Answers)
      finishShake();
    else
      shakeMs = 0;
    return;
  }

  if (ui.screen == Screen::Launcher) {
    if (shakeMs >= LAUNCHER_SHAKE_MS) {
      go(Screen::Answers);
      return;
    }
    // Rotation: the in-plane axis carrying gravity changed and stayed put.
    if (!shaking) {
      int8_t axis = -1;
      if (std::fabs(s.ax) > 0.75f && std::fabs(s.ax) > std::fabs(s.ay))
        axis = 0;
      else if (std::fabs(s.ay) > 0.75f && std::fabs(s.ay) > std::fabs(s.ax))
        axis = 1;
      if (axis >= 0) {
        if (baseAxis < 0) {
          baseAxis = axis;
        } else if (axis != baseAxis) {
          if (++rotateStableCount >= 5) go(Screen::StatusMenu);
          return;
        }
      }
      rotateStableCount = 0;
    }
    return;
  }

  // Book of Answers: three seconds of real shaking.
  if (shakeMs >= SHAKE_NEEDED_MS) {
    {
      RenderLock lock;
      ui.ans = AnsPhase::Thinking;
      ui.thinkFrame = 0;
      ui.shakePct = 0;
    }
    shakeMs = 0;
    thinkNextMs = now;
    halTiltSensor.setMotionHold(false);
    chirp(4);
    requestUpdate();
    return;
  }
  const int pct = static_cast<int>(shakeMs * 100 / SHAKE_NEEDED_MS);
  if (pct / 25 != lastShownShakePct / 25) {
    lastShownShakePct = pct;
    {
      RenderLock lock;
      ui.shakePct = pct;
    }
    requestUpdate();
  }
}
