#pragma once
// Pocket Pets: a virtual pet fed by reading time, with a focus timer, a status
// board and a Book of Answers. Drawing and layout live in src/pocketpets/.

#include <Buzzer.h>

#include "activities/Activity.h"
#include "pocketpets/Screens.h"

class PocketPetsActivity final : public Activity {
 public:
  PocketPetsActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("PocketPets", renderer, mappedInput) {}

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;
  bool preventAutoSleep() override;
  bool skipLoopDelay() override { return motionActive(); }
  // The status board stays on the e-paper through deep sleep.
  bool keepsFrameOnSleep() const override { return ui.screen == pp::Screen::StatusFull; }

 private:
  pp::UiState ui;
  pp::Button buttons[pp::MAX_BUTTONS];
  int buttonCount = 0;

  GfxRenderer::Orientation savedOrientation = GfxRenderer::Portrait;
  Buzzer buzzer;
  bool buzzerReady = false;

  uint32_t lastTickMs = 0;
  uint32_t lastHomeRefreshMs = 0;
  uint32_t poseResetAtMs = 0;
  uint32_t todoSyncAtMs = 0;  // a to-do sync waiting for the "Syncing" screen to show
  int fastRefreshes = 0;
  bool fullRefreshNext = true;

  // Focus timer
  uint32_t pomoStartMs = 0;
  uint32_t pomoStartLeft = 0;
  uint32_t lastPomoDrawMs = 0;
  uint32_t alarmNextMs = 0;
  uint32_t alarmUntilMs = 0;
  pp::PomoPhase pomoBeforeConfirm = pp::PomoPhase::Running;

  // Motion (shake and rotation)
  uint32_t lastMotionMs = 0;
  uint32_t shakeMs = 0;
  uint32_t lastShakeMs = 0;
  uint32_t holdStartMs = 0;
  uint32_t thinkNextMs = 0;
  int8_t baseAxis = -1;
  int8_t rotateStableCount = 0;
  int lastShownShakePct = 0;

  void go(pp::Screen screen);
  void relayout();
  void act(const pp::Button& b);
  void back();
  void say(const char* text);
  void sayLine();
  void afterCare(pp::Event e);
  void checkBanners();
  void chirp(int kind);
  void startPomodoro(uint32_t seconds);
  void updatePomodoro(uint32_t now);
  void updateMotion(uint32_t now);
  void finishShake();
  bool motionActive() const;
  void editText(bool petName);
  void editTodoText(bool token);
  void requestTodoSync();
  void adoptKeepingCredit(pp::Species sp);
};
