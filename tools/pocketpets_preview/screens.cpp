// Renders every Pocket Pets screen from sample states into PGM files.
#define STB_TRUETYPE_IMPLEMENTATION
#include "pocketpets/Screens.h"

#include "HostSurface.h"
using namespace pp;

static std::string root;
static void shot(const char* name, const UiState& ui, PetModel& pet, const Clock& clk) {
  const bool land = isLandscape(ui.screen);
  HostSurface s(land ? 800 : 480, land ? 480 : 800);
  Canvas c(s);
  drawScreen(c, ui, pet, clk);
  s.writePgm(std::string("/tmp/pp_") + name + ".pgm");
}

int main(int argc, char** argv) {
  root = argc > 1 ? argv[1] : ".";
  HostSurface::loadFonts((root + "/lib/EpdFont/builtinFonts/source/NotoSans/NotoSans-Regular.ttf").c_str(),
                         (root + "/lib/EpdFont/builtinFonts/source/NotoSans/NotoSans-Bold.ttf").c_str());
  Clock clk{true, 1763280000, 20408, 14, 30};  // a 2:30pm
  PetModel pet;
  UiState ui;
  ui.screen = Screen::Adopt;
  ui.focus = 1;
  shot("01_adopt", ui, pet, clk);

  pet.adopt(Species::Dog, nullptr, clk);
  ui = UiState{};
  shot("02_egg", ui, pet, clk);
  pet.tapEgg(clk);
  pet.tapEgg(clk);
  pet.tapEgg(clk);
  ui.banner = Event::Hatched;
  shot("03_hatched", ui, pet, clk);
  ui.banner = Event::None;
  pet.addReading(3 * 3600 + 25 * 60, clk);
  pet.s.growth = 70;
  pet.s.stage = Stage::Teen;
  pet.s.personality = Personality::Bookworm;
  pet.s.love = 76;
  pet.s.fullness = 58;
  pet.s.energy = 64;
  pet.s.streak = 4;
  pet.s.bornEpoch = clk.epoch - 9 * 86400;
  char b[96];
  snprintf(ui.bubble, sizeof(ui.bubble), "%s", pet.line(clk, b, sizeof(b)));
  shot("04_home", ui, pet, clk);
  ui.screen = Screen::Food;
  ui.focus = 3;
  shot("05_food", ui, pet, clk);
  ui.screen = Screen::Launcher;
  ui.focus = -1;
  shot("06_launcher", ui, pet, clk);
  ui.screen = Screen::Pomodoro;
  ui.pomo = PomoPhase::Setup;
  shot("07_pomo_setup", ui, pet, clk);
  ui.pomo = PomoPhase::Custom;
  ui.custom[0] = 0;
  ui.custom[1] = 45;
  ui.customField = 1;
  shot("08_pomo_custom", ui, pet, clk);
  ui.pomo = PomoPhase::Running;
  ui.pomoTotalSec = 1500;
  ui.pomoLeftSec = 1312;
  shot("09_pomo_running", ui, pet, clk);
  ui.pomo = PomoPhase::ConfirmEnd;
  shot("10_pomo_confirm", ui, pet, clk);
  ui.pomo = PomoPhase::Alarm;
  shot("11_pomo_alarm", ui, pet, clk);
  ui.screen = Screen::StatusMenu;
  shot("12_status_menu", ui, pet, clk);
  ui.screen = Screen::StatusFull;
  ui.sinceValid = true;
  ui.sinceHour = 9;
  ui.sinceMinute = 41;
  for (int i = 0; i < STATUS_COUNT; ++i) {
    ui.status = i;
    char n[32];
    snprintf(n, sizeof n, "30_status_%02d", i);
    shot(n, ui, pet, clk);
  }
  ui.screen = Screen::Answers;
  ui.ans = AnsPhase::Ask;
  ui.shakePct = 40;
  shot("14_answers", ui, pet, clk);
  ui.ans = AnsPhase::Result;
  ui.answer = 15;
  shot("15_answer_msg", ui, pet, clk);
  ui.ansMode = 1;
  ui.answer = 0;
  shot("16_answer_yes", ui, pet, clk);
  ui.ansMode = 2;
  ui.ans = AnsPhase::Ask;
  shot("16b_tarot_ask", ui, pet, clk);
  ui.ans = AnsPhase::Result;
  for (int i = 0; i < TAROT_COUNT; ++i) {
    ui.answer = i;
    ui.reversed = i == 16;
    char n[32];
    snprintf(n, sizeof n, "40_tarot_%02d", i);
    shot(n, ui, pet, clk);
  }
  // Real-style tarot readings.
  ui.ans = AnsPhase::Ask;
  ui.spread = 1;
  snprintf(ui.question, sizeof ui.question, "Should I apply for the summer internship?");
  {
    int s2 = -1;
    ui.birthCard = static_cast<int8_t>(tarotBirthCards(1998, 7, 14, s2));
    ui.birthCard2 = static_cast<int8_t>(s2);
  }
  shot("41_tarot_ask_full", ui, pet, clk);
  ui.ans = AnsPhase::Result;
  ui.cards[0] = 16;
  ui.cards[1] = 12;
  ui.cards[2] = 19;
  ui.cardRev[0] = true;
  ui.cardRev[1] = false;
  ui.cardRev[2] = false;
  shot("42_tarot_three", ui, pet, clk);
  ui.spread = 2;
  ui.question[0] = 0;
  ui.cards[0] = 17;
  ui.cardRev[0] = false;
  shot("43_tarot_day", ui, pet, clk);
  ui.spread = 0;
  ui.cards[0] = 12;
  ui.cardRev[0] = true;
  snprintf(ui.question, sizeof ui.question, "Will the move go well?");
  shot("44_tarot_one_birth", ui, pet, clk);
  ui.question[0] = 0;
  ui.spread = 0;
  ui.ansMode = 0;
  {
    TodoList todo;
    snprintf(todo.token, sizeof todo.token, "x");
    const char* tasks[] = {"Submit cryptography assignment 3",
                           "Reply to the case comp team",
                           "Buy oat milk",
                           "Read two chapters",
                           "Book flu shot",
                           "Water the real plants",
                           "Plan weekend"};
    const char* dues[] = {"2025-11-15", "2025-11-16", "2025-11-16", "2025-11-16", "2025-11-17", "2025-11-21", ""};
    for (int i = 0; i < 7; ++i) {
      TodoItem& it = todo.items[todo.count++];
      snprintf(it.id, sizeof it.id, "%d", i);
      snprintf(it.text, sizeof it.text, "%s", tasks[i]);
      snprintf(it.due, sizeof it.due, "%s", dues[i]);
      it.priority = i == 0 ? 4 : i == 1 ? 3 : 1;
      it.done = i == 2;
    }
    todo.lastSync = clk.epoch - 2 * 3600;
    UiState t;
    t.todo = &todo;
    t.screen = Screen::Todo;
    shot("50_todo", t, pet, clk);
    t.todoPage = 1;
    shot("51_todo_p2", t, pet, clk);
    t.screen = Screen::TodoSettings;
    shot("52_todo_settings", t, pet, clk);
    {
      // The card under CrossPoint's home menu, at two heights.
      HostSurface hs(480, 520);
      Canvas hc(hs);
      hc.clear();
      drawHomeTodo(hc, Box{22, 10, 432, 270}, todo, pet, clk);
      TodoList none;
      drawHomeTodo(hc, Box{22, 310, 432, 160}, none, pet, clk);
      hs.writePgm("/tmp/pp_55_home_card.pgm");
      // Card at the size it gets under a five-row menu on the Sticky (y 676 to 794).
      HostSurface hs2(480, 800);
      Canvas hc2(hs2);
      hc2.clear();
      drawHomeTodo(hc2, Box{24, 676, 428, 118}, todo, pet, clk);
      hs2.writePgm("/tmp/pp_56_home_card_fit.pgm");
    }
    TodoList empty;
    t.todo = &empty;
    t.screen = Screen::Todo;
    t.todoPage = 0;
    shot("53_todo_notoken", t, pet, clk);
  }
  ui.screen = Screen::Guide;
  for (int i = 0; i < GUIDE_PAGES; ++i) {
    ui.guidePage = i;
    char n[32];
    snprintf(n, sizeof n, "17_guide_%d", i);
    shot(n, ui, pet, clk);
  }
  ui.screen = Screen::Settings;
  shot("18_settings", ui, pet, clk);
  pet.s.keepsakes = 0x2D;
  ui.screen = Screen::Collection;
  shot("19_collection", ui, pet, clk);
  ui.screen = Screen::ConfirmNew;
  shot("20_confirm", ui, pet, clk);
  // Night, hungry, away variants.
  ui = UiState{};
  Clock night = clk;
  night.hour = 23;
  shot("21_night", ui, pet, night);
  pet.s.fullness = 12;
  ui.bubble[0] = 0;
  snprintf(ui.bubble, sizeof(ui.bubble), "%s", pet.line(clk, b, sizeof(b)));
  shot("22_hungry", ui, pet, clk);
  pet.s.awayUntil = clk.epoch + 2 * 3600 + 600;
  ui.bubble[0] = 0;
  shot("23_away", ui, pet, clk);
  pet.s.awayUntil = 0;
  pet.s.pendingKeepsake = 2;
  ui.banner = Event::Returned;
  shot("24_returned", ui, pet, clk);
  // Other species homes.
  for (int sp = 1; sp < 5; ++sp) {
    PetModel p2;
    p2.adopt((Species)sp, nullptr, clk);
    p2.tapEgg(clk);
    p2.tapEgg(clk);
    p2.tapEgg(clk);
    p2.s.stage = (Stage)(sp + 1 > 4 ? 4 : sp + 1);
    p2.s.personality = sp == 3 ? Personality::Foodie : sp == 2 ? Personality::Cuddly : Personality::None;
    p2.s.readCreditSec = 50 * 60;
    p2.s.fullness = 80;
    UiState u;
    snprintf(u.bubble, sizeof(u.bubble), "%s", p2.line(clk, b, sizeof(b)));
    char n[32];
    snprintf(n, sizeof n, "25_home_sp%d", sp);
    shot(n, u, p2, clk);
  }
  // Rabbit through every stage and pose.
  {
    PetModel r;
    r.adopt(Species::Rabbit, nullptr, clk);
    UiState u;
    shot("60_bunny_egg", u, r, clk);
    r.tapEgg(clk);
    r.tapEgg(clk);
    shot("61_bunny_egg_cracked", u, r, clk);
    r.tapEgg(clk);
    r.s.fullness = 80;
    r.lastPose = Pose::Idle;
    shot("62_bunny_baby", u, r, clk);
    r.s.stage = Stage::Child;
    r.lastPose = Pose::Excited;
    shot("63_bunny_child", u, r, clk);
    r.s.stage = Stage::Teen;
    r.s.personality = Personality::Foodie;
    r.lastPose = Pose::Eating;
    shot("64_bunny_youth", u, r, clk);
    r.s.stage = Stage::Adult;
    r.s.personality = Personality::Active;
    r.lastPose = Pose::Idle;
    shot("65_bunny_adult", u, r, clk);
    shot("66_bunny_night", u, r, Clock{true, clk.epoch, clk.day, 23, 10});
    u.screen = Screen::StatusFull;
    u.status = 4;
    shot("67_bunny_status", u, r, clk);
    u.screen = Screen::Adopt;
    shot("68_adopt_five", u, r, clk);
  }
  return 0;
}
