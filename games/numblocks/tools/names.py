#!/usr/bin/env python3
"""Minecraft's names for blocks and items, for the commands (/give, /setblock,
/clear) and their completion: writes src/names.c.

    python3 tools/names.py > src/names.c

Blocks go by their registry name or numeric id (Block.blockRegistry), items by
theirs (Item.itemRegistry, where a block's item has the block's name). The names
are 1.13's: the flattening renamed the ids themselves and split the old ones -
`planks` became `oak_planks` (with its five other woods), `dye` the sixteen
colours of their own, `spawn_egg` the eight mobs' eggs - so a name now says
which block state or item it means, with no data value beside it.

Three tables come out of this, and every name in them is a modern one:
  mc_block_name[198], mc_item_name[176]  the 1.8 id numbering, for the numeric
      ids the commands still take: an id's modern name (of its first state, for
      the ones the flattening split), '' where NumBlocks has no such block
  mc_state_name[]    parallel to give_key/give_id: the modern name of every
      block state and item /give knows, one name per give entry
The ids NumBlocks has no block or item for keep their 1.8 spelling in the two
lists: nothing reads them (they are filtered out of the name tables)."""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import blocks  # noqa: E402
import items  # noqa: E402

# Block.registerBlocks: the numeric ids 0..197. The ids NumBlocks has are 1.13's,
# with the id of the state that comes first where the flattening split one: planks
# is oak_planks and tallgrass's first state, the dead bush, is dead_bush. (The
# flowing liquids keep 1.8's names: water and lava are one block each in the
# modern game, and only /setblock can place the two apart.)
BLOCK = '''air stone grass_block dirt cobblestone oak_planks oak_sapling bedrock flowing_water water
flowing_lava lava sand gravel gold_ore iron_ore coal_ore oak_log oak_leaves sponge glass lapis_ore lapis_block
dispenser sandstone noteblock white_bed golden_rail detector_rail sticky_piston cobweb dead_bush dead_bush piston
piston_head white_wool piston_extension dandelion poppy brown_mushroom red_mushroom gold_block iron_block
smooth_stone smooth_stone_slab bricks tnt bookshelf mossy_cobblestone obsidian torch fire spawner oak_stairs
chest redstone_wire diamond_ore diamond_block crafting_table wheat farmland furnace furnace standing_sign
oak_door ladder rail cobblestone_stairs wall_sign lever stone_pressure_plate iron_door wooden_pressure_plate
redstone_ore lit_redstone_ore unlit_redstone_torch redstone_torch stone_button snow ice snow_block cactus clay
sugar_cane jukebox oak_fence pumpkin netherrack soul_sand glowstone portal jack_o_lantern cake
unpowered_repeater powered_repeater stained_glass trapdoor infested_stone stone_bricks brown_mushroom_block
red_mushroom_block iron_bars glass_pane melon pumpkin_stem melon_stem vine fence_gate brick_stairs
stone_brick_stairs mycelium lily_pad nether_brick nether_brick_fence nether_brick_stairs nether_wart
enchanting_table brewing_stand cauldron end_portal end_portal_frame end_stone dragon_egg redstone_lamp
lit_redstone_lamp double_wooden_slab oak_slab cocoa sandstone_stairs emerald_ore ender_chest tripwire_hook
tripwire emerald_block spruce_stairs birch_stairs jungle_stairs command_block beacon cobblestone_wall
flower_pot carrots potatoes wooden_button skull anvil trapped_chest light_weighted_pressure_plate
heavy_weighted_pressure_plate unpowered_comparator powered_comparator daylight_detector redstone_block
nether_quartz_ore hopper quartz_block quartz_stairs activator_rail dropper white_terracotta stained_glass_pane
acacia_leaves acacia_log acacia_stairs dark_oak_stairs slime barrier iron_trapdoor prismarine sea_lantern
hay_block carpet terracotta coal_block packed_ice sunflower standing_banner wall_banner
daylight_detector_inverted red_sandstone red_sandstone_stairs double_stone_slab2 stone_slab2
spruce_fence_gate birch_fence_gate jungle_fence_gate dark_oak_fence_gate acacia_fence_gate spruce_fence
birch_fence jungle_fence dark_oak_fence acacia_fence spruce_door birch_door jungle_door acacia_door
dark_oak_door'''.split()
assert len(BLOCK) == 198, len(BLOCK)

