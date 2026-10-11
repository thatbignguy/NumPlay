/* The Final Earth 2: what tools/pack.py packed in data.bin (made by it: do not edit). */
#ifndef FE_DATA_H
#define FE_DATA_H
#include <stdint.h>

extern const uint8_t fe_data[];   /* data.bin (data.S) */

/* sprites: each frame of the original's atlas (often a strip of frames, 2 pixels apart) */
enum {
  SPR_9P_BUTTON_ACTIVE,                    /* spr_9p_button_active.png, 22 x 22 */
  SPR_9P_BUTTON_DISABLED,                  /* spr_9p_button_disabled.png, 22 x 22 */
  SPR_9P_HINTINFO,                         /* spr_9p_hintinfo.png, 60 x 60 */
  SPR_9P_INFO,                             /* spr_9p_info.png, 32 x 32 */
  SPR_9P_TOOLTIP,                          /* spr_9p_tooltip.png, 10 x 10 */
  SPR_9P_WINDOW,                           /* spr_9p_window.png, 60 x 60 */
  SPR_9P_WINDOW_MOREOPAQUE,                /* spr_9p_window_moreopaque.png, 60 x 60 */
  SPR_9P_WINDOW_PURPLE,                    /* spr_9p_window_purple.png, 60 x 60 */
  SPR_ADVANCEDMEDICALCLINIC,               /* spr_advancedmedicalclinic.png, 64 x 20 */
  SPR_AIMINING,                            /* spr_aiMining.png, 20 x 20 */
  SPR_ALIENDECRYPTOR,                      /* spr_aliendecryptor.png, 64 x 20 */
  SPR_ALIENHOUSE,                          /* spr_alienhouse.png, 64 x 20 */
  SPR_ALIENHOUSE_LIVINGCOMPUTER,           /* spr_alienhouse_livingcomputer.png, 152 x 20 */
  SPR_ALIENRUINS,                          /* spr_alienruins.png, 20 x 20 */
  SPR_ALIENRUINS_2,                        /* spr_alienruins_2.png, 20 x 20 */
  SPR_ALIENRUINS_3,                        /* spr_alienruins_3.png, 20 x 20 */
  SPR_ARCADE,                              /* spr_arcade.png, 64 x 20 */
  SPR_ARROW_BACK,                          /* spr_arrow_back.png, 20 x 20 */
  SPR_ARROWDOWN,                           /* spr_arrowdown.png, 7 x 5 */
  SPR_ARROWDOWN_BIG,                       /* spr_arrowdown_big.png, 16 x 16 */
  SPR_ARROWUP,                             /* spr_arrowup.png, 7 x 5 */
  SPR_ARROWUP_BIG,                         /* spr_arrowup_big.png, 16 x 16 */
  SPR_ARTCOLONY,                           /* spr_artcolony.png, 64 x 20 */
  SPR_ARTCOLONY_BUTTONBACK,                /* spr_artcolony_buttonback.png, 20 x 20 */
  SPR_ARTCOLONYWORK,                       /* spr_artcolonywork.png, 306 x 20 */
  SPR_BEACON,                              /* spr_beacon.png, 64 x 20 */
  SPR_BEACON_LIGHT,                        /* spr_beacon_light.png, 4 x 11 */
  SPR_BIGGERTV,                            /* spr_biggertv.png, 20 x 20 */
  SPR_BIRCHFOREST,                         /* spr_birchforest.png, 20 x 20 */
  SPR_BLOOMHOUSE,                          /* spr_bloomhouse.png, 108 x 20 */
  SPR_BLOOMHUT_BUTTONBACK,                 /* spr_bloomhut_buttonback.png, 20 x 20 */
  SPR_BLOOMRESTAURANT,                     /* spr_bloomrestaurant.png, 328 x 20 */
  SPR_BLOOMRESTAURANT_FOREGROUND,          /* spr_bloomrestaurant_foreground.png, 108 x 20 */
  SPR_BLOSSOMHIPPIEHQ,                     /* spr_blossomhippiehq.png, 64 x 20 */
  SPR_BLUEWORLDGLOW,                       /* spr_blueworldglow.png, 20 x 5 */
  SPR_BOARDGAMESTORE,                      /* spr_boardgamestore.png, 64 x 20 */
  SPR_BONUSRANGE_PLUS,                     /* spr_bonusrange_plus.png, 9 x 10 */
  SPR_BONUSRANGE_TOPBOTTOM,                /* spr_bonusrange_topbottom.png, 9 x 10 */
  SPR_BOTANICALGARDEN_DOME_BUTTONBACK,     /* spr_botanicalgarden_dome_buttonback.png, 20 x 20 */
  SPR_BOTANICALGARDENS,                    /* spr_botanicalgardens.png, 328 x 20 */
  SPR_BOTANICALGARDENS_BUTTONBACK,         /* spr_botanicalgardens_buttonback.png, 20 x 20 */
  SPR_BOTANICALGARDENS_PLANTS,             /* spr_botanicalgardens_plants.png, 300 x 80 */
  SPR_BOULDER,                             /* spr_boulder.png, 164 x 20 */
  SPR_BUILDHEREOUTLINE,                    /* spr_buildhereoutline.png, 20 x 20 */
  SPR_BUILDINGBUTTON,                      /* spr_buildingbutton.png, 70 x 22 */
  SPR_BUTTON,                              /* spr_button.png, 70 x 22 */
  SPR_BUTTON_ACTIVABLE,                    /* spr_button_activable.png, 70 x 22 */
  SPR_BUTTON_CHECKED,                      /* spr_button_checked.png, 70 x 22 */
  SPR_BUTTON_GRAY,                         /* spr_button_gray.png, 70 x 22 */
  SPR_BUTTON_MEDIUM,                       /* spr_button_medium.png, 70 x 22 */
  SPR_BUTTON_SMALL,                        /* spr_button_small.png, 58 x 18 */
  SPR_BUTTON_SMALLER,                      /* spr_button_smaller.png, 49 x 15 */
  SPR_BUTTON_SMALLER_MOBILE,               /* spr_button_smaller_mobile.png, 61 x 19 */
  SPR_BUTTON_WINDOWHEADER,                 /* spr_button_windowheader.png, 73 x 17 */
  SPR_BUTTON_WINDOWHEADER_MOBI,            /* spr_button_windowheader_mobi.png, 79 x 19 */
  SPR_BUTTONNOTIFICATION,                  /* spr_buttonnotification.png, 22 x 22 */
  SPR_CACAOFARM,                           /* spr_cacaofarm.png, 64 x 20 */
  SPR_CACAOFARM_TREES,                     /* spr_cacaofarm_trees.png, 152 x 20 */
  SPR_CAMERA,                              /* spr_camera.png, 14 x 14 */
  SPR_CARROTJUICEBAR,                      /* spr_carrotjuicebar.png, 64 x 20 */
  SPR_CHECKBOX,                            /* spr_checkbox.png, 18 x 8 */
  SPR_CHILDREN,                            /* spr_children.png, 202 x 5 */
  SPR_CHOCOLATEFACTORY,                    /* spr_chocolatefactory.png, 64 x 20 */
  SPR_CHOCOLATEFACTORY_FRAMES,             /* spr_chocolatefactory_frames.png, 240 x 20 */
  SPR_CHOCOLATEFACTORY_IDLE,               /* spr_chocolatefactory_idle.png, 20 x 20 */
  SPR_CITIZEN_BALL,                        /* spr_citizen_ball.png, 4 x 4 */
  SPR_CITYHALL,                            /* spr_cityhall.png, 64 x 20 */
  SPR_CLOUDS,                              /* spr_clouds.png, 990 x 122 */
  SPR_COLORCIRCLE,                         /* spr_colorcircle.png, 200 x 223 */
  SPR_COLOREDBLOCK,                        /* spr_coloredblock.png, 438 x 20 */
  SPR_COLOREDDOORS,                        /* spr_coloreddoors.png, 130 x 20 */
  SPR_COLORPICKER_CIRCLE,                  /* spr_colorpicker_circle.png, 12 x 12 */
  SPR_COLORPICKER_COLORPICKED,             /* spr_colorpicker_colorpicked.png, 10 x 10 */
  SPR_COLORPICKER_TRIANGLE,                /* spr_colorpicker_triangle.png, 12 x 12 */
  SPR_COMFORTABLECOUCH,                    /* spr_comfortablecouch.png, 20 x 20 */
  SPR_COMFORTABLEHOUSE,                    /* spr_comfortablehouse.png, 64 x 20 */
  SPR_COMMUNITYHOUSE_BUTTONBACK,           /* spr_communityhouse_buttonback.png, 20 x 20 */
  SPR_COMMUNITYHOUSE_CROP,                 /* spr_communityhouse_crop.png, 68 x 14 */
  SPR_COMMUNITYHOUSE_INDOORPOND,           /* spr_communityhouse_indoorpond.png, 20 x 20 */
  SPR_COMMUNITYHOUSE_ROCK,                 /* spr_communityhouse_rock.png, 61 x 14 */
  SPR_COMMUNITYHOUSE_TREE,                 /* spr_communityhouse_tree.png, 47 x 14 */
  SPR_COMMUNITYHOUSE_TREE_GROW,            /* spr_communityhouse_tree_grow.png, 75 x 14 */
  SPR_COMPUTER_OBSERVATORY,                /* spr_computer_observatory.png, 20 x 20 */
  SPR_COMPUTERCHIPFACTORY,                 /* spr_computerchipfactory.png, 64 x 20 */
  SPR_COMPUTERCHIPFACTORY_FRAMES,          /* spr_computerchipfactory_frames.png, 526 x 20 */
  SPR_COMPUTERCHIPFACTORY_IDLE,            /* spr_computerchipfactory_idle.png, 20 x 20 */
  SPR_COMPUTERRESEARCHCENTER,              /* spr_computerresearchcenter.png, 64 x 20 */
  SPR_COMPUTERRESEARCHCENTER_FRAMES,       /* spr_computerresearchcenter_frames.png, 372 x 20 */
  SPR_COMPUTERRESEARCHCENTER_IDLE,         /* spr_computerresearchcenter_idle.png, 20 x 20 */
  SPR_CONTRAPTION,                         /* spr_contraption.png, 64 x 20 */
  SPR_CREDITS,                             /* spr_credits.png, 14 x 14 */
  SPR_CUTTINGEDGEHOME,                     /* spr_cuttingedgehome.png, 64 x 20 */
  SPR_DECORATION_KEYCITY,                  /* spr_decoration_keycity.png, 86 x 20 */
  SPR_DECORATION_ROCKS,                    /* spr_decoration_rocks.png, 64 x 20 */
  SPR_DESTROYING,                          /* spr_destroying.png, 20 x 20 */
  SPR_DIESHRINK,                           /* spr_dieshrink.png, 20 x 20 */
  SPR_DOME_BOTANICALGARDEN,                /* spr_dome_botanicalgarden.png, 130 x 20 */
  SPR_DOME_BOTANICALGARDEN_ALT1,           /* spr_dome_botanicalgarden_alt1.png, 130 x 20 */
  SPR_DOME_BOTANICALGARDEN_ALT1B,          /* spr_dome_botanicalgarden_alt1b.png, 130 x 20 */
  SPR_DOME_BOTANICALGARDEN_ALT2,           /* spr_dome_botanicalgarden_alt2.png, 130 x 20 */
  SPR_DOME_BOTANICALGARDEN_ALT2B,          /* spr_dome_botanicalgarden_alt2b.png, 130 x 20 */
  SPR_DOME_BOTANICALGARDEN_B,              /* spr_dome_botanicalgarden_b.png, 130 x 20 */
  SPR_DOME_BOTANICALGARDEN_PLANTS,         /* spr_dome_botanicalgarden_plants.png, 300 x 40 */
  SPR_DOMEROOF,                            /* spr_domeroof.png, 460 x 20 */
  SPR_DRACULA_ADULT,                       /* spr_dracula_adult.png, 2 x 5 */
  SPR_DRACULA_CHILD,                       /* spr_dracula_child.png, 2 x 5 */
  SPR_ECOFARM,                             /* spr_ecofarm.png, 64 x 20 */
  SPR_ECOFARM_BUTTONBACK,                  /* spr_ecofarm_buttonback.png, 20 x 20 */
  SPR_ECOFARM_CROP1,                       /* spr_ecofarm_crop1.png, 94 x 14 */
  SPR_ECOFARM_CROP2,                       /* spr_ecofarm_crop2.png, 68 x 5 */
  SPR_ECOFARM_CROP2_ALT,                   /* spr_ecofarm_crop2_alt.png, 58 x 5 */
  SPR_ECOFARM_CROP2_ALT2,                  /* spr_ecofarm_crop2_alt2.png, 78 x 5 */
  SPR_ECOFARM_CROP3,                       /* spr_ecofarm_crop3.png, 78 x 5 */
  SPR_ECOFARM_CROP3_ALT,                   /* spr_ecofarm_crop3_alt.png, 58 x 5 */
  SPR_EXPERIMENTALFARM,                    /* spr_experimentalfarm.png, 64 x 20 */
  SPR_EXPERIMENTALFARM_BUTTONBACK,         /* spr_experimentalfarm_buttonback.png, 20 x 20 */
  SPR_EXPERIMENTALFARM_CROP1,              /* spr_experimentalfarm_crop1.png, 75 x 14 */
  SPR_EXPERIMENTALFARM_CROP2,              /* spr_experimentalfarm_crop2.png, 61 x 5 */
  SPR_EXPERIMENTALFARM_CROP3,              /* spr_experimentalfarm_crop3.png, 33 x 5 */
  SPR_EXPERIMENTATIONLAB,                  /* spr_experimentationlab.png, 64 x 20 */
  SPR_EXPERIMENTATIONLAB_IDLE,             /* spr_experimentationlab_idle.png, 42 x 20 */
  SPR_EXPERIMENTATIONLAB_PIPE_BOTTOM,      /* spr_experimentationlab_pipe_bottom.png, 5 x 9 */
  SPR_EXPERIMENTATIONLAB_PIPE_TOP,         /* spr_experimentationlab_pipe_top.png, 5 x 14 */
  SPR_EXPERIMENTATIONLAB_SCREWDRIVER,      /* spr_experimentationlab_screwdriver.png, 20 x 20 */
  SPR_EXPLORATIONCENTRE,                   /* spr_explorationcentre.png, 64 x 20 */
  SPR_EXPLORATIONSHIP,                     /* spr_explorationship.png, 64 x 20 */
  SPR_FARMBYPRODUCTPROCESSOR,              /* spr_farmbyproductprocessor.png, 64 x 20 */
  SPR_FARMBYPRODUCTPROCESSOR_FRAMES,       /* spr_farmbyproductprocessor_frames.png, 988 x 20 */
  SPR_FARMBYPRODUCTPROCESSOR_IDLE,         /* spr_farmbyproductprocessor_idle.png, 20 x 20 */
  SPR_FARMHOUSE,                           /* spr_farmhouse.png, 64 x 20 */
  SPR_FEATHERALLIANCE,                     /* spr_featheralliance.png, 64 x 20 */
  SPR_FESTIVAL_PODIUM_SMALL,               /* spr_festival_podium_small.png, 5 x 4 */
  SPR_FESTIVALHQ,                          /* spr_festivalhq.png, 64 x 20 */
  SPR_FISH,                                /* spr_fish.png, 18 x 1 */
  SPR_FISH_BIG,                            /* spr_fish_big.png, 16 x 3 */
  SPR_FISH_BIG2,                           /* spr_fish_big2.png, 5 x 3 */
  SPR_FISH_JELLY,                          /* spr_fish_jelly.png, 5 x 5 */
  SPR_FISH_SNAKE,                          /* spr_fish_snake.png, 8 x 2 */
  SPR_FLOATINGPLATFORM,                    /* spr_floatingplatform.png, 20 x 20 */
  SPR_FLOWER_UP,                           /* spr_flower_up.png, 14 x 3 */
  SPR_FLOWERCRAFT,                         /* spr_flowercraft.png, 20 x 20 */
  SPR_FLOWERPENTHOUSE,                     /* spr_flowerpenthouse.png, 64 x 20 */
  SPR_FLOWERS,                             /* spr_flowers.png, 86 x 20 */
  SPR_FLOWERS_BLUE,                        /* spr_flowers_blue.png, 64 x 20 */
  SPR_FLOWERS_KEYCITY,                     /* spr_flowers_keycity.png, 86 x 20 */
  SPR_FLYINGSAUCER,                        /* spr_flyingsaucer.png, 20 x 20 */
  SPR_FLYINGSAUCER_ALT,                    /* spr_flyingsaucer_alt.png, 20 x 20 */
  SPR_FLYINGSAUCER_CUBE,                   /* spr_flyingsaucer_cube.png, 20 x 20 */
  SPR_FLYINGSAUCER_DEEPBLUE,               /* spr_flyingsaucer_deepblue.png, 20 x 20 */
  SPR_FLYINGSAUCER_SHADE,                  /* spr_flyingsaucer_shade.png, 20 x 20 */
  SPR_FORCEDBUILDINGLOCATION,              /* spr_forcedbuildinglocation.png, 22 x 52 */
  SPR_FOREST,                              /* spr_forest.png, 262 x 20 */
  SPR_FOREST_BRANCHES,                     /* spr_forest_branches.png, 262 x 20 */
  SPR_FOREST_BRANCHES_GROW,                /* spr_forest_branches_grow.png, 284 x 20 */
  SPR_FOREST_GROW,                         /* spr_forest_grow.png, 284 x 20 */
  SPR_FOSSILMUSEUM,                        /* spr_fossilmuseum.png, 64 x 20 */
  SPR_FROG,                                /* spr_frog.png, 16 x 7 */
  SPR_FULLSCREEN,                          /* spr_fullscreen.png, 14 x 14 */
  SPR_GRASS,                               /* spr_grass.png, 64 x 20 */
  SPR_GREENOUTLINE,                        /* spr_greenoutline.png, 22 x 22 */
  SPR_HACKERHQ,                            /* spr_hackerhq.png, 64 x 20 */
  SPR_HACKERS_BOLT,                        /* spr_hackers_bolt.png, 4 x 7 */
  SPR_HAPPINESS,                           /* spr_happiness.png, 10 x 10 */
  SPR_HERBGARDEN,                          /* spr_herbgarden.png, 196 x 20 */
  SPR_HERBGARDEN_BUTTONBACK,               /* spr_herbgarden_buttonback.png, 20 x 20 */
  SPR_HERBGARDEN_MEDIC,                    /* spr_herbgarden_medic.png, 196 x 20 */
  SPR_HERBGARDEN_PLANTS,                   /* spr_herbgarden_plants.png, 130 x 20 */
  SPR_HGH_BLOCK,                           /* spr_hgh_block.png, 4 x 4 */
  SPR_HGH_BOOSTER,                         /* spr_hgh_booster.png, 9 x 8 */
  SPR_HGH_CITIZENCLOTHES_1,                /* spr_hgh_citizenclothes_1.png, 2 x 5 */
  SPR_HGH_MONSTER_1,                       /* spr_hgh_monster_1.png, 14 x 9 */
  SPR_HIDEUI,                              /* spr_hideui.png, 26 x 12 */
  SPR_HINTINFO_ARROW,                      /* spr_hintinfo_arrow.png, 10 x 6 */
  SPR_HOLOGAMEHALL,                        /* spr_hologamehall.png, 130 x 20 */
  SPR_HOUSEOFTHEKEYFRIENDS,                /* spr_houseofthekeyfriends.png, 64 x 20 */
  SPR_HOUSING,                             /* spr_housing.png, 10 x 10 */
  SPR_HOVERHINT,                           /* spr_hoverhint.png, 70 x 22 */
  SPR_HOVERHINTS_CITIZEN,                  /* spr_hoverhints_citizen.png, 94 x 22 */
  SPR_HUMANS,                              /* spr_humans.png, 202 x 5 */
  SPR_HYPERELEVATOR,                       /* spr_hyperelevator.png, 64 x 20 */
  SPR_HYPERELEVATOR_DOOR,                  /* spr_hyperelevator_door.png, 46 x 10 */
  SPR_ICON_BOOST,                          /* spr_icon_boost.png, 10 x 14 */
  SPR_ICON_BUILD,                          /* spr_icon_build.png, 20 x 20 */
  SPR_ICON_BUILDINGMODE_DESTROY,           /* spr_icon_buildingmode_destroy.png, 20 x 20 */
  SPR_ICON_BUILDINGMODE_DESTROYLEAVINGHOLE, /* spr_icon_buildingmode_destroyleavinghole.png, 20 x 20 */
  SPR_ICON_BUILDINGMODE_DRAG,              /* spr_icon_buildingmode_drag.png, 20 x 20 */
  SPR_ICON_BUILDINGMODE_DRAGREPLACE,       /* spr_icon_buildingmode_dragreplace.png, 20 x 20 */
  SPR_ICON_BUILDINGMODE_INSERT,            /* spr_icon_buildingmode_insert.png, 20 x 20 */
  SPR_ICON_BUILDINGMODE_ONTOP,             /* spr_icon_buildingmode_ontop.png, 20 x 20 */
  SPR_ICON_BUILDINGMODE_REPLACE,           /* spr_icon_buildingmode_replace.png, 20 x 20 */
  SPR_ICON_CLOSE,                          /* spr_icon_close.png, 21 x 15 */
  SPR_ICON_ENTERTAINMENT,                  /* spr_icon_entertainment.png, 20 x 20 */
  SPR_ICON_FINDBUILDING,                   /* spr_icon_findbuilding.png, 20 x 20 */
  SPR_ICON_FOLLOWCITIZEN,                  /* spr_icon_followcitizen.png, 20 x 20 */
  SPR_ICON_HALFSPEED,                      /* spr_icon_halfspeed.png, 12 x 12 */
  SPR_ICON_HELP,                           /* spr_icon_help.png, 12 x 12 */
  SPR_ICON_HOUSES,                         /* spr_icon_houses.png, 20 x 20 */
  SPR_ICON_INVITEPEOPLE,                   /* spr_icon_invitepeople.png, 20 x 20 */
  SPR_ICON_MANAGEMENT,                     /* spr_icon_management.png, 20 x 20 */
  SPR_ICON_MENU,                           /* spr_icon_menu.png, 12 x 12 */
  SPR_ICON_PAUSE,                          /* spr_icon_pause.png, 12 x 12 */
  SPR_ICON_PLAY_BIG,                       /* spr_icon_play_big.png, 36 x 36 */
  SPR_ICON_PLAY_NORMALSPEED,               /* spr_icon_play_normalspeed.png, 12 x 12 */
  SPR_ICON_ROCKET,                         /* spr_icon_rocket.png, 12 x 12 */
  SPR_ICON_SCHOOL,                         /* spr_icon_school.png, 10 x 10 */
  SPR_ICON_SERVICES,                       /* spr_icon_services.png, 20 x 20 */
  SPR_ICON_SLIGHTLYSLOWERSPEED,            /* spr_icon_slightlyslowerspeed.png, 12 x 12 */
  SPR_ICON_SPECIALBUILDINGS,               /* spr_icon_specialbuildings.png, 20 x 20 */
  SPR_ICON_SPEEDUP,                        /* spr_icon_speedup.png, 12 x 12 */
  SPR_ICON_TRANSPORT,                      /* spr_icon_transport.png, 20 x 20 */
  SPR_ICON_UNLIMITED,                      /* spr_icon_unlimited.png, 13 x 13 */
  SPR_ICON_UPGRADE,                        /* spr_icon_upgrade.png, 20 x 20 */
  SPR_ICON_WORK,                           /* spr_icon_work.png, 20 x 20 */
  SPR_IMPROVEDPICKAXES,                    /* spr_improvedpickaxes.png, 20 x 20 */
  SPR_INDOORFARM,                          /* spr_indoorfarm.png, 64 x 20 */
  SPR_INDOORFARM_BUTTONBACK,               /* spr_indoorfarm_buttonback.png, 20 x 20 */
  SPR_INDOORFARM_CROPS,                    /* spr_indoorfarm_crops.png, 240 x 20 */
  SPR_INDOORPARK,                          /* spr_indoorpark.png, 328 x 20 */
  SPR_INDOORPARK_ALT1,                     /* spr_indoorpark_alt1.png, 328 x 20 */
  SPR_INSERTBUILDINGHERE,                  /* spr_insertbuildinghere.png, 20 x 21 */
  SPR_INTERGALACTICLIBRARY,                /* spr_intergalacticlibrary.png, 64 x 20 */
  SPR_LABESCAPEROOM,                       /* spr_labescaperoom.png, 64 x 20 */
  SPR_LABORATORY,                          /* spr_laboratory.png, 64 x 20 */
  SPR_LABORATORY_LIGHTSOFF,                /* spr_laboratory_lightsoff.png, 20 x 20 */
  SPR_LANDINGPLACE,                        /* spr_landingplace.png, 64 x 20 */
  SPR_LANDINGPLACE_ALT,                    /* spr_landingplace_alt.png, 64 x 20 */
  SPR_LANDINGPLACE_DEEPBLUE,               /* spr_landingplace_deepblue.png, 64 x 20 */
  SPR_LIBRARY,                             /* spr_library.png, 130 x 20 */
  SPR_LIVINGRESEARCHCENTER,                /* spr_livingresearchcenter.png, 64 x 20 */
  SPR_LOCKEDBUILDING,                      /* spr_lockedbuilding.png, 42 x 20 */
  SPR_LOFT,                                /* spr_loft.png, 64 x 20 */
  SPR_LOFT_FURNISHINGS,                    /* spr_loft_furnishings.png, 130 x 20 */
  SPR_LUXURYRESTAURANT,                    /* spr_luxuryrestaurant.png, 64 x 20 */
  SPR_MACHINEPARTSFACTORY,                 /* spr_machinepartsfactory.png, 64 x 20 */
  SPR_MACHINEPARTSFACTORY_FRAMES,          /* spr_machinepartsfactory_frames.png, 86 x 20 */
  SPR_MACHINEPARTSFACTORY_IDLE,            /* spr_machinepartsfactory_idle.png, 42 x 20 */
  SPR_MANSION,                             /* spr_mansion.png, 64 x 20 */
  SPR_MANSION_LEFT,                        /* spr_mansion_left.png, 64 x 20 */
  SPR_MANSION_RIGHT,                       /* spr_mansion_right.png, 64 x 20 */
  SPR_MECHANICALHOUSE,                     /* spr_mechanicalhouse.png, 64 x 20 */
  SPR_MECHANICALHOUSE_BOTTOM,              /* spr_mechanicalhouse_bottom.png, 64 x 20 */
  SPR_MECHANICALHOUSE_BOTTOM_BGANIM,       /* spr_mechanicalhouse_bottom_bganim.png, 174 x 20 */
  SPR_MECHANICALHOUSE_HEATEDBED,           /* spr_mechanicalhouse_heatedbed.png, 20 x 20 */
  SPR_MECHANICALHOUSE_LEFT,                /* spr_mechanicalhouse_left.png, 64 x 20 */
  SPR_MECHANICALHOUSE_LEFT_BGANIM,         /* spr_mechanicalhouse_left_bganim.png, 130 x 20 */
  SPR_MECHANICALHOUSE_LEFTRIGHT,           /* spr_mechanicalhouse_leftright.png, 64 x 20 */
  SPR_MECHANICALHOUSE_LEFTRIGHT_BGANIM,    /* spr_mechanicalhouse_leftright_bganim.png, 130 x 20 */
  SPR_MECHANICALHOUSE_PIPE_H,              /* spr_mechanicalhouse_pipe_h.png, 3 x 5 */
  SPR_MECHANICALHOUSE_PIPE_H2,             /* spr_mechanicalhouse_pipe_h2.png, 3 x 5 */
  SPR_MECHANICALHOUSE_PIPE_H2_BF,          /* spr_mechanicalhouse_pipe_h2_bf.png, 5 x 11 */
  SPR_MECHANICALHOUSE_PIPE_H3,             /* spr_mechanicalhouse_pipe_h3.png, 26 x 20 */
  SPR_MECHANICALHOUSE_PIPE_H3_BF,          /* spr_mechanicalhouse_pipe_h3_bf.png, 26 x 20 */
  SPR_MECHANICALHOUSE_PIPE_V1,             /* spr_mechanicalhouse_pipe_v1.png, 5 x 3 */
  SPR_MECHANICALHOUSE_PIPE_V2,             /* spr_mechanicalhouse_pipe_v2.png, 5 x 3 */
  SPR_MECHANICALHOUSE_PIPE_V3,             /* spr_mechanicalhouse_pipe_v3.png, 7 x 26 */
  SPR_MECHANICALHOUSE_RIGHT,               /* spr_mechanicalhouse_right.png, 64 x 20 */
  SPR_MECHANICALHOUSE_RIGHT_BGANIM,        /* spr_mechanicalhouse_right_bganim.png, 130 x 20 */
  SPR_MECHANICALHOUSE_TOP,                 /* spr_mechanicalhouse_top.png, 64 x 20 */
  SPR_MECHANICALHOUSE_TOP_BGANIM,          /* spr_mechanicalhouse_top_bganim.png, 42 x 20 */
  SPR_MEDICALCLINIC,                       /* spr_medicalclinic.png, 64 x 20 */
  SPR_MINIBUTTON,                          /* spr_minibutton.png, 31 x 7 */
  SPR_MIRROR,                              /* spr_mirror.png, 20 x 20 */
  SPR_MISDIRECTOR,                         /* spr_misdirector.png, 64 x 20 */
  SPR_MODERNART,                           /* spr_modernart.png, 218 x 20 */
  SPR_MODERNARTMUSEUM,                     /* spr_modernartmuseum.png, 64 x 20 */
  SPR_MODERNARTMUSEUM_BUTTONBACK,          /* spr_modernartmuseum_buttonback.png, 20 x 20 */
  SPR_MODERNCONVENIENCES,                  /* spr_modernconveniences.png, 20 x 20 */
  SPR_MODERNHOMEPLANT,                     /* spr_modernhomeplant.png, 218 x 20 */
  SPR_MODERNRUSTICHOME,                    /* spr_modernrustichome.png, 64 x 20 */
  SPR_MOREBUILDINGMODEBUTTONS,             /* spr_morebuildingmodebuttons.png, 40 x 22 */
  SPR_MOREBUILDINGMODES_ARROWUP,           /* spr_morebuildingmodes_arrowup.png, 10 x 20 */
  SPR_MORESETTINGS,                        /* spr_moresettings.png, 14 x 14 */
  SPR_NATURE,                              /* spr_nature.png, 20 x 20 */
  SPR_NIGHTCLUB,                           /* spr_nightclub.png, 130 x 20 */
  SPR_NIGHTCLUB_LASER,                     /* spr_nightclub_laser.png, 358 x 16 */
  SPR_NIGHTCLUB_LIGHT,                     /* spr_nightclub_light.png, 5 x 15 */
  SPR_NIGHTCLUB_LIGHT_RAINBOW,             /* spr_nightclub_light_rainbow.png, 5 x 15 */
  SPR_NORMALHOUSE,                         /* spr_normalhouse.png, 64 x 20 */
  SPR_OBSERVATORY,                         /* spr_observatory.png, 64 x 20 */
  SPR_ONEANDHALFSPEED,                     /* spr_oneandhalfspeed.png, 12 x 12 */
  SPR_PARK_BUTTONBG,                       /* spr_park_buttonbg.png, 20 x 20 */
  SPR_PARK_PLANTS,                         /* spr_park_plants.png, 284 x 20 */
  SPR_PARKPOD,                             /* spr_parkpod.png, 152 x 20 */
  SPR_PARKPOD_BUTTONBACK,                  /* spr_parkpod_buttonback.png, 20 x 20 */
  SPR_PINEFOREST,                          /* spr_pineforest.png, 262 x 20 */
  SPR_PINEFOREST_GROW,                     /* spr_pineforest_grow.png, 240 x 20 */
  SPR_PIXEL,                               /* spr_pixel.png, 1 x 1 */
  SPR_PLANET_RING_BIG,                     /* spr_planet_ring_big.png, 835 x 200 */
  SPR_PLANET_RING_SMALL,                   /* spr_planet_ring_small.png, 580 x 110 */
  SPR_PLANETS_BIG,                         /* spr_planets_big.png, 1018 x 100 */
  SPR_PLANETS_BIGGER,                      /* spr_planets_bigger.png, 730 x 120 */
  SPR_PLANETS_GIANT,                       /* spr_planets_giant.png, 566 x 140 */
  SPR_PLANETS_MEDIUM,                      /* spr_planets_medium.png, 654 x 80 */
  SPR_PLANETS_SMALL,                       /* spr_planets_small.png, 250 x 40 */
  SPR_POPULATION,                          /* spr_population.png, 10 x 10 */
  SPR_PROTESTSIGN_HAPPY,                   /* spr_protestsign_happy.png, 9 x 14 */
  SPR_PROTESTSIGN_SAD,                     /* spr_protestsign_sad.png, 9 x 14 */
  SPR_PUB,                                 /* spr_pub.png, 64 x 20 */
  SPR_PUB_MARKING_KEY,                     /* spr_pub_marking_key.png, 9 x 9 */
  SPR_PURPLEFOREST,                        /* spr_purpleforest.png, 284 x 20 */
  SPR_PURPLEFOREST_GROW,                   /* spr_purpleforest_grow.png, 218 x 20 */
  SPR_PURPLEGRASS,                         /* spr_purplegrass.png, 64 x 20 */
  SPR_RECOLORABLE,                         /* spr_recolorable.png, 22 x 22 */
  SPR_REDFOREST,                           /* spr_redforest.png, 262 x 20 */
  SPR_REDFOREST_GROW,                      /* spr_redforest_grow.png, 284 x 20 */
  SPR_REDGRASS,                            /* spr_redgrass.png, 64 x 20 */
  SPR_REDWOOD,                             /* spr_redwood.png, 262 x 20 */
  SPR_REDWOODTREEHOUSE,                    /* spr_redwoodtreehouse.png, 64 x 20 */
  SPR_REFINEDMETALFACTORY,                 /* spr_refinedmetalfactory.png, 64 x 20 */
  SPR_REFINEDMETALFACTORY_FRAMES,          /* spr_refinedmetalfactory_frames.png, 306 x 20 */
  SPR_REFINEDMETALFACTORY_IDLE,            /* spr_refinedmetalfactory_idle.png, 42 x 20 */
  SPR_REFINEDMETALFACTORYPUB,              /* spr_refinedmetalfactorypub.png, 130 x 20 */
  SPR_REFINEDMETALFACTORYPUB_FRAMES,       /* spr_refinedmetalfactorypub_frames.png, 482 x 20 */
  SPR_REFINEDMETALFACTORYPUB_IDLE,         /* spr_refinedmetalfactorypub_idle.png, 42 x 20 */
  SPR_REMOVEDECORATION,                    /* spr_removedecoration.png, 20 x 20 */
  SPR_REPLACEBUILDINGHERE,                 /* spr_replacebuildinghere.png, 20 x 21 */
  SPR_RESEARCHBOT,                         /* spr_researchbot.png, 20 x 20 */
  SPR_RESEARCHBOT_LANDINGPLACE,            /* spr_researchbot_landingplace.png, 64 x 20 */
  SPR_RESIDENTS,                           /* spr_residents.png, 8 x 10 */
  SPR_RESOURCE_COMPUTERCHIPS,              /* spr_resource_computerchips.png, 10 x 10 */
  SPR_RESOURCE_DOWNTREND,                  /* spr_resource_downtrend.png, 10 x 10 */
  SPR_RESOURCE_FOOD,                       /* spr_resource_food.png, 10 x 10 */
  SPR_RESOURCE_GRAPHENE,                   /* spr_resource_graphene.png, 10 x 10 */
  SPR_RESOURCE_KNOWLEDGE,                  /* spr_resource_knowledge.png, 10 x 10 */
  SPR_RESOURCE_MACHINEPARTS,               /* spr_resource_machineparts.png, 10 x 10 */
  SPR_RESOURCE_REFINEDMETAL,               /* spr_resource_refinedmetal.png, 10 x 10 */
  SPR_RESOURCE_ROCKETFUEL,                 /* spr_resource_rocketfuel.png, 10 x 10 */
  SPR_RESOURCE_STONE,                      /* spr_resource_stone.png, 10 x 10 */
  SPR_RESOURCE_WOOD,                       /* spr_resource_wood.png, 10 x 10 */
  SPR_RESTAURANT,                          /* spr_restaurant.png, 130 x 20 */
  SPR_ROCK,                                /* spr_rock.png, 152 x 20 */
  SPR_ROOFBOTTOM,                          /* spr_roofbottom.png, 438 x 20 */
  SPR_ROOFTOPPARK,                         /* spr_rooftoppark.png, 130 x 20 */
  SPR_ROOFTOPPARK_BUTTONBG,                /* spr_rooftoppark_buttonbg.png, 20 x 20 */
  SPR_ROOFTOPPARK_ONBUILD,                 /* spr_rooftoppark_onbuild.png, 20 x 20 */
  SPR_ROOFTOPPARK_PLANTS,                  /* spr_rooftoppark_plants.png, 130 x 20 */
  SPR_SCHOOL,                              /* spr_school.png, 64 x 20 */
  SPR_SCRAPYARDNIGHTCLUB,                  /* spr_scrapyardnightclub.png, 130 x 20 */
  SPR_SECRETSOCIETYHOUSE,                  /* spr_secretsocietyhouse.png, 64 x 20 */
  SPR_SECRETSOCIETYMACHINE,                /* spr_secretsocietymachine.png, 64 x 20 */
  SPR_SECRETSOCIETYMACHINE_ANIM,           /* spr_secretsocietymachine_anim.png, 548 x 20 */
  SPR_SELECTEDBUILDING,                    /* spr_selectedbuilding.png, 22 x 22 */
  SPR_SELECTEDFISH,                        /* spr_selectedfish.png, 10 x 10 */
  SPR_SELECTEDHUMAN,                       /* spr_selectedhuman.png, 4 x 7 */
  SPR_SHADYHOUSE,                          /* spr_shadyhouse.png, 64 x 20 */
  SPR_SLANTEDROOF,                         /* spr_slantedroof.png, 460 x 20 */
  SPR_SLANTEDROOFTOP,                      /* spr_slantedrooftop.png, 438 x 20 */
  SPR_SLIDER_BG,                           /* spr_slider_bg.png, 22 x 22 */
  SPR_SLIDER_FILL,                         /* spr_slider_fill.png, 22 x 22 */
  SPR_SMALLTELEPORTER_RAY,                 /* spr_smallteleporter_ray.png, 28 x 5 */
  SPR_SMALLTELEPORTER_RAY_PENTHOUSE,       /* spr_smallteleporter_ray_penthouse.png, 33 x 5 */
  SPR_SMALLTELEPORTER_RAY_PENTHOUSE_MIRROR, /* spr_smallteleporter_ray_penthouse_mirror.png, 33 x 5 */
  SPR_SMARTCHAIRS,                         /* spr_smartchairs.png, 20 x 20 */
  SPR_SMARTDIAGNOSE,                       /* spr_smartdiagnose.png, 20 x 20 */
  SPR_SMARTLIGHTS,                         /* spr_smartlights.png, 20 x 20 */
  SPR_SMARTSPEAKER,                        /* spr_smartspeaker.png, 20 x 20 */
  SPR_SOFTERBEDS,                          /* spr_softerbeds.png, 20 x 20 */
  SPR_SPACIOUSHOUSE,                       /* spr_spacioushouse.png, 64 x 20 */
  SPR_STATUEGARDEN_BUTTONBACK,             /* spr_statuegarden_buttonback.png, 20 x 20 */
  SPR_STATUEGARDEN_STATUES,                /* spr_statuegarden_statues.png, 174 x 20 */
  SPR_STATUEOFHAPPINESS,                   /* spr_statueofhappiness.png, 64 x 20 */
  SPR_STATUEOFHAPPINESS_BG,                /* spr_statueofhappiness_bg.png, 20 x 20 */
  SPR_STATUEOFTHEKEY,                      /* spr_statueofthekey.png, 64 x 20 */
  SPR_STATUEOFTHEKEY_BG,                   /* spr_statueofthekey_bg.png, 20 x 20 */
  SPR_STONEMINE,                           /* spr_stonemine.png, 64 x 20 */
  SPR_STONERESEARCHCENTRE,                 /* spr_stoneresearchcentre.png, 64 x 20 */
  SPR_STONERESEARCHCENTRE_COMPUTER,        /* spr_stoneresearchcentre_computer.png, 20 x 20 */
  SPR_STONERESEARCHCENTRE_PICKAXETECH,     /* spr_stoneresearchcentre_pickaxetech.png, 20 x 20 */
  SPR_STONETELEPORTER,                     /* spr_stoneteleporter.png, 64 x 20 */
  SPR_STONETELEPORTER_REMOTE,              /* spr_stoneteleporter_remote.png, 20 x 20 */
  SPR_STONETELEPORTER_STONE,               /* spr_stoneteleporter_stone.png, 196 x 20 */
  SPR_STONETELEPORTER_STONE_BIG,           /* spr_stoneteleporter_stone_big.png, 196 x 20 */
  SPR_SUPERCOMPUTER,                       /* spr_supercomputer.png, 64 x 20 */
  SPR_SYNIGHTCLUB_BG,                      /* spr_synightclub_bg.png, 20 x 20 */
  SPR_SYNIGHTCLUB_LEFTTEXTURES,            /* spr_synightclub_lefttextures.png, 64 x 19 */
  SPR_SYNIGHTCLUB_LIGHTS,                  /* spr_synightclub_lights.png, 1 x 10 */
  SPR_SYNIGHTCLUB_LIGHTS_H,                /* spr_synightclub_lights_h.png, 10 x 1 */
  SPR_SYNIGHTCLUB_MAINTEXTURES,            /* spr_synightclub_maintextures.png, 64 x 19 */
  SPR_SYNIGHTCLUB_MINILIGHT,               /* spr_synightclub_minilight.png, 3 x 3 */
  SPR_SYNIGHTCLUB_WATERFALLLIGHT,          /* spr_synightclub_waterfalllight.png, 7 x 2 */
  SPR_SYNIGHTCLUB_WATERFALLLIGHT_LAST,     /* spr_synightclub_waterfalllight_last.png, 5 x 6 */
  SPR_TECH_NIGHTCLUB,                      /* spr_tech_nightclub.png, 328 x 20 */
  SPR_TECH_NIGHTCLUB_HOVERER,              /* spr_tech_nightclub_hoverer.png, 4 x 4 */
  SPR_TELEPORTER,                          /* spr_teleporter.png, 64 x 20 */
  SPR_TELEPORTER_RAY,                      /* spr_teleporter_ray.png, 34 x 15 */
  SPR_TELEPORTER_RAY_KEY,                  /* spr_teleporter_ray_key.png, 46 x 15 */
  SPR_TELEPORTER_RAY_MISDIRECT,            /* spr_teleporter_ray_misdirect.png, 34 x 15 */
  SPR_TELESCOPE,                           /* spr_telescope.png, 4 x 5 */
  SPR_TINKERERSHOUSE,                      /* spr_tinkerershouse.png, 64 x 20 */
  SPR_TINKERERSHOUSE_BACK_ALT,             /* spr_tinkerershouse_back_alt.png, 108 x 20 */
  SPR_TINKERERSHOUSE_BETTERTOOLS,          /* spr_tinkerershouse_bettertools.png, 86 x 20 */
  SPR_TINKERERSHOUSE_FRONT_ALT,            /* spr_tinkerershouse_front_alt.png, 196 x 20 */
  SPR_TITLE,                               /* spr_title.png, 261 x 55 */
  SPR_TRANSPARENTBUTTON,                   /* spr_transparentbutton.png, 70 x 22 */
  SPR_TRANSPARENTBUTTON_INFO,              /* spr_transparentbutton_info.png, 70 x 22 */
  SPR_TRANSPARENTBUTTON_INFO_MINI,         /* spr_transparentbutton_info_mini.png, 46 x 14 */
  SPR_TRANSPARENTBUTTON_INFO_MINIPLUS,     /* spr_transparentbutton_info_miniplus.png, 64 x 20 */
  SPR_TREEGROWTHBOOST,                     /* spr_treegrowthboost.png, 20 x 20 */
  SPR_TREEHUGGERBASE,                      /* spr_treehuggerbase.png, 20 x 20 */
  SPR_TREEPLANTATION,                      /* spr_treeplantation.png, 64 x 20 */
  SPR_TREEPLANTATION_BUTTONBACK,           /* spr_treeplantation_buttonback.png, 20 x 20 */
  SPR_TREEPLANTATION_TREES,                /* spr_treeplantation_trees.png, 262 x 20 */
  SPR_TREEPLANTATION_TREES_GROW,           /* spr_treeplantation_trees_grow.png, 240 x 20 */
  SPR_TUTORIALARROWDOWN,                   /* spr_tutorialarrowdown.png, 20 x 30 */
  SPR_TUTORIALARROWLEFT,                   /* spr_tutorialarrowleft.png, 30 x 20 */
  SPR_UNIQUEBUILDING,                      /* spr_uniquebuilding.png, 10 x 10 */
  SPR_UNKNOWNBUILDING,                     /* spr_unknownbuilding.png, 64 x 20 */
  SPR_UPGRADE_FARMBOOST,                   /* spr_upgrade_farmboost.png, 20 x 20 */
  SPR_UPGRADE_FLOWERS,                     /* spr_upgrade_flowers.png, 64 x 20 */
  SPR_VILLA,                               /* spr_villa.png, 64 x 20 */
  SPR_WALDO_ADULT,                         /* spr_waldo_adult.png, 2 x 5 */
  SPR_WALDO_CHILD,                         /* spr_waldo_child.png, 2 x 5 */
  SPR_WHITEOUTLINE,                        /* spr_whiteoutline.png, 22 x 22 */
  SPR_WINDOWPARTS,                         /* spr_windowparts.png, 62 x 24 */
  SPR_WINDOWPARTS_PURPLE,                  /* spr_windowparts_purple.png, 62 x 24 */
  SPR_WOODCUTTINGCENTRE,                   /* spr_woodcuttingcentre.png, 64 x 20 */
  SPR_WORK,                                /* spr_work.png, 10 x 10 */
  SPR_WORK_HOUSING,                        /* spr_work_housing.png, 21 x 10 */
  SPR_WORKSHOP,                            /* spr_workshop.png, 64 x 20 */
  SPR_WORKSHOP_MAKEMACHINEPARTS,           /* spr_workshop_makemachineparts.png, 20 x 20 */
  SPR_WORKSHOP_PETPROJECTS,                /* spr_workshop_petprojects.png, 20 x 20 */
  SPR_COUNT
};

