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
  ui.status = 3;
  shot("13_status_full", ui, pet, clk);
  ui.status = 4;
  shot("13b_status_rest", ui, pet, clk);
  ui.screen = Screen::Answers;
  ui.ans = AnsPhase::Ask;
  ui.shakePct = 40;
  shot("14_answers", ui, pet, clk);
  ui.ans = AnsPhase::Result;
  ui.answer = 15;
  shot("15_answer_msg", ui, pet, clk);
  ui.yesNo = true;
  ui.answer = 0;
  shot("16_answer_yes", ui, pet, clk);
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
  for (int sp = 1; sp < 4; ++sp) {
    PetModel p2;
    p2.adopt((Species)sp, nullptr, clk);
    p2.tapEgg(clk);
    p2.tapEgg(clk);
    p2.tapEgg(clk);
    p2.s.stage = (Stage)(sp + 1);
    p2.s.personality = sp == 3 ? Personality::Foodie : sp == 2 ? Personality::Cuddly : Personality::None;
    p2.s.readCreditSec = 50 * 60;
    p2.s.fullness = 80;
    UiState u;
    snprintf(u.bubble, sizeof(u.bubble), "%s", p2.line(clk, b, sizeof(b)));
    char n[32];
    snprintf(n, sizeof n, "25_home_sp%d", sp);
    shot(n, u, p2, clk);
  }
  return 0;
}
