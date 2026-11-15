// Pet gallery: every species and stage, poses, eggs, food and icons.
#define STB_TRUETYPE_IMPLEMENTATION
#include "HostSurface.h"
#include "pocketpets/PetArt.h"
using namespace pp;
int main(int argc, char** argv) {
  const std::string root = argc > 1 ? argv[1] : ".";
  HostSurface::loadFonts((root + "/lib/EpdFont/builtinFonts/source/NotoSans/NotoSans-Regular.ttf").c_str(),
                         (root + "/lib/EpdFont/builtinFonts/source/NotoSans/NotoSans-Bold.ttf").c_str());
  HostSurface s(1200, 1250);
  Canvas c(s);
  c.clear();
  const char* stages[] = {"Egg", "Baby", "Child", "Teen", "Adult"};
  for (int sp = 0; sp < 4; ++sp) {
    for (int st = 0; st < 5; ++st) {
      const int cx = 120 + st * 240, by = 200 + sp * 230;
      if (st == 0)
        drawEgg(c, (Species)sp, cx, by, 150, sp % 3);
      else {
        PetLook l;
        l.species = (Species)sp;
        l.stage = (Stage)st;
        l.personality = st >= 3 ? (Personality)(1 + (sp + st) % 4) : Personality::None;
        if (st == 4 && sp == 0) l.personality = Personality::None;
        drawPet(c, l, cx, by, 170);
      }
      if (sp == 0) c.textCentered(cx, 4, stages[st], 22, true);
    }
  }
  // Poses row for each species (adult, teen)
  const Pose poses[] = {Pose::Happy, Pose::Eating, Pose::Sad, Pose::Excited};
  for (int i = 0; i < 8; ++i) {
    PetLook l;
    l.species = (Species)(i / 2);
    l.stage = Stage::Child;
    if (i % 2) {
      l.sleeping = true;
    } else {
      l.pose = poses[(i / 2) % 4];
      l.hungry = (i / 2) == 2;
    }
    drawPet(c, l, 75 + i * 150, 1150, 170);
  }
  s.writePgm("gallery.pgm");
  HostSurface f(1200, 260);
  Canvas d(f);
  d.clear();
  for (int i = 0; i < 6; ++i) drawFood(d, i, 80 + i * 110, 60, 80);
  for (int i = 0; i < 24; ++i) drawIcon(d, (Icon)i, 40 + (i % 12) * 95, 150 + (i / 12) * 60, 40);
  for (int i = 0; i < 8; ++i) drawKeepsake(d, i, 760 + i * 55, 60, 44);
  f.writePgm("icons.pgm");
}
