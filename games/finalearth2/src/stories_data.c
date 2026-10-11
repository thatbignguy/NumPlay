/* The Final Earth 2: the scenarios and free play maps, made by tools/gen_stories.py from the
 * original's stories and texts. Don't edit: edit the generator. */
#include "story.h"
#include "classes.h"
#include "data.h"

static const StoryRes s0w0r[] = {{0, CLS_Forest}, {6, CLS_Rock}, {7, CLS_Forest}, {9, CLS_Rock}, {12, CLS_AlienRuins}, {14, CLS_Forest}, {17, CLS_AlienRuins}, {18, CLS_Rock}, {19, CLS_Forest}, {21, CLS_Forest}};
static const uint16_t s0w0s0[] = {CLS_LandedExplorationShip};
static const StoryStack s0w0s[] = {{3, 1, s0w0s0}};
static const StoryCitizens s0w0c[] = {{3, 60, 64, 60, 80}};
static const StoryWorld s0w[] = {
  {0, 160, 440, 100, 1000u, 0, 0, 0, 10, s0w0r, 1, s0w0s, 1, s0w0c, 0, NULL}};
static const SubGoal s0g0sub[] = {{.type = SG_BUILDPERMANENTS, .cls = CLS_WoodcuttingCentre, .amount = 1, .world = -1, .notWorld = -1}};
static const char *const s0g0u[] = {"WoodcuttingCentre"};
static const SubGoal s0g1sub[] = {{.type = SG_BUILDPERMANENTS, .cls = CLS_StoneMine, .amount = 1, .world = -1, .notWorld = -1}};
static const char *const s0g1u[] = {"StoneMine"};
static const SubGoal s0g2sub[] = {{.type = SG_GATHERMATERIALS, .mats = {[M_WOOD] = 6, [M_STONE] = 4}}};
static const SubGoal s0g3sub[] = {{.type = SG_BUILDPERMANENTS, .cls = CLS_NormalHouse, .amount = 1, .world = -1, .notWorld = -1}, {.type = SG_BUILDPERMANENTS, .cls = CLS_IndoorFarm, .amount = 1, .world = -1, .notWorld = -1}};
static const char *const s0g3u[] = {"NormalHouse", "IndoorFarm"};
static const SubGoal s0g4sub[] = {{.type = SG_BUILDPERMANENTS, .cls = CLS_ExplorationCentre, .amount = 1, .world = -1, .notWorld = -1}};
static const char *const s0g4u[] = {"ExplorationCentre"};
static const StoryEvent s0g4e[] = {{.type = EV_SAUCER, .time = 1, .amount = 4, .ageMin = 60, .ageMax = 64, .world = 0}};
static const SubGoal s0g5sub[] = {{.type = SG_BUILDPERMANENTS, .cls = CLS_StoneMine, .amount = 2, .world = -1, .notWorld = -1}};
static const char *const s0g5u[] = {"School"};
static const StoryEvent s0g5e[] = {{.type = EV_REGULARLY, .time = 0, .amount = 60, .ageMin = 18, .ageMax = 40, .world = 0, .groupMin = 2, .groupMax = 3, .betweenMin = 36, .betweenMax = 40, .afterDone = "All the people from your ship have now arrived. Of course, your population can still grow through their offspring."}, {.type = EV_SAUCER, .time = 360, .amount = 2, .ageMin = 18, .ageMax = 24, .world = 0}};
static const SubGoal s0g6sub[] = {{.type = SG_PERFORMBUILDINGUPGRADES, .name = "BetterPickaxes", .amount = 1}};
static const SubGoal s0g7sub[] = {{.type = SG_GATHERMATERIALS, .mats = {[M_FOOD] = 10}}, {.type = SG_BUILDPERMANENTS, .cls = CLS_Pub, .amount = 1, .world = -1, .notWorld = -1}};
static const char *const s0g7u[] = {"buildingModes", "managementOptions", "Pub", "Park"};
static const SubGoal s0g8sub[] = {{.type = SG_BUILDPERMANENTS, .cls = CLS_MachinePartsFactory, .amount = 1, .world = -1, .notWorld = -1}};
static const char *const s0g8u[] = {"MachinePartsFactory"};
static const SubGoal s0g9sub[] = {{.type = SG_MINIMUMHAPPINESS, .value = 52}};
static const char *const s0g9u[] = {"Workshop", "ExperimentalFarm", "NightClub", "ModernRusticHome"};
static const SubGoal s0g10sub[] = {{.type = SG_BUILDPERMANENTS, .cls = CLS_StoneTeleporter, .amount = 1, .world = -1, .notWorld = -1}};
static const char *const s0g10u[] = {"StoneTeleporter", "Laboratory"};
static const SubGoal s0g11sub[] = {{.type = SG_BUILDPERMANENTS, .cls = CLS_Observatory, .amount = 1, .world = -1, .notWorld = -1}};
static const char *const s0g11u[] = {"Observatory"};
static const char *const s0g12u[] = {"LandingSite", "Arcade", "TreePlantation", "RefinedMetalFactory", "decorations", "MedicalClinic", "ModernArtMuseum", "ComfortableHouse", "ColoredBlock", "RooftopDecoration", "RooftopDecoration2", "RooftopDecoration3", "RooftopDecoration4"};
static const StoryGoal s0g[] = {
  {"Start", 1, -1, 1, s0g0sub, 1, s0g0u, 0, NULL, 0, NULL, GF_DISABLEDYING | GF_DISABLEDESTROY | GF_HIDDENBOOST, GF_DISABLEDYING | GF_DISABLEDESTROY | GF_HIDDENBOOST, 1, "It's 2142, and earth is a wasteland. Colonizing this tiny world is humanity's only hope.\n\nFirst, you'll need some more building materials. Let's build a woodcutting center.", "Build a woodcutting center.", NULL},
  {"BuildStoneMine", 2, -1, 1, s0g1sub, 1, s0g1u, 0, NULL, 0, NULL, 0, 0, 101, "Good job! Now, collect some wood and build a stone mine.", "Build a stone mine.", NULL},
  {"GatherWoodAndStone", 3, -1, 1, s0g2sub, 0, NULL, 0, NULL, 0, NULL, 0, 0, 2, "Well done! Right now, nobody is going to gather stone, as everybody already has a job at the woodcutting center. Let's reassign one of these workers.", "Gather 6 wood and 4 stone. You'll need to reassign some workers.", NULL},
  {"BuildIndoorFarmAndHouse", 4, -1, 2, s0g3sub, 2, s0g3u, 0, NULL, 0, NULL, GF_HIDDENBOOST, 0, 3, "You used a space ship to get here. Some of your friends are still on it. You'll want to give them a place to live. A steady food supply would be nice, too.\n\nTo accomplish these goals, let's build a house and an indoor farm.[nonmobi] Please note that buildings can be stacked.", "Build a house and an indoor farm.", NULL},
  {"SpawnMorePeople", 5, -1, 1, s0g4sub, 1, s0g4u, 0, NULL, 1, s0g4e, 0, 0, 4, "Good job! Some more people will arrive soon.\nIt could be helpful to learn some more about this world. In particular, the alien ruins look quite interesting. Let's build an exploration center to learn all about them and maybe get some useful resources too.", "Build an exploration center.", NULL},
  {"NowArrivingRegularly", 6, -1, 1, s0g5sub, 1, s0g5u, 0, NULL, 2, s0g5e, 0, 0, 0, "New people will now start arriving regularly. Make sure to keep them fed and housed. Also, children can be born from now on. You can build a school for them if you want.[n]\nTo build up your city, you'll need lots of stone, so let's build one more stone mine.", "Build a second stone mine.", NULL},
  {"StoneMineUpgrade", 7, -1, 1, s0g6sub, 0, NULL, 0, NULL, 0, NULL, 0, 0, 0, "[c]Your stone miners have good equipment thanks to the Better Pickaxes upgrade. Great job![/][n]Your stone miners are currently working with low-quality mining equipment. Better pickaxes would improve their mining speed. Upgrade at least one of your stone mines. To do so, [nonmobi]click[mobi]tap[/] on the stone mine, then on the Better Pickaxes upgrade.", "Upgrade a stone mine with better pickaxes.", NULL},
  {"Happiness", 8, -1, 2, s0g7sub, 4, s0g7u, 0, NULL, 0, NULL, 0, 0, 0, "Now that you've got a pretty good economy, it's time to focus on your citizens' wellbeing for a bit. Not that there's no economic advantage to that at all, as happy citizens work faster. To make them more happy, let's build a pub. Also, make sure you have at least 10 food.", "Build a pub and have at least 10 food.", NULL},
  {"BuildMachinePartsFactory", 9, -1, 1, s0g8sub, 1, s0g8u, 0, NULL, 0, NULL, GF_DISABLEDYING | GF_DISABLEDESTROY, 0, 0, "Great job! Make sure there's actually someone working at the pub or it won't have any effect.\nThe city will need machines. [c]Fortunately, you looked ahead and have already built a machine parts factory.[/c][n]Let's build a machine parts factory to be able to construct them.[/n]", "Build a machine parts factory.", NULL},
  {"GetMinHappiness", 10, -1, 1, s0g9sub, 4, s0g9u, 0, NULL, 0, NULL, 0, 0, 4002, "The machine parts factory uses lots of stone, so make sure you have enough! If you need stone for something else, just switch it off for a while.[n]\nIt's always nice to see happy faces, isn't it? Let's make sure the happiness in your city is at least 52. Just [nnonmobi]click[nmobi]tap[n] on the Happiness button if you'd like to see which areas could use improvement.[/n]", "Bring the happiness in your city above 52.", NULL},
  {"BuildStoneTeleporter", 11, -1, 1, s0g10sub, 2, s0g10u, 0, NULL, 0, NULL, 0, 0, 0, "The amount of stone on this planet is quite limited. Fortunately, you've just thought of a way to solve that problem: a rock teleporter could teleport mineable rocks from elsewhere. Let's research and build one. You may want to build some labs to get more knowledge.", "Build a rock teleporter.", NULL},
  {"BuildObservatory", 12, -1, 1, s0g11sub, 1, s0g11u, 0, NULL, 0, NULL, 0, 0, 0, "Well done! You'll never run out of stone now. Keep your stone mines, as you need those workers to mine the meteroids.\nYour population is growing quickly, and this world is quite small. It's probably a good idea to look for other worlds to colonize, so let's build an observatory.", "Build the observatory.", NULL},
  {"Win", -1, 1, 0, NULL, 13, s0g12u, 0, NULL, 0, NULL, 0, 0, 0, "Great job! Your scientists will now look for other worlds to colonize. You can play the next scenario to see what they'll find.\nIf you'd like to continue improving this city, you can always go to the next scenario later from the menu. Either way, you can always return to this city later.", "", "Scenario Complete!"}};
