// Host test for Pocket Pets book chatter:
// g++ -std=c++17 -I src test/pocketpets/BookChatterTest.cpp src/pocketpets/BookChatter.cpp src/pocketpets/PetModel.cpp
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "pocketpets/BookChatter.h"
using namespace pp;

static int failures = 0;
#define CHECK(c)                                              \
  do {                                                        \
    if (!(c)) {                                               \
      std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); \
      ++failures;                                             \
    }                                                         \
  } while (0)

static void feed(BookChatter& bc, const char* title, const char* chapter, uint32_t key, const std::string& text) {
  if (!bc.beginPage(title, "A. Writer", chapter, key)) return;
  size_t i = 0;
  while (i < text.size()) {
    while (i < text.size() && text[i] == ' ') ++i;
    size_t j = i;
    while (j < text.size() && text[j] != ' ') ++j;
    if (j > i) bc.addWord(text.data() + i, j - i);
    i = j;
  }
  bc.endPage();
}

int main() {
  BookChatter bc;
  const std::string page =
      "The rain had not stopped for three days. Inspector Barnaby studied the evidence while Elsie "
      "paced by the window. \xE2\x80\x9CSomebody in this house is lying,\xE2\x80\x9D said Elsie quietly. "
      "Barnaby nodded. The murder had left no clue, only a strange, unbreakable silence. Elsie "
      "looked at Barnaby and then at the poison bottle. A suspect? Barnaby smiled. The detective "
      "knew the killer was close, and Elsie knew it too.";
  for (uint32_t p = 1; p <= 4; ++p) feed(bc, "The Quiet House", "Chapter 3: The Locked Room", p, page);
  feed(bc, "The Quiet House", "Chapter 3: The Locked Room", 4, page);  // same page again: ignored

  CHECK(bc.m.pages == 4);
  const char* name = bc.topName();
  CHECK(name && (std::strcmp(name, "Barnaby") == 0 || std::strcmp(name, "Elsie") == 0));
  CHECK(bc.topGenre() == Genre::Mystery);
  CHECK(std::strcmp(bc.m.word, "unbreakable") == 0);
  CHECK(std::strstr(bc.m.quote, "lying") != nullptr);
  for (int i = 0; i < BookMemory::NAME_SLOTS; ++i) {
    CHECK(std::strcmp(bc.m.names[i].text, "The") != 0);
    CHECK(std::strcmp(bc.m.names[i].text, "Inspector") != 0 || bc.m.names[i].count < 3);
  }

  PetState pet;
  pet.personality = Personality::Bookworm;
  char buf[96];
  std::printf("name=%s genre=%s word=%s quote=%s\n", name ? name : "-", BookChatter::genreName(bc.topGenre()),
              bc.m.word, bc.m.quote);
  for (uint32_t r = 0; r < 12; ++r) {
    const char* l = bc.line(pet, r * 7 + 1, buf, sizeof(buf));
    CHECK(l != nullptr);
    if (l) std::printf("  %s\n", l);
  }

  // A new book remembers the old title and forgets its people.
  for (uint32_t p = 1; p <= 25; ++p) feed(bc, "The Quiet House", "Chapter 4", 100 + p, "Barnaby walked.");
  feed(bc, "Stars Over Mars", "One", 1, "The ship left orbit and the captain watched the planet shrink.");
  CHECK(std::strcmp(bc.m.prevTitle, "The Quiet House") == 0);
  CHECK(bc.topName() == nullptr);

  std::printf(failures ? "%d FAILED\n" : "book chatter tests passed\n", failures);
  return failures ? 1 : 0;
}