# Item.registerItems: the numeric ids 256..431. The ids NumBlocks has are 1.13's; the
# dye item's meta 0 is the ink sac, the fish's the cod and the cooked fish's the
# cooked cod, which are the modern ids of the first of the states under the old id.
# '-' is the spawn_egg id, which the flattening split into the eight mobs' eggs
# (SPLIT below): it has no single modern name to hold.
ITEM = '''iron_shovel iron_pickaxe iron_axe flint_and_steel apple bow arrow coal diamond iron_ingot gold_ingot iron_sword
wooden_sword wooden_shovel wooden_pickaxe wooden_axe stone_sword stone_shovel stone_pickaxe stone_axe diamond_sword
diamond_shovel diamond_pickaxe diamond_axe stick bowl mushroom_stew golden_sword golden_shovel golden_pickaxe
golden_axe string feather gunpowder wooden_hoe stone_hoe iron_hoe diamond_hoe golden_hoe wheat_seeds wheat bread
leather_helmet leather_chestplate leather_leggings leather_boots chainmail_helmet chainmail_chestplate
chainmail_leggings chainmail_boots iron_helmet iron_chestplate iron_leggings iron_boots diamond_helmet
diamond_chestplate diamond_leggings diamond_boots golden_helmet golden_chestplate golden_leggings golden_boots flint
porkchop cooked_porkchop painting golden_apple sign oak_door bucket water_bucket lava_bucket minecart saddle
iron_door redstone snowball boat leather milk_bucket brick clay_ball sugar_cane paper book slime_ball
chest_minecart furnace_minecart egg compass fishing_rod clock glowstone_dust cod cooked_cod ink_sac bone sugar
cake white_bed repeater cookie filled_map shears melon_slice pumpkin_seeds melon_seeds beef cooked_beef chicken
cooked_chicken rotten_flesh ender_pearl blaze_rod ghast_tear gold_nugget nether_wart potion glass_bottle spider_eye
fermented_spider_eye blaze_powder magma_cream brewing_stand cauldron ender_eye speckled_melon - experience_bottle
fire_charge writable_book written_book emerald item_frame flower_pot carrot potato baked_potato poisonous_potato
map golden_carrot skull carrot_on_a_stick nether_star pumpkin_pie fireworks firework_charge enchanted_book
comparator netherbrick quartz tnt_minecart hopper_minecart prismarine_shard prismarine_crystals rabbit
cooked_rabbit rabbit_stew rabbit_foot rabbit_hide armor_stand iron_horse_armor golden_horse_armor
diamond_horse_armor lead name_tag command_block_minecart mutton cooked_mutton banner - spruce_door birch_door
jungle_door acacia_door dark_oak_door'''.split()
assert len(ITEM) == 176, len(ITEM)

# NumBlocks' items Minecraft named differently, or that shared one id told apart by their
# data value (1.8's dye, the fish, coal, the bed, the spawn eggs): the modern id, and the
# 1.8 data value the give key still takes
DYES = ['BLACK', 'RED', 'GREEN', 'BROWN', 'BLUE', 'PURPLE', 'CYAN', 'SILVER', 'GRAY', 'PINK', 'LIME', 'YELLOW',
        'LIGHT_BLUE', 'MAGENTA', 'ORANGE', 'WHITE']
# 1.13's ids of the one 1.8 dye item (id 351)
DYE_NAME = {'BLACK': 'ink_sac', 'RED': 'red_dye', 'GREEN': 'green_dye', 'BROWN': 'cocoa_beans',
            'BLUE': 'lapis_lazuli', 'PURPLE': 'purple_dye', 'CYAN': 'cyan_dye', 'SILVER': 'light_gray_dye',
            'GRAY': 'gray_dye', 'PINK': 'pink_dye', 'LIME': 'lime_dye', 'YELLOW': 'yellow_dye',
            'LIGHT_BLUE': 'light_blue_dye', 'MAGENTA': 'magenta_dye', 'ORANGE': 'orange_dye',
            'WHITE': 'bone_meal'}
