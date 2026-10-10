## 1.3.4
- The version is written in full where it shows: the title screen reads "Numblocks
  custom 1.3.4" and the F3 overlay "Numblock custom 1.3.4 (minecraft.1.8.8)",
  where both used to stop at 1.3. One version in the source now feeds both, so
  they cannot drift from versions.md again.
## 1.3.3
- Options, Controls is a menu of its own now, not the key bindings opened at once:
  the key sheet wide across the top, with Sprint and Keybinds side by side under
  it. The key sheet also opens from the key bindings' first row as before, and Done
  on either screen goes one step back out.
- Sprint can be toggled instead of held. Options, Controls, Sprint switches between
  Hold (the i key held while walking, as it was) and Toggle, where a tap switches
  sprinting on, forward may be let go, and another tap switches it off - the way
  1.15's sprint toggle works. Sneaking, walking into a wall, eating or getting
  hungry still end it either way.
## 1.3.2
- Sprint is on the i key, next to forward, where it can be held while walking with the
  other hand. It was on backspace, which is also the key that deletes while you type a
  world's name or a seed, so it got in the way of typing and was a key nobody would
  look for. Keys saved on backspace move to i; keys set by hand stay as they were set,
  and the control sheet shows again now that the keys changed, with i marked Sprint.
  Forward twice still sprints too.
## 1.3.1
- More of everything alive: the world's entity slots went from 24 to 56 (mobs, dropped
  items, arrows and TNT share them), the particle cap from 16 to 64, and the mobs
  the world may have from 8 to 16 - eight of a kind at most for monsters and eight
  for animals naturally, six of a kind about a spawner, thirty-two mobs in all
  where the spawners keep to what the slots allow. A mob costs 0.9 us a tick to
  keep up, so none of it shows in the frame rate.
- Monster spawners are empty until set: a spawner fresh from the inventory, or one of
  the dungeons', shows no egg and makes no mobs until a spawn egg is used on it, which
  sets it to that mob and starts it making them.
- The egg inside a spawner is the spawn egg item's own picture, flat and turning about
  the cage's centre (a turn in five seconds), no longer a small cube with the sprite
  on every face; it costs less to draw than the cube did.
- A spawner's mobs appear in the air within three blocks of it and fall down unhurt
  (mobs take no fall damage here), in any light: torches do not switch a spawner off.
- A spawn egg used on one sets it quietly: no chat line any more, and a set spawner
  keeps its mob across a save and load (a world record with a sheep or chicken
  spawner used to read back as unset, leaving it empty).
- When a spawner makes a mob, it puffs: smoke about the mob where it appears, and flames
  rising out of the top of the cage for a moment.
## 1.3.0
- Added a render distance setting (1–2 chunks, default 2, saved in the config)
  in Video Settings, as a slider.
- The whole world in sight stays the 40 x 40 x 24 block view cache, fully in
  memory as before — nothing is cached or simplified past it. A chunk here is 8
  blocks, the cache's slide step: 1 -> 11, 2 -> 19 blocks of rays. The fog closes
  where the ground does (the cache's edge, as the camera keeps 16 of its blocks
  to each side), so the ground fades into the sky without a seam at either
  setting, and the rays never walk past the world into nothing.