static const StoryBackground s0b[] = {{SPR_PLANETS_SMALL, 40, 2, 120, -80, 0.5f}, {SPR_PLANETS_MEDIUM, 80, 1, 300, -200, 1.0f}, {SPR_PLANETS_SMALL, 40, 0, 40, -500, 0.2f}, {SPR_PLANETS_BIG, 100, 1, 900, 0, 1.3f}, {SPR_PLANETS_BIG, 100, 3, -900, 400, 1.5f}, {SPR_PLANETS_MEDIUM, 80, 3, -400, 100, 0.1f}};

static const StoryRes s1w0r[] = {{3, CLS_Forest}, {6, CLS_Rock}, {7, CLS_Forest}, {8, CLS_Rock}, {9, CLS_AlienRuins}};
static const uint16_t s1w0s0[] = {CLS_LandedExplorationShip};
static const StoryStack s1w0s[] = {{2, 1, s1w0s0}};
static const StoryCitizens s1w0c[] = {{3, 20, 25, 60, 80}};
static const StoryRes s1w1r[] = {{1, CLS_Rock}, {2, CLS_Forest}, {4, CLS_Rock}, {5, CLS_AlienRuins}, {7, CLS_Rock}};
static const StoryRes s1w2r[] = {{2, CLS_Forest}, {3, CLS_Rock}, {5, CLS_Rock}};
static const StoryRes s1w3r[] = {{0, CLS_Forest}, {2, CLS_Rock}, {3, CLS_Forest}, {5, CLS_Rock}, {7, CLS_Forest}, {8, CLS_Rock}};
static const StoryRes s1w4r[] = {{0, CLS_Forest}, {3, CLS_Forest}, {5, CLS_Rock}};
static const StoryRes s1w5r[] = {{0, CLS_AlienRuins}, {1, CLS_Rock}, {5, CLS_Forest}, {6, CLS_Rock}, {8, CLS_Forest}, {10, CLS_Rock}};
static const StoryRes s1w6r[] = {{1, CLS_Forest}, {2, CLS_AlienRuins}, {3, CLS_Forest}};
static const StoryWorld s1w[] = {
  {-20, 160, 200, 80, 2000u, 0, 0, 0, 5, s1w0r, 1, s1w0s, 1, s1w0c, 0, NULL},
  {240, 140, 180, 100, 2001u, 0, 0, 0, 5, s1w1r, 0, NULL, 0, NULL, 0, NULL},
  {-160, 120, 120, 60, 2002u, 0, 0, 0, 3, s1w2r, 0, NULL, 0, NULL, 0, NULL},
  {420, 40, 180, 80, 2003u, 0, 0, 0, 6, s1w3r, 0, NULL, 0, NULL, 0, NULL},
  {400, 200, 120, 60, 2004u, 0, 0, 0, 3, s1w4r, 0, NULL, 0, NULL, 0, NULL},
  {640, 100, 220, 100, 2005u, 0, 0, 0, 6, s1w5r, 0, NULL, 0, NULL, 0, NULL},
  {-300, 160, 100, 60, 2006u, 0, 0, 0, 3, s1w6r, 0, NULL, 0, NULL, 0, NULL}};
static const SubGoal s1g0sub[] = {{.type = SG_BUILDPERMANENTS, .cls = CLS_WoodcuttingCentre, .amount = 1, .world = -1, .notWorld = -1}, {.type = SG_BUILDPERMANENTS, .cls = CLS_StoneMine, .amount = 1, .world = -1, .notWorld = -1}, {.type = SG_BUILDPERMANENTS, .cls = CLS_IndoorFarm, .amount = 1, .world = -1, .notWorld = -1}, {.type = SG_BUILDPERMANENTS, .cls = CLS_NormalHouse, .amount = 1, .world = -1, .notWorld = -1}};
static const SubGoal s1g1sub[] = {{.type = SG_BUILDPERMANENTS, .cls = CLS_Pub, .amount = 1, .world = -1, .notWorld = -1}};
static const StoryEvent s1g1e[] = {{.type = EV_REGULARLY, .time = 0, .amount = 70, .ageMin = 18, .ageMax = 40, .world = 0, .groupMin = 3, .groupMax = 4, .betweenMin = 30, .betweenMax = 34, .afterDone = "All the people from your previous city have now arrived. Of course, your population can still grow through their offspring."}};
static const SubGoal s1g2sub[] = {{.type = SG_BUILDPERMANENTS, .cls = CLS_MachinePartsFactory, .amount = 1, .world = -1, .notWorld = -1}};
static const SubGoal s1g3sub[] = {{.type = SG_PERFORMBUILDINGUPGRADES, .name = "MachinePartsFactoryEfficiency", .amount = 1}};
static const SubGoal s1g4sub[] = {{.type = SG_BUILDPERMANENTS, .cls = CLS_LandingSite, .amount = 1, .world = 0, .notWorld = -1}, {.type = SG_BUILDPERMANENTS, .cls = CLS_LandingSite, .amount = 1, .world = -1, .notWorld = 0}};
static const SubGoal s1g5sub[] = {{.type = SG_BUILDPERMANENTS, .cls = CLS_ExperimentalFarm, .amount = 1, .world = -1, .notWorld = -1}};
static const SubGoal s1g6sub[] = {{.type = SG_BUILDPERMANENTS, .cls = CLS_RefinedMetalFactory, .amount = 1, .world = -1, .notWorld = -1}};
static const SubGoal s1g7sub[] = {{.type = SG_MINIMUMHAPPINESS, .value = 80}};
static const StoryGoal s1g[] = {
  {"Start", 1, -1, 4, s1g0sub, 0, NULL, 0, NULL, 0, NULL, 0, 0, 0, "Soon after building the observatory, your scientists found a world that looked promising. You decided to fly there and take a look. Apparently, your scientists didn't look very closely, because what they thought to be one world are actually multiple tiny worlds. Still, this looks like a fairly suitable location to build a city.\nTo get started, let's build a woodcutting center, a stone mine, an indoor farm and a house.", "Build a woodcutting center, a stone mine, an indoor farm and a house.", NULL},
  {"MorePeople", 2, -1, 1, s1g1sub, 0, NULL, 0, NULL, 1, s1g1e, 0, 0, 0, "That's a good start! You've invited some people from your previous city. They'll arrive in the coming days.\nYour citizens don't seem very happy right now. Let's build a pub to cheer them up a bit.", "Build a pub.", NULL},
  {"GoBuildMPF", 3, -1, 1, s1g2sub, 0, NULL, 0, NULL, 0, NULL, 0, 0, 0, "The pub should make your citizens a bit less sad! To get to the other worlds, you'll need machine parts, so let's create a machine parts factory.", "Build a machine parts factory.", NULL},
  {"MachinePartsFactoryBuilt", 4, -1, 1, s1g3sub, 0, NULL, 0, NULL, 0, NULL, 0, 0, 0, "The machine parts factory is very useful, but currently not very efficient. Let's upgrade it to recycle scrap metal. This will reduce the amount of stone it uses.", "Upgrade your machine parts factory to make it recycle scrap metal.", NULL},
  {"GoToOtherWorlds", 5, -1, 2, s1g4sub, 0, NULL, 0, NULL, 0, NULL, 0, 0, 0, "[c]You already built some landing pads to connect worlds. Great job![/][n]Good job! Now, it's finally time to get to another world. The easiest way to do that is by building landing pads. Let's build two of them: one on your current world and one on another world you'd like to populate.[/]", "Connect the currently populated world with another by building landing pads.", NULL},
  {"GoToOtherWorldsDone", 6, -1, 1, s1g5sub, 0, NULL, 0, NULL, 0, NULL, 0, 0, 0, "[c]You already built an experimental farm! Well done![/][n]Now that your city has some more space, it'll probably grow rapidly. You'll soon have many more mouths to feed. To make that a bit easier, let's build an experimental farm.[/]", "Build an experimental farm.", NULL},
  {"GoBuildRefinedMetalFactory", 7, -1, 1, s1g6sub, 0, NULL, 0, NULL, 0, NULL, 0, 0, 0, "[c]You already built a refined metals factory! Good job![/][n]To build some types of buildings, raw stone isn't good enough. You'll need to refine it first. Let's build a refined metals factory to do just that.[/]", "Build a refined metals factory.", NULL},
  {"GetHighHappiness", 8, -1, 1, s1g7sub, 0, NULL, 0, NULL, 0, NULL, 0, 0, 0, "[c]Wow, I'm impressed! You already got the happiness high enough to beat this goal.[/][n]Refined metals are nice, but happy citizens are certainly even nicer. So let's use some of these refined metals to improve the happiness of your citizens. Bring it to at least 80.[/]", "Bring the happiness above 80.", NULL},
  {"Win", -1, 2, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, 0, 0, "Your citizens are really happy now, and they wouldn't want to live anywhere else. Great job!\n\nYou can now play the next scenario, or continue improving your city if you'd like. Thanks for playing!", "", "Scenario Complete!"}};
