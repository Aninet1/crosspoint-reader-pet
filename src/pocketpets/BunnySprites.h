#pragma once
// Rabbit sprites from reTerminal Sticky Bunny (MIT License, Copyright (c) 2026
// limengdu; see third_party/STICKY_BUNNY_LICENSE), packed as run-length rows
// by tools/pocketpets_assets/gen_bunny.py.

#include <cstdint>

namespace pp {

struct Sprite {
  uint16_t w, h;
  const uint16_t* rows;  // byte offset of each row in data
  const uint8_t* data;   // runs: top 2 bits colour (0 clear, 1 white, 2 black), low 6 bits length - 1
};

enum class BunnySprite : uint8_t {
  EggIntact,
  EggCrackOne,
  EggCrackTwo,
  BabyIdle,
  BabyFeed,
  BabyPet,
  BabyPlay,
  BabyHungry,
  BabyTired,
  ChildIdle,
  ChildFeed,
  ChildPet,
  ChildPlay,
  ChildHungry,
  ChildTired,
  YouthFoodieIdle,
  YouthFoodieFeed,
  YouthFoodiePet,
  YouthFoodiePlay,
  YouthAffectionateIdle,
  YouthAffectionateFeed,
  YouthAffectionatePet,
  YouthAffectionatePlay,
  YouthActiveIdle,
  YouthActiveFeed,
  YouthActivePet,
  YouthActivePlay,
  AdultFoodieIdle,
  AdultFoodieFeed,
  AdultFoodiePet,
  AdultFoodiePlay,
  AdultAffectionateIdle,
  AdultAffectionateFeed,
  AdultAffectionatePet,
  AdultAffectionatePlay,
  AdultActiveIdle,
  AdultActiveFeed,
  AdultActivePet,
  AdultActivePlay,
  Count
};

extern const Sprite BUNNY_SPRITES[static_cast<int>(BunnySprite::Count)];

}  // namespace pp
