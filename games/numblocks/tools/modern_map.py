#!/usr/bin/env python3
"""Maps Minecraft 1.8.8 texture names (what tools/blocks.py and tools/items.py
name) to their equivalents in a modern client (1.20-ish layout, as in
minecraft-textures/: textures/block/, textures/item/, ...).

Used by tools/pack.py --modern: every block and item texture comes from the
modern set; anything with no modern equivalent (bed block textures, most GUI
and HUD sprites, the fishing bobber) falls back to 1.8.8's client.jar, whose
grey interface pictures are version-neutral anyway.
"""

import os

# --- blocks/<old> -> block/<new> (only what differs; the rest keep their name)
BLOCK_MAP = {
    'log_oak': 'oak_log', 'log_spruce': 'spruce_log', 'log_birch': 'birch_log',
    'log_jungle': 'jungle_log', 'log_acacia': 'acacia_log', 'log_big_oak': 'dark_oak_log',
    'log_oak_top': 'oak_log_top', 'log_spruce_top': 'spruce_log_top', 'log_birch_top': 'birch_log_top',
    'log_jungle_top': 'jungle_log_top', 'log_acacia_top': 'acacia_log_top',
    'log_big_oak_top': 'dark_oak_log_top',
    'leaves_oak': 'oak_leaves', 'leaves_spruce': 'spruce_leaves', 'leaves_birch': 'birch_leaves',
    'leaves_jungle': 'jungle_leaves', 'leaves_acacia': 'acacia_leaves', 'leaves_big_oak': 'dark_oak_leaves',
    'planks_oak': 'oak_planks', 'planks_spruce': 'spruce_planks', 'planks_birch': 'birch_planks',
    'planks_jungle': 'jungle_planks', 'planks_acacia': 'acacia_planks', 'planks_big_oak': 'dark_oak_planks',
    'sapling_oak': 'oak_sapling', 'sapling_spruce': 'spruce_sapling', 'sapling_birch': 'birch_sapling',
    'sapling_jungle': 'jungle_sapling', 'sapling_acacia': 'acacia_sapling',
    'sapling_roofed_oak': 'dark_oak_sapling',
    'door_wood_lower': 'oak_door_bottom', 'door_wood_upper': 'oak_door_top',
    'stone_granite': 'granite', 'stone_granite_smooth': 'polished_granite',
    'stone_diorite': 'diorite', 'stone_diorite_smooth': 'polished_diorite',
    'stone_andesite': 'andesite', 'stone_andesite_smooth': 'polished_andesite',
    'cobblestone_mossy': 'mossy_cobblestone',
    'stonebrick': 'stone_bricks', 'stonebrick_mossy': 'mossy_stone_bricks',
    'nether_brick': 'nether_bricks', 'endframe_top': 'end_portal_frame_top',
    'endframe_side': 'end_portal_frame_side', 'endframe_eye': 'end_portal_frame_eye',
    'door_iron_lower': 'iron_door_bottom', 'door_iron_upper': 'iron_door_top',
    'stonebrick_cracked': 'cracked_stone_bricks', 'stonebrick_carved': 'chiseled_stone_bricks',
    'stone_slab_top': 'smooth_stone', 'stone_slab_side': 'smooth_stone',
    'sandstone_carved': 'chiseled_sandstone', 'sandstone_smooth': 'cut_sandstone',
    'sandstone_normal': 'sandstone', 'red_sandstone_normal': 'red_sandstone',
    'brick': 'bricks', 'hardened_clay': 'terracotta',
    'hardened_clay_stained_white': 'white_terracotta', 'hardened_clay_stained_orange': 'orange_terracotta',
    'hardened_clay_stained_magenta': 'magenta_terracotta',
    'hardened_clay_stained_light_blue': 'light_blue_terracotta',
    'hardened_clay_stained_yellow': 'yellow_terracotta', 'hardened_clay_stained_lime': 'lime_terracotta',
    'hardened_clay_stained_pink': 'pink_terracotta', 'hardened_clay_stained_gray': 'gray_terracotta',
    'hardened_clay_stained_silver': 'light_gray_terracotta', 'hardened_clay_stained_cyan': 'cyan_terracotta',
    'hardened_clay_stained_purple': 'purple_terracotta', 'hardened_clay_stained_blue': 'blue_terracotta',
    'hardened_clay_stained_brown': 'brown_terracotta', 'hardened_clay_stained_green': 'green_terracotta',
    'hardened_clay_stained_red': 'red_terracotta', 'hardened_clay_stained_black': 'black_terracotta',
    'wool_colored_white': 'white_wool', 'wool_colored_orange': 'orange_wool',
    'wool_colored_magenta': 'magenta_wool', 'wool_colored_light_blue': 'light_blue_wool',
    'wool_colored_yellow': 'yellow_wool', 'wool_colored_lime': 'lime_wool',
    'wool_colored_pink': 'pink_wool', 'wool_colored_gray': 'gray_wool',
    'wool_colored_silver': 'light_gray_wool', 'wool_colored_cyan': 'cyan_wool',
    'wool_colored_purple': 'purple_wool', 'wool_colored_blue': 'blue_wool',
    'wool_colored_brown': 'brown_wool', 'wool_colored_green': 'green_wool',
    'wool_colored_red': 'red_wool', 'wool_colored_black': 'black_wool',
    'tallgrass': 'grass', 'deadbush': 'dead_bush', 'reeds': 'sugar_cane', 'web': 'cobweb',
    'flower_rose': 'poppy', 'flower_blue_orchid': 'blue_orchid', 'flower_allium': 'allium',
    'flower_houstonia': 'azure_bluet', 'flower_tulip_red': 'red_tulip',
    'flower_tulip_orange': 'orange_tulip', 'flower_tulip_white': 'white_tulip',
    'flower_tulip_pink': 'pink_tulip', 'flower_oxeye_daisy': 'oxeye_daisy',
    'flower_dandelion': 'dandelion',
    'double_plant_sunflower_bottom': 'sunflower_bottom', 'double_plant_sunflower_top': 'sunflower_top',
    'double_plant_syringa_bottom': 'lilac_bottom', 'double_plant_syringa_top': 'lilac_top',
    'double_plant_grass_bottom': 'tall_grass_bottom', 'double_plant_grass_top': 'tall_grass_top',
    'double_plant_fern_bottom': 'large_fern_bottom', 'double_plant_fern_top': 'large_fern_top',
    'double_plant_rose_bottom': 'rose_bush_bottom', 'double_plant_rose_top': 'rose_bush_top',
    'double_plant_paeonia_bottom': 'peony_bottom', 'double_plant_paeonia_top': 'peony_top',
    'waterlily': 'lily_pad',
    'torch_on': 'torch', 'fire_layer_0': 'fire_0', 'mob_spawner': 'spawner',
    'furnace_front_off': 'furnace_front',
    'mushroom_block_skin_brown': 'brown_mushroom_block',
    'mushroom_block_skin_red': 'red_mushroom_block',
    'mushroom_block_skin_stem': 'mushroom_stem',
    'mushroom_brown': 'brown_mushroom', 'mushroom_red': 'red_mushroom',
    'farmland_dry': 'farmland', 'farmland_wet': 'farmland_moist',
    'grass_top': 'grass_block_top', 'grass_side': 'grass_block_side',
    'grass_side_overlay': 'grass_block_side_overlay', 'grass_side_snowed': 'grass_block_snow',
    'dirt_podzol_top': 'podzol_top', 'dirt_podzol_side': 'podzol_side',
    'ice_packed': 'packed_ice', 'rail_normal': 'rail',
    'pumpkin_face_off': 'carved_pumpkin', 'pumpkin_face_on': 'jack_o_lantern',
    'carrots_stage_0': 'carrots_stage0', 'carrots_stage_1': 'carrots_stage1',
    'carrots_stage_2': 'carrots_stage2', 'carrots_stage_3': 'carrots_stage3',
    'potatoes_stage_0': 'potatoes_stage0', 'potatoes_stage_1': 'potatoes_stage1',
    'potatoes_stage_2': 'potatoes_stage2', 'potatoes_stage_3': 'potatoes_stage3',
    'wheat_stage_0': 'wheat_stage0', 'wheat_stage_1': 'wheat_stage1',
    'wheat_stage_2': 'wheat_stage2', 'wheat_stage_3': 'wheat_stage3',
    'wheat_stage_4': 'wheat_stage4', 'wheat_stage_5': 'wheat_stage5',
    'wheat_stage_6': 'wheat_stage6', 'wheat_stage_7': 'wheat_stage7',
}

