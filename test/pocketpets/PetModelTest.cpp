// Host tests for the Pocket Pets model: g++ -std=c++17 -I src test/pocketpets/PetModelTest.cpp
// src/pocketpets/PetModel.cpp
#include <cassert>
#include <cstdio>
#include <cstring>

#include "pocketpets/PetModel.h"
using namespace pp;

static Clock at(int day, int hour, int minute = 0) {
  Clock c;
  c.valid = true;
  c.day = day;
  c.hour = static_cast<uint8_t>(hour);
  c.minute = static_cast<uint8_t>(minute);
  c.epoch = static_cast<int64_t>(day) * 86400 + hour * 3600 + minute * 60;  // UTC == local here
  return c;
}

static int failures = 0;
#define CHECK(cond)                                               \
  do {                                                            \
    if (!(cond)) {                                                \
      std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); \
      ++failures;                                                 \
    }                                                             \
  } while (0)

int main() {
  // Hatching takes three taps and starts the bond.
  PetModel p;
  p.adopt(Species::Cat, nullptr, at(100, 9));
  CHECK(std::strcmp(p.s.name, "Tofu") == 0);
  CHECK(p.s.stage == Stage::Egg);
  CHECK(p.tapEgg(at(100, 9)) == Event::None);
  CHECK(p.tapEgg(at(100, 9)) == Event::None);
  CHECK(p.tapEgg(at(100, 9)) == Event::Hatched);
  CHECK(p.s.stage == Stage::Baby);
  CHECK(p.s.streak == 1);

  // Food costs reading time; no reading, no food.
  p.s.fullness = 40;
  CHECK(p.feed(0, at(100, 10)) == Event::NotEnoughReading);
  p.addReading(20 * 60, at(100, 10));
  CHECK(p.s.readCreditSec == 20 * 60);
  CHECK(p.feed(0, at(100, 10)) == Event::None);
  CHECK(p.s.readCreditSec == 5 * 60);
  CHECK(p.s.fullness == 48);
  CHECK(p.feed(1, at(100, 10)) == Event::NotEnoughReading);
  p.s.fullness = 96;
  p.addReading(60 * 60, at(100, 10));
  CHECK(p.feed(0, at(100, 10)) == Event::Full);

  // Growth is capped at 10 a day however much care happens.
  const int g0 = p.s.growth;
  for (int i = 0; i < 30; ++i) p.pet(at(100, 11));
  CHECK(p.s.growth - g0 <= MAX_DAILY_GROWTH);
  CHECK(p.s.dayGrowth == MAX_DAILY_GROWTH);
  CHECK(p.s.dayLove == MAX_DAILY_LOVE);

  // A new day resets the caps and extends the streak; skipping a day resets it.
  p.pet(at(101, 9));
  CHECK(p.s.streak == 2);
  CHECK(p.s.dayGrowth == 1);
  p.pet(at(103, 9));
  CHECK(p.s.streak == 1);
  CHECK(p.s.bestStreak == 2);

  // Sleep at night; can't play or eat then, but pats still count.
  CHECK(p.mode(at(103, 23)) == Mode::Sleeping);
  CHECK(p.mode(at(103, 6)) == Mode::Sleeping);
  CHECK(p.mode(at(103, 7)) == Mode::Awake);
  CHECK(p.play(at(103, 23)) == Event::Sleepy);

  // Needs drop with time; energy recovers overnight.
  PetModel d;
  d.adopt(Species::Dog, nullptr, at(200, 8));
  for (int i = 0; i < 3; ++i) d.tapEgg(at(200, 8));
  d.s.fullness = 80;
  d.s.energy = 60;
  d.tick(at(200, 8));
  d.tick(at(200, 12));  // 4 hours awake
  CHECK(d.s.fullness == 64);
  CHECK(d.s.energy == 40);
  d.tick(at(201, 6));  // an evening and a night
  CHECK(d.s.energy == 100);
  CHECK(d.s.fullness < 40);

  // Low energy in the day means a nap until rested.
  d.s.energy = 12;
  d.tick(at(201, 10));
  d.tick(at(201, 10, 30));
  CHECK(d.s.napping || d.s.energy >= 70);

  // Stages advance with growth, and the teen picks a personality from care.
  PetModel g;
  g.adopt(Species::Plant, nullptr, at(300, 9));
  for (int i = 0; i < 3; ++i) g.tapEgg(at(300, 9));
  int day = 300;
  while (g.s.stage < Stage::Teen && day < 340) {
    ++day;
    g.addReading(60 * 60, at(day, 9));  // an hour of reading a day
    g.feed(2, at(day, 10));
  }
  CHECK(g.s.stage == Stage::Teen);
  CHECK(g.s.personality == Personality::Bookworm || g.s.personality == Personality::Foodie);
  std::printf("plant became a teen on day %d as %s\n", day - 300, PetModel::personalityName(g.s.personality));
  while (g.s.stage < Stage::Adult && day < 400) {
    ++day;
    g.addReading(60 * 60, at(day, 9));
  }
  CHECK(g.s.stage == Stage::Adult);
  std::printf("plant became an adult on day %d\n", day - 300);
  CHECK(g.pendingMilestone() == 3);
  g.ackMilestone();
  CHECK(g.pendingMilestone() == 7);

  // Outings happen on some days and bring keepsakes back.
  PetModel o;
  o.adopt(Species::Cloud, nullptr, at(500, 9));
  for (int i = 0; i < 3; ++i) o.tapEgg(at(500, 9));
  o.s.stage = Stage::Child;
  int outings = 0;
  for (int dd = 501; dd < 541; ++dd) {
    o.tick(at(dd, 9, 50));
    o.s.fullness = 90;
    o.s.energy = 90;
    o.s.napping = false;
    o.tick(at(dd, 10, 5));
    if (o.mode(at(dd, 10, 5)) == Mode::Away) {
      ++outings;
      CHECK(o.callBack(at(dd, 10, 10)) == Event::Returned);
      CHECK(o.mode(at(dd, 10, 10)) != Mode::Away);
      CHECK(o.s.pendingKeepsake >= 0);
    }
  }
  std::printf("cloud went out on %d of 40 days, keepsakes 0x%02x\n", outings, o.s.keepsakes);
  CHECK(outings > 4 && outings < 30);

  // Dialogue fills %n and avoids immediate repeats.
  char a[96], b[96];
  p.line(at(103, 12), a, sizeof(a));
  p.line(at(103, 12), b, sizeof(b));
  CHECK(std::strcmp(a, b) != 0);
  CHECK(std::strstr(a, "%n") == nullptr);

  // No trusted clock: nothing decays, nothing crashes.
  PetModel n;
  n.adopt(Species::Dog, nullptr, Clock{});
  for (int i = 0; i < 3; ++i) n.tapEgg(Clock{});
  const int16_t f = n.s.fullness;
  n.tick(Clock{});
  CHECK(n.s.fullness == f);
  CHECK(n.ageDays(Clock{}) == 0);

  std::printf(failures ? "%d FAILED\n" : "all pet model tests passed\n", failures);
  return failures ? 1 : 0;
}