static const StoryBackground s1b[] = {{SPR_PLANET_RING_BIG, 277, 0, 400, -200, 0.5f}, {SPR_PLANETS_GIANT, 140, 1, -400, -500, -0.8f}, {SPR_PLANETS_MEDIUM, 80, 5, 200, -100, 0.5f}, {SPR_PLANETS_SMALL, 40, 3, 950, 50, 0.2f}, {SPR_PLANETS_BIGGER, 120, 0, -900, 600, 0.8f}};

static const StoryRes s2w0r[] = {{7, CLS_Forest}, {8, CLS_Rock}, {9, CLS_Forest}, {12, CLS_Rock}, {13, CLS_ComputerAlienRuins}, {14, CLS_Rock}, {15, CLS_Forest}};
static const uint16_t s2w0s0[] = {CLS_StoneMine, CLS_NormalHouse, CLS_LandedExplorationShip | STACK_MIRROR};
static const uint16_t s2w0s1[] = {CLS_IndoorFarm, CLS_NormalHouse, CLS_NormalHouse, CLS_RooftopDecoration2 | STACK_DECOR(1)};
static const uint16_t s2w0s2[] = {CLS_WoodcuttingCentre, CLS_NormalHouse, CLS_LandedExplorationShip};
static const StoryStack s2w0s[] = {{2, 3, s2w0s0}, {3, 4, s2w0s1}, {4, 3, s2w0s2}};
static const StoryCitizens s2w0c[] = {{20, 20, 25, 40, 100}};
static const StoryRes s2w1r[] = {{0, CLS_Forest}, {3, CLS_Forest}, {5, CLS_Rock}, {6, CLS_Forest}, {10, CLS_Rock}, {11, CLS_Rock}, {16, CLS_Forest}, {17, CLS_Rock}};
static const StoryRes s2w2r[] = {{2, CLS_Rock}, {5, CLS_Forest}, {6, CLS_Rock}, {10, CLS_Forest}, {13, CLS_Rock}};
static const StoryRes s2w3r[] = {{1, CLS_Rock}, {7, CLS_AlienRuins}, {9, CLS_Rock}, {12, CLS_Rock}};
static const StoryRes s2w4r[] = {{0, CLS_Forest}, {1, CLS_Rock}, {3, CLS_AlienRuins}, {10, CLS_Rock}, {13, CLS_Forest}, {15, CLS_Rock}};
static const StoryRes s2w5r[] = {{1, CLS_AlienRuins}};
static const StoryRes s2w6r[] = {{3, CLS_AlienRuins}, {0, CLS_Rock}, {5, CLS_Rock}, {6, CLS_Forest}, {7, CLS_Forest}, {11, CLS_Rock}, {13, CLS_Forest}};
static const StoryRes s2w7r[] = {{1, CLS_Forest}, {3, CLS_Rock}, {5, CLS_Rock}, {7, CLS_Rock}, {9, CLS_Forest}, {12, CLS_Forest}, {15, CLS_Rock}};
static const StoryRes s2w8r[] = {{0, CLS_Forest}, {3, CLS_Rock}, {7, CLS_Forest}, {8, CLS_Forest}, {12, CLS_Rock}, {15, CLS_Forest}, {16, CLS_Rock}, {20, CLS_Rock}, {21, CLS_Rock}, {22, CLS_Rock}};
static const StoryWorld s2w[] = {
  {-20, 160, 400, 80, 3000u, 0, 0, 0, 7, s2w0r, 3, s2w0s, 1, s2w0c, 0, NULL},
  {-300, 300, 400, 60, 3001u, 0, 0, 0, 8, s2w1r, 0, NULL, 0, NULL, 0, NULL},
  {320, 320, 340, 60, 3002u, 0, 0, 0, 5, s2w2r, 0, NULL, 0, NULL, 0, NULL},
  {-400, -40, 280, 60, 3003u, 0, WF_UNBUILDABLE_ALIENS, 0, 4, s2w3r, 0, NULL, 0, NULL, 0, NULL},
  {500, -20, 320, 60, 3004u, 0, WF_UNBUILDABLE_ALIENS, 0, 6, s2w4r, 0, NULL, 0, NULL, 0, NULL},
  {440, 100, 40, 20, 3005u, 0, WF_UNBUILDABLE_ALIENS, 0, 1, s2w5r, 0, NULL, 0, NULL, 0, NULL},
  {-740, 180, 420, 60, 3006u, 0, WF_UNBUILDABLE_ALIENS, 0, 7, s2w6r, 0, NULL, 0, NULL, 0, NULL},
  {720, 200, 360, 80, 3007u, 0, WF_UNBUILDABLE_ALIENS, 0, 7, s2w7r, 0, NULL, 0, NULL, 0, NULL},
  {-100, 500, 500, 80, 3008u, 0, WF_UNBUILDABLE_ALIENS, 0, 10, s2w8r, 0, NULL, 0, NULL, 0, NULL},
  {-780, 180, 20, 20, 3009u, 0, WF_UNBUILDABLE_ALIENS, 0, 0, NULL, 0, NULL, 0, NULL, 0, NULL}};
static const SubGoal s2g0sub[] = {{.type = SG_HOUSINGCAPACITY, .amount = 50}};
static const StoryEvent s2g0e[] = {{.type = EV_REGULARLY, .time = 0, .amount = 100, .ageMin = 18, .ageMax = 40, .world = 0, .groupMin = 3, .groupMax = 4, .betweenMin = 16, .betweenMax = 24, .afterDone = "The people who wanted to move here have now all arrived. Of course, your population can still grow through their offspring."}};
static const SubGoal s2g1sub[] = {{.type = SG_EXPLOREALIENRUINS}};
static const SubGoal s2g2sub[] = {{.type = SG_BUILDPERMANENTS, .cls = CLS_ResearchBot, .amount = 1, .world = -1, .notWorld = -1}};
static const char *const s2g2u[] = {"ResearchBot"};
static const SubGoal s2g3sub[] = {{.type = SG_EXPLOREDWITHRESEARCHSPACECRAFT}};
static const SubGoal s2g4sub[] = {{.type = SG_EXPLOREALIENRUINS}};
static const SubGoal s2g5sub[] = {{.type = SG_BUILDPERMANENTS, .cls = CLS_RefinedMetalFactory, .amount = 1, .world = -1, .notWorld = -1}};
static const SubGoal s2g6sub[] = {{.type = SG_BUILDPERMANENTS, .cls = CLS_ComputerResearchCenter, .amount = 1, .world = -1, .notWorld = -1}};
static const SubGoal s2g7sub[] = {{.type = SG_BUILDPERMANENTS, .cls = CLS_ComputerChipFactory, .amount = 1, .world = -1, .notWorld = -1}, {.type = SG_GATHERMATERIALS, .mats = {[M_COMPUTERCHIPS] = 5}}};
static const SubGoal s2g8sub[] = {{.type = SG_BUILDPERMANENTS, .cls = CLS_Arcade, .amount = 3, .world = -1, .notWorld = -1}};
static const SubGoal s2g9sub[] = {{.type = SG_BUILDPERMANENTS, .cls = CLS_HackerHQ, .amount = 1, .world = -1, .notWorld = -1}};
static const SubGoal s2g10sub[] = {{.type = SG_COMPLETEHACKERMISSIONS, .amount = 4}};
static const SubGoal s2g11sub[] = {{.type = SG_BUILDPERMANENTS, .cls = CLS_AlienDecryptor, .amount = 1, .world = -1, .notWorld = -1}};
static const char *const s2g11u[] = {"AlienDecryptor"};
static const SubGoal s2g12sub[] = {{.type = SG_WAITFORDECRYPTION}};
static const StoryGoal s2g[] = {
  {"Start", 2, -1, 1, s2g0sub, 0, NULL, 0, NULL, 1, s2g0e, 0, 0, 0, "Humanity is slowly colonizing space. This is the newest outpost. It is still quite small, but new people are arriving quickly.\nYou don't want them to be homeless, of course. To prevent this, build houses until you have at least fifty capacity. It's also a good idea to build some more production buildings and entertainment.", "Build houses until you have fifty capacity in total.", NULL},
  {"ExploreAlienRuins", 5, -1, 1, s2g1sub, 0, NULL, 0, NULL, 0, NULL, 0, 0, 0, "(this story event is for compatibility reasons)", "Explore the alien ruins on this world.", NULL},
  {"ExploreAlienRuins2", 3, -1, 1, s2g2sub, 1, s2g2u, 0, NULL, 0, NULL, 0, 0, 0, "Well done!\nThe alien ruins here look different from the ones you saw before. Let's explore them to find out more.\n\nAlso, some of the worlds here have a strange glow. Let's build a research craft to investigate this while you're exploring the ruins.", "Build a research craft and explore the alien ruins on this world.", NULL},
  {"InTheMeantime2", 4, -1, 1, s2g3sub, 0, NULL, 0, NULL, 0, NULL, 0, 0, 0, "Now you've built the spacecraft, it's time to do some research! Make sure a worker is assigned to the research spacecraft and wait until they explored a bit.", "Assign a worker to the research spacecraft and wait until they explored a bit.", NULL},
  {"InTheMeantime3", 5, -1, 1, s2g4sub, 0, NULL, 0, NULL, 0, NULL, 0, 0, 0, "The worker in the research spacecraft noticed a strange power prevented them from getting close to the glowing worlds. This also means you can't build on them. Maybe the alien ruins can help?", "Explore the alien ruins on this world.", NULL},
  {"BuildRefinedMetalsFactory", 6, -1, 1, s2g5sub, 0, NULL, 0, NULL, 0, NULL, 0, 0, 0, "While investigating the alien ruins, your explorers found a small device. You think it could have something to do with the inaccessible worlds.\nHowever, none of your citizens has been able to make sense of it. Maybe having some computers would help? To build them, you will first need some refined metals. Build a Refined Metals Factory to produce those.", "Build a Refined Metals Factory.", NULL},
  {"BuildComputerResearchCenter", 7, -1, 1, s2g6sub, 0, NULL, 0, NULL, 0, NULL, 0, 0, 0, "Your new refined metals factory is looking good!\nBuilding a computer is actually pretty complicated. Therefore, you will need to build the Computing Research Center first.", "Build the Computing Research Center.", NULL},
  {"BuildComputerChipFactory", 8, -1, 2, s2g7sub, 0, NULL, 0, NULL, 0, NULL, 0, 0, 0, "Great job! Now, you just need some chips to be able to make a computer. Let's build a Computer Chip Factory and make five computer chips.", "Build a Computer Chip Factory and make five computer chips.", NULL},
  {"BuildArcades", 9, -1, 1, s2g8sub, 0, NULL, 0, NULL, 0, NULL, 0, 0, 0, "You built a computer and plugged in the alien device... but there's a bit of a problem. All the data is encrypted!\nMaybe the Hackers of the Bolt can help. However, before they want to settle here, they request you to build three Arcades.", "Build three arcades.", NULL},
  {"BuildHackers", 10, -1, 1, s2g9sub, 0, NULL, 0, NULL, 0, NULL, 0, 0, 0, "The Hackers of the Bolt seem satisfied with your city. Let's build their HQ now!", "Build the Hackers of the Bolt HQ.", NULL},
  {"CompleteHackerMissions", 11, -1, 1, s2g10sub, 0, NULL, 0, NULL, 0, NULL, 0, 0, 0, "The Hackers of the Bolt have taken a look at the encrypted data, but they need some more experience to be able to crack it. Let's complete a few of their missions to help them with that!", "Complete a few Hackers of the Bolt missions.", NULL},
  {"BuildDecryptBuilding", 12, -1, 1, s2g11sub, 1, s2g11u, 0, NULL, 0, NULL, 0, 0, 0, "The Hackers of the Bolt think they can now crack the encryption, but they need a specialized computer first. Let's build the Alien Decryptor for them!", "Build the Alien Decryptor.", NULL},
  {"Wait", 13, -1, 1, s2g12sub, 0, NULL, 0, NULL, 0, NULL, 0, 0, 0, "Great job! Now the only thing left to do is to wait while the Decryptor does its job. This should take no more than a day.", "Let the Decryptor do its job.", NULL},
  {"Win", -1, 3, 0, NULL, 0, NULL, 0, NULL, 0, NULL, GF_SETBUILDABLEALIENS, GF_SETBUILDABLEALIENS, 0, "The decryption succeeded, and the Hackers of the Bolt will soon be able to remove the restrictive glow from the locked worlds. You can now continue to build an amazing city here! You have also been invited to the City of the Key. Go to the next scenario to accept this invitiation.", "", "Scenario Complete!"}};