# --- items/<old> -> item/<new> (only what differs; the rest keep their name)
ITEM_MAP = {
    'wood_sword': 'wooden_sword', 'wood_shovel': 'wooden_shovel', 'wood_pickaxe': 'wooden_pickaxe',
    'wood_axe': 'wooden_axe', 'wood_hoe': 'wooden_hoe',
    'gold_sword': 'golden_sword', 'gold_shovel': 'golden_shovel', 'gold_pickaxe': 'golden_pickaxe',
    'gold_axe': 'golden_axe', 'gold_hoe': 'golden_hoe',
    'gold_helmet': 'golden_helmet', 'gold_chestplate': 'golden_chestplate',
    'gold_leggings': 'golden_leggings', 'gold_boots': 'golden_boots',
    'bow_standby': 'bow', 'fishing_rod_uncast': 'fishing_rod',
    'book_normal': 'book', 'slimeball': 'slime_ball',
    'bucket_empty': 'bucket', 'bucket_water': 'water_bucket', 'bucket_lava': 'lava_bucket',
    'bucket_milk': 'milk_bucket', 'door_wood': 'oak_door',
    'redstone_dust': 'redstone', 'reeds': 'sugar_cane', 'seeds_wheat': 'wheat_seeds',
    'apple_golden': 'golden_apple',
    'porkchop_raw': 'porkchop', 'porkchop_cooked': 'cooked_porkchop',
    'beef_raw': 'beef', 'beef_cooked': 'cooked_beef',
    'chicken_raw': 'chicken', 'chicken_cooked': 'cooked_chicken',
    'mutton_raw': 'mutton', 'mutton_cooked': 'cooked_mutton',
    'melon': 'melon_slice', 'potato_baked': 'baked_potato', 'potato_poisonous': 'poisonous_potato',
    'fish_cod_raw': 'cod', 'fish_salmon_raw': 'salmon', 'fish_clownfish_raw': 'tropical_fish',
    'fish_pufferfish_raw': 'pufferfish', 'fish_cod_cooked': 'cooked_cod',
    'fish_salmon_cooked': 'cooked_salmon',
    'dye_powder_black': 'ink_sac', 'dye_powder_red': 'red_dye', 'dye_powder_green': 'green_dye',
    'dye_powder_brown': 'brown_dye', 'dye_powder_blue': 'blue_dye', 'dye_powder_purple': 'purple_dye',
    'dye_powder_cyan': 'cyan_dye', 'dye_powder_silver': 'light_gray_dye', 'dye_powder_gray': 'gray_dye',
    'dye_powder_pink': 'pink_dye', 'dye_powder_lime': 'lime_dye', 'dye_powder_yellow': 'yellow_dye',
    'dye_powder_light_blue': 'light_blue_dye', 'dye_powder_magenta': 'magenta_dye',
    'dye_powder_orange': 'orange_dye', 'dye_powder_white': 'bone_meal',
    'clock': 'clock_00', 'compass': 'compass_00',
    'netherbrick': 'nether_brick', 'fireball': 'fire_charge',
}

