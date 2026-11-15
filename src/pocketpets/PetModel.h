#pragma once
// Pocket Pets: the pet's state and rules. Plain C++ (no Arduino) so the host
// preview tool and tests can run it.

#include <cstdint>

namespace pp {

enum class Species : uint8_t { Dog, Cat, Cloud, Plant, Count };
enum class Stage : uint8_t { Egg, Baby, Child, Teen, Adult };
enum class Personality : uint8_t { None, Foodie, Cuddly, Active, Bookworm };
enum class Mode : uint8_t { Awake, Sleeping, Napping, Away };
enum class Pose : uint8_t { Idle, Happy, Eating, Sleeping, Sad, Excited };

// Wall clock as seen by the model. valid == false when the device has no
// trusted time; time-based rules then pause instead of guessing.
struct Clock {
  bool valid = false;
  int64_t epoch = 0;   // UTC seconds
  int32_t day = 0;     // local day number (days since epoch, local time)
  uint8_t hour = 0;    // local hour 0..23
  uint8_t minute = 0;  // local minute
};

struct FoodItem {
  const char* name;
  uint16_t costMin;  // reading minutes
  uint8_t fullness;  // fullness restored
};
constexpr int FOOD_COUNT = 6;
extern const FoodItem FOODS[FOOD_COUNT];

// Growth needed to enter each stage (index = Stage).
constexpr int16_t STAGE_GROWTH[] = {0, 0, 20, 60, 140};
constexpr int MAX_DAILY_GROWTH = 10;
constexpr int MAX_DAILY_LOVE = 8;
constexpr uint16_t MILESTONES[] = {3, 7, 30, 100};
constexpr int KEEPSAKE_COUNT = 8;
extern const char* const KEEPSAKES[KEEPSAKE_COUNT];

struct PetState {
  bool adopted = false;
  Species species = Species::Dog;
  char name[16] = "";
  Stage stage = Stage::Egg;
  Personality personality = Personality::None;
  uint8_t hatchTaps = 0;

  int16_t growth = 0;
  int16_t love = 20;      // 0..100
  int16_t fullness = 70;  // 0..100
  int16_t energy = 80;    // 0..100

  uint32_t readCreditSec = 0;  // spendable on food
  uint32_t totalReadSec = 0;
  uint32_t todayReadSec = 0;  // resets each local day

  // Personality evidence, gathered from Child stage on.
  uint16_t evFeed = 0, evPet = 0, evPlay = 0, evRead = 0;

  // Daily care pacing and streak.
  int32_t careDay = -1;  // day of the last care action
  int32_t capDay = -1;   // day the caps below belong to
  uint8_t dayGrowth = 0, dayLove = 0;
  uint16_t streak = 0, bestStreak = 0;
  uint8_t milestonesShown = 0;  // bit per MILESTONES entry

  int64_t bornEpoch = 0;
  int64_t lastTick = 0;  // epoch of the last applied time step
  bool napping = false;

  // Outings: one chance per day.
  int64_t awayUntil = 0;
  int32_t outingDay = -1;
  uint8_t keepsakes = 0;  // bit per KEEPSAKES entry
  int8_t pendingKeepsake = -1;

  uint16_t pomodoros = 0;
  bool soundOn = true;
  bool guideSeen = false;
  uint8_t recentLines[3] = {255, 255, 255};
  uint32_t rng = 0x9E3779B9u;
};

// Event a screen should celebrate once (shown as a banner).
enum class Event : uint8_t {
  None,
  Hatched,
  GrewUp,
  Personality,
  Milestone,
  Returned,
  NotEnoughReading,
  TooTired,
  Sleepy,
  Full,
  Away
};

class PetModel {
 public:
  PetState s;

  void adopt(Species sp, const char* name, const Clock& c);
  static const char* defaultName(Species sp);
  static const char* speciesName(Species sp);
  static const char* stageName(Stage st, Species sp);
  static const char* personalityName(Personality p);

  // Advance needs, sleep and outings to the clock. Safe to call often.
  void tick(const Clock& c);
  // Reading time from the reader. Earns food credit and care.
  void addReading(uint32_t seconds, const Clock& c);

  Mode mode(const Clock& c) const;
  bool isNight(const Clock& c) const { return c.valid && (c.hour >= 22 || c.hour < 7); }
  int ageDays(const Clock& c) const;

  // Interactions. Each returns the event to show (None on plain success).
  Event tapEgg(const Clock& c);
  Event feed(int item, const Clock& c);
  Event pet(const Clock& c);
  Event play(const Clock& c);
  Event callBack(const Clock& c);
  void pomodoroDone(const Clock& c);

  bool canAfford(int item) const { return s.readCreditSec >= FOODS[item].costMin * 60u; }
  int stageProgressPct() const;  // progress to the next stage, 100 for adults
  int pendingMilestone() const;  // milestone days to celebrate, or 0
  void ackMilestone();

  // Dialogue for the current state. Avoids repeating the last three lines.
  const char* line(const Clock& c, char* buf, int bufLen);

  Pose lastPose = Pose::Idle;
  Event lastEvent = Event::None;

 private:
  uint32_t rand32();
  void care(int growth, int love, const Clock& c);
  void checkStage(const Clock& c);
  static int16_t clamp(int v) { return static_cast<int16_t>(v < 0 ? 0 : (v > 100 ? 100 : v)); }
};

}  // namespace pp
