#include "PetModel.h"

#include <cstdio>
#include <cstring>

namespace pp {

const FoodItem FOODS[FOOD_COUNT] = {
    {"Berry", 15, 8},     {"Cookie", 30, 15}, {"Juice", 60, 25},
    {"Onigiri", 120, 45}, {"Ramen", 240, 75}, {"Bento", 360, 100},
};

const char* const KEEPSAKES[KEEPSAKE_COUNT] = {
    "a shiny pebble", "a clover leaf", "a tiny shell", "a bottle cap",
    "a lost button",  "a pine cone",   "a paper star", "a bird feather",
};

namespace {

// Dialogue conditions. A line is eligible when every bit in `need` holds.
enum Cond : uint32_t {
  HUNGRY = 1u << 0,
  TIRED = 1u << 1,
  LOVED = 1u << 2,
  LONELY = 1u << 3,
  NIGHT = 1u << 4,
  READ_TODAY = 1u << 5,
  NO_READ = 1u << 6,
  CONTENT = 1u << 7,
  YOUNG = 1u << 8,
  GROWN = 1u << 9,
  FOODIE = 1u << 10,
  CUDDLY = 1u << 11,
  ACTIVE = 1u << 12,
  BOOKWORM = 1u << 13,
  MORNING = 1u << 14,
  STREAK = 1u << 15,
};

struct Line {
  uint32_t need;
  uint8_t species;  // 0 = any, else 1 + Species
  const char* text;
};

constexpr uint8_t ANY = 0;
constexpr uint8_t DOG = 1 + static_cast<uint8_t>(Species::Dog);
constexpr uint8_t CAT = 1 + static_cast<uint8_t>(Species::Cat);
constexpr uint8_t CLOUD = 1 + static_cast<uint8_t>(Species::Cloud);
constexpr uint8_t PLANT = 1 + static_cast<uint8_t>(Species::Plant);

// %n is replaced with the pet's name.
const Line LINES[] = {
    {HUNGRY, ANY, "My tummy is rumbling..."},
    {HUNGRY, ANY, "Read a chapter, then snacks?"},
    {HUNGRY | NO_READ, ANY, "No reading today... no food today."},
    {HUNGRY, DOG, "*stares at food bowl*"},
    {HUNGRY, CAT, "Mrrp. Bowl. Empty. Fix?"},
    {HUNGRY, CLOUD, "I'm feeling a bit thin..."},
    {HUNGRY, PLANT, "Soil's dry. Just saying."},
    {TIRED, ANY, "*yawn* ...five more minutes."},
    {TIRED, ANY, "Too sleepy to play."},
    {TIRED, CLOUD, "Drifting off..."},
    {LONELY, ANY, "Did you forget me?"},
    {LONELY, ANY, "A little pat would be nice."},
    {LONELY, CAT, "I'm not lonely. ...Okay, a bit."},
    {LOVED, ANY, "You're my favourite human!"},
    {LOVED, ANY, "Best day ever with you."},
    {LOVED, DOG, "*tail wag intensifies*"},
    {LOVED, CAT, "*purrs loudly*"},
    {LOVED, CLOUD, "I'm so fluffy and happy!"},
    {LOVED, PLANT, "I feel a bloom coming!"},
    {READ_TODAY, ANY, "Good reading today!"},
    {READ_TODAY, ANY, "What happens next in your book?"},
    {READ_TODAY | BOOKWORM, ANY, "Another page! Another!"},
    {NO_READ, ANY, "Read with me? Pages = snacks."},
    {NO_READ | GROWN, ANY, "Still on chapter one?"},
    {CONTENT, ANY, "Hello there!"},
    {CONTENT, ANY, "Hi, I'm %n!"},
    {CONTENT, ANY, "What are we doing today?"},
    {CONTENT, ANY, "*happy wiggle*"},
    {CONTENT, DOG, "Woof! Walk later?"},
    {CONTENT, DOG, "I'm a very good pup."},
    {CONTENT, CAT, "Mrrp~"},
    {CONTENT, CAT, "This spot is mine now."},
    {CONTENT, CLOUD, "*floats happily*"},
    {CONTENT, CLOUD, "Partly cloudy, fully cute."},
    {CONTENT, PLANT, "*rustle rustle*"},
    {CONTENT, PLANT, "Growing a little every day."},
    {CONTENT | YOUNG, ANY, "Everything is so big!"},
    {CONTENT | YOUNG, ANY, "What's that? And that?"},
    {CONTENT | FOODIE, ANY, "Thinking about bento..."},
    {CONTENT | FOODIE, ANY, "Is it snack time yet?"},
    {CONTENT | CUDDLY, ANY, "Hug? Hug. Hug!"},
    {CONTENT | CUDDLY, ANY, "Stay close, okay?"},
    {CONTENT | ACTIVE, ANY, "Let's play! Let's play!"},
    {CONTENT | ACTIVE, ANY, "I did ten laps already!"},
    {CONTENT | BOOKWORM, ANY, "Tell me about your book."},
    {CONTENT | BOOKWORM, ANY, "Shh... I'm reading too."},
    {MORNING, ANY, "Good morning!"},
    {MORNING, ANY, "Morning! Breakfast?"},
    {STREAK, ANY, "We've been together every day!"},
    {NIGHT, ANY, "Zzz..."},
};
constexpr int LINE_COUNT = sizeof(LINES) / sizeof(LINES[0]);

}  // namespace

uint32_t PetModel::rand32() {
  // xorshift32
  uint32_t x = s.rng ? s.rng : 0x9E3779B9u;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  s.rng = x;
  return x;
}

const char* PetModel::defaultName(Species sp) {
  switch (sp) {
    case Species::Dog:
      return "Mochi";
    case Species::Cat:
      return "Tofu";
    case Species::Cloud:
      return "Puff";
    case Species::Plant:
      return "Sprout";
    default:
      return "Pet";
  }
}

const char* PetModel::speciesName(Species sp) {
  switch (sp) {
    case Species::Dog:
      return "Dog";
    case Species::Cat:
      return "Cat";
    case Species::Cloud:
      return "Cloud";
    case Species::Plant:
      return "Plant";
    default:
      return "Pet";
  }
}

const char* PetModel::stageName(Stage st, Species sp) {
  switch (st) {
    case Stage::Egg:
      return sp == Species::Plant ? "Seed" : "Egg";
    case Stage::Baby:
      return sp == Species::Plant ? "Sprout" : "Baby";
    case Stage::Child:
      return "Child";
    case Stage::Teen:
      return "Teen";
    case Stage::Adult:
      return "Adult";
  }
  return "";
}

const char* PetModel::personalityName(Personality p) {
  switch (p) {
    case Personality::Foodie:
      return "Foodie";
    case Personality::Cuddly:
      return "Cuddly";
    case Personality::Active:
      return "Active";
    case Personality::Bookworm:
      return "Bookworm";
    default:
      return "";
  }
}

void PetModel::adopt(Species sp, const char* name, const Clock& c) {
  const uint32_t seed = s.rng ^ static_cast<uint32_t>(c.epoch) ^ 0xA5A5u;
  const bool sound = s.soundOn;
  const bool guide = s.guideSeen;
  s = PetState{};
  s.rng = seed ? seed : 1;
  s.soundOn = sound;
  s.guideSeen = guide;
  s.adopted = true;
  s.species = sp;
  snprintf(s.name, sizeof(s.name), "%s", (name && *name) ? name : defaultName(sp));
  s.lastTick = c.valid ? c.epoch : 0;
  lastPose = Pose::Idle;
  lastEvent = Event::None;
}

Mode PetModel::mode(const Clock& c) const {
  if (s.awayUntil != 0 && (!c.valid || c.epoch < s.awayUntil)) return Mode::Away;
  if (s.stage != Stage::Egg && isNight(c)) return Mode::Sleeping;
  if (s.napping) return Mode::Napping;
  return Mode::Awake;
}

int PetModel::ageDays(const Clock& c) const {
  if (!c.valid || s.bornEpoch == 0 || c.epoch < s.bornEpoch) return 0;
  return static_cast<int>((c.epoch - s.bornEpoch) / 86400);
}

void PetModel::tick(const Clock& c) {
  if (!s.adopted || !c.valid) return;
  if (s.lastTick == 0 || c.epoch < s.lastTick) {
    s.lastTick = c.epoch;
    return;
  }
  if (s.capDay != c.day) {
    s.capDay = c.day;
    s.dayGrowth = 0;
    s.dayLove = 0;
    s.todayReadSec = 0;
  }

  // Back from an outing?
  if (s.awayUntil != 0 && c.epoch >= s.awayUntil) {
    s.awayUntil = 0;
    const int k = static_cast<int>(rand32() % KEEPSAKE_COUNT);
    s.keepsakes |= static_cast<uint8_t>(1u << k);
    s.pendingKeepsake = static_cast<int8_t>(k);
    lastEvent = Event::Returned;
  }

  int64_t minutes = (c.epoch - s.lastTick) / 60;
  if (minutes <= 0) return;
  s.lastTick += minutes * 60;
  if (minutes > 7 * 24 * 60) minutes = 7 * 24 * 60;  // a week away is plenty of decay
  if (s.stage == Stage::Egg) return;

  const int nowMinOfDay = c.hour * 60 + c.minute;
  const int64_t lastAbsMin = s.lastTick / 60;
  for (int64_t i = minutes - 1; i >= 0; --i) {
    const int64_t absMin = lastAbsMin - i;
    int mod = static_cast<int>((nowMinOfDay - i) % 1440);
    if (mod < 0) mod += 1440;
    const int hour = mod / 60;
    const bool night = hour >= 22 || hour < 7;
    const bool away = s.awayUntil != 0;
    const bool asleep = night || s.napping;

    if (absMin % (asleep || away ? 30 : 15) == 0) s.fullness = clamp(s.fullness - 1);
    if (asleep) {
      s.energy = clamp(s.energy + 2);
      if (s.napping && s.energy >= 70) s.napping = false;
    } else if (!away && absMin % 12 == 0) {
      s.energy = clamp(s.energy - 1);
      if (s.energy <= 10) s.napping = true;
    }
    if (s.fullness == 0 && absMin % 60 == 0) s.love = clamp(s.love - 1);
  }

  // One outing chance per day, daytime only, once the pet is old enough.
  if (s.stage >= Stage::Child && s.outingDay != c.day && s.awayUntil == 0 && !s.napping && c.hour >= 10 &&
      c.hour < 18) {
    s.outingDay = c.day;
    if (s.fullness >= 40 && s.energy >= 40 && rand32() % 100 < 35) {
      s.awayUntil = c.epoch + static_cast<int64_t>(1 + rand32() % 7) * 3600;
    }
  }
}

void PetModel::care(int growth, int love, const Clock& c) {
  const int32_t day = c.valid ? c.day : s.capDay;
  if (c.valid && s.capDay != day) {
    s.capDay = day;
    s.dayGrowth = 0;
    s.dayLove = 0;
    s.todayReadSec = 0;
  }
  const int g = growth < MAX_DAILY_GROWTH - s.dayGrowth ? growth : MAX_DAILY_GROWTH - s.dayGrowth;
  const int l = love < MAX_DAILY_LOVE - s.dayLove ? love : MAX_DAILY_LOVE - s.dayLove;
  if (g > 0) {
    s.growth = static_cast<int16_t>(s.growth + g);
    s.dayGrowth = static_cast<uint8_t>(s.dayGrowth + g);
  }
  if (l > 0) {
    s.love = clamp(s.love + l);
    s.dayLove = static_cast<uint8_t>(s.dayLove + l);
  }
  if (c.valid && s.careDay != day) {
    s.streak = (s.careDay == day - 1) ? static_cast<uint16_t>(s.streak + 1) : 1;
    if (s.streak > s.bestStreak) s.bestStreak = s.streak;
    s.careDay = day;
  }
  checkStage(c);
}

void PetModel::checkStage(const Clock&) {
  while (s.stage >= Stage::Baby && s.stage < Stage::Adult && s.growth >= STAGE_GROWTH[static_cast<int>(s.stage) + 1]) {
    s.stage = static_cast<Stage>(static_cast<int>(s.stage) + 1);
    lastEvent = Event::GrewUp;
    if (s.stage == Stage::Teen) {
      const uint16_t ev[4] = {s.evFeed, s.evPet, s.evPlay, s.evRead};
      int best = 0;
      for (int i = 1; i < 4; ++i)
        if (ev[i] > ev[best]) best = i;
      s.personality = static_cast<Personality>(best + 1);
      lastEvent = Event::Personality;
    }
  }
}

void PetModel::addReading(uint32_t seconds, const Clock& c) {
  if (!s.adopted || seconds == 0) return;
  if (c.valid && s.capDay != c.day) {
    s.capDay = c.day;
    s.dayGrowth = 0;
    s.dayLove = 0;
    s.todayReadSec = 0;
  }
  const uint32_t before = s.todayReadSec / 600;
  s.readCreditSec += seconds;
  s.totalReadSec += seconds;
  s.todayReadSec += seconds;
  const uint32_t after = s.todayReadSec / 600;
  if (after > before && s.stage != Stage::Egg) {
    // Every 10 minutes of reading counts as care.
    if (s.stage >= Stage::Child) s.evRead = static_cast<uint16_t>(s.evRead + (after - before));
    care(2 * static_cast<int>(after - before), 1, c);
  }
}

Event PetModel::tapEgg(const Clock& c) {
  if (s.stage != Stage::Egg) return Event::None;
  if (++s.hatchTaps < 3) return Event::None;
  s.stage = Stage::Baby;
  s.bornEpoch = c.valid ? c.epoch : 0;
  s.lastTick = c.valid ? c.epoch : 0;
  s.fullness = 70;
  s.energy = 90;
  s.love = 30;
  lastPose = Pose::Excited;
  care(1, 1, c);
  lastEvent = Event::Hatched;
  return Event::Hatched;
}

Event PetModel::feed(int item, const Clock& c) {
  if (item < 0 || item >= FOOD_COUNT) return Event::None;
  const Mode m = mode(c);
  if (m == Mode::Away) return Event::Away;
  if (m == Mode::Sleeping || m == Mode::Napping) return Event::Sleepy;
  if (s.fullness >= 95) return Event::Full;
  if (!canAfford(item)) return Event::NotEnoughReading;
  s.readCreditSec -= FOODS[item].costMin * 60u;
  s.fullness = clamp(s.fullness + FOODS[item].fullness);
  if (s.stage >= Stage::Child) ++s.evFeed;
  lastPose = Pose::Eating;
  lastEvent = Event::None;
  care(item >= 3 ? 3 : 2, 1, c);
  return lastEvent;
}

Event PetModel::pet(const Clock& c) {
  const Mode m = mode(c);
  if (m == Mode::Away) return Event::Away;
  lastEvent = Event::None;
  if (m == Mode::Sleeping || m == Mode::Napping) {
    lastPose = Pose::Sleeping;
    care(0, 1, c);
    return lastEvent;
  }
  if (s.stage >= Stage::Child) ++s.evPet;
  lastPose = Pose::Happy;
  care(1, 2, c);
  return lastEvent;
}

Event PetModel::play(const Clock& c) {
  const Mode m = mode(c);
  if (m == Mode::Away) return Event::Away;
  if (m == Mode::Sleeping || m == Mode::Napping) return Event::Sleepy;
  if (s.energy < 15) return Event::TooTired;
  s.energy = clamp(s.energy - 15);
  s.fullness = clamp(s.fullness - 5);
  if (s.stage >= Stage::Child) ++s.evPlay;
  lastPose = Pose::Excited;
  lastEvent = Event::None;
  care(3, 2, c);
  return lastEvent;
}

Event PetModel::callBack(const Clock& c) {
  if (s.awayUntil == 0) return Event::None;
  s.awayUntil = c.valid ? c.epoch : 1;
  lastEvent = Event::None;
  tick(c);
  if (s.awayUntil != 0) {  // no trusted clock: return anyway
    s.awayUntil = 0;
    lastEvent = Event::Returned;
  }
  lastPose = Pose::Happy;
  return Event::Returned;
}

void PetModel::pomodoroDone(const Clock& c) {
  if (!s.adopted || s.stage == Stage::Egg) return;
  ++s.pomodoros;
  if (s.stage >= Stage::Child) ++s.evRead;
  care(2, 1, c);
}

int PetModel::stageProgressPct() const {
  if (s.stage == Stage::Adult) return 100;
  if (s.stage == Stage::Egg) return s.hatchTaps * 100 / 3;
  const int cur = STAGE_GROWTH[static_cast<int>(s.stage)];
  const int next = STAGE_GROWTH[static_cast<int>(s.stage) + 1];
  int pct = (s.growth - cur) * 100 / (next - cur);
  return pct < 0 ? 0 : (pct > 100 ? 100 : pct);
}

int PetModel::pendingMilestone() const {
  for (int i = 0; i < 4; ++i) {
    if (s.streak >= MILESTONES[i] && !(s.milestonesShown & (1u << i))) return MILESTONES[i];
  }
  return 0;
}

void PetModel::ackMilestone() {
  for (int i = 0; i < 4; ++i) {
    if (s.streak >= MILESTONES[i] && !(s.milestonesShown & (1u << i))) {
      s.milestonesShown |= static_cast<uint8_t>(1u << i);
      return;
    }
  }
}

const char* PetModel::line(const Clock& c, char* buf, int bufLen) {
  uint32_t have = 0;
  if (s.fullness < 30) have |= HUNGRY;
  if (s.energy < 25) have |= TIRED;
  if (s.love >= 70) have |= LOVED;
  if (s.love < 25) have |= LONELY;
  if (isNight(c)) have |= NIGHT;
  if (c.valid && c.hour >= 7 && c.hour < 10) have |= MORNING;
  if (s.todayReadSec >= 600) have |= READ_TODAY;
  if (s.todayReadSec == 0) have |= NO_READ;
  if (!(have & (HUNGRY | TIRED | LONELY))) have |= CONTENT;
  if (s.stage <= Stage::Child) have |= YOUNG;
  if (s.stage >= Stage::Teen) have |= GROWN;
  if (s.streak >= 3) have |= STREAK;
  switch (s.personality) {
    case Personality::Foodie:
      have |= FOODIE;
      break;
    case Personality::Cuddly:
      have |= CUDDLY;
      break;
    case Personality::Active:
      have |= ACTIVE;
      break;
    case Personality::Bookworm:
      have |= BOOKWORM;
      break;
    default:
      break;
  }
  const uint8_t sp = static_cast<uint8_t>(1 + static_cast<uint8_t>(s.species));

  // Needs speak first: when hungry or tired, prefer those lines.
  const uint32_t urgent = have & (HUNGRY | TIRED);
  uint8_t pool[LINE_COUNT];
  int n = 0;
  for (int pass = 0; pass < 2 && n == 0; ++pass) {
    for (int i = 0; i < LINE_COUNT; ++i) {
      const Line& l = LINES[i];
      if ((l.need & have) != l.need) continue;
      if (l.species != ANY && l.species != sp) continue;
      if (pass == 0 && urgent && !(l.need & urgent)) continue;
      if (pass == 0 && (l.need & NIGHT) != (have & NIGHT)) continue;
      bool recent = false;
      for (uint8_t r : s.recentLines) recent |= (r == i);
      if (recent && pass == 0) continue;
      pool[n++] = static_cast<uint8_t>(i);
    }
  }
  const uint8_t pick = n ? pool[rand32() % n] : 24;
  s.recentLines[2] = s.recentLines[1];
  s.recentLines[1] = s.recentLines[0];
  s.recentLines[0] = pick;

  // Expand %n.
  const char* src = LINES[pick].text;
  int o = 0;
  for (const char* p = src; *p && o < bufLen - 1; ++p) {
    if (p[0] == '%' && p[1] == 'n') {
      for (const char* q = s.name; *q && o < bufLen - 1; ++q) buf[o++] = *q;
      ++p;
    } else {
      buf[o++] = *p;
    }
  }
  buf[o] = 0;
  return buf;
}

}  // namespace pp