RENAME = {'CHARCOAL': ('charcoal', 1), 'MELON_SLICE': ('melon_slice', 0), 'RAW_FISH': ('cod', 0),
          'RAW_SALMON': ('salmon', 1), 'CLOWNFISH': ('tropical_fish', 2), 'PUFFERFISH': ('pufferfish', 3),
          'COOKED_FISH': ('cooked_cod', 0), 'COOKED_SALMON': ('cooked_salmon', 1),
          'REEDS': ('sugar_cane', 0), 'WOODEN_DOOR': ('oak_door', 0), 'BED': ('white_bed', 0)}
for k, d in enumerate(DYES):
    RENAME['DYE_' + d] = (DYE_NAME[d], k)
for m, _lab, mc_id in items.MOBS:
    RENAME[f'{m.upper()}_SPAWN_EGG'] = (f'{m}_spawn_egg', mc_id)
# the modern ids of the 1.8 ids the flattening split: their 1.8 id, told apart by the data
# value in the give key (351 dye, 349 fish, 350 cooked_fish, 263 coal, 383 spawn_egg)
SPLIT = {'charcoal': 256 + ITEM.index('coal')}
for d in DYES:
    SPLIT[DYE_NAME[d]] = 256 + ITEM.index('ink_sac')
SPLIT.update({'salmon': 256 + ITEM.index('cod'), 'pufferfish': 256 + ITEM.index('cod'),
              'tropical_fish': 256 + ITEM.index('cod'), 'cooked_salmon': 256 + ITEM.index('cooked_cod')})
SPAWN_EGG = 383   # Item.registerItems' spawn_egg: one 1.8 id, the eight eggs the mob's
for m, _lab, _mc_id in items.MOBS:
    SPLIT[f'{m}_spawn_egg'] = SPAWN_EGG

# NumBlocks' block states 1.13 named differently (their display names are in items.LABELS):
# the modern id of the state, where the state's own name is not already it
MODERN = {'GRASS': 'grass_block',
          'DEAD_SHRUB': 'dead_bush', 'DEAD_BUSH': 'dead_bush', 'TALL_GRASS': 'grass',
          'BROWN_MUSHROOM_CAP': 'brown_mushroom_block', 'RED_MUSHROOM_CAP': 'red_mushroom_block',
          'STONE_SLAB_DOUBLE': 'smooth_stone', 'STONE_SLAB': 'smooth_stone_slab',
          'WOOD_SLAB_OLD': 'petrified_oak_slab', 'SANDSTONE_CHISELED': 'chiseled_sandstone',
          'SANDSTONE_SMOOTH': 'smooth_sandstone',
          'SNOW': 'snow_block', 'SNOW_LAYER': 'snow',
          'HARDENED_CLAY': 'terracotta', 'HAY_BALE': 'hay_block', 'MOB_SPAWNER': 'spawner',
          'FENCE_OAK': 'oak_fence',
          'SUNFLOWER_LOWER': 'sunflower', 'LILAC_LOWER': 'lilac', 'DOUBLE_GRASS_LOWER': 'tall_grass',
          'LARGE_FERN_LOWER': 'large_fern', 'ROSE_BUSH_LOWER': 'rose_bush', 'PEONY_LOWER': 'peony'}
# the blocks that were one id per kind of wood or colour: 1.13's id is per wood, per colour
for w in ['OAK', 'SPRUCE', 'BIRCH', 'JUNGLE', 'ACACIA', 'DARK_OAK']:
    MODERN[f'PLANKS_{w}'] = f'{w.lower()}_planks'
    MODERN[f'SAPLING_{w}'] = f'{w.lower()}_sapling'
    MODERN[f'LOG_{w}'] = f'{w.lower()}_log'
    MODERN[f'LEAVES_{w}'] = f'{w.lower()}_leaves'
    MODERN[f'WOOD_SLAB_{w}'] = f'{w.lower()}_slab'
for c, l in zip(items.COLORS, items.CLAB):
    MODERN[f'WOOL_{c}'] = l.lower().replace(' ', '_') + '_wool'
    MODERN[f'STAINED_CLAY_{c}'] = l.lower().replace(' ', '_') + '_terracotta'
