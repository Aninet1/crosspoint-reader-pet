#pragma once
// Pocket Pets artwork: original kawaii line art built from canvas shapes.

#include "PetCanvas.h"
#include "PetModel.h"

namespace pp {

struct PetLook {
  Species species = Species::Dog;
  Stage stage = Stage::Baby;
  Personality personality = Personality::None;
  Pose pose = Pose::Idle;
  bool hungry = false;
  bool sleeping = false;
  enum Wear : uint8_t { NoWear, Headphones, Nightcap };
  Wear wear = NoWear;
};

enum class Icon : uint8_t {
  None,
  Food,
  Play,
  Pet,
  Apps,
  Help,
  Gear,
  Close,
  Back,
  Timer,
  Status,
  Answers,
  Book,
  Heart,
  Bolt,
  Star,
  Leaf,
  Moon,
  Bowl,
  Sound,
  Mute,
  Crystal,
  Pause,
  PlayArrow,
  Stop
};

// Pet standing on the ground line baseY, centred on cx. size is the height of
// an adult; younger stages draw smaller.
void drawPet(Canvas& c, const PetLook& look, int cx, int baseY, int size);
// Egg (or seed pot for plants) with 0..2 cracks.
void drawEgg(Canvas& c, Species sp, int cx, int baseY, int size, int cracks);
void drawFood(Canvas& c, int item, int cx, int cy, int size);
void drawIcon(Canvas& c, Icon icon, int cx, int cy, int size);
void drawZzz(Canvas& c, int x, int y, int size);
// Little keepsake picture for the collection row.
void drawKeepsake(Canvas& c, int index, int cx, int cy, int size);

}  // namespace pp
