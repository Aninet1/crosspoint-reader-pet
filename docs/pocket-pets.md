# Pocket Pets (personal fork)

A virtual pet for CrossPoint on the reTerminal Sticky, fed by the time you
spend reading. Open it from **Home → Pocket Pets** (list home layouts; the
cover-grid layout keeps its fixed tabs).

## The pet

- **Five species:** dog, cat, cloud, plant and the Sticky Bunny rabbit. Pick an
  egg (or a seed pot) and tap it three times to hatch it. Every species follows
  the same rules (stages, personality, needs, sleep, outings, streaks); only the
  artwork differs.
- **The rabbit** uses the original artwork from
  [reTerminal Sticky Bunny](https://github.com/limengdu/reTerminal_Sticky_Bunny)
  (MIT License, Copyright (c) 2026 limengdu; notice in
  `src/pocketpets/third_party/STICKY_BUNNY_LICENSE`): egg and cracks, hatchling,
  child, and the Foodie, Affectionate and Active youth and adult forms, each with
  idle, feed, pet and play poses. A Bookworm or Cuddly rabbit takes the
  Affectionate form. The sprites are packed by `tools/pocketpets_assets/gen_bunny.py`
  into about 100 KB of flash.
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

## Pet talk

Settings → **Pet talk** cycles through: *Pet only*, *+ my quotes*, *+ book chatter*,
and *All three*. The pet's own lines are always there.

- **My quotes:** one quote per line in `pocketpets/quotes.txt` on the SD card
  (lines starting with `#` are skipped). Add them from a computer, the File
  Transfer page, or Settings → **Add a quote**.
- **Book chatter:** while you read an EPUB, the pet notices names that keep
  coming up, the chapter title, the kind of book (spooky, romantic, mystery,
  space, magic, adventure, history, sad), short lines of dialogue and long new
  words, then talks about them. It is worked out on the device from each new
  page (one pass over its words); nothing leaves the device. Inspired by
  Silverfish for KOReader, written from scratch.

## Apps

Open **Apps** on the pet screen.

- **To-do on the home screen:** the list layout of CrossPoint's home shows a
  to-do card in the space under the menu: open tasks first, with Today and
  Overdue tags. Tap a box to tick it (sent to Todoist at the next sync), or tap
  the title to open the full list.
- **To-do:** your Todoist tasks (default filter `today | overdue`). Set the
  token under To-do → settings, or save it as `todoist.txt` at the root of the
  SD card. Sync daily at a chosen hour or every 1, 3, 6 or 12 hours: the list
  syncs when you open it and when the Sticky goes to sleep after the sync time
  (battery 20% or more), over the last Wi-Fi network you used. Tick tasks on the
  device; they are closed in Todoist at the next sync. Ticked tasks count as care.

- **Focus timer:** 15, 25 or 60 minutes, or a custom time with a keypad. Pause,
  end with confirmation, and a repeating buzzer chime at the end. Finished
  sessions count as care.
- **Status board** (landscape): AT WORK, BUSY, IN A MEETING, ON A CALL,
  FOCUSING, DO NOT DISTURB, ON A BREAK, OUT FOR LUNCH, SLEEPING, BACK SOON,
  OPEN TO TALK or a custom message (hold the tile to edit it). Each has its own
  pet scene and a "since 9:41am" time. The status stays on the screen while the
  Sticky sleeps.
- **Book of Answers:** hold a question in mind and shake for three seconds.
  MESSAGE picks one of 135 answers; YES / NO uses the crystal ball.
- **Tarot** (in the Book of Answers): the 22 Major Arcana, read like a real
  deck. Shaking shuffles the whole deck (the chip's hardware random source)
  and each card lands upright or reversed by chance, so about half come up
  reversed. Spreads: *One card*, *Past-Now-Next* (three different cards read
  by position), or *Today* (a card of the day that stays the same until
  midnight). Type an optional question before you shuffle. Add your birthday
  for your birth cards (month + day + year, digits added until 21 or less,
  22 is the Fool); the reading says when one comes up. Each card has upright
  and reversed keywords and a short reading.
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
| `src/pocketpets/PetStore.*` | Saves to `/.crosspoint/pet.json`; reading time, pet talk and quotes |
| `src/pocketpets/BookChatter.*` | Book chatter: what the pet notices in your book |
| `src/pocketpets/Tarot.cpp` | Tarot cards and readings |
| `src/pocketpets/BunnySprites.*` | Rabbit sprites (generated; MIT, reTerminal Sticky Bunny) |
| `src/pocketpets/TodoList.*`, `TodoSync.*` | To-do list, schedule, Todoist sync (`/.crosspoint/todo.json`) |
| `src/activities/pocketpets/` | The activity: input, timers, IMU, buzzer |
| `test/pocketpets/PetModelTest.cpp` | Host tests for the model |
| `tools/pocketpets_preview/` | Renders every screen to images on a computer |

Build and flash: `pio run -e sticky -t upload`.

Host tests (from the repository root):

```
g++ -std=c++17 -I src test/pocketpets/PetModelTest.cpp src/pocketpets/PetModel.cpp && ./a.out
g++ -std=c++17 -I src test/pocketpets/BookChatterTest.cpp src/pocketpets/BookChatter.cpp src/pocketpets/PetModel.cpp && ./a.out
g++ -std=c++17 -I src -I .pio/libdeps/sticky/ArduinoJson/src test/pocketpets/TodoListTest.cpp src/pocketpets/TodoList.cpp && ./a.out
g++ -std=c++17 -I src -I .pio/libdeps/sticky/ArduinoJson/src test/pocketpets/TarotTest.cpp $(ls src/pocketpets/*.cpp | grep -v -e PetStore -e TodoSync) && ./a.out
```

Previews: compile `tools/pocketpets_preview/screens.cpp` with `src/pocketpets/*.cpp`
and `-I freeink-sdk/libs/font/FreeInkFont/third_party/stb`, then run it from the
repository root.

## Check on hardware

- Which landscape orientation reads correctly for the Status board
  (`GfxRenderer::LandscapeCounterClockwise` is used).
- Shake and rotation thresholds on the Sticky's IMU.
- Buzzer volume and tones.
- A Todoist sync on your network (token, filter, closing a task).