/* each sprite's width and height */
#define SPR_9P_BUTTON_ACTIVE_W 22
#define SPR_9P_BUTTON_ACTIVE_H 22
#define SPR_9P_BUTTON_DISABLED_W 22
#define SPR_9P_BUTTON_DISABLED_H 22
#define SPR_9P_HINTINFO_W 60
#define SPR_9P_HINTINFO_H 60
#define SPR_9P_INFO_W 32
#define SPR_9P_INFO_H 32
#define SPR_9P_TOOLTIP_W 10
#define SPR_9P_TOOLTIP_H 10
#define SPR_9P_WINDOW_W 60
#define SPR_9P_WINDOW_H 60
#define SPR_9P_WINDOW_MOREOPAQUE_W 60
#define SPR_9P_WINDOW_MOREOPAQUE_H 60
#define SPR_9P_WINDOW_PURPLE_W 60
#define SPR_9P_WINDOW_PURPLE_H 60
#define SPR_ADVANCEDMEDICALCLINIC_W 64
#define SPR_ADVANCEDMEDICALCLINIC_H 20
#define SPR_AIMINING_W 20
#define SPR_AIMINING_H 20
#define SPR_ALIENDECRYPTOR_W 64
#define SPR_ALIENDECRYPTOR_H 20
#define SPR_ALIENHOUSE_W 64
#define SPR_ALIENHOUSE_H 20
#define SPR_ALIENHOUSE_LIVINGCOMPUTER_W 152
#define SPR_ALIENHOUSE_LIVINGCOMPUTER_H 20
#define SPR_ALIENRUINS_W 20
#define SPR_ALIENRUINS_H 20
#define SPR_ALIENRUINS_2_W 20
#define SPR_ALIENRUINS_2_H 20
#define SPR_ALIENRUINS_3_W 20
#define SPR_ALIENRUINS_3_H 20
#define SPR_ARCADE_W 64
#define SPR_ARCADE_H 20
#define SPR_ARROW_BACK_W 20
#define SPR_ARROW_BACK_H 20
#define SPR_ARROWDOWN_W 7
#define SPR_ARROWDOWN_H 5
#define SPR_ARROWDOWN_BIG_W 16
#define SPR_ARROWDOWN_BIG_H 16
#define SPR_ARROWUP_W 7
#define SPR_ARROWUP_H 5
#define SPR_ARROWUP_BIG_W 16
#define SPR_ARROWUP_BIG_H 16
#define SPR_ARTCOLONY_W 64
#define SPR_ARTCOLONY_H 20
#define SPR_ARTCOLONY_BUTTONBACK_W 20
#define SPR_ARTCOLONY_BUTTONBACK_H 20
#define SPR_ARTCOLONYWORK_W 306
#define SPR_ARTCOLONYWORK_H 20
#define SPR_BEACON_W 64
#define SPR_BEACON_H 20
#define SPR_BEACON_LIGHT_W 4
#define SPR_BEACON_LIGHT_H 11
#define SPR_BIGGERTV_W 20
#define SPR_BIGGERTV_H 20
#define SPR_BIRCHFOREST_W 20
#define SPR_BIRCHFOREST_H 20
#define SPR_BLOOMHOUSE_W 108
#define SPR_BLOOMHOUSE_H 20
#define SPR_BLOOMHUT_BUTTONBACK_W 20
#define SPR_BLOOMHUT_BUTTONBACK_H 20
#define SPR_BLOOMRESTAURANT_W 328
#define SPR_BLOOMRESTAURANT_H 20
#define SPR_BLOOMRESTAURANT_FOREGROUND_W 108
#define SPR_BLOOMRESTAURANT_FOREGROUND_H 20
#define SPR_BLOSSOMHIPPIEHQ_W 64
#define SPR_BLOSSOMHIPPIEHQ_H 20
#define SPR_BLUEWORLDGLOW_W 20
#define SPR_BLUEWORLDGLOW_H 5
#define SPR_BOARDGAMESTORE_W 64
#define SPR_BOARDGAMESTORE_H 20
#define SPR_BONUSRANGE_PLUS_W 9
#define SPR_BONUSRANGE_PLUS_H 10
#define SPR_BONUSRANGE_TOPBOTTOM_W 9
#define SPR_BONUSRANGE_TOPBOTTOM_H 10
#define SPR_BOTANICALGARDEN_DOME_BUTTONBACK_W 20
#define SPR_BOTANICALGARDEN_DOME_BUTTONBACK_H 20
#define SPR_BOTANICALGARDENS_W 328
#define SPR_BOTANICALGARDENS_H 20
#define SPR_BOTANICALGARDENS_BUTTONBACK_W 20
#define SPR_BOTANICALGARDENS_BUTTONBACK_H 20
#define SPR_BOTANICALGARDENS_PLANTS_W 300
#define SPR_BOTANICALGARDENS_PLANTS_H 80
#define SPR_BOULDER_W 164
#define SPR_BOULDER_H 20
#define SPR_BUILDHEREOUTLINE_W 20
#define SPR_BUILDHEREOUTLINE_H 20
#define SPR_BUILDINGBUTTON_W 70
#define SPR_BUILDINGBUTTON_H 22
#define SPR_BUTTON_W 70
#define SPR_BUTTON_H 22
#define SPR_BUTTON_ACTIVABLE_W 70
#define SPR_BUTTON_ACTIVABLE_H 22
#define SPR_BUTTON_CHECKED_W 70
#define SPR_BUTTON_CHECKED_H 22
#define SPR_BUTTON_GRAY_W 70
#define SPR_BUTTON_GRAY_H 22
#define SPR_BUTTON_MEDIUM_W 70
#define SPR_BUTTON_MEDIUM_H 22
#define SPR_BUTTON_SMALL_W 58
#define SPR_BUTTON_SMALL_H 18
#define SPR_BUTTON_SMALLER_W 49
#define SPR_BUTTON_SMALLER_H 15
#define SPR_BUTTON_SMALLER_MOBILE_W 61
#define SPR_BUTTON_SMALLER_MOBILE_H 19
#define SPR_BUTTON_WINDOWHEADER_W 73
#define SPR_BUTTON_WINDOWHEADER_H 17
#define SPR_BUTTON_WINDOWHEADER_MOBI_W 79
#define SPR_BUTTON_WINDOWHEADER_MOBI_H 19
#define SPR_BUTTONNOTIFICATION_W 22
#define SPR_BUTTONNOTIFICATION_H 22
#define SPR_CACAOFARM_W 64
#define SPR_CACAOFARM_H 20
#define SPR_CACAOFARM_TREES_W 152
#define SPR_CACAOFARM_TREES_H 20
#define SPR_CAMERA_W 14
#define SPR_CAMERA_H 14
#define SPR_CARROTJUICEBAR_W 64
#define SPR_CARROTJUICEBAR_H 20
#define SPR_CHECKBOX_W 18
#define SPR_CHECKBOX_H 8
#define SPR_CHILDREN_W 202
#define SPR_CHILDREN_H 5
#define SPR_CHOCOLATEFACTORY_W 64
#define SPR_CHOCOLATEFACTORY_H 20
#define SPR_CHOCOLATEFACTORY_FRAMES_W 240
#define SPR_CHOCOLATEFACTORY_FRAMES_H 20
#define SPR_CHOCOLATEFACTORY_IDLE_W 20
#define SPR_CHOCOLATEFACTORY_IDLE_H 20
#define SPR_CITIZEN_BALL_W 4
#define SPR_CITIZEN_BALL_H 4
#define SPR_CITYHALL_W 64
#define SPR_CITYHALL_H 20
#define SPR_CLOUDS_W 990
#define SPR_CLOUDS_H 122
#define SPR_COLORCIRCLE_W 200
#define SPR_COLORCIRCLE_H 223
#define SPR_COLOREDBLOCK_W 438
#define SPR_COLOREDBLOCK_H 20
#define SPR_COLOREDDOORS_W 130
#define SPR_COLOREDDOORS_H 20
#define SPR_COLORPICKER_CIRCLE_W 12
#define SPR_COLORPICKER_CIRCLE_H 12
#define SPR_COLORPICKER_COLORPICKED_W 10
#define SPR_COLORPICKER_COLORPICKED_H 10
#define SPR_COLORPICKER_TRIANGLE_W 12
#define SPR_COLORPICKER_TRIANGLE_H 12
#define SPR_COMFORTABLECOUCH_W 20
#define SPR_COMFORTABLECOUCH_H 20
#define SPR_COMFORTABLEHOUSE_W 64
#define SPR_COMFORTABLEHOUSE_H 20
#define SPR_COMMUNITYHOUSE_BUTTONBACK_W 20
#define SPR_COMMUNITYHOUSE_BUTTONBACK_H 20
#define SPR_COMMUNITYHOUSE_CROP_W 68
#define SPR_COMMUNITYHOUSE_CROP_H 14
#define SPR_COMMUNITYHOUSE_INDOORPOND_W 20
#define SPR_COMMUNITYHOUSE_INDOORPOND_H 20
#define SPR_COMMUNITYHOUSE_ROCK_W 61
#define SPR_COMMUNITYHOUSE_ROCK_H 14
#define SPR_COMMUNITYHOUSE_TREE_W 47
#define SPR_COMMUNITYHOUSE_TREE_H 14
#define SPR_COMMUNITYHOUSE_TREE_GROW_W 75
#define SPR_COMMUNITYHOUSE_TREE_GROW_H 14
#define SPR_COMPUTER_OBSERVATORY_W 20
#define SPR_COMPUTER_OBSERVATORY_H 20
#define SPR_COMPUTERCHIPFACTORY_W 64
#define SPR_COMPUTERCHIPFACTORY_H 20
#define SPR_COMPUTERCHIPFACTORY_FRAMES_W 526
#define SPR_COMPUTERCHIPFACTORY_FRAMES_H 20
#define SPR_COMPUTERCHIPFACTORY_IDLE_W 20
#define SPR_COMPUTERCHIPFACTORY_IDLE_H 20
#define SPR_COMPUTERRESEARCHCENTER_W 64
#define SPR_COMPUTERRESEARCHCENTER_H 20
#define SPR_COMPUTERRESEARCHCENTER_FRAMES_W 372
#define SPR_COMPUTERRESEARCHCENTER_FRAMES_H 20
#define SPR_COMPUTERRESEARCHCENTER_IDLE_W 20
#define SPR_COMPUTERRESEARCHCENTER_IDLE_H 20
#define SPR_CONTRAPTION_W 64
#define SPR_CONTRAPTION_H 20
#define SPR_CREDITS_W 14
#define SPR_CREDITS_H 14
#define SPR_CUTTINGEDGEHOME_W 64
#define SPR_CUTTINGEDGEHOME_H 20
#define SPR_DECORATION_KEYCITY_W 86
#define SPR_DECORATION_KEYCITY_H 20
#define SPR_DECORATION_ROCKS_W 64
#define SPR_DECORATION_ROCKS_H 20
#define SPR_DESTROYING_W 20
#define SPR_DESTROYING_H 20
#define SPR_DIESHRINK_W 20
#define SPR_DIESHRINK_H 20
#define SPR_DOME_BOTANICALGARDEN_W 130
#define SPR_DOME_BOTANICALGARDEN_H 20
#define SPR_DOME_BOTANICALGARDEN_ALT1_W 130
#define SPR_DOME_BOTANICALGARDEN_ALT1_H 20
#define SPR_DOME_BOTANICALGARDEN_ALT1B_W 130
#define SPR_DOME_BOTANICALGARDEN_ALT1B_H 20
#define SPR_DOME_BOTANICALGARDEN_ALT2_W 130
#define SPR_DOME_BOTANICALGARDEN_ALT2_H 20
#define SPR_DOME_BOTANICALGARDEN_ALT2B_W 130
#define SPR_DOME_BOTANICALGARDEN_ALT2B_H 20
#define SPR_DOME_BOTANICALGARDEN_B_W 130
#define SPR_DOME_BOTANICALGARDEN_B_H 20
#define SPR_DOME_BOTANICALGARDEN_PLANTS_W 300
#define SPR_DOME_BOTANICALGARDEN_PLANTS_H 40
#define SPR_DOMEROOF_W 460
#define SPR_DOMEROOF_H 20
#define SPR_DRACULA_ADULT_W 2
#define SPR_DRACULA_ADULT_H 5
#define SPR_DRACULA_CHILD_W 2
#define SPR_DRACULA_CHILD_H 5
#define SPR_ECOFARM_W 64
#define SPR_ECOFARM_H 20
#define SPR_ECOFARM_BUTTONBACK_W 20
#define SPR_ECOFARM_BUTTONBACK_H 20
#define SPR_ECOFARM_CROP1_W 94
#define SPR_ECOFARM_CROP1_H 14
#define SPR_ECOFARM_CROP2_W 68
#define SPR_ECOFARM_CROP2_H 5
#define SPR_ECOFARM_CROP2_ALT_W 58
#define SPR_ECOFARM_CROP2_ALT_H 5
#define SPR_ECOFARM_CROP2_ALT2_W 78
#define SPR_ECOFARM_CROP2_ALT2_H 5
#define SPR_ECOFARM_CROP3_W 78
#define SPR_ECOFARM_CROP3_H 5
#define SPR_ECOFARM_CROP3_ALT_W 58
#define SPR_ECOFARM_CROP3_ALT_H 5
#define SPR_EXPERIMENTALFARM_W 64
#define SPR_EXPERIMENTALFARM_H 20
#define SPR_EXPERIMENTALFARM_BUTTONBACK_W 20
#define SPR_EXPERIMENTALFARM_BUTTONBACK_H 20
#define SPR_EXPERIMENTALFARM_CROP1_W 75
#define SPR_EXPERIMENTALFARM_CROP1_H 14
#define SPR_EXPERIMENTALFARM_CROP2_W 61
#define SPR_EXPERIMENTALFARM_CROP2_H 5
#define SPR_EXPERIMENTALFARM_CROP3_W 33
#define SPR_EXPERIMENTALFARM_CROP3_H 5
#define SPR_EXPERIMENTATIONLAB_W 64
#define SPR_EXPERIMENTATIONLAB_H 20
#define SPR_EXPERIMENTATIONLAB_IDLE_W 42
#define SPR_EXPERIMENTATIONLAB_IDLE_H 20
#define SPR_EXPERIMENTATIONLAB_PIPE_BOTTOM_W 5
#define SPR_EXPERIMENTATIONLAB_PIPE_BOTTOM_H 9
#define SPR_EXPERIMENTATIONLAB_PIPE_TOP_W 5
#define SPR_EXPERIMENTATIONLAB_PIPE_TOP_H 14
#define SPR_EXPERIMENTATIONLAB_SCREWDRIVER_W 20
#define SPR_EXPERIMENTATIONLAB_SCREWDRIVER_H 20
#define SPR_EXPLORATIONCENTRE_W 64
#define SPR_EXPLORATIONCENTRE_H 20
#define SPR_EXPLORATIONSHIP_W 64
#define SPR_EXPLORATIONSHIP_H 20
#define SPR_FARMBYPRODUCTPROCESSOR_W 64
#define SPR_FARMBYPRODUCTPROCESSOR_H 20
#define SPR_FARMBYPRODUCTPROCESSOR_FRAMES_W 988
#define SPR_FARMBYPRODUCTPROCESSOR_FRAMES_H 20
#define SPR_FARMBYPRODUCTPROCESSOR_IDLE_W 20
#define SPR_FARMBYPRODUCTPROCESSOR_IDLE_H 20
#define SPR_FARMHOUSE_W 64
#define SPR_FARMHOUSE_H 20
#define SPR_FEATHERALLIANCE_W 64
#define SPR_FEATHERALLIANCE_H 20
#define SPR_FESTIVAL_PODIUM_SMALL_W 5
#define SPR_FESTIVAL_PODIUM_SMALL_H 4
#define SPR_FESTIVALHQ_W 64
#define SPR_FESTIVALHQ_H 20
#define SPR_FISH_W 18
#define SPR_FISH_H 1
#define SPR_FISH_BIG_W 16
#define SPR_FISH_BIG_H 3
#define SPR_FISH_BIG2_W 5
#define SPR_FISH_BIG2_H 3
#define SPR_FISH_JELLY_W 5
#define SPR_FISH_JELLY_H 5
#define SPR_FISH_SNAKE_W 8
#define SPR_FISH_SNAKE_H 2
#define SPR_FLOATINGPLATFORM_W 20
#define SPR_FLOATINGPLATFORM_H 20
#define SPR_FLOWER_UP_W 14
#define SPR_FLOWER_UP_H 3
#define SPR_FLOWERCRAFT_W 20
#define SPR_FLOWERCRAFT_H 20
#define SPR_FLOWERPENTHOUSE_W 64
#define SPR_FLOWERPENTHOUSE_H 20
#define SPR_FLOWERS_W 86
#define SPR_FLOWERS_H 20
#define SPR_FLOWERS_BLUE_W 64
#define SPR_FLOWERS_BLUE_H 20
#define SPR_FLOWERS_KEYCITY_W 86
#define SPR_FLOWERS_KEYCITY_H 20
#define SPR_FLYINGSAUCER_W 20
#define SPR_FLYINGSAUCER_H 20
#define SPR_FLYINGSAUCER_ALT_W 20
#define SPR_FLYINGSAUCER_ALT_H 20
#define SPR_FLYINGSAUCER_CUBE_W 20
#define SPR_FLYINGSAUCER_CUBE_H 20
#define SPR_FLYINGSAUCER_DEEPBLUE_W 20
#define SPR_FLYINGSAUCER_DEEPBLUE_H 20
#define SPR_FLYINGSAUCER_SHADE_W 20
#define SPR_FLYINGSAUCER_SHADE_H 20
#define SPR_FORCEDBUILDINGLOCATION_W 22
#define SPR_FORCEDBUILDINGLOCATION_H 52
#define SPR_FOREST_W 262
#define SPR_FOREST_H 20
#define SPR_FOREST_BRANCHES_W 262
#define SPR_FOREST_BRANCHES_H 20
#define SPR_FOREST_BRANCHES_GROW_W 284
#define SPR_FOREST_BRANCHES_GROW_H 20
#define SPR_FOREST_GROW_W 284
#define SPR_FOREST_GROW_H 20
#define SPR_FOSSILMUSEUM_W 64
#define SPR_FOSSILMUSEUM_H 20
#define SPR_FROG_W 16
#define SPR_FROG_H 7
#define SPR_FULLSCREEN_W 14
#define SPR_FULLSCREEN_H 14
#define SPR_GRASS_W 64
#define SPR_GRASS_H 20
#define SPR_GREENOUTLINE_W 22
#define SPR_GREENOUTLINE_H 22
#define SPR_HACKERHQ_W 64
#define SPR_HACKERHQ_H 20
#define SPR_HACKERS_BOLT_W 4
#define SPR_HACKERS_BOLT_H 7
#define SPR_HAPPINESS_W 10
#define SPR_HAPPINESS_H 10
#define SPR_HERBGARDEN_W 196
#define SPR_HERBGARDEN_H 20
#define SPR_HERBGARDEN_BUTTONBACK_W 20
#define SPR_HERBGARDEN_BUTTONBACK_H 20
#define SPR_HERBGARDEN_MEDIC_W 196
#define SPR_HERBGARDEN_MEDIC_H 20
#define SPR_HERBGARDEN_PLANTS_W 130
#define SPR_HERBGARDEN_PLANTS_H 20
#define SPR_HGH_BLOCK_W 4
#define SPR_HGH_BLOCK_H 4
#define SPR_HGH_BOOSTER_W 9
#define SPR_HGH_BOOSTER_H 8
#define SPR_HGH_CITIZENCLOTHES_1_W 2
#define SPR_HGH_CITIZENCLOTHES_1_H 5
#define SPR_HGH_MONSTER_1_W 14
#define SPR_HGH_MONSTER_1_H 9
#define SPR_HIDEUI_W 26
#define SPR_HIDEUI_H 12
#define SPR_HINTINFO_ARROW_W 10
#define SPR_HINTINFO_ARROW_H 6
#define SPR_HOLOGAMEHALL_W 130
#define SPR_HOLOGAMEHALL_H 20
#define SPR_HOUSEOFTHEKEYFRIENDS_W 64
#define SPR_HOUSEOFTHEKEYFRIENDS_H 20
#define SPR_HOUSING_W 10
#define SPR_HOUSING_H 10
#define SPR_HOVERHINT_W 70
#define SPR_HOVERHINT_H 22
#define SPR_HOVERHINTS_CITIZEN_W 94
#define SPR_HOVERHINTS_CITIZEN_H 22
#define SPR_HUMANS_W 202
#define SPR_HUMANS_H 5
#define SPR_HYPERELEVATOR_W 64
#define SPR_HYPERELEVATOR_H 20
#define SPR_HYPERELEVATOR_DOOR_W 46
#define SPR_HYPERELEVATOR_DOOR_H 10
#define SPR_ICON_BOOST_W 10
#define SPR_ICON_BOOST_H 14
#define SPR_ICON_BUILD_W 20
#define SPR_ICON_BUILD_H 20
#define SPR_ICON_BUILDINGMODE_DESTROY_W 20
#define SPR_ICON_BUILDINGMODE_DESTROY_H 20
#define SPR_ICON_BUILDINGMODE_DESTROYLEAVINGHOLE_W 20
#define SPR_ICON_BUILDINGMODE_DESTROYLEAVINGHOLE_H 20
#define SPR_ICON_BUILDINGMODE_DRAG_W 20
#define SPR_ICON_BUILDINGMODE_DRAG_H 20
#define SPR_ICON_BUILDINGMODE_DRAGREPLACE_W 20
#define SPR_ICON_BUILDINGMODE_DRAGREPLACE_H 20
#define SPR_ICON_BUILDINGMODE_INSERT_W 20
#define SPR_ICON_BUILDINGMODE_INSERT_H 20
#define SPR_ICON_BUILDINGMODE_ONTOP_W 20
#define SPR_ICON_BUILDINGMODE_ONTOP_H 20
#define SPR_ICON_BUILDINGMODE_REPLACE_W 20
#define SPR_ICON_BUILDINGMODE_REPLACE_H 20
#define SPR_ICON_CLOSE_W 21
#define SPR_ICON_CLOSE_H 15
#define SPR_ICON_ENTERTAINMENT_W 20
#define SPR_ICON_ENTERTAINMENT_H 20
#define SPR_ICON_FINDBUILDING_W 20
#define SPR_ICON_FINDBUILDING_H 20
#define SPR_ICON_FOLLOWCITIZEN_W 20
#define SPR_ICON_FOLLOWCITIZEN_H 20
#define SPR_ICON_HALFSPEED_W 12
#define SPR_ICON_HALFSPEED_H 12
#define SPR_ICON_HELP_W 12
#define SPR_ICON_HELP_H 12
#define SPR_ICON_HOUSES_W 20
#define SPR_ICON_HOUSES_H 20
#define SPR_ICON_INVITEPEOPLE_W 20
#define SPR_ICON_INVITEPEOPLE_H 20
#define SPR_ICON_MANAGEMENT_W 20
#define SPR_ICON_MANAGEMENT_H 20
#define SPR_ICON_MENU_W 12
#define SPR_ICON_MENU_H 12
#define SPR_ICON_PAUSE_W 12
#define SPR_ICON_PAUSE_H 12
#define SPR_ICON_PLAY_BIG_W 36
#define SPR_ICON_PLAY_BIG_H 36
#define SPR_ICON_PLAY_NORMALSPEED_W 12
#define SPR_ICON_PLAY_NORMALSPEED_H 12
#define SPR_ICON_ROCKET_W 12
#define SPR_ICON_ROCKET_H 12
#define SPR_ICON_SCHOOL_W 10
#define SPR_ICON_SCHOOL_H 10
#define SPR_ICON_SERVICES_W 20
#define SPR_ICON_SERVICES_H 20
#define SPR_ICON_SLIGHTLYSLOWERSPEED_W 12
#define SPR_ICON_SLIGHTLYSLOWERSPEED_H 12
#define SPR_ICON_SPECIALBUILDINGS_W 20
#define SPR_ICON_SPECIALBUILDINGS_H 20
#define SPR_ICON_SPEEDUP_W 12
#define SPR_ICON_SPEEDUP_H 12
#define SPR_ICON_TRANSPORT_W 20
#define SPR_ICON_TRANSPORT_H 20
#define SPR_ICON_UNLIMITED_W 13
#define SPR_ICON_UNLIMITED_H 13
#define SPR_ICON_UPGRADE_W 20
#define SPR_ICON_UPGRADE_H 20
#define SPR_ICON_WORK_W 20
#define SPR_ICON_WORK_H 20
#define SPR_IMPROVEDPICKAXES_W 20
#define SPR_IMPROVEDPICKAXES_H 20
#define SPR_INDOORFARM_W 64
#define SPR_INDOORFARM_H 20
#define SPR_INDOORFARM_BUTTONBACK_W 20
#define SPR_INDOORFARM_BUTTONBACK_H 20
#define SPR_INDOORFARM_CROPS_W 240
#define SPR_INDOORFARM_CROPS_H 20
#define SPR_INDOORPARK_W 328
#define SPR_INDOORPARK_H 20
#define SPR_INDOORPARK_ALT1_W 328
#define SPR_INDOORPARK_ALT1_H 20
#define SPR_INSERTBUILDINGHERE_W 20
#define SPR_INSERTBUILDINGHERE_H 21
#define SPR_INTERGALACTICLIBRARY_W 64
#define SPR_INTERGALACTICLIBRARY_H 20
#define SPR_LABESCAPEROOM_W 64
#define SPR_LABESCAPEROOM_H 20
#define SPR_LABORATORY_W 64
#define SPR_LABORATORY_H 20
#define SPR_LABORATORY_LIGHTSOFF_W 20
#define SPR_LABORATORY_LIGHTSOFF_H 20
#define SPR_LANDINGPLACE_W 64
#define SPR_LANDINGPLACE_H 20
#define SPR_LANDINGPLACE_ALT_W 64
#define SPR_LANDINGPLACE_ALT_H 20
#define SPR_LANDINGPLACE_DEEPBLUE_W 64
#define SPR_LANDINGPLACE_DEEPBLUE_H 20
#define SPR_LIBRARY_W 130
#define SPR_LIBRARY_H 20
#define SPR_LIVINGRESEARCHCENTER_W 64
#define SPR_LIVINGRESEARCHCENTER_H 20
#define SPR_LOCKEDBUILDING_W 42
#define SPR_LOCKEDBUILDING_H 20
#define SPR_LOFT_W 64
#define SPR_LOFT_H 20
#define SPR_LOFT_FURNISHINGS_W 130
#define SPR_LOFT_FURNISHINGS_H 20
#define SPR_LUXURYRESTAURANT_W 64
#define SPR_LUXURYRESTAURANT_H 20
#define SPR_MACHINEPARTSFACTORY_W 64
#define SPR_MACHINEPARTSFACTORY_H 20
#define SPR_MACHINEPARTSFACTORY_FRAMES_W 86
#define SPR_MACHINEPARTSFACTORY_FRAMES_H 20
#define SPR_MACHINEPARTSFACTORY_IDLE_W 42
#define SPR_MACHINEPARTSFACTORY_IDLE_H 20
#define SPR_MANSION_W 64
#define SPR_MANSION_H 20
#define SPR_MANSION_LEFT_W 64
#define SPR_MANSION_LEFT_H 20
#define SPR_MANSION_RIGHT_W 64
#define SPR_MANSION_RIGHT_H 20
#define SPR_MECHANICALHOUSE_W 64
#define SPR_MECHANICALHOUSE_H 20
#define SPR_MECHANICALHOUSE_BOTTOM_W 64
#define SPR_MECHANICALHOUSE_BOTTOM_H 20
#define SPR_MECHANICALHOUSE_BOTTOM_BGANIM_W 174
#define SPR_MECHANICALHOUSE_BOTTOM_BGANIM_H 20
#define SPR_MECHANICALHOUSE_HEATEDBED_W 20
#define SPR_MECHANICALHOUSE_HEATEDBED_H 20
#define SPR_MECHANICALHOUSE_LEFT_W 64
#define SPR_MECHANICALHOUSE_LEFT_H 20
#define SPR_MECHANICALHOUSE_LEFT_BGANIM_W 130
#define SPR_MECHANICALHOUSE_LEFT_BGANIM_H 20
#define SPR_MECHANICALHOUSE_LEFTRIGHT_W 64
#define SPR_MECHANICALHOUSE_LEFTRIGHT_H 20
#define SPR_MECHANICALHOUSE_LEFTRIGHT_BGANIM_W 130
#define SPR_MECHANICALHOUSE_LEFTRIGHT_BGANIM_H 20
#define SPR_MECHANICALHOUSE_PIPE_H_W 3
#define SPR_MECHANICALHOUSE_PIPE_H_H 5
#define SPR_MECHANICALHOUSE_PIPE_H2_W 3
#define SPR_MECHANICALHOUSE_PIPE_H2_H 5
#define SPR_MECHANICALHOUSE_PIPE_H2_BF_W 5
#define SPR_MECHANICALHOUSE_PIPE_H2_BF_H 11
#define SPR_MECHANICALHOUSE_PIPE_H3_W 26
#define SPR_MECHANICALHOUSE_PIPE_H3_H 20
#define SPR_MECHANICALHOUSE_PIPE_H3_BF_W 26
#define SPR_MECHANICALHOUSE_PIPE_H3_BF_H 20
#define SPR_MECHANICALHOUSE_PIPE_V1_W 5
#define SPR_MECHANICALHOUSE_PIPE_V1_H 3
#define SPR_MECHANICALHOUSE_PIPE_V2_W 5
#define SPR_MECHANICALHOUSE_PIPE_V2_H 3
#define SPR_MECHANICALHOUSE_PIPE_V3_W 7
#define SPR_MECHANICALHOUSE_PIPE_V3_H 26
#define SPR_MECHANICALHOUSE_RIGHT_W 64
#define SPR_MECHANICALHOUSE_RIGHT_H 20
#define SPR_MECHANICALHOUSE_RIGHT_BGANIM_W 130
#define SPR_MECHANICALHOUSE_RIGHT_BGANIM_H 20
#define SPR_MECHANICALHOUSE_TOP_W 64
#define SPR_MECHANICALHOUSE_TOP_H 20
#define SPR_MECHANICALHOUSE_TOP_BGANIM_W 42
#define SPR_MECHANICALHOUSE_TOP_BGANIM_H 20
#define SPR_MEDICALCLINIC_W 64
#define SPR_MEDICALCLINIC_H 20
#define SPR_MINIBUTTON_W 31
#define SPR_MINIBUTTON_H 7
#define SPR_MIRROR_W 20
#define SPR_MIRROR_H 20
#define SPR_MISDIRECTOR_W 64
#define SPR_MISDIRECTOR_H 20
#define SPR_MODERNART_W 218
#define SPR_MODERNART_H 20
#define SPR_MODERNARTMUSEUM_W 64
#define SPR_MODERNARTMUSEUM_H 20
#define SPR_MODERNARTMUSEUM_BUTTONBACK_W 20
#define SPR_MODERNARTMUSEUM_BUTTONBACK_H 20
#define SPR_MODERNCONVENIENCES_W 20
#define SPR_MODERNCONVENIENCES_H 20
#define SPR_MODERNHOMEPLANT_W 218
#define SPR_MODERNHOMEPLANT_H 20
#define SPR_MODERNRUSTICHOME_W 64
#define SPR_MODERNRUSTICHOME_H 20
#define SPR_MOREBUILDINGMODEBUTTONS_W 40
#define SPR_MOREBUILDINGMODEBUTTONS_H 22
#define SPR_MOREBUILDINGMODES_ARROWUP_W 10
#define SPR_MOREBUILDINGMODES_ARROWUP_H 20
#define SPR_MORESETTINGS_W 14
#define SPR_MORESETTINGS_H 14
#define SPR_NATURE_W 20
#define SPR_NATURE_H 20
#define SPR_NIGHTCLUB_W 130
#define SPR_NIGHTCLUB_H 20
#define SPR_NIGHTCLUB_LASER_W 358
#define SPR_NIGHTCLUB_LASER_H 16
#define SPR_NIGHTCLUB_LIGHT_W 5
#define SPR_NIGHTCLUB_LIGHT_H 15
#define SPR_NIGHTCLUB_LIGHT_RAINBOW_W 5
#define SPR_NIGHTCLUB_LIGHT_RAINBOW_H 15
#define SPR_NORMALHOUSE_W 64
#define SPR_NORMALHOUSE_H 20
#define SPR_OBSERVATORY_W 64
#define SPR_OBSERVATORY_H 20
#define SPR_ONEANDHALFSPEED_W 12
#define SPR_ONEANDHALFSPEED_H 12
#define SPR_PARK_BUTTONBG_W 20
#define SPR_PARK_BUTTONBG_H 20
#define SPR_PARK_PLANTS_W 284
#define SPR_PARK_PLANTS_H 20
#define SPR_PARKPOD_W 152
#define SPR_PARKPOD_H 20
#define SPR_PARKPOD_BUTTONBACK_W 20
#define SPR_PARKPOD_BUTTONBACK_H 20
#define SPR_PINEFOREST_W 262
#define SPR_PINEFOREST_H 20
#define SPR_PINEFOREST_GROW_W 240
#define SPR_PINEFOREST_GROW_H 20
#define SPR_PIXEL_W 1
#define SPR_PIXEL_H 1
#define SPR_PLANET_RING_BIG_W 835
#define SPR_PLANET_RING_BIG_H 200
#define SPR_PLANET_RING_SMALL_W 580
#define SPR_PLANET_RING_SMALL_H 110
#define SPR_PLANETS_BIG_W 1018
#define SPR_PLANETS_BIG_H 100
#define SPR_PLANETS_BIGGER_W 730
#define SPR_PLANETS_BIGGER_H 120
#define SPR_PLANETS_GIANT_W 566
#define SPR_PLANETS_GIANT_H 140
#define SPR_PLANETS_MEDIUM_W 654
#define SPR_PLANETS_MEDIUM_H 80
#define SPR_PLANETS_SMALL_W 250
#define SPR_PLANETS_SMALL_H 40
#define SPR_POPULATION_W 10
#define SPR_POPULATION_H 10
#define SPR_PROTESTSIGN_HAPPY_W 9
#define SPR_PROTESTSIGN_HAPPY_H 14
#define SPR_PROTESTSIGN_SAD_W 9
#define SPR_PROTESTSIGN_SAD_H 14
#define SPR_PUB_W 64
#define SPR_PUB_H 20
#define SPR_PUB_MARKING_KEY_W 9
#define SPR_PUB_MARKING_KEY_H 9
#define SPR_PURPLEFOREST_W 284
#define SPR_PURPLEFOREST_H 20
#define SPR_PURPLEFOREST_GROW_W 218
#define SPR_PURPLEFOREST_GROW_H 20
#define SPR_PURPLEGRASS_W 64
#define SPR_PURPLEGRASS_H 20
#define SPR_RECOLORABLE_W 22
#define SPR_RECOLORABLE_H 22
#define SPR_REDFOREST_W 262
#define SPR_REDFOREST_H 20
#define SPR_REDFOREST_GROW_W 284
#define SPR_REDFOREST_GROW_H 20
#define SPR_REDGRASS_W 64
#define SPR_REDGRASS_H 20
#define SPR_REDWOOD_W 262
#define SPR_REDWOOD_H 20
#define SPR_REDWOODTREEHOUSE_W 64
#define SPR_REDWOODTREEHOUSE_H 20
#define SPR_REFINEDMETALFACTORY_W 64
#define SPR_REFINEDMETALFACTORY_H 20
#define SPR_REFINEDMETALFACTORY_FRAMES_W 306
#define SPR_REFINEDMETALFACTORY_FRAMES_H 20
#define SPR_REFINEDMETALFACTORY_IDLE_W 42
#define SPR_REFINEDMETALFACTORY_IDLE_H 20
#define SPR_REFINEDMETALFACTORYPUB_W 130
#define SPR_REFINEDMETALFACTORYPUB_H 20
#define SPR_REFINEDMETALFACTORYPUB_FRAMES_W 482
#define SPR_REFINEDMETALFACTORYPUB_FRAMES_H 20
#define SPR_REFINEDMETALFACTORYPUB_IDLE_W 42
#define SPR_REFINEDMETALFACTORYPUB_IDLE_H 20
#define SPR_REMOVEDECORATION_W 20
#define SPR_REMOVEDECORATION_H 20
#define SPR_REPLACEBUILDINGHERE_W 20
#define SPR_REPLACEBUILDINGHERE_H 21
#define SPR_RESEARCHBOT_W 20
#define SPR_RESEARCHBOT_H 20
#define SPR_RESEARCHBOT_LANDINGPLACE_W 64
#define SPR_RESEARCHBOT_LANDINGPLACE_H 20
#define SPR_RESIDENTS_W 8
#define SPR_RESIDENTS_H 10
#define SPR_RESOURCE_COMPUTERCHIPS_W 10
#define SPR_RESOURCE_COMPUTERCHIPS_H 10
#define SPR_RESOURCE_DOWNTREND_W 10
#define SPR_RESOURCE_DOWNTREND_H 10
#define SPR_RESOURCE_FOOD_W 10
#define SPR_RESOURCE_FOOD_H 10
#define SPR_RESOURCE_GRAPHENE_W 10
#define SPR_RESOURCE_GRAPHENE_H 10
#define SPR_RESOURCE_KNOWLEDGE_W 10
#define SPR_RESOURCE_KNOWLEDGE_H 10
#define SPR_RESOURCE_MACHINEPARTS_W 10
#define SPR_RESOURCE_MACHINEPARTS_H 10
#define SPR_RESOURCE_REFINEDMETAL_W 10
#define SPR_RESOURCE_REFINEDMETAL_H 10
#define SPR_RESOURCE_ROCKETFUEL_W 10
#define SPR_RESOURCE_ROCKETFUEL_H 10
#define SPR_RESOURCE_STONE_W 10
#define SPR_RESOURCE_STONE_H 10
#define SPR_RESOURCE_WOOD_W 10
#define SPR_RESOURCE_WOOD_H 10
#define SPR_RESTAURANT_W 130
#define SPR_RESTAURANT_H 20
#define SPR_ROCK_W 152
#define SPR_ROCK_H 20
#define SPR_ROOFBOTTOM_W 438
#define SPR_ROOFBOTTOM_H 20
#define SPR_ROOFTOPPARK_W 130
#define SPR_ROOFTOPPARK_H 20
#define SPR_ROOFTOPPARK_BUTTONBG_W 20
#define SPR_ROOFTOPPARK_BUTTONBG_H 20
#define SPR_ROOFTOPPARK_ONBUILD_W 20
#define SPR_ROOFTOPPARK_ONBUILD_H 20
#define SPR_ROOFTOPPARK_PLANTS_W 130
#define SPR_ROOFTOPPARK_PLANTS_H 20
#define SPR_SCHOOL_W 64
#define SPR_SCHOOL_H 20
#define SPR_SCRAPYARDNIGHTCLUB_W 130
#define SPR_SCRAPYARDNIGHTCLUB_H 20
#define SPR_SECRETSOCIETYHOUSE_W 64
#define SPR_SECRETSOCIETYHOUSE_H 20
#define SPR_SECRETSOCIETYMACHINE_W 64
#define SPR_SECRETSOCIETYMACHINE_H 20
#define SPR_SECRETSOCIETYMACHINE_ANIM_W 548
#define SPR_SECRETSOCIETYMACHINE_ANIM_H 20
#define SPR_SELECTEDBUILDING_W 22
#define SPR_SELECTEDBUILDING_H 22
#define SPR_SELECTEDFISH_W 10
#define SPR_SELECTEDFISH_H 10
#define SPR_SELECTEDHUMAN_W 4
#define SPR_SELECTEDHUMAN_H 7
#define SPR_SHADYHOUSE_W 64
#define SPR_SHADYHOUSE_H 20
#define SPR_SLANTEDROOF_W 460
#define SPR_SLANTEDROOF_H 20
#define SPR_SLANTEDROOFTOP_W 438
#define SPR_SLANTEDROOFTOP_H 20
#define SPR_SLIDER_BG_W 22
#define SPR_SLIDER_BG_H 22
#define SPR_SLIDER_FILL_W 22
#define SPR_SLIDER_FILL_H 22
#define SPR_SMALLTELEPORTER_RAY_W 28
#define SPR_SMALLTELEPORTER_RAY_H 5
#define SPR_SMALLTELEPORTER_RAY_PENTHOUSE_W 33
#define SPR_SMALLTELEPORTER_RAY_PENTHOUSE_H 5
#define SPR_SMALLTELEPORTER_RAY_PENTHOUSE_MIRROR_W 33
#define SPR_SMALLTELEPORTER_RAY_PENTHOUSE_MIRROR_H 5
#define SPR_SMARTCHAIRS_W 20
#define SPR_SMARTCHAIRS_H 20
#define SPR_SMARTDIAGNOSE_W 20
#define SPR_SMARTDIAGNOSE_H 20
#define SPR_SMARTLIGHTS_W 20
#define SPR_SMARTLIGHTS_H 20
#define SPR_SMARTSPEAKER_W 20
#define SPR_SMARTSPEAKER_H 20
#define SPR_SOFTERBEDS_W 20
#define SPR_SOFTERBEDS_H 20
#define SPR_SPACIOUSHOUSE_W 64
#define SPR_SPACIOUSHOUSE_H 20
#define SPR_STATUEGARDEN_BUTTONBACK_W 20
#define SPR_STATUEGARDEN_BUTTONBACK_H 20
#define SPR_STATUEGARDEN_STATUES_W 174
#define SPR_STATUEGARDEN_STATUES_H 20
#define SPR_STATUEOFHAPPINESS_W 64
#define SPR_STATUEOFHAPPINESS_H 20
#define SPR_STATUEOFHAPPINESS_BG_W 20
#define SPR_STATUEOFHAPPINESS_BG_H 20
#define SPR_STATUEOFTHEKEY_W 64
#define SPR_STATUEOFTHEKEY_H 20
#define SPR_STATUEOFTHEKEY_BG_W 20
#define SPR_STATUEOFTHEKEY_BG_H 20
#define SPR_STONEMINE_W 64
#define SPR_STONEMINE_H 20
#define SPR_STONERESEARCHCENTRE_W 64
#define SPR_STONERESEARCHCENTRE_H 20
#define SPR_STONERESEARCHCENTRE_COMPUTER_W 20
#define SPR_STONERESEARCHCENTRE_COMPUTER_H 20
#define SPR_STONERESEARCHCENTRE_PICKAXETECH_W 20
#define SPR_STONERESEARCHCENTRE_PICKAXETECH_H 20
#define SPR_STONETELEPORTER_W 64
#define SPR_STONETELEPORTER_H 20
#define SPR_STONETELEPORTER_REMOTE_W 20
#define SPR_STONETELEPORTER_REMOTE_H 20
#define SPR_STONETELEPORTER_STONE_W 196
#define SPR_STONETELEPORTER_STONE_H 20
#define SPR_STONETELEPORTER_STONE_BIG_W 196
#define SPR_STONETELEPORTER_STONE_BIG_H 20
#define SPR_SUPERCOMPUTER_W 64
#define SPR_SUPERCOMPUTER_H 20
#define SPR_SYNIGHTCLUB_BG_W 20
#define SPR_SYNIGHTCLUB_BG_H 20
#define SPR_SYNIGHTCLUB_LEFTTEXTURES_W 64
#define SPR_SYNIGHTCLUB_LEFTTEXTURES_H 19
#define SPR_SYNIGHTCLUB_LIGHTS_W 1
#define SPR_SYNIGHTCLUB_LIGHTS_H 10
#define SPR_SYNIGHTCLUB_LIGHTS_H_W 10
#define SPR_SYNIGHTCLUB_LIGHTS_H_H 1
#define SPR_SYNIGHTCLUB_MAINTEXTURES_W 64
#define SPR_SYNIGHTCLUB_MAINTEXTURES_H 19
#define SPR_SYNIGHTCLUB_MINILIGHT_W 3
#define SPR_SYNIGHTCLUB_MINILIGHT_H 3
#define SPR_SYNIGHTCLUB_WATERFALLLIGHT_W 7
#define SPR_SYNIGHTCLUB_WATERFALLLIGHT_H 2
#define SPR_SYNIGHTCLUB_WATERFALLLIGHT_LAST_W 5
#define SPR_SYNIGHTCLUB_WATERFALLLIGHT_LAST_H 6
#define SPR_TECH_NIGHTCLUB_W 328
#define SPR_TECH_NIGHTCLUB_H 20
#define SPR_TECH_NIGHTCLUB_HOVERER_W 4
#define SPR_TECH_NIGHTCLUB_HOVERER_H 4
#define SPR_TELEPORTER_W 64
#define SPR_TELEPORTER_H 20
#define SPR_TELEPORTER_RAY_W 34
#define SPR_TELEPORTER_RAY_H 15
#define SPR_TELEPORTER_RAY_KEY_W 46
#define SPR_TELEPORTER_RAY_KEY_H 15
#define SPR_TELEPORTER_RAY_MISDIRECT_W 34
#define SPR_TELEPORTER_RAY_MISDIRECT_H 15
#define SPR_TELESCOPE_W 4
#define SPR_TELESCOPE_H 5
#define SPR_TINKERERSHOUSE_W 64
#define SPR_TINKERERSHOUSE_H 20
#define SPR_TINKERERSHOUSE_BACK_ALT_W 108
#define SPR_TINKERERSHOUSE_BACK_ALT_H 20
#define SPR_TINKERERSHOUSE_BETTERTOOLS_W 86
#define SPR_TINKERERSHOUSE_BETTERTOOLS_H 20
#define SPR_TINKERERSHOUSE_FRONT_ALT_W 196
#define SPR_TINKERERSHOUSE_FRONT_ALT_H 20
#define SPR_TITLE_W 261
#define SPR_TITLE_H 55
#define SPR_TRANSPARENTBUTTON_W 70
#define SPR_TRANSPARENTBUTTON_H 22
#define SPR_TRANSPARENTBUTTON_INFO_W 70
#define SPR_TRANSPARENTBUTTON_INFO_H 22
#define SPR_TRANSPARENTBUTTON_INFO_MINI_W 46
#define SPR_TRANSPARENTBUTTON_INFO_MINI_H 14
#define SPR_TRANSPARENTBUTTON_INFO_MINIPLUS_W 64
#define SPR_TRANSPARENTBUTTON_INFO_MINIPLUS_H 20
#define SPR_TREEGROWTHBOOST_W 20
#define SPR_TREEGROWTHBOOST_H 20
#define SPR_TREEHUGGERBASE_W 20
#define SPR_TREEHUGGERBASE_H 20
#define SPR_TREEPLANTATION_W 64
#define SPR_TREEPLANTATION_H 20
#define SPR_TREEPLANTATION_BUTTONBACK_W 20
#define SPR_TREEPLANTATION_BUTTONBACK_H 20
#define SPR_TREEPLANTATION_TREES_W 262
#define SPR_TREEPLANTATION_TREES_H 20
#define SPR_TREEPLANTATION_TREES_GROW_W 240
#define SPR_TREEPLANTATION_TREES_GROW_H 20
#define SPR_TUTORIALARROWDOWN_W 20
#define SPR_TUTORIALARROWDOWN_H 30
#define SPR_TUTORIALARROWLEFT_W 30
#define SPR_TUTORIALARROWLEFT_H 20
#define SPR_UNIQUEBUILDING_W 10
#define SPR_UNIQUEBUILDING_H 10
#define SPR_UNKNOWNBUILDING_W 64
#define SPR_UNKNOWNBUILDING_H 20
#define SPR_UPGRADE_FARMBOOST_W 20
#define SPR_UPGRADE_FARMBOOST_H 20
#define SPR_UPGRADE_FLOWERS_W 64
#define SPR_UPGRADE_FLOWERS_H 20
#define SPR_VILLA_W 64
#define SPR_VILLA_H 20
#define SPR_WALDO_ADULT_W 2
#define SPR_WALDO_ADULT_H 5
#define SPR_WALDO_CHILD_W 2
#define SPR_WALDO_CHILD_H 5
#define SPR_WHITEOUTLINE_W 22
#define SPR_WHITEOUTLINE_H 22
#define SPR_WINDOWPARTS_W 62
#define SPR_WINDOWPARTS_H 24
#define SPR_WINDOWPARTS_PURPLE_W 62
#define SPR_WINDOWPARTS_PURPLE_H 24
#define SPR_WOODCUTTINGCENTRE_W 64
#define SPR_WOODCUTTINGCENTRE_H 20
#define SPR_WORK_W 10
#define SPR_WORK_H 10
#define SPR_WORK_HOUSING_W 21
#define SPR_WORK_HOUSING_H 10
#define SPR_WORKSHOP_W 64
#define SPR_WORKSHOP_H 20
#define SPR_WORKSHOP_MAKEMACHINEPARTS_W 20
#define SPR_WORKSHOP_MAKEMACHINEPARTS_H 20
#define SPR_WORKSHOP_PETPROJECTS_W 20
#define SPR_WORKSHOP_PETPROJECTS_H 20

/* fonts (the original's BMFont fonts) */
enum { FONT_STANDARD, FONT_10, FONT_15, FONT_16, FONT_18, FONT_COUNT };

#endif
