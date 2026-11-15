#pragma once
// Saves the to-do list to /.crosspoint/todo.json and syncs it with Todoist
// over the last-used Wi-Fi network.

#include <ArduinoJson.h>
#include <PersistableStore.h>

#include "TodoList.h"

class TodoStore : public PersistableStore<TodoStore> {
  friend class PersistableStore<TodoStore>;
  TodoStore() = default;
  ~TodoStore() = default;

 public:
  pp::TodoList list;

  static const char* getFilePath() { return "/.crosspoint/todo.json"; }
  void toJson(JsonDocument& doc) const;
  bool fromJson(JsonVariantConst doc);

  // Reads a token from /todoist.txt (or /.crosspoint/todoist.txt) when one
  // has not been set yet. Returns true when a token was loaded.
  bool importTokenFromSd();

  // Joins Wi-Fi if needed, sends ticked items to Todoist, then fetches the
  // list. Turns Wi-Fi back off if it was off. Saves on success.
  bool sync();
  // sync() when the schedule says so and a token is set.
  bool syncIfDue();
};

#define TODO_STORE TodoStore::getInstance()
