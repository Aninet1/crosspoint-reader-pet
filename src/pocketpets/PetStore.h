#pragma once
// Saves the Pocket Pets state to /.crosspoint/pet.json and collects reading
// time from the reader.

#include <ArduinoJson.h>
#include <PersistableStore.h>

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
  char customStatus[24] = "BACK SOON";

  static const char* getFilePath() { return "/.crosspoint/pet.json"; }
  void toJson(JsonDocument& doc) const;
  bool fromJson(JsonVariantConst doc);

  // Local wall clock for the model; invalid when the time was never set.
  static pp::Clock now();

  // Reader time, called from the main loop while a book is open and in use.
  // Writes to SD at most every five minutes of reading.
  void addReadingMs(uint32_t ms);
  void markDirty() { dirty = true; }
  // Writes pending changes (sleep entry, leaving the pet app).
  void flush();
};

#define PET_STORE PetStore::getInstance()