static const StoryBackground s2b[] = {{SPR_PLANETS_BIGGER, 120, 3, -400, -300, 0.6f}, {SPR_PLANETS_MEDIUM, 80, 0, 300, -50, 0.6f}, {SPR_PLANETS_SMALL, 40, 1, -50, -200, -0.5f}, {SPR_PLANETS_BIGGER, 120, 1, -50, 600, 0.9f}, {SPR_PLANETS_MEDIUM, 80, 2, -600, 0, 0.7f}, {SPR_PLANETS_MEDIUM, 80, 4, 1100, -500, -0.4f}};

static const StoryRes s3w0r[] = {{0, CLS_PineForest}, {4, CLS_AlienRuins}, {5, CLS_Rock}, {9, CLS_AlienRuins}, {12, CLS_Rock}, {14, CLS_Rock}, {18, CLS_PineForest}, {22, CLS_Rock}, {23, CLS_PineForest}, {28, CLS_Rock}, {27, CLS_PineForest}};
static const uint16_t s3w0s0[] = {CLS_Pub};
static const uint16_t s3w0s1[] = {CLS_LandedExplorationShip};
static const uint16_t s3w0s2[] = {CLS_LandingSite};
static const uint16_t s3w0s3[] = {CLS_RooftopDecoration | STACK_DECOR(9)};
static const uint16_t s3w0s4[] = {CLS_Teleporter, CLS_RooftopDecoration2 | STACK_DECOR(9)};
static const uint16_t s3w0s5[] = {CLS_RooftopDecoration | STACK_DECOR(9) | STACK_MIRROR};
static const uint16_t s3w0s6[] = {CLS_Laboratory, CLS_IndoorFarm, CLS_Laboratory, CLS_TreePlantation, CLS_RooftopDecoration | STACK_DECOR(10) | STACK_MIRROR};
static const uint16_t s3w0s7[] = {CLS_IndoorFarm, CLS_Laboratory, CLS_TreePlantation, CLS_Laboratory, CLS_RooftopDecoration2 | STACK_DECOR(10)};
static const uint16_t s3w0s8[] = {CLS_Laboratory, CLS_IndoorFarm, CLS_Laboratory, CLS_TreePlantation, CLS_RooftopDecoration | STACK_DECOR(10)};
static const uint16_t s3w0s9[] = {CLS_StoneMine, CLS_StoneMine, CLS_StoneMine, CLS_StoneMine, CLS_RooftopDecoration};
static const uint16_t s3w0s10[] = {CLS_StoneMine, CLS_StoneTeleporter, CLS_StoneMine, CLS_StoneMine, CLS_StoneMine, CLS_RooftopDecoration2 | STACK_DECOR(0)};
static const uint16_t s3w0s11[] = {CLS_StoneMine, CLS_StoneMine, CLS_StoneMine, CLS_StoneMine, CLS_RooftopDecoration | STACK_DECOR(0) | STACK_MIRROR};
static const StoryStack s3w0s[] = {{1, 1, s3w0s0}, {20, 1, s3w0s1}, {40, 1, s3w0s2}, {35, 1, s3w0s3}, {36, 2, s3w0s4}, {37, 1, s3w0s5}, {24, 5, s3w0s6}, {25, 5, s3w0s7}, {26, 5, s3w0s8}, {30, 5, s3w0s9}, {31, 6, s3w0s10}, {32, 5, s3w0s11}};
static const StoryCitizens s3w0c[] = {{24, 20, 25, 700, 790}};
static const StoryDeco s3w0d[] = {{34, SPR_FLOWERS_KEYCITY}, {38, SPR_FLOWERS_KEYCITY}};
static const uint16_t s3w1s0[] = {CLS_SecretSocietyHouse, CLS_LandingSite};
static const StoryStack s3w1s[] = {{0, 2, s3w1s0}};
static const StoryCitizens s3w1c[] = {{8, 40, 45, 2, 18}};
static const StoryRes s3w2r[] = {{40, CLS_PineForest}, {36, CLS_AlienRuins}, {35, CLS_Rock}, {31, CLS_AlienRuins}, {28, CLS_Rock}, {26, CLS_Rock}, {22, CLS_PineForest}, {18, CLS_Rock}, {17, CLS_PineForest}, {13, CLS_PineForest}, {12, CLS_Rock}};
static const uint16_t s3w2s0[] = {CLS_RooftopDecoration | STACK_DECOR(9)};
static const uint16_t s3w2s1[] = {CLS_Teleporter, CLS_RooftopDecoration2 | STACK_DECOR(9)};
static const uint16_t s3w2s2[] = {CLS_RooftopDecoration | STACK_DECOR(9) | STACK_MIRROR};
static const uint16_t s3w2s3[] = {CLS_LandingSite};
static const uint16_t s3w2s4[] = {CLS_MechanicalHouse, CLS_RefinedMetalFactory, CLS_ExperimentationLab, CLS_RefinedMetalFactory, CLS_LandingSite};
static const uint16_t s3w2s5[] = {CLS_MachinePartsFactory, CLS_ComputerResearchCenter, CLS_ComputerChipFactory, CLS_LandingSite};
static const uint16_t s3w2s6[] = {CLS_MechanicalHouse, CLS_RefinedMetalFactory, CLS_ExperimentationLab, CLS_RefinedMetalFactory, CLS_LandingSite};
static const uint16_t s3w2s7[] = {CLS_ComfortableHouse, CLS_ComfortableHouse, CLS_ComfortableHouse, CLS_ComfortableHouse, CLS_RooftopDecoration | STACK_DECOR(3) | STACK_MIRROR};
static const uint16_t s3w2s8[] = {CLS_Workshop, CLS_ComfortableHouse, CLS_ComfortableHouse, CLS_ComfortableHouse, CLS_ComfortableHouse, CLS_RooftopDecoration2 | STACK_DECOR(3)};
static const uint16_t s3w2s9[] = {CLS_ComfortableHouse, CLS_ComfortableHouse, CLS_ComfortableHouse, CLS_ComfortableHouse, CLS_RooftopDecoration | STACK_DECOR(3)};
static const uint16_t s3w2s10[] = {CLS_LandedExplorationShip | STACK_MIRROR};
static const uint16_t s3w2s11[] = {CLS_Pub};
static const StoryStack s3w2s[] = {{3, 1, s3w2s0}, {4, 2, s3w2s1}, {5, 1, s3w2s2}, {0, 1, s3w2s3}, {14, 5, s3w2s4}, {15, 4, s3w2s5}, {16, 5, s3w2s6}, {10, 5, s3w2s7}, {9, 6, s3w2s8}, {8, 5, s3w2s9}, {20, 1, s3w2s10}, {39, 1, s3w2s11}};
static const StoryCitizens s3w2c[] = {{24, 20, 25, 2, 100}};
static const StoryDeco s3w2d[] = {{2, SPR_FLOWERS_KEYCITY}, {6, SPR_FLOWERS_KEYCITY}};
static const StoryWorld s3w[] = {
  {-900, 140, 820, 100, 14000u, 1, 0, SPR_DECORATION_KEYCITY, 11, s3w0r, 12, s3w0s, 1, s3w0c, 2, s3w0d},
  {0, 140, 20, 20, 14003u, 2, WF_PROTECTED_KEY, SPR_DECORATION_KEYCITY, 0, NULL, 1, s3w1s, 1, s3w1c, 0, NULL},
  {100, 140, 820, 100, 14001u, 1, 0, SPR_DECORATION_KEYCITY, 11, s3w2r, 12, s3w2s, 1, s3w2c, 2, s3w2d}};