# the blocks no item has: their modern id (the crops and the multi-state blocks the
# flattening gave one id of their own)
BLOCK_ONLY = {0: 'air', 8: 'flowing_water', 9: 'water', 10: 'flowing_lava', 11: 'lava', 26: 'white_bed',
              51: 'fire', 59: 'wheat', 62: 'furnace', 64: 'oak_door', 83: 'sugar_cane',
              97: 'infested_stone', 99: 'brown_mushroom_block', 100: 'red_mushroom_block',
              141: 'carrots', 142: 'potatoes'}
# a block's data value only for /setblock, where the block's own name has no modern id
FACING_ONLY = {50, 54, 61, 65}


def modern_of(name):
    """the modern (1.13+) id a command names a block state by"""
    return MODERN.get(name, name.lower())


def main():
    give = []   # (mc id << 4 | data, NumBlocks id, the modern name)
    have_block = set(s['id'] for s in blocks.S)
    item_block = set()
    for i, s in enumerate(blocks.S):
        if items.tab_of(i, True) is None:
            continue
        give.append(((s['id'] << 4) | (0 if s['id'] in FACING_ONLY else s['meta'] & 15), i, modern_of(s['name'])))
        item_block.add(s['id'])
    have_item = set()
    item_give = []
    for k, it in enumerate(items.I):
        name, data = RENAME.get(it['name'], (it['name'].lower(), 0))
        if name not in ITEM and name not in SPLIT:
            continue   # modern-only items (e.g. raw iron/gold): no 1.8 id, not /give-able
        mid = SPLIT[name] if name in SPLIT else 256 + ITEM.index(name)
        give.append(((mid << 4) | data, 256 + k, name))
        item_give.append((mid, 256 + k, name))
        have_item.add(mid)
    give.sort()
    keys = [g[0] for g in give]
    assert len(keys) == len(set(keys)), 'two things with one id and data'
    # the id tables: each id's modern name is the first state's (or BLOCK_ONLY's for the
    # ids no item has), so the lists above and the state names cannot drift apart
    first_state = {}
    for s in blocks.S:
        if s['id'] not in first_state:
            first_state[s['id']] = BLOCK_ONLY.get(s['id'], modern_of(s['name']))
    for i in have_block:
        assert BLOCK[i] == first_state[i], (i, BLOCK[i], first_state[i])
    # an item's modern name: NumBlocks' first item with that 1.8 id
    il = [''] * 176
    for mid, _nb, name in item_give:
        if not il[mid - 256]:
            il[mid - 256] = name
    for i in have_item & set(range(256, 256 + 176)):
        assert ITEM[i - 256] == il[i - 256] or (i == SPAWN_EGG and il[i - 256]), (i, ITEM[i - 256], il[i - 256])
    bl = [BLOCK[i] if i in have_block else '' for i in range(198)]
    out = ['/* Generated by tools/names.py: Minecraft 1.13\'s names for blocks and items (see nb.h). */',
           '#include "nb.h"', '']
    out.append('const char *const mc_block_name[198] = {' + ','.join(f'"{n}"' for n in bl) + '};')
    out.append('const char *const mc_item_name[176] = {' + ','.join(f'"{n}"' for n in il) + '};')
    out.append('/* the blocks that are items too (their names are items\' names) */')
    mask = [0] * 25
    for i in item_block:
        mask[i >> 3] |= 1 << (i & 7)
    out.append('const uint8_t mc_block_item[25] = {' + ','.join(str(v) for v in mask) + '};')
    out.append(f'const uint16_t give_key[{len(give)}] = {{' + ','.join(str(g[0]) for g in give) + '};')
    out.append(f'const uint16_t give_id[{len(give)}] = {{' + ','.join(str(g[1]) for g in give) + '};')
    # the modern name of every give entry: one name per block state and item. (Two 1.8
    # blocks are the modern dead bush, so the second one has no name: the first is the
    # one a command gives.)
    names, used = [], set()
    for _key, _id, name in give:
        names.append('' if name in used else name)
        used.add(name)
    out.append(f'const char *const mc_state_name[{len(give)}] = {{' + ','.join(f'"{n}"' for n in names) + '};')
    out.append(f'const int n_give = {len(give)};')
    sys.stdout.write('\n'.join(out) + '\n')


if __name__ == '__main__':
    main()