# --- anything else pack.py asks for: 1.8.8 path -> modern path (same unless listed)
PATH_MAP = {
    'entity/steve': 'entity/player/wide/steve',
}


def _exists(root, rel):
    return os.path.exists(os.path.join(root, 'textures', rel + '.png'))


def modern_rel(name):
    """where a 1.8.8 texture path (as tools/blocks.py and tools/items.py name it)
    lives in a modern client, or None if it has no modern equivalent."""
    if name.startswith('@'):
        return None
    if '/' not in name:
        return None
    cat, base = name.split('/', 1)
    if cat == 'blocks':
        return 'block/' + BLOCK_MAP.get(base, base)
    if cat == 'items':
        return 'item/' + ITEM_MAP.get(base, base)
    mapped = PATH_MAP.get(name, name)
    if name == 'gui/container/creative_inventory/tab_inventory':
        # the creative survival tab's own panel: gui.c's slots match the modern file
        # (the offhand slot, the armour in two columns at the player's sides). Every
        # other container background stays on the 1.8.8 jar (see below).
        return 'gui/container/creative_inventory/tab_inventory'
    if name == 'gui/container/gamemode_switcher':
        # the game mode switcher's panel (sin + cos): 1.8.8 has none of it
        return 'gui/container/gamemode_switcher'
    # GUI/HUD pictures with no modern same-layout equivalent stay on the jar:
    #   - the container backgrounds: gui.c lays its slots out at 1.8's
    #     coordinates, and the modern ones were rearranged (1.9 moved the
    #     inventory's 2 x 2 grid from (88,26) to (97,19) and added the
    #     offhand slot at (77,61)), so a modern background puts the slots
    #     and the drawn items in different places
    #   - gui/container/furnace: the modern file lacks 1.8's flame/arrow
    #     template strip at x >= 176 that SP_FLAME/SP_ARROW are cropped from
    for prefix in ('gui/widgets', 'gui/icons', 'gui/container',
                   'particle/particles'):
        if mapped == prefix or mapped.startswith(prefix + '/'):
            return None
    return mapped


