#include "TodoList.h"

#include <ArduinoJson.h>

#include <cstdio>
#include <cstring>

namespace pp {

int TodoList::remaining() const {
  int n = 0;
  for (int i = 0; i < count; ++i) n += items[i].done ? 0 : 1;
  return n;
}

bool TodoList::syncDue(const Clock& now) const {
  if (!hasToken()) return false;
  if (lastSync == 0) return true;
  if (!now.valid) return false;
  switch (schedule) {
    case SyncSchedule::Every1h:
      return now.epoch - lastSync >= 3600;
    case SyncSchedule::Every3h:
      return now.epoch - lastSync >= 3 * 3600;
    case SyncSchedule::Every6h:
      return now.epoch - lastSync >= 6 * 3600;
    case SyncSchedule::Every12h:
      return now.epoch - lastSync >= 12 * 3600;
    default: {
      // Daily: due once the chosen hour has passed today and we haven't synced since.
      const int64_t midnight = now.epoch - (now.hour * 3600 + now.minute * 60);
      int64_t slot = midnight + dailyHour * 3600LL;
      if (now.epoch < slot) slot -= 86400;  // before today's time: yesterday's slot counts
      return lastSync < slot;
    }
  }
}

const char* TodoList::scheduleName(SyncSchedule s, uint8_t hour, char* buf, int len) {
  switch (s) {
    case SyncSchedule::Every1h:
      snprintf(buf, len, "Every hour");
      break;
    case SyncSchedule::Every3h:
      snprintf(buf, len, "Every 3 hours");
      break;
    case SyncSchedule::Every6h:
      snprintf(buf, len, "Every 6 hours");
      break;
    case SyncSchedule::Every12h:
      snprintf(buf, len, "Every 12 hours");
      break;
    default: {
      const int h12 = hour % 12 == 0 ? 12 : hour % 12;
      snprintf(buf, len, "Daily at %d%s", h12, hour < 12 ? "am" : "pm");
    }
  }
  return buf;
}

bool TodoList::toggle(int index) {
  if (index < 0 || index >= count) return false;
  TodoItem& t = items[index];
  if (t.closeSent) return false;  // already completed in Todoist
  t.done = !t.done;
  return t.done;
}

bool TodoList::applyResponse(const char* json, size_t len) {
  // Keep only the fields we show; descriptions and comments can be large.
  JsonDocument filter;
  JsonObject task = filter["results"].add<JsonObject>();
  task["id"] = true;
  task["content"] = true;
  task["priority"] = true;
  task["due"]["date"] = true;
  JsonDocument doc;
  if (deserializeJson(doc, json, len, DeserializationOption::Filter(filter))) return false;
  JsonArrayConst results = doc["results"].as<JsonArrayConst>();
  if (results.isNull()) return false;

  // Ticked items that Todoist hasn't heard about yet stay on top (moved in
  // place: a copy of the list would put 4 KB on the stack).
  int kept = 0;
  for (int i = 0; i < count; ++i) {
    if (items[i].done && !items[i].closeSent) {
      if (i != kept) items[kept] = items[i];
      ++kept;
    }
  }
  count = kept;

  for (JsonObjectConst t : results) {
    if (count >= MAX_ITEMS) break;
    const char* id = t["id"] | "";
    if (!*id) continue;
    bool dup = false;
    for (int i = 0; i < kept; ++i) dup |= strcmp(items[i].id, id) == 0;
    if (dup) continue;
    TodoItem& it = items[count++];
    it = TodoItem{};
    snprintf(it.id, sizeof(it.id), "%s", id);
    snprintf(it.text, sizeof(it.text), "%s", t["content"] | "");
    const char* due = t["due"]["date"] | "";
    snprintf(it.due, sizeof(it.due), "%.10s", due);  // drop a time part if present
    it.priority = static_cast<uint8_t>(t["priority"] | 1);
  }
  return true;
}

}  // namespace pp
