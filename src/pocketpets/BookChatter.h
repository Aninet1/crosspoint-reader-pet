#pragma once
// Book chatter: the pet notices what you read (names that keep coming up, the
// chapter, the kind of book, a quote, a new long word) and talks about it.
// Everything is counted on the device from the pages you turn; one pass over a
// page's words is all it costs. Plain C++ so it can be tested on a computer.

#include <cstddef>
#include <cstdint>

#include "PetModel.h"

namespace pp {

enum class Genre : uint8_t { Spooky, Romance, Mystery, Space, Magic, Adventure, History, Feelings, Count };

struct BookMemory {
  char title[48] = "";
  char author[32] = "";
  char chapter[48] = "";
  char prevTitle[48] = "";
  struct Name {
    char text[16];
    uint16_t count;
  };
  static constexpr int NAME_SLOTS = 16;
  Name names[NAME_SLOTS] = {};
  uint16_t genre[static_cast<int>(Genre::Count)] = {};
  char word[24] = "";   // a long word seen recently
  char quote[64] = "";  // a short line of dialogue
  uint32_t pages = 0;   // pages seen in this book
  uint32_t lastPageKey = 0;
};

class BookChatter {
 public:
  BookMemory m;

  // Starts a page. Returns false (and ignores the page's words) when the same
  // page is drawn again.
  bool beginPage(const char* title, const char* author, const char* chapter, uint32_t pageKey);
  // Each word of the page, in reading order. Not NUL-terminated.
  void addWord(const char* w, size_t len);
  void endPage();

  // A line about the book, or nullptr when nothing is known yet.
  const char* line(const PetState& pet, uint32_t rnd, char* buf, int len) const;

  // The name seen most (3+ times), or nullptr.
  const char* topName() const;
  // The strongest genre with enough evidence, or Genre::Count.
  Genre topGenre() const;
  static const char* genreName(Genre g);

 private:
  bool active_ = false;
  bool sentenceStart_ = true;
  bool inQuote_ = false;
  int quoteWords_ = 0;
  char quoteBuf_[64] = "";
  char pageWord_[24] = "";
  void addName(const char* w, size_t len);
};

}  // namespace pp
