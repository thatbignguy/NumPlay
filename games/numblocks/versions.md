## 1.4.1
**The world generator's summary cache left the RAM it used to sit in for the stack, so NumBlocks needs 13 KB less of it and fits where it did not**
- The summary cache (`sumc`, 25 entries of a chunk's column summaries) was a `static SumEntry[NSUM]`
  in `.bss`, 13 KB of the app's RAM. It is a pointer now, pointed at a buffer on `main`'s stack
  before `plat_begin()` (`gen_set_cache`, `games/numblocks/src/main.c`), and the calculator is known
  to give apps 32 KB of stack apart from their RAM.
- NumBlocks' RAM went from 133340 to 120144 bytes, under the 133652 bytes custom builds of Epsilon
  25.2 give apps. On the calculator `sumc` is that stack buffer; on the host build it stays a static
  array (`#ifdef HOST`), so tests and screenshots are unchanged — the same frames, the same worlds.
- Ported from upstream commit `d5228d5`, which did the same there. The rest of that commit (a strip's
  colours and depths on `render_frame`'s stack) was already here in 1.4.0.

## 1.4.0
**A void opens under the world — fall out of it and it kills you in Survival and Creative alike, a spectator flies down into it unharmed — and the game mode switcher opens on the next mode round its cycle**
- There is a void under the world now, as in Minecraft under its lowest block: what was solid
  rock under `y = 0` is empty, so a player who breaks the bedrock floor out from under
  themselves (Creative breaks it at once) falls past the end of the world and takes the 4
  damage a tick `Entity.checkBelowWorld` gives below 64 blocks under it. Health goes 20, 16,
  12, 8, 4, 0 over the five ticks the 10-tick grace between hits lets through, so it is the
  slow death Minecraft gives rather than the instant one before it. Spectator is immune to
  it (`Player.isInvulnerableTo`) and flies on down; `/kill` kills a spectator, as it does
  there, so it is its own damage kind rather than the void's.
- The pictures come with it: a camera under the world used to see sky whichever way it
  looked, because the rays gave up below the cache's bottom. They come up in through it now,
  as the ones above the cache come down in through its top, so the rock overhead shows, and
  the underside of the world's lowest layer is lit by the void's full sky light, which is the
  default value Minecraft's light layer gives outside the world, so it reads as the lit
  bedrock it is. Under the world the cells are drawn trace by trace rather than from the
  plane their samples agree on: that plane can be the ceiling of a hole a nearer face of
  stands in front of, and only a ray knows which comes first — the fast fills would have drawn
  the rock across the hole's walls. The adaptive picture is checked against `play_ref`'s full
  trace either way: it agrees to the pixel, in the void and in the world above, before and
  after alike (`bench_render.py`'s checksums are unchanged).
- A liquid stops at the world's floor as it does at its top: water on the bedrock spreads
  sideways as it did, rather than draining into the void as Minecraft's does, because there
  is nothing here under the world for it to drain into.
- `sin + cos` opened the switcher on Creative from Survival and on Survival from Creative, so
  from Creative alone the third mode stood one press of cos away, and the other two could not
  reach it in one press. It now opens on the mode after the one you are in, as Minecraft's
  does, so letting go of sin (or OK) takes you round the cycle: Creative, Survival, Spectator,
  Creative. cos still moves the outline one on from there, and Back still leaves nothing
  changed. Tests: the switcher from each of the three modes, and from Survival two cos presses
  round to Survival again.

## 1.3.17
**Dungeons are found two to three times as often as 1.3.16's, and a dungeon is the same whichever way the world is walked**
- The dungeon tries of a population that passed were remembered in eight bits, so the
  ninth and later ones were forgotten: a population replayed by the chunks it writes
  into stopped making their dungeons, and the world came to depend on the order its
  chunks were generated in (the same place held different dungeons, and different
  blocks, walked into from one side than from the other). They are kept in the
  thirty-two bits their number asks for, and the memo says so with a static assert,
  so the tries cannot outgrow it again.
- Thirty-two tries a chunk where 1.3.16 made twenty-four (Minecraft makes eight): two
  to three times as many dungeons as 1.3.16 gave, four or so times what the game had
  before them. A monster spawner is the one thing a dungeon has that nothing else
  makes, and they were too rare to find.
- Two tests that leaned on the world: the hall of the cow that watches the player
  stands in the dark with monsters about it, and a creeper's blast could take the
  floor from under the cow (the test is about the watching, so the world's own
  spawning is off while it runs); and the cache that slides up a step at a time is
  now compared with one made at the destination where the two windows overlap, as
  the climb flies and arriving at once does not yet know to, so they can come to rest
  in different bands without holding anything different.

## 1.3.16
**Dungeons are found twice as often, and a monster spawner's mobs come out about three times as fast**
- WorldGenDungeons is tried eight times a chunk in Minecraft, and eight times a chunk here;
  a dungeon is the one place a monster spawner comes from, and they were too rare to find.
  Twenty-four tries a chunk give about twice as many dungeons (not twice the tries: the ones
  of a population that pass dig the ground out from under the floor and ceiling test of the
  tries after them). The population's memo keeps a bit for each of the twenty-four, so a
  population replayed by the chunks it writes into still makes the same dungeons.
- A spawner near the player tries about once in six a second, where it was once in sixteen:
  the first mob is out within seconds rather than the minute or two it took, and the ones
  after it keep coming (the world still keeps at most six of a kind about one, and the try
  stays the short one the spawners here have always had against MobSpawner's 200 to 800
  ticks, now shorter still).

## 1.3.15
**Brightness comes in at 8, its top stop is a little dimmer, and it sits alone under the Video Settings' pairs**
- The Brightness slider of 1.3.14 comes in at 8, just over the middle of Moody to
  Bright, rather than at 0: a room with no light block in it can be made out
  without opening the settings (Minecraft's own default is half of its slider),
  and 0 is still there at the first stop for the pitch black it has always been.
- 15 gives a little less light than it did: it is what stops 12 and 13 used to
  give (level 0 lifted by 0.375 where the light table's curve puts 0.03).
- It has moved under the two pairs of buttons in Video Settings, as the pairs
  are laid out there: Render Distance beside Graphics, View Bobbing beside
  Clouds, then Brightness on its own row under them, with Done below it.

## 1.3.14
**A spectator flying into a tree sees the world through its leaves, and the Video Settings have a Brightness slider**
- The eyes' own block was drawn as the inside of one face at zero distance, so flying into
  a canopy filled the whole screen with one leaf (and the same in a pane of glass): the
  green flashes a spectator saw with its head inside a tree. A ray that starts inside a
  see-through block now goes on through it, the way Minecraft's back-face culling leaves
  nothing of the block you are in, so you see the world through the leaves you are flying
  through, or the sky above the canopy.
- Brightness is in Video Settings beside the Render Distance and the Graphics buttons, a
  slider of sixteen stops, 0 (Moody) to 15 (Bright): the word at either end where
  Minecraft writes it, the number between them, one stop a press of left or right (or OK,
  which steps it along and wraps round). It is the player's setting, as Minecraft's is,
  and not the world's: the light a block has is the same at every stop (a torch is a torch
  wherever the slider is) and only the dark end of the light table is lifted, so a cave
  with no light in it stays the pitch black it has always been at 0 and can be made out at
  15, while a lit cave and the sky look the same at every stop. Minecraft's own is a gamma
  over every level of its table; here the lift fades away as the light comes up, which
  keeps 0 exactly the game it was and a torch-lit cave unchanged. The setting comes in at
  0: nothing about the game's look changes until the player asks for it.

## 1.3.13
**The control sheet no longer crashes the game when the keys fit on one line**
- The three notes under the key list ("Jump twice: fly", "sin + cos: the game
  mode switcher", "Menus: OK takes or puts") were drawn by a loop that counted
  to five rows instead of to the three notes there are, so as soon as the keys
  above them wrapped onto one line the sheet's fourth line read past the end of
  the list and drew text from whatever followed it in memory. On the calculator
  that was the address 0, and the game stopped there on the first boot — the
  sheet every new player sees before anything else. The loop now stops at the
  list's length.
- The other fix the device build needed to bring the sheet back: nothing else.
  Verified on the device emulator (tools/emu.py): the first-boot sheet, then
  Home quitting on its own, 46 fps, no violations.

## 1.3.12
**Spectator mode: fly through the blocks, see through them, and look at the world through a mob's eyes**
- `/gamemode spectator` (cheats on) is Minecraft's spectator: you always fly,
  through every block, with nothing to touch — no mining, placing, eating or
  opening, no damage, and no mob noticing you, so the world goes on living
  around you. Its whole HUD is the crosshair, and that only while it is on a mob:
  OK then sees the world through that mob's eyes, at its eye height, turning
  where it looks, until alpha brings you back to your own flying body. Flying
  into the ground, the rock around your head turns see-through, so the cave walls
  and the grass above show through the stone.
- sin + cos together is the game mode switcher, as Minecraft's F3 + F4: the modes
  come up in the middle of the screen with the one last used outlined, cos moves
  the outline along them, and letting go of sin (or pressing OK) takes the one
  outlined — Back leaves without changing anything. It needs cheats, as the
  command does; sin on its own still turns the F3 overlay on and off.
- The hand is drawn from a model of its own (`src/hand.c` + `src/hand.h`) put
  together for the frame it is drawn in, on render_frame's stack, instead of the
  fixed animation tables: it follows the swing, the mining progress and the item
  held, and the spectator's empty hand is nothing at all.
- The generator's scratch buffer (7.7 KB) is given to it by its caller and lives
  on that caller's stack — `world_follow`'s for a move, `world_spawn`'s for the
  spawn search — instead of in the app's RAM, and a dungeon's monster spawner is
  placed with the mob its draw made of it, which the world then keeps (as a spawn
  egg on it would have set it), so a dungeon keeps its mob once it is loaded again.
- NumBlocks no longer rides inside NumPlay: `games/games.json` drops it, the
  launcher builds without it, and it ships as `build/apps/NumBlocks.nwa` (its own
  app, "too big to share the calculator with NumPlay", as Champion Island and
  Hollow Knight already do). The interest front that hides NumBlocks behind an
  Interest calculator (`games/numblocks-interest`) and the Interest app itself
  (`games/finance`) build beside it.

## 1.3.11
**The modern names: terracotta, mossy cobblestone, smooth stone, oak planks**
- Every block and item reads the way the modern game names it (todo [6]): the
  inventory and the creative tabs show Oak Planks and Oak Log (not "Oak Wood
  Planks" and "Oak Wood"), Mossy Cobblestone, Smooth Stone and Smooth Stone
  Slab, Petrified Oak Slab, the sixteen Terracottas ("White Terracotta" ..
  "Black Terracotta"), Snow Block beside Snow, Dead Bush, Tall Grass, the Brown
  and Red Mushrooms, Oak Stairs, and the items 1.13 and 1.14 renamed: Wheat
  Seeds, Sugar Cane, Clay Ball, Milk Bucket, White Bed, Melon Slice, Raw Cod,
  Cooked Cod, Tropical Fish and the dyes (Red Dye, Yellow Dye ... Bone Meal).
- The commands name a block state or an item, 1.13's way, so a name carries no
  data value beside it: `/give @p oak_planks`, `/give @p zombie_spawn_egg` (the
  spawn eggs are give-able by name for the first time), `/setblock ~ ~ ~
  orange_terracotta` (its handling word now follows the name directly), and
  `/clear @p red_wool` clears the one item it names. Tab completion offers the
  same names; the numeric ids and the 1.8 ids keep taking what has no single
  modern name.
- The superflat customizer's layer names put each block's modern name first and
  keep its 1.8 spelling as an alias, so a preset saved with the old names
  ("hardenedclay", "whiteclay", "logoak", "snow") still loads and is shown
  modern.
- tools/names.py now writes a third table, `mc_state_name`: the modern name of
  every block state and item `/give` knows. The give keys, the ids, the
  recipes and the saves are untouched, so nothing moves but the names; the
  build grows by 4 KB of flash for the new table.

## 1.3.10
**A dungeon's spawner comes set to a mob: skeletons, zombies and spiders**
- The monster spawner a dungeon is built with now has the mob Minecraft gives it, instead of
  coming out of the ground empty: WorldGenDungeons' own draw (a nextInt(4) — half skeletons,
  a quarter zombies and a quarter spiders, as 1.8 picks them), which the generator now hands
  out rather than throwing away. The world keeps it in its monster spawner record, so the same
  dungeon has the same mob again when the world is loaded and its chunk is made once more.
- Such a spawner shows its egg turning inside the cage and makes its mobs as the game is
  played, exactly as one you have set with a spawn egg does: a dungeon is a working farm from
  the moment you find it. A spawner fresh out of the inventory stays empty until you set it,
  as before.
- The world itself is unchanged: that draw was always taken from the population's stream, and
  the terrain still comes out block for block as it did.
## 1.3.9
**Spectator: fly through the world, and see through a mob's eyes**
- A third game mode, Minecraft's spectator: /gamemode spectator (or sp, or 3),
  and the sin + cos switcher has it as a third slot after Survival, with an eye
  for its icon. It needs cheats, as the other two modes do.
- You fly, always, and through every block: nothing stops you, nothing lands
  you, and nothing you do touches the world - no mining, placing, eating or
  opening, no damage, no mob noticing you (a zombie wanders off instead of
  coming, animals graze). The HUD goes away with it all: no hotbar, hearts,
  hunger, air or experience, no first-person hand, no block outlined, only the
  crosshair - and that only while it is on a mob, as in Minecraft.
- With the crosshair on a mob, OK sees the world through its eyes: the camera
  is the mob's, at its eye height, and turns where it looks, for as long as it
  lives. alpha, the dismount key, brings you back to your own flying body. The
  chat, the commands, the F3 overlay and the save all work as before.
- Flying into the ground, the rock goes see-through around your head, as in
  Minecraft: the faces with nothing solid in front of them show - a cave's
  walls, the grass above - through the stone, until you are out the other side.
  It is the most a frame does: 14 steps a pixel where the surface is 7, and
  only with your head inside a block.
## 1.3.8
**The sprint key alone sprints: forward twice no longer does**
- Pressing forward twice in quick succession no longer starts a sprint: it is
  the i key that sprints, held while walking in Hold, or tapped in Toggle, as
  Options, Controls, Sprint chooses. The 7-tick double-tap window and everything
  it needed (the timer, forward's last state) are gone; the sprint still ends
  when forward is let go, on a wall, on sneaking, while eating or when hungry.
- The key sheet drops the "Forward twice: sprint" note and the "EXE: as OK"
  line, which no longer said anything the sheet's own keys did not. With the
  sheet changed it shows once more on the next start (KEY_SHEET 8).
## 1.3.7
**Hold mode: letting go of the sprint key ends the sprint**
- Sprint: Hold ran on while forward was held, whatever the key was doing: it now ends
  the moment the i key is let go, which is what Hold means. A double-tap of forward
  keeps its own sprint - that one is not the key's to end, so it carries on while
  forward is held, as before.
- Toggle is as it was: a tap switches sprinting on, letting go of the key does not
  end it, and letting go of forward does.
## 1.3.6
**The game mode switcher on sin + cos, as Minecraft's F3 + F4**
- Hold sin and press cos (the key beside it, the two of them next to each other
  like F3 and F4) to switch game mode. It opens the switcher's own panel, as
  Minecraft draws it: the gray bar across the top says which mode is picked
  ("Creative Mode"), the two modes sit in the middle in their slots with
  Minecraft's own icons - the grass block and the iron sword - the one picked
  inside its golden outline, and "[cos] Next" under them says the key that moves
  along the row. The world stays bright behind the panel, as in Minecraft.
- cos marches the outline along the modes while sin is held, the arrows move it
  too, letting go of sin takes the mode outlined, OK takes it straight away, and
  Back leaves without changing anything. It opens on the other mode - in Creative
  on Survival, in Survival on Creative - which is Minecraft's rule of opening on
  the mode you were in last, and with two modes that is always the other one. The
  mode is kept with the world.
- Adventure and Spectator are left out: the first needs "you cannot break a
  block" all through the game, and the second a view that flies through blocks
  and shows no player, which the calculator can only fake. The switcher shows the
  two modes NumBlocks has, so cos just goes back and forth.
- The switcher needs cheats on, as it is the /gamemode command; without them the
  chat says "You do not have permission to use this command", exactly as running
  that command by hand does.
- sin alone still turns the debug overlay on and off, and it now waits until sin
  is let go to do it, so that holding sin and pressing cos never brings the
  overlay up: the pair wins, as F3 + F4 does.
- The control sheet shows the pair and shows again at startup (KEY_SHEET 7), and
  the panel, the slots and the outline are packed from the modern client's
  gui/container/gamemode_switcher.png and gui/sprites/gamemode_switcher, not
  the 1.8.8 jar. The panel's body is 40% see-through, as the texture's own alpha.
## 1.3.5
**Toggle sprint ends with forward; F3 counts the velocity; version entries get titles**
- Sprint: Toggle now ends the sprint when forward is let go, as Hold does: the
  toggle means the i key need not be held while walking, nothing more. Releasing
  forward was keeping the sprint on across a walk, which was wrong.
- The F3 overlay has a velocity line under XYZ: the speed in blocks a second, with
  the three axes with it, so sprinting, swimming and falling can be measured.
- Every entry in versions.md now starts with a one-line title saying what the
  change is, and agents.md asks for one on every change.
## 1.3.4
**The version in full, on the title screen and in F3**
- The version is written in full where it shows: the title screen reads "Numblocks
  custom 1.3.4" and the F3 overlay "Numblock custom 1.3.4 (minecraft.1.8.8)",
  where both used to stop at 1.3. One version in the source now feeds both, so
  they cannot drift from versions.md again.
## 1.3.3
**Controls is a menu of its own, and sprint may be toggled**
- Options, Controls is a menu of its own now, not the key bindings opened at once:
  the key sheet wide at the top, with Sprint and Keybinds side by side under
  it. The key sheet also opens from the key bindings' first row as before, and Done
  on either screen goes one step back out.
- Sprint can be toggled instead of held. Options, Controls, Sprint switches between
  Hold (the i key held while walking, as it was) and Toggle, where a tap switches
  sprinting on and another switches it off. Sneaking, walking into a wall, eating or
  getting hungry still end it either way, as does letting go of forward.
## 1.3.2
**Sprint on the i key**
- Sprint is on the i key, next to forward, where it can be held while walking with the
  other hand. It was on backspace, which is also the key that deletes while you type a
  world's name or a seed, so it got in the way of typing and was a key nobody would
  look for. Keys saved on backspace move to i; keys set by hand stay as they were set,
  and the control sheet shows again now that the keys changed, with i marked Sprint.
  Forward twice still sprints too.
## 1.3.1
**More of everything alive: bigger entity, particle and mob caps, spawners that start empty**
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
**Render distance setting (1-2 chunks)**
- Added a render distance setting (1–2 chunks, default 2, saved in the config)
  in Video Settings, as a slider.
- The whole world in sight stays the 40 x 40 x 24 block view cache, fully in
  memory as before — nothing is cached or simplified past it. A chunk here is 8
  blocks, the cache's slide step: 1 -> 11, 2 -> 19 blocks of rays. The fog closes
  where the ground does (the cache's edge, as the camera keeps 16 of its blocks
  to each side), so the ground fades into the sky without a seam at either
  setting, and the rays never walk past the world into nothing.
