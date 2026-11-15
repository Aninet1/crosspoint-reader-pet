#pragma once
// Saves the Pocket Pets state to /.crosspoint/pet.json and collects reading
// time from the reader.

#include <ArduinoJson.h>
#include <PersistableStore.h>

#include <mutex>

#include "BookChatter.h"
#include "PetModel.h"

class PetStore : public PersistableStore<PetStore> {
  friend class PersistableStore<PetStore>;
  PetStore() = default;
  ~PetStore() = default;

  uint32_t pendingMs = 0;   // reading time not yet credited
  uint32_t unsavedSec = 0;  // credited reading not yet written to SD
  bool dirty = false;

 public:
  pp::PetModel model;
  // Written by the reader's render task, read by the pet screens and saving.
  pp::BookChatter chatter;
  mutable std::mutex chatterMutex;
  char customStatus[24] = "BACK SOON";
  int32_t birthday = 0;  // YYYYMMDD for tarot birth cards, 0 = not set

  static const char* getFilePath() { return "/.crosspoint/pet.json"; }
  void toJson(JsonDocument& doc) const;
  bool fromJson(JsonVariantConst doc);

  // Local wall clock for the model; invalid when the time was never set.
  static pp::Clock now();

  // Reader time, called from the main loop while a book is open and in use.
  // Writes to SD at most every five minutes of reading.
  void addReadingMs(uint32_t ms);
  void markDirty() { dirty = true; }
  // Pet speech: its own lines, mixed with book chatter and your quotes when
  // those are switched on.
  const char* speak(const pp::Clock& clk, char* buf, int len);

  static constexpr const char* QUOTES_FILE = "/pocketpets/quotes.txt";
  // A random line from the quotes file (blank lines and # comments skipped).
  bool randomQuote(uint32_t rnd, char* buf, int len) const;
  int quoteCount() const;
  bool addQuote(const char* text);
  // Writes pending changes (sleep entry, leaving the pet app).
  void flush();
};

#define PET_STORE PetStore::getInstance()
