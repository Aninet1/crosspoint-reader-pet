#pragma once
// Pocket Pets screens: one layout function per screen gives the buttons, and
// the same layout is used to draw and to hit-test taps.

#include "PetArt.h"
#include "PetCanvas.h"
#include "PetModel.h"

namespace pp {

enum class Screen : uint8_t {
  Adopt,
  Home,
  Food,
  Launcher,
  Pomodoro,
  StatusMenu,
  StatusFull,
  Answers,
  Guide,
  Settings,
  Collection,
  ConfirmNew
};

enum class Act : uint8_t {
  None,
  TapPet,
  OpenFood,
  Play,
  PetPet,
  OpenApps,
  OpenHelp,
  OpenSettings,
  Close,
  CallBack,
  Feed,
  Adopt,
  AppPet,
  AppPomodoro,
  AppStatus,
  AppAnswers,
  ExitToReader,
  PomoPreset,
  PomoCustom,
  PomoKey,
  PomoClear,
  PomoDelete,
  PomoField,
  PomoStart,
  PomoPause,
  PomoEnd,
  PomoEndYes,
  PomoEndNo,
  PomoAlarmStop,
  StatusPick,
  StatusBack,
  AnsMode,
  AnsHold,
  AnsAgain,
  GuidePrev,
  GuideNext,
  SetRename,
  SetSound,
  SetCollection,
  SetGuide,
  SetNewPet,
  ConfirmYes,
  ConfirmNo,
  DismissBanner
};

struct Button {
  Box box;
  Act act = Act::None;
  int arg = 0;
};
constexpr int MAX_BUTTONS = 20;

enum class PomoPhase : uint8_t { Setup, Custom, Running, Paused, ConfirmEnd, Alarm };
enum class AnsPhase : uint8_t { Ask, Retry, Thinking, Result };

struct UiState {
  Screen screen = Screen::Home;
  int focus = -1;  // button index chosen with the physical keys, -1 = none

  // Home
  char bubble[96] = "";
  Event banner = Event::None;
  int bannerValue = 0;

  // Pomodoro
  PomoPhase pomo = PomoPhase::Setup;
  uint32_t pomoTotalSec = 25 * 60;
  uint32_t pomoLeftSec = 25 * 60;
  uint8_t customField = 1;  // 0 h, 1 m, 2 s
  uint8_t custom[3] = {0, 25, 0};

  // Status board
  int status = 0;
  char customStatus[24] = "BACK SOON";

  // Book of Answers
  bool yesNo = false;
  AnsPhase ans = AnsPhase::Ask;
  int answer = 0;
  int thinkFrame = 0;
  bool hasImu = true;
  int shakePct = 0;

  int guidePage = 0;
  int adoptFocus = 0;
};

constexpr int STATUS_COUNT = 6;
extern const char* const STATUS_LABELS[STATUS_COUNT];
constexpr int GUIDE_PAGES = 6;

// Fills `out` with the buttons of the current screen; returns how many.
int layout(const UiState& ui, const PetModel& pet, const Clock& clk, int w, int h, Button* out);
// Draws the current screen.
void drawScreen(Canvas& c, const UiState& ui, const PetModel& pet, const Clock& clk);
// Screens that are laid out in landscape.
inline bool isLandscape(Screen s) { return s == Screen::StatusMenu || s == Screen::StatusFull; }

// Text for a celebration banner (empty when the event has none).
const char* bannerTitle(Event e);

// Chunky rounded digits for the timer, e.g. "24:59".
void drawBigDigits(Canvas& c, int cx, int y, int digitH, const char* text);

// Book of Answers content.
int answerCount();
const char* answerText(int index);
const char* yesNoText(int index);  // index 0..2

}  // namespace pp
