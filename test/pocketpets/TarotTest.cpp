// Host test for Pocket Pets tarot dealing and birth cards:
// g++ -std=c++17 -I src test/pocketpets/TarotTest.cpp src/pocketpets/Tarot.cpp src/pocketpets/PetCanvas.cpp
//     src/pocketpets/PetArt.cpp src/pocketpets/PetModel.cpp
#include <cstdio>

#include "pocketpets/Screens.h"
using namespace pp;

static int failures = 0;
#define CHECK(c)                                              \
  do {                                                        \
    if (!(c)) {                                               \
      std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); \
      ++failures;                                             \
    }                                                         \
  } while (0)

int main() {
  // Birth cards: 7 + 14 + 1998 = 2019 -> 2+0+1+9 = 12 (Hanged Man), paired with 1+2 = 3 (Empress).
  int second = 0;
  CHECK(tarotBirthCards(1998, 7, 14, second) == 12 && second == 3);
  // 12 + 31 + 1999 = 2042 -> 8 (Strength), single card.
  CHECK(tarotBirthCards(1999, 12, 31, second) == 8 && second == -1);
  // 2 + 29 + 1984 = 2015 -> 8.
  CHECK(tarotBirthCards(1984, 2, 29, second) == 8);
  CHECK(tarotBirthCards(0, 1, 1, second) == -1);

  // Shuffles: three different cards, every card can come up, about half reversed.
  int seen[TAROT_COUNT] = {};
  int reversedCount = 0, dealt = 0;
  for (uint32_t seed = 1; seed <= 20000; ++seed) {
    int cards[3];
    bool rev[3];
    tarotShuffleDraw(seed * 2654435761u, 3, cards, rev);
    CHECK(cards[0] != cards[1] && cards[1] != cards[2] && cards[0] != cards[2]);
    for (int k = 0; k < 3; ++k) {
      CHECK(cards[k] >= 0 && cards[k] < TAROT_COUNT);
      ++seen[cards[k]];
      reversedCount += rev[k];
      ++dealt;
    }
  }
  int lo = dealt, hi = 0;
  for (int i = 0; i < TAROT_COUNT; ++i) {
    lo = seen[i] < lo ? seen[i] : lo;
    hi = seen[i] > hi ? seen[i] : hi;
  }
  std::printf("each card dealt %d..%d times of %d; reversed %.1f%%\n", lo, hi, dealt, 100.0 * reversedCount / dealt);
  CHECK(lo > dealt / TAROT_COUNT * 0.9 && hi < dealt / TAROT_COUNT * 1.1);
  CHECK(reversedCount > dealt * 0.47 && reversedCount < dealt * 0.53);

  // The same seed deals the same reading (card of the day).
  int a[1], b[1];
  bool ra[1], rb[1];
  tarotShuffleDraw(12345, 1, a, ra);
  tarotShuffleDraw(12345, 1, b, rb);
  CHECK(a[0] == b[0] && ra[0] == rb[0]);

  std::printf(failures ? "%d FAILED\n" : "tarot tests passed\n", failures);
  return failures ? 1 : 0;
}
