#pragma once
// To-do list kept in sync with Todoist. The data, the sync schedule and the
// response parsing live here (no networking) so they can be tested on a
// computer; TodoSync does the Wi-Fi and HTTPS part on the device.

#include <cstddef>
#include <cstdint>

#include "PetModel.h"

namespace pp {

struct TodoItem {
  char id[24] = "";
  char text[96] = "";
  char due[12] = "";       // YYYY-MM-DD, empty when undated
  uint8_t priority = 1;    // Todoist: 4 is the most urgent
  bool done = false;       // ticked on the device
  bool closeSent = false;  // Todoist knows it is done
};

enum class SyncSchedule : uint8_t { Daily, Every1h, Every3h, Every6h, Every12h, Count };

struct TodoList {
  static constexpr int MAX_ITEMS = 30;
  TodoItem items[MAX_ITEMS];
  int count = 0;

  char token[64] = "";
  char filter[64] = "today | overdue";
  SyncSchedule schedule = SyncSchedule::Daily;
  uint8_t dailyHour = 7;
  int64_t lastSync = 0;  // epoch of the last good sync, 0 = never
  char lastError[48] = "";

  bool hasToken() const { return token[0] != 0; }
  int remaining() const;
  // True when a sync is due at `now` under the chosen schedule.
  bool syncDue(const Clock& now) const;
  static const char* scheduleName(SyncSchedule s, uint8_t dailyHour, char* buf, int len);

  // Ticks or unticks an item. Returns true when it became done.
  bool toggle(int index);

  // Replaces the list with Todoist's /tasks/filter response. Items ticked on
  // the device whose close was not sent yet are kept. Returns false on bad JSON.
  bool applyResponse(const char* json, size_t len);
};

}  // namespace pp