static const SubGoal s3g0sub[] = {{.type = SG_WAIT, .value = 1}};
static const char *const s3g0u[] = {"ComputerChipFactory", "ExperimentationLab"};
static const char *const s3g0r[] = {"ComputerChipFactory", "ExperimentationLab", "StoneTeleporter", "ComfortableHouse", "RefinedMetalFactory", "Workshop", "Teleporter", "SecretSocietyHouse", "LandingSite", "MachinePartsFactory", "MechanicalHouse"};
static const SubGoal s3g1sub[] = {{.type = SG_INVITEPEOPLE}};
static const SubGoal s3g2sub[] = {{.type = SG_PRODUCTIONGOAL, .mats = {[M_KNOWLEDGE] = 50, [M_STONE] = 100, [M_FOOD] = 100}}};
static const SubGoal s3g3sub[] = {{.type = SG_BUILDPERMANENTS, .cls = CLS_StatueGarden, .amount = 5, .world = -1, .notWorld = -1}};
static const char *const s3g3u[] = {"StatueGarden"};
static const SubGoal s3g4sub[] = {{.type = SG_MINIMUMCITIZENS, .amount = 300}};
static const SubGoal s3g5sub[] = {{.type = SG_PRODUCTIONGOAL, .mats = {[M_KNOWLEDGE] = 150}}};
static const SubGoal s3g6sub[] = {{.type = SG_BUILDPERMANENTS, .cls = CLS_Teleporter, .amount = 5, .world = -1, .notWorld = -1}};
static const SubGoal s3g7sub[] = {{.type = SG_BUILDPERMANENTS, .cls = CLS_LuxuryRestaurant, .amount = 1, .world = -1, .notWorld = -1}};
static const char *const s3g7u[] = {"LuxuryRestaurant"};
static const SubGoal s3g8sub[] = {{.type = SG_PERFORMCITYUPGRADE, .name = "SecretiveLiving", .amount = 1}};
static const SubGoal s3g9sub[] = {{.type = SG_BUILDPERMANENTS, .cls = CLS_Villa, .amount = 3, .world = -1, .notWorld = -1}};
static const char *const s3g9u[] = {"Villa"};
static const SubGoal s3g10sub[] = {{.type = SG_GLORYGOAL}};
static const SubGoal s3g11sub[] = {{.type = SG_PRODUCTIONGOAL, .mats = {[M_KNOWLEDGE] = 750}}};
static const SubGoal s3g12sub[] = {{.type = SG_BUILDPERMANENTS, .cls = CLS_TheMachine, .amount = 1, .world = -1, .notWorld = -1}};
static const char *const s3g12u[] = {"TheMachine"};
static const char *const s3g13u[] = {"HouseOfTheKeyFriends"};
static const StoryGoal s3g[] = {
  {"Start", 1, -1, 1, s3g0sub, 2, s3g0u, 11, s3g0r, 0, NULL, 0, 0, 0, "We, the Secret Society of the Key, hope and expect you are grateful to have arrived in our beautiful city. We want to build a city that proves our greatness, but we are tired of dealing with day-to-day business - we anticipate you can help.", "Wait for your first mission.", NULL},
  {"InvitePeople", 2, -1, 1, s3g1sub, 0, NULL, 0, NULL, 0, NULL, 0, 0, 4001, "Nowadays, there is a lot of competition between cities. People don't just move to a new colony for free, not even to the glorious City of the Key. They want food as a relocation reward, and who knows what else they'll want in the future. We think this is ridiculous, but it is how it is. Please invite a few more citizens.", "Invite some citizens using the Management option.", NULL},
  {"ImproveEconomy", 3, -1, 1, s3g2sub, 0, NULL, 0, NULL, 0, NULL, 0, 0, 0, "You can invite new people whenever you desire.\n\nWe feel there may be room for improvement in the production department. We'd like you to produce at least 100 food, 100 stone and 50 knowledge in a day. It may be a good idea to disable our teleporters once in a while, as they seem to require a lot of knowledge.", "Produce at least 100 food, 100 stone and 50 knowledge in a day.", NULL},
  {"BuildSculptures", 4, -1, 1, s3g3sub, 1, s3g3u, 0, NULL, 0, NULL, 0, 0, 0, "The people in this city, rightfully, care about the Glory of the Key. Build five sculpture gardens to improve our Glory, and thus the happiness of our citizens.", "Build five sculpture gardens.", NULL},
  {"HaveEnoughPeople", 5, -1, 1, s3g4sub, 0, NULL, 0, NULL, 0, NULL, 0, 0, 0, "[c]We are pleased to see our city has successfully grown to 300 citizens.[/][n]We would like to see our city grow to at least 300 citizens. Maybe you can invite a few people.", "Have at least 300 citizens in your city. You can use the invitiation feature.", NULL},
  {"GatherKnowledge", 6, -1, 1, s3g5sub, 0, NULL, 0, NULL, 0, NULL, 0, 0, 0, "Our city should gather all the knowledge in the universe. To start, gather at least 150 in a day.", "Gather at least 150 knowledge in a day.", NULL},
  {"BuildTeleporters", 7, -1, 1, s3g6sub, 0, NULL, 0, NULL, 0, NULL, 0, 0, 0, "Having good transportation is important. [c]We are pleased to see you understand that and have already built some teleporters.[/][n]There should be at least five teleporters in our city.", "Have at least five teleporters in your city.", NULL},
  {"BuildLuxuryRestaurants", 8, -1, 1, s3g7sub, 1, s3g7u, 0, NULL, 0, NULL, 0, 0, 0, "It is time for some more glory and luxury. Please build an upscale restaurant for us to improve the Glory of the Key.", "Build an upscale restaurant.", NULL},
  {"ImproveOurHousing", 9, -1, 1, s3g8sub, 0, NULL, 0, NULL, 0, NULL, 0, 0, 0, "It's good to care for our citizens, but in the end we are obviously the most important. Our house is fine, but it could be better. Research Secretive Living in the Living Research Center.", "Research Secretive Living in the Living Research Center.", NULL},
  {"BuildLuxuryHousing", 10, -1, 1, s3g9sub, 1, s3g9u, 0, NULL, 0, NULL, 0, 0, 0, "Now that our house is better, it is time to build some great homes for people we like, too. Build three villas. This increases the Glory of the Key.", "Build three villas.", NULL},
  {"FinalGlory", 11, -1, 1, s3g10sub, 0, NULL, 0, NULL, 0, NULL, 0, 0, 0, "The Key should be ever glorious. [c]We are pleased to see that it indeed is.[/][n]Improve the Glory of the Key to 100.", "Improve the Glory of the Key to 100. You can check it in the Happiness window.", NULL},
  {"GatherKnowledge2", 12, -1, 1, s3g11sub, 0, NULL, 0, NULL, 0, NULL, 0, 0, 0, "We have said it before, but we want to stress that knowledge is really important to us. Therefore, gather at least 750 knowledge in a day.", "Gather at least 750 knowledge in a day.", NULL},
  {"BuildTheMachine", 13, -1, 1, s3g12sub, 1, s3g12u, 0, NULL, 0, NULL, 0, 0, 0, "Now that we have gathered enough knowledge, it is time to build our secret project. We would like you to build the Machine.", "Build the Machine.", NULL},
  {"Win", -1, -1, 0, NULL, 1, s3g13u, 0, NULL, 0, NULL, 0, 0, 0, "We are quite pleased with your work. Therefore, you can now build a house for the Friends of the Key, who will live forever just like us.", "", "Scenario Complete!"}};
static const StoryBackground s3b[] = {{SPR_PLANETS_SMALL, 40, 0, -50, -700, 0.62f}, {SPR_PLANET_RING_SMALL, 192, 0, 250, -200, 0.51f}, {SPR_PLANETS_BIGGER, 120, 0, 0, 0, 0.0f}, {SPR_PLANETS_MEDIUM, 80, 5, -430, -150, -0.56f}, {SPR_PLANETS_BIG, 100, 2, 800, -300, 0.93f}, {SPR_PLANETS_BIG, 100, 3, 700, 530, 1.35f}, {SPR_PLANETS_MEDIUM, 80, 0, -1080, 550, 1.25f}, {SPR_PLANETS_BIGGER, 120, 3, -800, -500, -0.99f}, {SPR_PLANETS_SMALL, 40, 1, 100, 100, 0.4f}, {SPR_PLANETS_SMALL, 40, 3, -100, -100, 0.4f}};

static const StoryRes s4w0r[] = {{0, CLS_Forest}, {1, CLS_Forest}, {4, CLS_AlienRuins}, {5, CLS_Rock}, {10, CLS_Rock}, {11, CLS_Forest}, {12, CLS_Rock}, {15, CLS_AlienRuins}, {17, CLS_Forest}, {20, CLS_Rock}, {22, CLS_Forest}, {25, CLS_Rock}, {26, CLS_Rock}, {29, CLS_Forest}, {30, CLS_Rock}, {32, CLS_AlienRuins}, {36, CLS_Rock}, {38, CLS_Forest}, {40, CLS_Forest}, {41, CLS_Forest}, {44, CLS_Rock}, {46, CLS_Rock}, {47, CLS_Forest}, {48, CLS_AlienRuins}};
static const uint16_t s4w0s0[] = {CLS_LandedExplorationShip};
static const StoryStack s4w0s[] = {{2, 1, s4w0s0}};
static const StoryCitizens s4w0c[] = {{3, 20, 25, 60, 80}};
static const StoryWorld s4w[] = {
  {0, 160, 1000, 80, 2000u, 0, 0, 0, 24, s4w0r, 1, s4w0s, 1, s4w0c, 0, NULL}};
