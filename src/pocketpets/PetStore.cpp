#include "PetStore.h"

#include <HalStorage.h>
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

namespace {
// Calls fn(line, len) for each quote line in the file; stops when fn returns false.
template <typename Fn>
void forEachQuote(const String& text, Fn fn) {
  int start = 0;
  const int n = static_cast<int>(text.length());
  while (start < n) {
    int end = text.indexOf('\n', start);
    if (end < 0) end = n;
    int a = start, b = end;
    while (a < b && (text[a] == ' ' || text[a] == '\t' || text[a] == '\r')) ++a;
    while (b > a && (text[b - 1] == ' ' || text[b - 1] == '\t' || text[b - 1] == '\r')) --b;
    if (b > a && text[a] != '#' && !fn(text.c_str() + a, b - a)) return;
    start = end + 1;
  }
}
}  // namespace

int PetStore::quoteCount() const {
  if (!Storage.exists(QUOTES_FILE)) return 0;
  const String text = Storage.readFile(QUOTES_FILE);
  int n = 0;
  forEachQuote(text, [&](const char*, int) {
    ++n;
    return true;
  });
  return n;
}

bool PetStore::randomQuote(uint32_t rnd, char* buf, int len) const {
  if (!Storage.exists(QUOTES_FILE)) return false;
  const String text = Storage.readFile(QUOTES_FILE);
  int n = 0;
  forEachQuote(text, [&](const char*, int) {
    ++n;
    return true;
  });
  if (n == 0) return false;
  int pick = static_cast<int>(rnd % n);
  bool found = false;
  forEachQuote(text, [&](const char* line, int lineLen) {
    if (pick-- > 0) return true;
    snprintf(buf, len, "%.*s", lineLen, line);
    found = true;
    return false;
  });
  return found;
}

bool PetStore::addQuote(const char* text) {
  if (!text || !*text) return false;
  Storage.ensureDirectoryExists("/pocketpets");
  String all = Storage.exists(QUOTES_FILE) ? Storage.readFile(QUOTES_FILE) : String();
  if (all.length() && !all.endsWith("\n")) all += "\n";
  all += text;
  all += "\n";
  return Storage.writeFile(QUOTES_FILE, all);
}

const char* PetStore::speak(const pp::Clock& clk, char* buf, int len) {
  const pp::PetState& s = model.s;
  const bool urgent = s.fullness < 30 || s.energy < 25 || model.isNight(clk);
  if (!urgent && s.stage != pp::Stage::Egg && (s.chatterOn || s.quotesOn)) {
    // Roughly a third of lines come from the extras, split between them.
    const uint32_t roll = (s.rng >> 5) % 100;
    model.s.rng = model.s.rng * 1664525u + 1013904223u;
    const bool both = s.chatterOn && s.quotesOn;
    const bool tryQuote = s.quotesOn && (both ? roll < 18 : roll < 35);
    const bool tryBook = s.chatterOn && !tryQuote && (both ? roll < 38 : roll < 35);
    if (tryQuote && randomQuote(model.s.rng >> 7, buf, len)) return buf;
    if (tryBook) {
      std::lock_guard<std::mutex> lock(chatterMutex);
      if (chatter.line(s, model.s.rng >> 9, buf, len)) return buf;
    }
  }
  return model.line(clk, buf, len);
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
  doc["chatter"] = s.chatterOn;
  doc["quotes"] = s.quotesOn;
  doc["customStatus"] = customStatus;
  doc["birthday"] = birthday;

  std::lock_guard<std::mutex> lock(chatterMutex);
  const pp::BookMemory& b = chatter.m;
  JsonObject book = doc["book"].to<JsonObject>();
  book["title"] = b.title;
  book["author"] = b.author;
  book["chapter"] = b.chapter;
  book["prev"] = b.prevTitle;
  book["word"] = b.word;
  book["quote"] = b.quote;
  book["pages"] = b.pages;
  JsonArray genre = book["genre"].to<JsonArray>();
  for (uint16_t g : b.genre) genre.add(g);
  JsonArray names = book["names"].to<JsonArray>();
  for (const auto& n : b.names) {
    if (!n.count) continue;
    JsonObject o = names.add<JsonObject>();
    o["n"] = n.text;
    o["c"] = n.count;
  }
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
  s.chatterOn = doc["chatter"] | false;
  s.quotesOn = doc["quotes"] | false;
  model.s = s;

  {
    std::lock_guard<std::mutex> lock(chatterMutex);
    pp::BookMemory b;
    JsonVariantConst book = doc["book"];
    strncpy(b.title, book["title"] | "", sizeof(b.title) - 1);
    strncpy(b.author, book["author"] | "", sizeof(b.author) - 1);
    strncpy(b.chapter, book["chapter"] | "", sizeof(b.chapter) - 1);
    strncpy(b.prevTitle, book["prev"] | "", sizeof(b.prevTitle) - 1);
    strncpy(b.word, book["word"] | "", sizeof(b.word) - 1);
    strncpy(b.quote, book["quote"] | "", sizeof(b.quote) - 1);
    b.pages = book["pages"] | 0u;
    int gi = 0;
    for (JsonVariantConst g : book["genre"].as<JsonArrayConst>()) {
      if (gi < static_cast<int>(pp::Genre::Count)) b.genre[gi++] = g | 0;
    }
    int ni = 0;
    for (JsonVariantConst o : book["names"].as<JsonArrayConst>()) {
      if (ni >= pp::BookMemory::NAME_SLOTS) break;
      strncpy(b.names[ni].text, o["n"] | "", sizeof(b.names[ni].text) - 1);
      b.names[ni].count = o["c"] | 0;
      ++ni;
    }
    chatter.m = b;
  }
  birthday = doc["birthday"] | 0;
  strncpy(customStatus, doc["customStatus"] | "BACK SOON", sizeof(customStatus) - 1);
  customStatus[sizeof(customStatus) - 1] = 0;
  return true;
}
