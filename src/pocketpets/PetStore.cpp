#include "PetStore.h"

#include <Logging.h>
#include <TrustedTime.h>

#include <cstring>
#include <ctime>

namespace {
constexpr uint32_t SAVE_EVERY_READ_SEC = 5 * 60;

// Days since 1970-01-01 for a calendar date (Howard Hinnant's algorithm).
int32_t daysFromCivil(int y, int m, int d) {
  y -= m <= 2;
  const int era = (y >= 0 ? y : y - 399) / 400;
  const int yoe = y - era * 400;
  const int doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  const int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + doe - 719468;
}
}  // namespace

pp::Clock PetStore::now() {
  pp::Clock c;
  const int64_t epoch = trustedtime::trustedNow();
  if (epoch <= 0) return c;
  const time_t t = static_cast<time_t>(epoch);
  struct tm local;
  if (!localtime_r(&t, &local)) return c;
  c.valid = true;
  c.epoch = epoch;
  c.hour = static_cast<uint8_t>(local.tm_hour);
  c.minute = static_cast<uint8_t>(local.tm_min);
  c.day = daysFromCivil(local.tm_year + 1900, local.tm_mon + 1, local.tm_mday);
  return c;
}

void PetStore::addReadingMs(uint32_t ms) {
  if (!model.s.adopted) return;
  pendingMs += ms;
  if (pendingMs < 1000) return;
  const uint32_t sec = pendingMs / 1000;
  pendingMs %= 1000;
  model.addReading(sec, now());
  unsavedSec += sec;
  dirty = true;
  if (unsavedSec >= SAVE_EVERY_READ_SEC) flush();
}

void PetStore::flush() {
  if (!dirty) return;
  if (saveToFile()) {
    dirty = false;
    unsavedSec = 0;
  }
}

void PetStore::toJson(JsonDocument& doc) const {
  const pp::PetState& s = model.s;
  doc["v"] = 1;
  doc["adopted"] = s.adopted;
  doc["species"] = static_cast<uint8_t>(s.species);
  doc["name"] = s.name;
  doc["stage"] = static_cast<uint8_t>(s.stage);
  doc["persona"] = static_cast<uint8_t>(s.personality);
  doc["taps"] = s.hatchTaps;
  doc["growth"] = s.growth;
  doc["love"] = s.love;
  doc["full"] = s.fullness;
  doc["energy"] = s.energy;
  doc["credit"] = s.readCreditSec;
  doc["readTotal"] = s.totalReadSec;
  doc["readToday"] = s.todayReadSec;
  doc["evFeed"] = s.evFeed;
  doc["evPet"] = s.evPet;
  doc["evPlay"] = s.evPlay;
  doc["evRead"] = s.evRead;
  doc["careDay"] = s.careDay;
  doc["capDay"] = s.capDay;
  doc["dayGrowth"] = s.dayGrowth;
  doc["dayLove"] = s.dayLove;
  doc["streak"] = s.streak;
  doc["bestStreak"] = s.bestStreak;
  doc["milestones"] = s.milestonesShown;
  doc["born"] = s.bornEpoch;
  doc["lastTick"] = s.lastTick;
  doc["napping"] = s.napping;
  doc["awayUntil"] = s.awayUntil;
  doc["outingDay"] = s.outingDay;
  doc["keepsakes"] = s.keepsakes;
  doc["pendingKeepsake"] = s.pendingKeepsake;
  doc["pomodoros"] = s.pomodoros;
  doc["sound"] = s.soundOn;
  doc["guideSeen"] = s.guideSeen;
  doc["rng"] = s.rng;
  doc["customStatus"] = customStatus;
}

bool PetStore::fromJson(JsonVariantConst doc) {
  pp::PetState s;
  s.adopted = doc["adopted"] | false;
  const uint8_t species = doc["species"] | 0;
  s.species = species < static_cast<uint8_t>(pp::Species::Count) ? static_cast<pp::Species>(species) : pp::Species::Dog;
  strncpy(s.name, doc["name"] | "", sizeof(s.name) - 1);
  const uint8_t stage = doc["stage"] | 0;
  s.stage = stage <= static_cast<uint8_t>(pp::Stage::Adult) ? static_cast<pp::Stage>(stage) : pp::Stage::Egg;
  const uint8_t persona = doc["persona"] | 0;
  s.personality = persona <= static_cast<uint8_t>(pp::Personality::Bookworm) ? static_cast<pp::Personality>(persona)
                                                                             : pp::Personality::None;
  s.hatchTaps = doc["taps"] | 0;
  s.growth = doc["growth"] | 0;
  s.love = doc["love"] | 20;
  s.fullness = doc["full"] | 70;
  s.energy = doc["energy"] | 80;
  s.readCreditSec = doc["credit"] | 0u;
  s.totalReadSec = doc["readTotal"] | 0u;
  s.todayReadSec = doc["readToday"] | 0u;
  s.evFeed = doc["evFeed"] | 0;
  s.evPet = doc["evPet"] | 0;
  s.evPlay = doc["evPlay"] | 0;
  s.evRead = doc["evRead"] | 0;
  s.careDay = doc["careDay"] | -1;
  s.capDay = doc["capDay"] | -1;
  s.dayGrowth = doc["dayGrowth"] | 0;
  s.dayLove = doc["dayLove"] | 0;
  s.streak = doc["streak"] | 0;
  s.bestStreak = doc["bestStreak"] | 0;
  s.milestonesShown = doc["milestones"] | 0;
  s.bornEpoch = doc["born"] | static_cast<int64_t>(0);
  s.lastTick = doc["lastTick"] | static_cast<int64_t>(0);
  s.napping = doc["napping"] | false;
  s.awayUntil = doc["awayUntil"] | static_cast<int64_t>(0);
  s.outingDay = doc["outingDay"] | -1;
  s.keepsakes = doc["keepsakes"] | 0;
  s.pendingKeepsake = doc["pendingKeepsake"] | -1;
  s.pomodoros = doc["pomodoros"] | 0;
  s.soundOn = doc["sound"] | true;
  s.guideSeen = doc["guideSeen"] | false;
  s.rng = doc["rng"] | 0x9E3779B9u;
  model.s = s;
  strncpy(customStatus, doc["customStatus"] | "BACK SOON", sizeof(customStatus) - 1);
  customStatus[sizeof(customStatus) - 1] = 0;
  return true;
}