static const StoryEvent s4g0e[] = {{.type = EV_REGULARLY, .time = 0, .amount = 100, .ageMin = 18, .ageMax = 40, .world = 0, .groupMin = 2, .groupMax = 3, .betweenMin = 20, .betweenMax = 30, .afterDone = "All the people from your ship have now arrived. Of course, your population can still grow through their offspring."}};
static const StoryGoal s4g[] = {
  {"FreePlayStart", -1, -1, 0, NULL, 0, NULL, 0, NULL, 1, s4g0e, 0, 0, 0, "Welcome to this Free Play Scenario!\nA few citizens are already here. Some more will arrive over the coming days.", "", ""}};
static const StoryBackground s4b[] = {{SPR_PLANETS_GIANT, 140, 0, 400, -300, 1.5f}, {SPR_PLANETS_BIG, 100, 0, 600, -200, 0.8f}, {SPR_PLANETS_SMALL, 40, 0, -400, 10, -0.5f}, {SPR_PLANETS_BIGGER, 120, 2, 150, 700, 0.84f}, {SPR_PLANETS_BIG, 100, 6, -950, 80, -0.9f}, {SPR_PLANETS_BIG, 100, 5, -100, -100, 0.3f}};

static const StoryRes s5w0r[] = {{2, CLS_Rock}, {3, CLS_Rock}, {4, CLS_AlienRuins}, {10, CLS_Forest}, {12, CLS_Rock}, {14, CLS_Forest}, {15, CLS_Forest}, {18, CLS_Rock}, {19, CLS_AlienRuins}};
static const uint16_t s5w0s0[] = {CLS_LandedExplorationShip};
static const StoryStack s5w0s[] = {{5, 1, s5w0s0}};
static const StoryCitizens s5w0c[] = {{3, 20, 25, 100, 120}};
static const StoryWorld s5w[] = {
  {0, 160, 400, 80, 8801u, 0, 0, 0, 9, s5w0r, 1, s5w0s, 1, s5w0c, 0, NULL}};
static const StoryEvent s5g0e[] = {{.type = EV_REGULARLY, .time = 0, .amount = 100, .ageMin = 18, .ageMax = 40, .world = 0, .groupMin = 2, .groupMax = 3, .betweenMin = 20, .betweenMax = 30, .afterDone = "All the people from your ship have now arrived. Of course, your population can still grow through their offspring."}};
static const StoryGoal s5g[] = {
  {"FreePlayStart", -1, -1, 0, NULL, 0, NULL, 0, NULL, 1, s5g0e, 0, 0, 0, "Welcome to this Free Play Scenario!\nA few citizens are already here. Some more will arrive over the coming days.", "", ""}};
static const GenItem s5x0[] = {{CLS_AlienRuins, 0, 1, 0}, {CLS_Forest, 12, 15, 0}, {CLS_Rock, 20, 25, 0}};
static const GenItem s5x1[] = {{CLS_AlienRuins, 2, 3, 0}, {CLS_Forest, 60, 61, 0}, {CLS_Rock, 50, 51, 0}};
static const WorldGenerator s5x[] = {{9, 10, -400, 120, 600, 500, 60, 240, 20, 60, 20, 3, s5x0, true, false, false, true}, {35, 42, -1000, 120, 940, 1200, 60, 300, 20, 60, 20, 3, s5x1, true, false, false, true}};
static const StoryBackground s5b[] = {{SPR_PLANETS_SMALL, 40, 1, 0, -300, 0.3f}, {SPR_PLANETS_BIG, 100, 6, 100, -200, 0.8f}, {SPR_PLANETS_MEDIUM, 80, 2, 300, 10, -0.5f}, {SPR_PLANETS_MEDIUM, 80, 0, 300, 320, 0.84f}, {SPR_PLANETS_BIG, 100, 1, -950, 80, 0.9f}, {SPR_PLANETS_BIG, 100, 5, -300, 120, 1.3f}, {SPR_PLANETS_BIGGER, 120, 0, 900, -300, -0.9f}};

static const uint16_t s6w0s0[] = {CLS_LandedExplorationShip};
static const uint16_t s6w0s1[] = {CLS_LandingSite};
static const StoryStack s6w0s[] = {{1, 1, s6w0s0}, {0, 1, s6w0s1}};
static const StoryRes s6w1r[] = {{2, CLS_Rock}, {3, CLS_Forest}, {4, CLS_Rock}, {7, CLS_Rock}, {8, CLS_Forest}, {9, CLS_AlienRuins}, {10, CLS_Forest}};
static const uint16_t s6w1s0[] = {CLS_LandingSite};
static const StoryStack s6w1s[] = {{0, 1, s6w1s0}};
static const StoryCitizens s6w1c[] = {{3, 20, 25, 20, 80}};
static const StoryRes s6w2r[] = {{1, CLS_Rock}, {5, CLS_Rock}, {6, CLS_Forest}};
static const uint16_t s6w2s0[] = {CLS_LandingSite};
static const StoryStack s6w2s[] = {{0, 1, s6w2s0}};
static const StoryRes s6w3r[] = {{3, CLS_Rock}, {4, CLS_AlienRuins}, {5, CLS_Rock}, {6, CLS_Forest}, {8, CLS_Forest}};
static const StoryRes s6w4r[] = {{1, CLS_AlienRuins}};
static const StoryRes s6w5r[] = {{1, CLS_Rock}, {2, CLS_Forest}, {4, CLS_Forest}, {5, CLS_Rock}, {6, CLS_Forest}, {9, CLS_AlienRuins}, {10, CLS_Forest}, {12, CLS_Rock}};
static const StoryRes s6w6r[] = {{1, CLS_Forest}, {2, CLS_Forest}};
static const StoryWorld s6w[] = {
  {0, 160, 1200, 100, 9201u, 0, 0, 0, 0, NULL, 2, s6w0s, 0, NULL, 0, NULL},
  {40, 400, 240, 60, 9202u, 0, 0, 0, 7, s6w1r, 1, s6w1s, 1, s6w1c, 0, NULL},
  {340, 440, 160, 40, 9203u, 0, 0, 0, 3, s6w2r, 1, s6w2s, 0, NULL, 0, NULL},
  {580, 380, 200, 60, 9204u, 0, 0, 0, 5, s6w3r, 0, NULL, 0, NULL, 0, NULL},
  {860, 340, 60, 40, 9205u, 0, 0, 0, 1, s6w4r, 0, NULL, 0, NULL, 0, NULL},
  {840, 440, 280, 60, 9206u, 0, 0, 0, 8, s6w5r, 0, NULL, 0, NULL, 0, NULL},
  {1100, 320, 80, 40, 9207u, 0, 0, 0, 2, s6w6r, 0, NULL, 0, NULL, 0, NULL}};
static const StoryEvent s6g0e[] = {{.type = EV_REGULARLY, .time = 0, .amount = 100, .ageMin = 18, .ageMax = 40, .world = 0, .groupMin = 2, .groupMax = 3, .betweenMin = 20, .betweenMax = 30, .afterDone = "All the people from your ship have now arrived. Of course, your population can still grow through their offspring."}};
static const StoryGoal s6g[] = {
  {"FreePlayStart", -1, -1, 0, NULL, 0, NULL, 0, NULL, 1, s6g0e, 0, 0, 0, "Welcome to this Free Play Scenario!\nA few citizens are already here. Some more will arrive over the coming days.", "", ""}};
static const StoryBackground s6b[] = {{SPR_PLANETS_SMALL, 40, 2, 100, -900, 0.25f}, {SPR_PLANETS_BIGGER, 120, 1, 140, -140, 0.8f}, {SPR_PLANETS_MEDIUM, 80, 4, 300, -80, -0.9f}, {SPR_PLANETS_MEDIUM, 80, 1, 900, 320, -0.54f}, {SPR_PLANETS_BIG, 100, 2, -950, -500, 2.0f}, {SPR_PLANETS_SMALL, 40, 0, 450, -300, 0.33f}, {SPR_PLANETS_GIANT, 140, 0, -1200, 800, 0.4f}};

