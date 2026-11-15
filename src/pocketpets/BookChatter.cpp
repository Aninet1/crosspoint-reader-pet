#include "BookChatter.h"

#include <cstdio>
#include <cstring>

namespace pp {

namespace {

// Capitalised words that are not names.
const char* const NOT_NAMES[] = {
    "The",       "This",     "That",   "These",     "Those",   "There",    "Then",     "They",    "Their",
    "When",      "What",     "Where",  "Which",     "While",   "Who",      "Why",      "How",     "And",
    "But",       "Yet",      "For",    "Not",       "Now",     "Yes",      "Yeah",     "Well",    "Oh",
    "Ah",        "His",      "Her",    "Him",       "She",     "You",      "Your",     "Our",     "We",
    "It",        "Its",      "If",     "In",        "On",      "At",       "As",       "To",      "Of",
    "By",        "My",       "Mine",   "One",       "Two",     "Three",    "All",      "Any",     "Some",
    "Each",      "Every",    "Just",   "Still",     "Even",    "Only",     "Perhaps",  "Maybe",   "After",
    "Before",    "Once",     "Again",  "Here",      "Chapter", "Part",     "Book",     "Mr",      "Mrs",
    "Miss",      "Ms",       "Dr",     "Sir",       "Lady",    "Lord",     "God",      "Monday",  "Tuesday",
    "Wednesday", "Thursday", "Friday", "Saturday",  "Sunday",  "January",  "February", "March",   "April",
    "June",      "July",     "August", "September", "October", "November", "December", "English", "Contents",
    "Prologue",  "Epilogue", "Okay",   "Please",    "Thank",   "Thanks",   "Good",     "No",      "So",
    "Oh",        "Hello",
};

struct Cue {
  const char* word;
  Genre genre;
};
// Words that hint at the kind of book. Lowercase.
const Cue CUES[] = {
    {"ghost", Genre::Spooky},      {"blood", Genre::Spooky},       {"scream", Genre::Spooky},
    {"shadow", Genre::Spooky},     {"grave", Genre::Spooky},       {"corpse", Genre::Spooky},
    {"monster", Genre::Spooky},    {"haunted", Genre::Spooky},     {"vampire", Genre::Spooky},
    {"creature", Genre::Spooky},   {"terror", Genre::Spooky},      {"darkness", Genre::Spooky},
    {"kiss", Genre::Romance},      {"kissed", Genre::Romance},     {"heart", Genre::Romance},
    {"love", Genre::Romance},      {"loved", Genre::Romance},      {"lips", Genre::Romance},
    {"marry", Genre::Romance},     {"married", Genre::Romance},    {"wedding", Genre::Romance},
    {"blushed", Genre::Romance},   {"darling", Genre::Romance},    {"embrace", Genre::Romance},
    {"murder", Genre::Mystery},    {"detective", Genre::Mystery},  {"clue", Genre::Mystery},
    {"suspect", Genre::Mystery},   {"alibi", Genre::Mystery},      {"inspector", Genre::Mystery},
    {"evidence", Genre::Mystery},  {"killer", Genre::Mystery},     {"police", Genre::Mystery},
    {"poison", Genre::Mystery},    {"mystery", Genre::Mystery},    {"secret", Genre::Mystery},
    {"ship", Genre::Space},        {"planet", Genre::Space},       {"robot", Genre::Space},
    {"orbit", Genre::Space},       {"galaxy", Genre::Space},       {"captain", Genre::Space},
    {"alien", Genre::Space},       {"stars", Genre::Space},        {"engine", Genre::Space},
    {"laser", Genre::Space},       {"station", Genre::Space},      {"computer", Genre::Space},
    {"magic", Genre::Magic},       {"spell", Genre::Magic},        {"wizard", Genre::Magic},
    {"witch", Genre::Magic},       {"dragon", Genre::Magic},       {"sword", Genre::Magic},
    {"kingdom", Genre::Magic},     {"castle", Genre::Magic},       {"prince", Genre::Magic},
    {"princess", Genre::Magic},    {"enchanted", Genre::Magic},    {"elves", Genre::Magic},
    {"journey", Genre::Adventure}, {"island", Genre::Adventure},   {"treasure", Genre::Adventure},
    {"map", Genre::Adventure},     {"mountain", Genre::Adventure}, {"jungle", Genre::Adventure},
    {"river", Genre::Adventure},   {"horse", Genre::Adventure},    {"quest", Genre::Adventure},
    {"storm", Genre::Adventure},   {"escape", Genre::Adventure},   {"pirate", Genre::Adventure},
    {"war", Genre::History},       {"king", Genre::History},       {"queen", Genre::History},
    {"empire", Genre::History},    {"soldiers", Genre::History},   {"battle", Genre::History},
    {"century", Genre::History},   {"army", Genre::History},       {"revolution", Genre::History},
    {"village", Genre::History},   {"letters", Genre::History},    {"duke", Genre::History},
    {"tears", Genre::Feelings},    {"wept", Genre::Feelings},      {"cried", Genre::Feelings},
    {"lonely", Genre::Feelings},   {"grief", Genre::Feelings},     {"funeral", Genre::Feelings},
    {"mother", Genre::Feelings},   {"memory", Genre::Feelings},    {"sorry", Genre::Feelings},
    {"sad", Genre::Feelings},      {"hospital", Genre::Feelings},  {"goodbye", Genre::Feelings},
};

bool isUpper(char c) { return c >= 'A' && c <= 'Z'; }
bool isLower(char c) { return c >= 'a' && c <= 'z'; }
bool isAlpha(char c) { return isUpper(c) || isLower(c); }

// UTF-8 opening/closing quote marks: “ ” and plain ".
bool startsQuote(const char* w, size_t len) {
  return (len >= 3 && static_cast<uint8_t>(w[0]) == 0xE2 && static_cast<uint8_t>(w[1]) == 0x80 &&
          static_cast<uint8_t>(w[2]) == 0x9C) ||
         (len >= 1 && w[0] == '"');
}
bool endsQuote(const char* w, size_t len) {
  // Look past trailing punctuation that may follow the mark.
  while (len > 0 && (w[len - 1] == ',' || w[len - 1] == '.' || w[len - 1] == '!' || w[len - 1] == '?')) --len;
  return (len >= 3 && static_cast<uint8_t>(w[len - 3]) == 0xE2 && static_cast<uint8_t>(w[len - 2]) == 0x80 &&
          static_cast<uint8_t>(w[len - 1]) == 0x9D) ||
         (len >= 1 && w[len - 1] == '"');
}

void copyTrim(char* dst, size_t cap, const char* src, size_t maxShow) {
  // Copies src, shortening long text with "...".
  const size_t n = strlen(src);
  if (n <= maxShow && n < cap) {
    memcpy(dst, src, n + 1);
    return;
  }
  size_t k = maxShow < cap - 4 ? maxShow : cap - 4;
  while (k > 0 && src[k] != ' ') --k;  // cut at a word
  if (k == 0) k = maxShow < cap - 4 ? maxShow : cap - 4;
  memcpy(dst, src, k);
  memcpy(dst + k, "...", 4);
}

}  // namespace

const char* BookChatter::genreName(Genre g) {
  static const char* const N[] = {"spooky", "romantic", "mysterious", "space", "magic", "adventure", "history", "sad"};
  return g < Genre::Count ? N[static_cast<int>(g)] : "";
}

bool BookChatter::beginPage(const char* title, const char* author, const char* chapter, uint32_t pageKey) {
  active_ = false;
  if (!title || !*title) return false;
  if (strncmp(m.title, title, sizeof(m.title) - 1) != 0) {
    // A new book: remember the old one, forget its people.
    if (m.title[0] && m.pages >= 20) snprintf(m.prevTitle, sizeof(m.prevTitle), "%s", m.title);
    const BookMemory keep = m;
    m = BookMemory{};
    memcpy(m.prevTitle, keep.prevTitle, sizeof(m.prevTitle));
    snprintf(m.title, sizeof(m.title), "%s", title);
  } else if (pageKey == m.lastPageKey) {
    return false;
  }
  snprintf(m.author, sizeof(m.author), "%s", author ? author : "");
  snprintf(m.chapter, sizeof(m.chapter), "%s", chapter ? chapter : "");
  m.lastPageKey = pageKey;
  ++m.pages;
  active_ = true;
  sentenceStart_ = true;
  inQuote_ = false;
  quoteWords_ = 0;
  pageWord_[0] = 0;
  return true;
}

void BookChatter::addName(const char* w, size_t len) {
  if (len < 3 || len > 15) return;
  for (const char* nn : NOT_NAMES) {
    if (strlen(nn) == len && strncmp(nn, w, len) == 0) return;
  }
  int weakest = 0;
  for (int i = 0; i < BookMemory::NAME_SLOTS; ++i) {
    BookMemory::Name& n = m.names[i];
    if (n.count && strlen(n.text) == len && strncmp(n.text, w, len) == 0) {
      if (n.count < 60000) ++n.count;
      return;
    }
    if (n.count < m.names[weakest].count) weakest = i;
  }
  // Replace the weakest entry; old one-offs fade so new names can enter.
  BookMemory::Name& slot = m.names[weakest];
  if (slot.count > 1) {
    --slot.count;
    return;
  }
  memcpy(slot.text, w, len);
  slot.text[len] = 0;
  slot.count = 1;
}

void BookChatter::addWord(const char* w, size_t len) {
  if (!active_ || len == 0) return;

  // Short dialogue: collect the words between quote marks.
  const bool opens = startsQuote(w, len);
  if (opens && !inQuote_) {
    inQuote_ = true;
    quoteWords_ = 0;
    quoteBuf_[0] = 0;
  }
  if (inQuote_) {
    const size_t used = strlen(quoteBuf_);
    if (used + len + 2 < sizeof(quoteBuf_) && quoteWords_ < 12) {
      if (used) quoteBuf_[used] = ' ';
      memcpy(quoteBuf_ + used + (used ? 1 : 0), w, len);
      quoteBuf_[used + (used ? 1 : 0) + len] = 0;
      ++quoteWords_;
    } else {
      inQuote_ = false;  // too long to repeat
    }
    if (inQuote_ && endsQuote(w, len) && !(opens && quoteWords_ == 1 && len < 2)) {
      inQuote_ = false;
      if (quoteWords_ >= 3 && quoteWords_ <= 9) memcpy(m.quote, quoteBuf_, sizeof(m.quote));
    }
  }

  // Strip punctuation around the letters.
  size_t a = 0, b = len;
  while (a < b && !isAlpha(w[a])) ++a;
  while (b > a && !isAlpha(w[b - 1])) --b;
  const bool endsSentence = len > 0 && (w[len - 1] == '.' || w[len - 1] == '!' || w[len - 1] == '?' ||
                                        (len >= 2 && (w[len - 2] == '.' || w[len - 2] == '!' || w[len - 2] == '?')));
  if (b > a) {
    const char* core = w + a;
    size_t n = b - a;
    if (n > 2 && core[n - 2] == '\'' && core[n - 1] == 's') n -= 2;  // Holmes's -> Holmes
    bool plain = true;
    for (size_t i = 0; i < n; ++i) plain &= isAlpha(core[i]);
    if (plain) {
      if (isUpper(core[0]) && !sentenceStart_) {
        bool restLower = n > 1;
        for (size_t i = 1; i < n; ++i) restLower &= isLower(core[i]);
        if (restLower) addName(core, n);
      } else if (!isUpper(core[0])) {
        char lw[24];
        if (n < sizeof(lw)) {
          memcpy(lw, core, n);
          lw[n] = 0;
          for (const Cue& cue : CUES) {
            if (strcmp(cue.word, lw) == 0) {
              uint16_t& g = m.genre[static_cast<int>(cue.genre)];
              if (g < 60000) ++g;
              break;
            }
          }
          if (n >= 11 && n > strlen(pageWord_)) memcpy(pageWord_, lw, n + 1);
        }
      }
    }
  }
  sentenceStart_ = endsSentence || opens;
}

void BookChatter::endPage() {
  if (active_ && pageWord_[0]) memcpy(m.word, pageWord_, sizeof(m.word));
  active_ = false;
}

const char* BookChatter::topName() const {
  int best = -1;
  for (int i = 0; i < BookMemory::NAME_SLOTS; ++i) {
    if (m.names[i].count >= 3 && (best < 0 || m.names[i].count > m.names[best].count)) best = i;
  }
  return best >= 0 ? m.names[best].text : nullptr;
}

Genre BookChatter::topGenre() const {
  int best = -1;
  for (int i = 0; i < static_cast<int>(Genre::Count); ++i) {
    if (m.genre[i] >= 4 && (best < 0 || m.genre[i] > m.genre[best])) best = i;
  }
  return best >= 0 ? static_cast<Genre>(best) : Genre::Count;
}

const char* BookChatter::line(const PetState& pet, uint32_t rnd, char* buf, int len) const {
  if (!m.title[0] || m.pages < 2) return nullptr;
  char title[40], chapter[40], quote[64];
  copyTrim(title, sizeof(title), m.title, 26);
  copyTrim(chapter, sizeof(chapter), m.chapter, 26);
  copyTrim(quote, sizeof(quote), m.quote, 54);
  const char* name = topName();
  const Genre g = topGenre();

  static const char* const MEET[] = {"hide behind you",
                                     "blush and look at my paws",
                                     "ask for an alibi",
                                     "ask for a ride to space",
                                     "ask for one tiny spell",
                                     "pack snacks and follow",
                                     "ask what it was really like",
                                     "give them a big hug"};
  static const char* const MOOD[] = {
      "This book is giving me goosebumps. Do pets get goosebumps?",
      "So much yearning in this book. My heart!",
      "I have a suspect. I'm not telling you who.",
      "I want to live on a spaceship now.",
      "If I had a wand, I'd make snacks appear.",
      "Let's go on a quest after this chapter!",
      "Back then nobody even had e-readers.",
      "This one is making me a little misty.",
  };

  // Build the list of lines we can say, then pick one.
  enum Kind : uint8_t { Meet, Again, Trust, HowGoing, Pages, Chapter, Word, Quote, Mood, Author, Prev, Next };
  Kind can[12];
  int n = 0;
  if (name) {
    can[n++] = Meet;
    can[n++] = Again;
    can[n++] = Trust;
  }
  can[n++] = HowGoing;
  if (m.pages >= 25) can[n++] = Pages;
  if (m.chapter[0]) can[n++] = Chapter;
  if (m.word[0]) can[n++] = Word;
  if (m.quote[0]) can[n++] = Quote;
  if (g != Genre::Count) {
    can[n++] = Mood;
    can[n++] = Mood;
  }
  if (m.author[0]) can[n++] = Author;
  if (m.prevTitle[0]) can[n++] = Prev;
  if (pet.personality == Personality::Bookworm) can[n++] = Next;

  switch (can[rnd % n]) {
    case Meet:
      snprintf(buf, len, "If I met %s, I'd %s.", name, MEET[g == Genre::Count ? 7 : static_cast<int>(g)]);
      break;
    case Again:
      snprintf(buf, len, "%s again! Main character energy.", name);
      break;
    case Trust:
      snprintf(buf, len,
               g == Genre::Romance ? "%s and who, though? Tell me everything." : "I don't trust %s. Just a feeling.",
               name);
      break;
    case HowGoing:
      snprintf(buf, len, "How's %s going? I'm hooked.", title);
      break;
    case Pages:
      snprintf(buf, len, "%lu pages of %s together!", static_cast<unsigned long>(m.pages), title);
      break;
    case Chapter:
      snprintf(buf, len, "\"%s\"... ooh, what happens?", chapter);
      break;
    case Word:
      snprintf(buf, len, "New word: %s. I'll use it today.", m.word);
      break;
    case Quote:
      snprintf(buf, len, "Someone said %s I keep thinking about it.", quote);
      break;
    case Mood:
      snprintf(buf, len, "%s", MOOD[static_cast<int>(g)]);
      break;
    case Author:
      snprintf(buf, len, "Do you think %s has a pet too?", m.author);
      break;
    case Prev:
      snprintf(buf, len, "I still think about %s.", m.prevTitle);
      break;
    case Next:
      snprintf(buf, len, "One more page of %s? Pleeease?", title);
      break;
  }
  return buf;
}

}  // namespace pp
