#pragma once
// Pocket Pets screens: one layout function per screen gives the buttons, and
// the same layout is used to draw and to hit-test taps.

#include "PetArt.h"
#include "PetCanvas.h"
#include "PetModel.h"
#include "TodoList.h"

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
  ConfirmNew,
  Todo,
  TodoSettings
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
  AppTodo,
  TodoToggle,
  TodoSync,
  TodoOpenSettings,
  TodoPrev,
  TodoNext,
  TodoSetToken,
  TodoSetFilter,
  TodoSetSchedule,
  TodoHourDown,
  TodoHourUp,
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
  TarotSpread,
  TarotQuestion,
  TarotBirthday,
  GuidePrev,
  GuideNext,
  SetRename,
  SetSound,
  SetChatter,
  SetAddQuote,
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
  char customStatus[24] = "HELLO!";
  bool sinceValid = false;  // time the status was set, shown as "since 9:41"
  uint8_t sinceHour = 0, sinceMinute = 0;

  // Book of Answers
  uint8_t ansMode = 0;    // 0 message, 1 yes or no, 2 tarot
  bool reversed = false;  // tarot card drawn upside down

  // Tarot reading
  uint8_t spread = 0;  // 0 one card, 1 past/present/future, 2 card of the day
  int8_t cards[3] = {0, 0, 0};
  bool cardRev[3] = {false, false, false};
  char question[72] = "";
  int8_t birthCard = -1, birthCard2 = -1;
  AnsPhase ans = AnsPhase::Ask;
  int answer = 0;
  int thinkFrame = 0;
  bool hasImu = true;
  int shakePct = 0;

  int guidePage = 0;

  // To-do
  const TodoList* todo = nullptr;  // owned by the device store
  int todoPage = 0;
  bool syncing = false;
  int quoteCount = 0;  // lines in the quotes file, for Settings
  int adoptFocus = 0;
};

constexpr int STATUS_COUNT = 12;
constexpr int STATUS_SLEEPING = 8;
constexpr int STATUS_CUSTOM = STATUS_COUNT - 1;
extern const char* const STATUS_LABELS[STATUS_COUNT];
constexpr int GUIDE_PAGES = 6;
constexpr int TODO_PER_PAGE = 6;

// Fills `out` with the buttons of the current screen; returns how many.
int layout(const UiState& ui, const PetModel& pet, const Clock& clk, int w, int h, Button* out);
// Draws the current screen.
void drawScreen(Canvas& c, const UiState& ui, const PetModel& pet, const Clock& clk);
// Screens that are laid out in landscape.
inline bool isLandscape(Screen s) { return s == Screen::StatusMenu || s == Screen::StatusFull; }

// To-do card for the empty space under CrossPoint's home menu.
// The card is also the way into Pocket Pets: the pet in its corner opens it.
constexpr int HOME_TODO_MIN_HEIGHT = 96;
void drawHomeTodo(Canvas& c, const Box& area, const TodoList& list, const PetModel& pet, const Clock& clk,
                  bool focused = false);
constexpr int HOME_TODO_HIT_NONE = -3, HOME_TODO_HIT_PET = -2, HOME_TODO_HIT_TODO = -1;
// What a tap at (x, y) hit: an item index, or one of the HOME_TODO_HIT_ values.
int homeTodoHit(const Box& area, const TodoList& list, int x, int y);

// Text for a celebration banner (empty when the event has none).
const char* bannerTitle(Event e);

// Chunky rounded digits for the timer, e.g. "24:59".
void drawBigDigits(Canvas& c, int cx, int y, int digitH, const char* text);

// Book of Answers content.
int answerCount();
const char* answerText(int index);
const char* yesNoText(int index);  // index 0..2

// Tarot: the 22 Major Arcana.
constexpr int TAROT_COUNT = 22;
const char* tarotName(int card);
const char* tarotMeaning(int card, bool reversed);
const char* tarotKeywords(int card, bool reversed);
// Shuffles the deck from `seed` and deals `count` cards, each upright or reversed.
void tarotShuffleDraw(uint32_t seed, int count, int* cards, bool* reversed);
// Birth card from a date of birth; `second` gets the paired card or -1.
int tarotBirthCards(int year, int month, int day, int& second);
void drawTarotCard(Canvas& c, int card, bool reversed, int cx, int cy, int h);
void drawTarotBack(Canvas& c, int cx, int cy, int h);

}  // namespace pp
