# Pocket Pets (personal fork)

A virtual pet for CrossPoint on the reTerminal Sticky, fed by the time you
spend reading. Open it from **Home → Pocket Pets** (list home layouts; the
cover-grid layout keeps its fixed tabs).

## The pet

- **Four species:** dog, cat, cloud and plant. Pick an egg (or a seed pot) and
  tap it three times to hatch it.
- **Five stages:** Egg → Baby → Child → Teen → Adult. Growth comes from daily
  care, at most 10 points a day; an hour of reading a day reaches adulthood in
  about two weeks.
- **Personality:** at Teen the pet becomes a Foodie, Cuddly, Active or Bookworm
  pet, depending on whether you mostly fed it, patted it, played with it or read.
  Each has its own look and dialogue.
- **Values:** Love (pats and play), Fullness (food), Energy (naps and night
  sleep, 10pm to 7am) and Growth.
- **Reading is food:** every minute with a book open and in use earns a minute
  of snack time (time stops counting after five idle minutes). Six foods cost
  15 minutes to 6 hours.
- **Daily life:** care streaks with celebrations at 3, 7, 30 and 100 days;
  outings on some days (one to seven hours) that bring back keepsakes; a call
  back button; dialogue that follows its needs, the time and its personality.

## Apps

Open **Apps** on the pet screen.

- **Focus timer:** 15, 25 or 60 minutes, or a custom time with a keypad. Pause,
  end with confirmation, and a repeating buzzer chime at the end. Finished
  sessions count as care.
- **Status board** (landscape): BUSY, MEETING, ON CALL, OPEN TO TALK, REST or a
  custom message (hold the tile to edit it).
- **Book of Answers:** hold a question in mind and shake for three seconds.
  MESSAGE mode picks one of 135 answers; YES OR NO uses the crystal ball.
- **Gestures in the Apps sheet:** shake for Answers, turn the device sideways for
  the Status board.

## Controls

Tap to choose. Up and Down move a focus ring, OK activates, swipe from the edge
to go back.

## Code

| Path | What it holds |
| --- | --- |
| `src/pocketpets/PetModel.*` | Pet state and rules (plain C++) |
| `src/pocketpets/PetCanvas.*`, `PetArt.*` | Span-based vector drawing and the artwork |
| `src/pocketpets/Screens.*`, `Answers.cpp` | Layout, drawing and hit-testing for every screen |
| `src/pocketpets/PetStore.*` | Saves to `/.crosspoint/pet.json`; collects reading time |
| `src/activities/pocketpets/` | The activity: input, timers, IMU, buzzer |
| `test/pocketpets/PetModelTest.cpp` | Host tests for the model |
| `tools/pocketpets_preview/` | Renders every screen to images on a computer |

Build and flash: `pio run -e sticky -t upload`.

Host tests: `g++ -std=c++17 -I src test/pocketpets/PetModelTest.cpp src/pocketpets/PetModel.cpp && ./a.out`

Previews: compile `tools/pocketpets_preview/screens.cpp` with `src/pocketpets/*.cpp`
and `-I freeink-sdk/libs/font/FreeInkFont/third_party/stb`, then run it from the
repository root.

## Check on hardware

- Which landscape orientation reads correctly for the Status board
  (`GfxRenderer::LandscapeCounterClockwise` is used).
- Shake and rotation thresholds on the Sticky's IMU.
- Buzzer volume and tones.