def extra_image(roots, name):
    """a picture from one of the extra modern roots, as RGBA, or None if none has it.
    Used for what neither the modern set nor 1.8.8's jar has: the per-mob spawn eggs
    (1.21.5's <mob>_spawn_egg.png, which a 1.20.1 client has none of), kept flat in
    their own folders."""
    for root in roots:
        p = os.path.join(root, name.split('/', 1)[1] + '.png')
        if os.path.exists(p):
            from PIL import Image
            return Image.open(p).convert('RGBA')
    return None


class ModernSource:
    """Drop-in for pack.Jar: reads a modern extracted client (minecraft-textures/
    layout), falling back to 1.8.8's client.jar for what has no modern equivalent
    (bed block textures, most GUI/HUD sprites, the fishing bobber). image() returns
    RGBA, exactly like Jar.image()."""

    def __init__(self, root, fallback_jar=None, extra=()):
        from PIL import Image  # noqa: F401  (kept local: pack.py already depends on PIL)
        self.root = root
        self.used_modern = 0
        self.used_fallback = []
        self.extra = tuple(extra)   # more modern roots, for what this one has none of
        self._fb = None
        if fallback_jar:
            import zipfile
            import io as _io
            from PIL import Image as _Image

            class _Fb:
                def __init__(self, path):
                    self.z = zipfile.ZipFile(path)

                def image(self, name):
                    return _Image.open(
                        _io.BytesIO(self.z.read('assets/minecraft/textures/' + name + '.png'))).convert('RGBA')

            self._fb = _Fb(fallback_jar)

    def image(self, name):
        im = extra_image(self.extra, name) if self.extra else None
        if im is not None:
            self.used_modern += 1
            return im
        rel = modern_rel(name)
        if rel is not None and _exists(self.root, rel):
            from PIL import Image
            self.used_modern += 1
            return Image.open(os.path.join(self.root, 'textures', rel + '.png')).convert('RGBA')
        if self._fb is None:
            raise FileNotFoundError(f'no modern texture for {name!r} and no fallback jar')
        self.used_fallback.append(name)
        return self._fb.image(name)

    def report(self):
        return self.used_modern, sorted(set(self.used_fallback))