static const StoryRes s7w0r[] = {{1, CLS_Forest}, {4, CLS_Forest}, {5, CLS_Forest}, {10, CLS_Forest}, {13, CLS_Forest}, {16, CLS_Forest}};
static const uint16_t s7w0s0[] = {CLS_LandedExplorationShip};
static const uint16_t s7w0s1[] = {CLS_LandingSite};
static const StoryStack s7w0s[] = {{17, 1, s7w0s0}, {19, 1, s7w0s1}};
static const StoryCitizens s7w0c[] = {{3, 20, 25, 340, 380}};
static const StoryRes s7w1r[] = {{2, CLS_Rock}, {4, CLS_Rock}, {6, CLS_Rock}, {7, CLS_Rock}, {10, CLS_Rock}, {13, CLS_Rock}, {17, CLS_Rock}, {18, CLS_Rock}};
static const uint16_t s7w1s0[] = {CLS_LandingSite};
static const uint16_t s7w1s1[] = {CLS_NormalHouse};
static const StoryStack s7w1s[] = {{0, 1, s7w1s0}, {1, 1, s7w1s1}};
static const StoryCitizens s7w1c[] = {{3, 20, 25, 0, 60}};
static const StoryRes s7w2r[] = {{1, CLS_AlienRuins}};
static const uint16_t s7w2s0[] = {CLS_LandingSite};
static const StoryStack s7w2s[] = {{0, 1, s7w2s0}};
static const StoryRes s7w3r[] = {{1, CLS_Forest}, {4, CLS_Forest}, {5, CLS_Forest}, {6, CLS_Forest}, {9, CLS_Forest}, {11, CLS_Forest}, {12, CLS_Forest}, {15, CLS_Forest}};
static const StoryRes s7w4r[] = {{0, CLS_Forest}, {1, CLS_Forest}, {4, CLS_Forest}, {5, CLS_Forest}, {8, CLS_Forest}, {12, CLS_Forest}, {15, CLS_Forest}, {18, CLS_Forest}};
static const StoryRes s7w5r[] = {{1, CLS_Forest}, {3, CLS_Forest}, {4, CLS_Forest}, {8, CLS_Forest}, {15, CLS_Forest}, {16, CLS_Forest}, {18, CLS_Forest}, {21, CLS_Forest}};
static const StoryRes s7w6r[] = {{1, CLS_Rock}, {5, CLS_Rock}, {7, CLS_Rock}, {10, CLS_Rock}, {11, CLS_Rock}, {14, CLS_Rock}, {15, CLS_AlienRuins}};
static const StoryRes s7w7r[] = {{2, CLS_AlienRuins}};
static const StoryRes s7w8r[] = {{1, CLS_AlienRuins}, {8, CLS_Rock}, {14, CLS_Rock}, {15, CLS_Rock}, {18, CLS_Rock}, {20, CLS_Rock}, {24, CLS_Rock}};
static const StoryRes s7w9r[] = {{0, CLS_Forest}, {2, CLS_Forest}, {5, CLS_Forest}, {10, CLS_Forest}, {11, CLS_Forest}, {14, CLS_Forest}};
static const StoryWorld s7w[] = {
  {-360, 160, 400, 80, 9101u, 0, 0, 0, 6, s7w0r, 2, s7w0s, 1, s7w0c, 0, NULL},
  {400, 160, 460, 80, 9102u, 0, 0, 0, 8, s7w1r, 2, s7w1s, 1, s7w1c, 0, NULL},
  {480, 320, 80, 40, 9103u, 0, 0, 0, 1, s7w2r, 1, s7w2s, 0, NULL, 0, NULL},
  {-700, 0, 340, 60, 9104u, 0, 0, 0, 8, s7w3r, 0, NULL, 0, NULL, 0, NULL},
  {-600, 440, 420, 100, 9105u, 0, 0, 0, 8, s7w4r, 0, NULL, 0, NULL, 0, NULL},
  {-940, 200, 460, 100, 9106u, 0, 0, 0, 8, s7w5r, 0, NULL, 0, NULL, 0, NULL},
  {740, 400, 340, 80, 9107u, 0, 0, 0, 7, s7w6r, 0, NULL, 0, NULL, 0, NULL},
  {940, 0, 80, 40, 9108u, 0, 0, 0, 1, s7w7r, 0, NULL, 0, NULL, 0, NULL},
  {520, 640, 500, 80, 9109u, 0, 0, 0, 7, s7w8r, 0, NULL, 0, NULL, 0, NULL},
  {-800, 660, 300, 80, 9110u, 0, 0, 0, 6, s7w9r, 0, NULL, 0, NULL, 0, NULL}};
static const StoryEvent s7g0e[] = {{.type = EV_REGULARLY, .time = 0, .amount = 100, .ageMin = 18, .ageMax = 40, .world = 0, .groupMin = 2, .groupMax = 3, .betweenMin = 50, .betweenMax = 70, .afterDone = "All the people from your ship have now arrived. Of course, your population can still grow through their offspring."}, {.type = EV_REGULARLY, .time = 0, .amount = 50, .ageMin = 18, .ageMax = 40, .world = 1, .groupMin = 2, .groupMax = 3, .betweenMin = 40, .betweenMax = 50, .afterDone = "All the people from your ship have now arrived. Of course, your population can still grow through their offspring."}};
static const StoryGoal s7g[] = {
  {"FreePlayStart", -1, -1, 0, NULL, 0, NULL, 0, NULL, 2, s7g0e, 0, 0, 0, "Welcome to this Free Play Scenario!\nA few citizens are already here. Some more will arrive over the coming days.", "", ""}};
static const StoryBackground s7b[] = {{SPR_PLANETS_SMALL, 40, 1, -200, -100, 0.5f}, {SPR_PLANETS_GIANT, 120, 1, -800, 600, 0.4f}, {SPR_PLANETS_BIGGER, 120, 3, -400, -250, 1.0f}, {SPR_PLANETS_MEDIUM, 80, 5, -330, 150, -0.5f}, {SPR_PLANETS_MEDIUM, 80, 3, 500, 320, -0.5f}, {SPR_PLANETS_BIG, 100, 3, 700, -300, 1.0f}, {SPR_PLANETS_SMALL, 40, 1, 300, -240, 0.5f}};

static const StoryRes s8w0r[] = {{0, CLS_Rock}, {2, CLS_Forest}, {1, CLS_AlienRuins}};
static const uint16_t s8w0s0[] = {CLS_LandedExplorationShip};
static const uint16_t s8w0s1[] = {CLS_LandingSite};
static const StoryStack s8w0s[] = {{3, 1, s8w0s0}, {4, 1, s8w0s1}};
static const StoryCitizens s8w0c[] = {{3, 20, 25, 0, 80}};
static const StoryWorld s8w[] = {
  {0, 160, 120, 40, 5601u, 0, 0, 0, 3, s8w0r, 2, s8w0s, 1, s8w0c, 0, NULL}};
static const StoryEvent s8g0e[] = {{.type = EV_REGULARLY, .time = 0, .amount = 100, .ageMin = 18, .ageMax = 40, .world = 0, .groupMin = 2, .groupMax = 3, .betweenMin = 20, .betweenMax = 30, .afterDone = "All the people from your ship have now arrived. Of course, your population can still grow through their offspring."}};
static const StoryGoal s8g[] = {
  {"FreePlayStart", -1, -1, 0, NULL, 0, NULL, 0, NULL, 1, s8g0e, 0, 0, 0, "Welcome to this Free Play Scenario!\nA few citizens are already here. Some more will arrive over the coming days.", "", ""}};
static const GenItem s8x0[] = {{CLS_LandingSite, 5, 6, 1}, {CLS_AlienRuins, 3, 4, 0}, {CLS_Forest, 150, 151, 0}, {CLS_Rock, 100, 101, 0}};
static const WorldGenerator s8x[] = {{400, 401, -1000, 120, 940, 1600, 20, 100, 20, 40, 20, 4, s8x0, true, false, false, true}};
static const StoryBackground s8b[] = {{SPR_PLANETS_SMALL, 40, 0, -400, -200, 0.31f}, {SPR_PLANETS_SMALL, 40, 2, 300, -300, -0.45f}, {SPR_PLANETS_BIGGER, 120, 0, 600, 100, 1.0f}, {SPR_PLANETS_MEDIUM, 80, 5, -380, 230, -0.5f}, {SPR_PLANETS_BIG, 100, 7, 0, -800, 1.0f}, {SPR_PLANETS_SMALL, 40, 3, -900, -100, 0.2f}, {SPR_PLANETS_BIG, 100, 5, 40, 0, 0.01f}, {SPR_PLANETS_BIG, 100, 1, -800, 500, 0.8f}};

static const StoryRes s9w0r[] = {{2, CLS_Rock}, {3, CLS_Rock}, {4, CLS_AlienRuins}, {10, CLS_Forest}, {12, CLS_Forest}, {14, CLS_Forest}, {15, CLS_Forest}, {18, CLS_Rock}, {19, CLS_Forest}, {21, CLS_AlienRuins}, {22, CLS_Rock}, {23, CLS_Rock}};
static const uint16_t s9w0s0[] = {CLS_LandedExplorationShip};
static const StoryStack s9w0s[] = {{5, 1, s9w0s0}};
static const StoryCitizens s9w0c[] = {{3, 20, 25, 100, 120}};
static const StoryRes s9w1r[] = {{2, CLS_Rock}, {3, CLS_Rock}, {5, CLS_Rock}};
static const StoryRes s9w2r[] = {{2, CLS_Forest}, {3, CLS_Rock}, {8, CLS_AlienRuins}};
static const StoryRes s9w3r[] = {{0, CLS_Forest}};
static const StoryRes s9w4r[] = {{1, CLS_Rock}, {2, CLS_Forest}, {4, CLS_Rock}, {5, CLS_Forest}};
static const StoryRes s9w5r[] = {{2, CLS_Forest}, {4, CLS_Forest}, {7, CLS_Rock}, {8, CLS_Forest}, {12, CLS_Rock}};
static const StoryRes s9w6r[] = {{2, CLS_Forest}};
static const StoryRes s9w7r[] = {{2, CLS_Rock}, {5, CLS_Rock}, {6, CLS_Rock}, {8, CLS_Forest}};
static const StoryRes s9w8r[] = {{1, CLS_Rock}};
static const StoryRes s9w9r[] = {{0, CLS_Rock}, {3, CLS_Rock}, {4, CLS_Forest}, {5, CLS_Forest}};
static const StoryRes s9w10r[] = {{0, CLS_Rock}, {3, CLS_Rock}, {4, CLS_Forest}, {5, CLS_Forest}};
static const StoryRes s9w11r[] = {{1, CLS_AlienRuins}};
static const StoryRes s9w12r[] = {{1, CLS_Rock}};
static const StoryRes s9w13r[] = {{1, CLS_Rock}, {3, CLS_Rock}, {4, CLS_Forest}};
static const StoryRes s9w14r[] = {{1, CLS_Forest}, {5, CLS_Rock}, {6, CLS_Rock}, {10, CLS_Forest}, {12, CLS_Forest}, {16, CLS_Rock}, {20, CLS_Rock}, {21, CLS_Forest}, {22, CLS_Rock}, {25, CLS_Rock}, {28, CLS_Forest}};
static const StoryRes s9w15r[] = {{1, CLS_Forest}, {4, CLS_Rock}, {5, CLS_Rock}};
static const StoryRes s9w16r[] = {{1, CLS_Forest}};
static const StoryRes s9w17r[] = {{3, CLS_Rock}, {4, CLS_Rock}, {5, CLS_Forest}, {6, CLS_Rock}, {12, CLS_Rock}, {16, CLS_Forest}, {18, CLS_Rock}};
static const StoryRes s9w18r[] = {{7, CLS_Forest}, {11, CLS_Forest}, {13, CLS_Rock}, {14, CLS_Rock}, {15, CLS_Forest}, {16, CLS_Forest}, {18, CLS_Forest}, {21, CLS_Rock}};
static const StoryWorld s9w[] = {
  {0, 160, 500, 80, 8901u, 0, 0, 0, 12, s9w0r, 1, s9w0s, 1, s9w0c, 0, NULL},
  {-100, 320, 200, 60, 8902u, 0, 0, 0, 3, s9w1r, 0, NULL, 0, NULL, 0, NULL},
  {200, 400, 180, 60, 8903u, 0, 0, 0, 3, s9w2r, 0, NULL, 0, NULL, 0, NULL},
  {420, 280, 60, 40, 8904u, 0, 0, 0, 1, s9w3r, 0, NULL, 0, NULL, 0, NULL},
  {-20, 500, 140, 40, 8905u, 0, 0, 0, 4, s9w4r, 0, NULL, 0, NULL, 0, NULL},
  {560, 520, 260, 60, 8906u, 0, 0, 0, 5, s9w5r, 0, NULL, 0, NULL, 0, NULL},
  {540, 100, 120, 40, 8907u, 0, 0, 0, 1, s9w6r, 0, NULL, 0, NULL, 0, NULL},
  {580, 300, 180, 60, 8908u, 0, 0, 0, 4, s9w7r, 0, NULL, 0, NULL, 0, NULL},
  {460, 420, 60, 40, 8909u, 0, 0, 0, 1, s9w8r, 0, NULL, 0, NULL, 0, NULL},
  {-260, 80, 180, 60, 8910u, 0, 0, 0, 4, s9w9r, 0, NULL, 0, NULL, 0, NULL},
  {-300, 400, 220, 60, 8911u, 0, 0, 0, 4, s9w10r, 0, NULL, 0, NULL, 0, NULL},
  {-300, 260, 60, 40, 8912u, 0, 0, 0, 1, s9w11r, 0, NULL, 0, NULL, 0, NULL},
  {-260, 540, 40, 40, 8913u, 0, 0, 0, 1, s9w12r, 0, NULL, 0, NULL, 0, NULL},
  {300, 600, 180, 60, 8914u, 0, 0, 0, 3, s9w13r, 0, NULL, 0, NULL, 0, NULL},
  {40, 860, 600, 100, 8915u, 0, 0, 0, 11, s9w14r, 0, NULL, 0, NULL, 0, NULL},
  {-100, 840, 140, 60, 8916u, 0, 0, 0, 3, s9w15r, 0, NULL, 0, NULL, 0, NULL},
  {-180, 740, 60, 40, 8917u, 0, 0, 0, 1, s9w16r, 0, NULL, 0, NULL, 0, NULL},
  {600, 740, 400, 80, 8918u, 0, 0, 0, 7, s9w17r, 0, NULL, 0, NULL, 0, NULL},
  {840, 400, 440, 80, 8919u, 0, 0, 0, 8, s9w18r, 0, NULL, 0, NULL, 0, NULL},
  {1100, 620, 40, 20, 8920u, 0, 0, 0, 0, NULL, 0, NULL, 0, NULL, 0, NULL},
  {960, 240, 120, 60, 8921u, 0, 0, 0, 0, NULL, 0, NULL, 0, NULL, 0, NULL}};
