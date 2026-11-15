// Host test for the Pocket Pets to-do list:
// g++ -std=c++17 -I src -I <ArduinoJson>/src test/pocketpets/TodoListTest.cpp src/pocketpets/TodoList.cpp
#include <cstdio>
#include <cstring>

#include "pocketpets/TodoList.h"
using namespace pp;

static int failures = 0;
#define CHECK(c)                                              \
  do {                                                        \
    if (!(c)) {                                               \
      std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); \
      ++failures;                                             \
    }                                                         \
  } while (0)

static Clock at(int day, int hour, int minute = 0) {
  Clock c;
  c.valid = true;
  c.day = day;
  c.hour = static_cast<uint8_t>(hour);
  c.minute = static_cast<uint8_t>(minute);
  c.epoch = static_cast<int64_t>(day) * 86400 + hour * 3600 + minute * 60;
  return c;
}

int main() {
  TodoList l;
  CHECK(!l.syncDue(at(100, 9)));  // no token, never due
  snprintf(l.token, sizeof l.token, "abc");
  CHECK(l.syncDue(at(100, 9)));  // never synced

  // Daily at 7am.
  l.lastSync = at(100, 7, 5).epoch;
  CHECK(!l.syncDue(at(100, 22)));
  CHECK(!l.syncDue(at(101, 6, 59)));
  CHECK(l.syncDue(at(101, 7, 0)));
  l.lastSync = at(99, 20).epoch;  // synced last evening
  CHECK(!l.syncDue(at(100, 6)));
  CHECK(l.syncDue(at(100, 8)));

  // Every 3 hours.
  l.schedule = SyncSchedule::Every3h;
  l.lastSync = at(100, 9).epoch;
  CHECK(!l.syncDue(at(100, 11, 59)));
  CHECK(l.syncDue(at(100, 12)));

  // Parsing a Todoist /tasks/filter response, extra fields ignored.
  const char* json = R"({"results":[
    {"id":"6X7rM8997g3RQmvh","content":"Submit assignment","priority":4,"description":"long text here",
     "due":{"date":"2026-11-16","string":"today","is_recurring":false},"labels":["uni"]},
    {"id":"6X7rfFVPjhvv84XG","content":"Buy oat milk","priority":1,"due":null},
    {"id":"6X7rfEVP8hvv25ZQ","content":"Dentist","priority":2,"due":{"date":"2026-11-18T10:00:00"}}
  ],"next_cursor":null})";
  CHECK(l.applyResponse(json, strlen(json)));
  CHECK(l.count == 3);
  CHECK(strcmp(l.items[0].text, "Submit assignment") == 0);
  CHECK(l.items[0].priority == 4);
  CHECK(strcmp(l.items[0].due, "2026-11-16") == 0);
  CHECK(l.items[1].due[0] == 0);
  CHECK(strcmp(l.items[2].due, "2026-11-18") == 0);

  // Ticked on the device: kept through a sync until Todoist has been told.
  CHECK(l.toggle(1));
  CHECK(l.remaining() == 2);
  const char* json2 = R"({"results":[{"id":"6X7rM8997g3RQmvh","content":"Submit assignment","priority":4}]})";
  CHECK(l.applyResponse(json2, strlen(json2)));
  CHECK(l.count == 2);
  CHECK(strcmp(l.items[0].text, "Buy oat milk") == 0 && l.items[0].done);
  l.items[0].closeSent = true;
  CHECK(l.applyResponse(json2, strlen(json2)));
  CHECK(l.count == 1);
  CHECK(!l.toggle(5));
  CHECK(!l.applyResponse("not json", 8));

  char buf[32];
  CHECK(strcmp(TodoList::scheduleName(SyncSchedule::Daily, 19, buf, sizeof buf), "Daily at 7pm") == 0);

  std::printf(failures ? "%d FAILED\n" : "to-do tests passed\n", failures);
  return failures ? 1 : 0;
}
