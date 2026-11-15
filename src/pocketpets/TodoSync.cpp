#include "TodoSync.h"

#include <HalStorage.h>
#include <Logging.h>
#include <TrustedTime.h>
#include <WiFi.h>

#include <cstring>
#include <string>

#include "PetStore.h"
#include "WifiCredentialStore.h"
#include "util/PluginHttp.h"

namespace {

constexpr const char* API = "https://api.todoist.com/api/v1";
constexpr size_t MAX_RESPONSE = 48 * 1024;
constexpr unsigned long JOIN_TIMEOUT_MS = 10000;

void setError(pp::TodoList& l, const char* msg) { snprintf(l.lastError, sizeof(l.lastError), "%s", msg); }

// Joins the last-used network. Returns true when connected; `joined` says we
// turned Wi-Fi on ourselves.
bool ensureWifi(bool& joined) {
  joined = false;
  if (WiFi.status() == WL_CONNECTED) return true;
  WIFI_STORE.loadFromFile();
  const auto cred = WIFI_STORE.findCredential(WIFI_STORE.getLastConnectedSsid());
  if (!cred) return false;
  WiFi.mode(WIFI_STA);
  WiFi.begin(cred->ssid.c_str(), cred->password.c_str());
  joined = true;
  const unsigned long deadline = millis() + JOIN_TIMEOUT_MS;
  while (WiFi.status() != WL_CONNECTED && millis() < deadline) delay(100);
  return WiFi.status() == WL_CONNECTED;
}

void dropWifi() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

}  // namespace

bool TodoStore::importTokenFromSd() {
  if (list.hasToken()) return false;
  for (const char* path : {"/todoist.txt", "/.crosspoint/todoist.txt"}) {
    if (!Storage.exists(path)) continue;
    String text = Storage.readFile(path);
    text.trim();
    if (text.length() < 20 || text.length() >= sizeof(list.token)) continue;
    snprintf(list.token, sizeof(list.token), "%s", text.c_str());
    list.lastSync = 0;
    saveToFile();
    LOG_INF("TODO", "Todoist token loaded from %s", path);
    return true;
  }
  return false;
}

bool TodoStore::sync() {
  if (!list.hasToken()) {
    setError(list, "Add your Todoist token first");
    return false;
  }
  bool joined = false;
  if (!ensureWifi(joined)) {
    setError(list, "Couldn't connect to Wi-Fi");
    if (joined) dropWifi();
    saveToFile();
    return false;
  }
  trustedtime::startSync();

  const pluginhttp::Headers headers = {{"Authorization", std::string("Bearer ") + list.token},
                                       {"Accept", "application/json"}};
  String body;
  bool ok = true;

  // Send items ticked on the device first.
  for (int i = 0; i < list.count; ++i) {
    pp::TodoItem& t = list.items[i];
    if (!t.done || t.closeSent) continue;
    const int status =
        pluginhttp::request(nullptr, std::string(API) + "/tasks/" + t.id + "/close", "POST", "", headers, body, 4096);
    if (status >= 200 && status < 300) {
      t.closeSent = true;
    } else if (status == 404) {
      t.closeSent = true;  // gone in Todoist already
    } else {
      ok = false;
      LOG_ERR("TODO", "close %s failed: %d", t.id, status);
    }
  }

  const std::string url = std::string(API) + "/tasks/filter?limit=30&query=" + pluginhttp::urlEncodeQuery(list.filter);
  const int status = pluginhttp::request(nullptr, url, "GET", "", headers, body, MAX_RESPONSE);
  if (status == 200 && list.applyResponse(body.c_str(), body.length())) {
    const pp::Clock clk = PetStore::now();
    list.lastSync = clk.valid ? clk.epoch : 1;
    list.lastError[0] = 0;
  } else {
    ok = false;
    if (status == 401 || status == 403)
      setError(list, "Todoist didn't accept the token");
    else if (status == 400)
      setError(list, "Todoist didn't understand the filter");
    else
      setError(list, "Todoist sync failed. Try again later");
    LOG_ERR("TODO", "fetch failed: %d", status);
  }
  body = String();
  if (joined) dropWifi();
  saveToFile();
  return ok;
}

bool TodoStore::syncIfDue() {
  if (!list.syncDue(PetStore::now())) return false;
  return sync();
}

void TodoStore::toJson(JsonDocument& doc) const {
  doc["token"] = list.token;
  doc["filter"] = list.filter;
  doc["schedule"] = static_cast<uint8_t>(list.schedule);
  doc["hour"] = list.dailyHour;
  doc["lastSync"] = list.lastSync;
  doc["error"] = list.lastError;
  JsonArray items = doc["items"].to<JsonArray>();
  for (int i = 0; i < list.count; ++i) {
    const pp::TodoItem& t = list.items[i];
    JsonObject o = items.add<JsonObject>();
    o["id"] = t.id;
    o["text"] = t.text;
    o["due"] = t.due;
    o["p"] = t.priority;
    o["done"] = t.done;
    o["sent"] = t.closeSent;
  }
}

bool TodoStore::fromJson(JsonVariantConst doc) {
  pp::TodoList l;
  strncpy(l.token, doc["token"] | "", sizeof(l.token) - 1);
  strncpy(l.filter, doc["filter"] | "today | overdue", sizeof(l.filter) - 1);
  const uint8_t sched = doc["schedule"] | 0;
  l.schedule = sched < static_cast<uint8_t>(pp::SyncSchedule::Count) ? static_cast<pp::SyncSchedule>(sched)
                                                                     : pp::SyncSchedule::Daily;
  l.dailyHour = doc["hour"] | 7;
  if (l.dailyHour > 23) l.dailyHour = 7;
  l.lastSync = doc["lastSync"] | static_cast<int64_t>(0);
  strncpy(l.lastError, doc["error"] | "", sizeof(l.lastError) - 1);
  for (JsonVariantConst o : doc["items"].as<JsonArrayConst>()) {
    if (l.count >= pp::TodoList::MAX_ITEMS) break;
    pp::TodoItem& t = l.items[l.count++];
    strncpy(t.id, o["id"] | "", sizeof(t.id) - 1);
    strncpy(t.text, o["text"] | "", sizeof(t.text) - 1);
    strncpy(t.due, o["due"] | "", sizeof(t.due) - 1);
    t.priority = o["p"] | 1;
    t.done = o["done"] | false;
    t.closeSent = o["sent"] | false;
  }
  list = l;
  return true;
}