static const StoryEvent s9g0e[] = {{.type = EV_REGULARLY, .time = 0, .amount = 100, .ageMin = 18, .ageMax = 40, .world = 0, .groupMin = 2, .groupMax = 3, .betweenMin = 20, .betweenMax = 30, .afterDone = "All the people from your ship have now arrived. Of course, your population can still grow through their offspring."}};
static const StoryGoal s9g[] = {
  {"FreePlayStart", -1, -1, 0, NULL, 0, NULL, 0, NULL, 1, s9g0e, 0, 0, 0, "Welcome to this Free Play Scenario!\nA few citizens are already here. Some more will arrive over the coming days.", "", ""}};
static const StoryBackground s9b[] = {{SPR_PLANETS_SMALL, 40, 1, 0, -300, 0.3f}, {SPR_PLANETS_BIG, 100, 6, 100, -200, 0.8f}, {SPR_PLANETS_MEDIUM, 80, 2, 300, 10, -0.5f}, {SPR_PLANETS_MEDIUM, 80, 0, 300, 320, 0.84f}, {SPR_PLANETS_BIG, 100, 1, -950, 80, 0.9f}, {SPR_PLANETS_BIG, 100, 5, -300, 120, 1.3f}, {SPR_PLANETS_BIGGER, 120, 0, 900, -300, -0.9f}};

const Story stories[] = {
  {"theLostShip", "A New Beginning", "Earth is a wasteland. You built a space ship, but now food is running out. A tiny world is your only hope.", ST_LOCK_ALL | ST_SPEED_UP_NIGHTS, {[M_FOOD] = 50, [M_STONE] = 7, [M_WOOD] = 0.95, [M_KNOWLEDGE] = 0, [M_MACHINEPARTS] = 0, [M_REFINEDMETAL] = 0, [M_COMPUTERCHIPS] = 0}, 1, s0w, 13, s0g, 0, 0, NULL, 0, NULL, 270, 50, 6, s0b},
  {"multipleWorlds", "An Interworld City", "When a world turns out to be way too small, you decide to build a city that crosses worlds.", 0, {[M_FOOD] = 50, [M_STONE] = 5, [M_WOOD] = 3, [M_KNOWLEDGE] = 3, [M_MACHINEPARTS] = 0, [M_REFINEDMETAL] = 0, [M_COMPUTERCHIPS] = 0}, 7, s1w, 9, s1g, 0, 0, NULL, 0, NULL, 270, 0, 5, s1b},
  {"hackersandaliens", "Hacking Alien Tech", "You discover some alien influences on your newest outpost.", 0, {[M_FOOD] = 150, [M_STONE] = 10, [M_WOOD] = 10, [M_KNOWLEDGE] = 25, [M_MACHINEPARTS] = 0, [M_REFINEDMETAL] = 0, [M_COMPUTERCHIPS] = 0}, 10, s2w, 14, s2g, 0, 0, NULL, 0, NULL, 270, 0, 6, s2b},
  {"cityofthekey", "City of the Key", "The Secret Society of the Key generously invites you to manage their glorious city.", ST_INVITE, {[M_FOOD] = 400, [M_STONE] = 150, [M_WOOD] = 100, [M_KNOWLEDGE] = 200, [M_MACHINEPARTS] = 10, [M_REFINEDMETAL] = 10, [M_COMPUTERCHIPS] = 3}, 3, s3w, 14, s3g, 0, 0, NULL, 0, NULL, 200, 10, 10, s3b},
  {"FreePlayMegaWorld", "Free Play - Mega World", "Build the city of your dreams on this huge world!", ST_FREEPLAY | ST_INVITE, {[M_FOOD] = 100, [M_STONE] = 20, [M_WOOD] = 20, [M_KNOWLEDGE] = 10, [M_MACHINEPARTS] = 0, [M_REFINEDMETAL] = 0, [M_COMPUTERCHIPS] = 0}, 1, s4w, 1, s4g, 0, 0, NULL, 0, NULL, 270, 0, 6, s4b},
  {"FreePlayManyWorldsRandom", "Free Play - Random Archipelago", "Construct a complex city on this random clique of worlds.", ST_FREEPLAY | ST_INVITE, {[M_FOOD] = 100, [M_STONE] = 20, [M_WOOD] = 20, [M_KNOWLEDGE] = 10, [M_MACHINEPARTS] = 0, [M_REFINEDMETAL] = 0, [M_COMPUTERCHIPS] = 0}, 1, s5w, 1, s5g, 0, 2, s5x, 0, NULL, 270, 0, 7, s5b},
  {"FreePlayEmptyCanvas", "Free Play - Empty Canvas", "A huge, barren world that you can furnish to your desire, plus some small worlds filled with resources.", ST_FREEPLAY | ST_INVITE, {[M_FOOD] = 100, [M_STONE] = 20, [M_WOOD] = 20, [M_KNOWLEDGE] = 10, [M_MACHINEPARTS] = 0, [M_REFINEDMETAL] = 0, [M_COMPUTERCHIPS] = 0}, 7, s6w, 1, s6g, 0, 0, NULL, 0, NULL, 450, 0, 7, s6b},
  {"FreePlayDivided", "Free Play - Divided", "Mind the gap! It separates forests and rocks.", ST_FREEPLAY | ST_INVITE, {[M_FOOD] = 150, [M_STONE] = 20, [M_WOOD] = 20, [M_KNOWLEDGE] = 10, [M_MACHINEPARTS] = 0, [M_REFINEDMETAL] = 0, [M_COMPUTERCHIPS] = 0}, 10, s7w, 1, s7g, 0, 0, NULL, 0, NULL, 270, 0, 7, s7b},
  {"FreePlayRandomMiniWorlds", "Free Play - Random Mini Worlds", "Countless tiny worlds, randomly placed in space.", ST_FREEPLAY | ST_INVITE, {[M_FOOD] = 100, [M_STONE] = 20, [M_WOOD] = 20, [M_KNOWLEDGE] = 10, [M_MACHINEPARTS] = 0, [M_REFINEDMETAL] = 0, [M_COMPUTERCHIPS] = 0}, 1, s8w, 1, s8g, 0, 1, s8x, 0, NULL, 270, 0, 8, s8b},
  {"FreePlayManyWorlds", "Free Play - Archipelago Classic", "Construct a complex city on this clique of worlds.", ST_FREEPLAY | ST_INVITE, {[M_FOOD] = 100, [M_STONE] = 20, [M_WOOD] = 20, [M_KNOWLEDGE] = 10, [M_MACHINEPARTS] = 0, [M_REFINEDMETAL] = 0, [M_COMPUTERCHIPS] = 0}, 21, s9w, 1, s9g, 0, 0, NULL, 0, NULL, 270, 0, 7, s9b}};
const int nstories = 10;
