# Interest front with a hidden NumBlocks: VAR boots Minecraft.
#   make          build ../../build/apps/numblocks-interest.nwa
Q ?= @
CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
STRIP = arm-none-eabi-strip --strip-unneeded
NWLINK ?= npx --yes -- nwlink@1.0.0
BUILD_DIR = output
DIST_DIR ?= ../../build/data/apps
DIST = $(DIST_DIR)/numblocks-interest.nwa

NB = ../numblocks
NB_GAME = $(NB)/src/main.c $(NB)/src/world.c $(NB)/src/edits.c $(NB)/src/portal.c $(NB)/src/render.c \
  $(NB)/src/player.c $(NB)/src/phys.c $(NB)/src/inv.c $(NB)/src/entity.c $(NB)/src/mob.c \
  $(NB)/src/tick.c $(NB)/src/gui.c $(NB)/src/hand.c $(NB)/src/save.c $(NB)/src/end.c $(NB)/src/data.c \
  $(NB)/src/names.c $(NB)/src/command.c $(NB)/src/gen.c $(NB)/src/test_data.c
NB_PLAT = $(NB)/src/plat_eadk.c
NB_HEADERS = $(wildcard $(NB)/src/*.h) $(NB)/../common/epsilon_app.h $(NB)/../common/epsilon_files.h
FIN_SRC = ../finance/src/main.c

CFLAGS = -std=gnu11 $(shell $(NWLINK) eadk-cflags-device)
CFLAGS += -O2 -Wall -Wextra -Wno-unused-parameter -fno-math-errno -fsingle-precision-constant -ffast-math
CFLAGS += -fno-tree-loop-distribute-patterns -flto -fno-fat-lto-objects -fvisibility=internal
CFLAGS += -flto-partition=one -I$(NB)/src
LDFLAGS = -Wl,--relocatable -nostartfiles --specs=nano.specs
LDFLAGS += -Wl,-e,main -Wl,-u,eadk_app_name -Wl,-u,eadk_app_icon -Wl,-u,eadk_api_level
LDFLAGS += -Wl,--gc-sections -flinker-output=nolto-rel

.PHONY: build check clean
build: $(DIST)

# the Interest front (provides main, the app name and the icon)
$(BUILD_DIR)/fin.o: $(FIN_SRC) ../common/epsilon_app.h | $(BUILD_DIR)
	@echo "CC      $<"
	$(Q) $(CC) $(CFLAGS) -c $< -o $@

# NumBlocks with main renamed; its app-name sections are stripped so only
# Interest's name survives the final link
$(BUILD_DIR)/nb.o: $(NB_GAME) $(NB_PLAT) $(NB_HEADERS) | $(BUILD_DIR)
	@echo "CC      numblocks (as numblocks_main)"
	$(Q) $(CC) $(CFLAGS) -Dmain=numblocks_main -Wl,--relocatable -nostartfiles -nostdlib \
	  -Wl,-e,numblocks_main -flinker-output=nolto-rel $(NB_GAME) $(NB_PLAT) -o $@
	$(Q) $(OBJCOPY) -R .rodata.eadk_app_name -R .rodata.eadk_api_level $@

$(BUILD_DIR)/icon.o: ../finance/src/icon.png | $(BUILD_DIR)
	@echo "ICON    $<"
	$(Q) $(NWLINK) png-icon-o $< $@

$(DIST): $(BUILD_DIR)/fin.o $(BUILD_DIR)/nb.o $(BUILD_DIR)/icon.o | $(DIST_DIR)
	@echo "LD      $@"
	$(Q) $(CC) $(CFLAGS) $(LDFLAGS) $^ -lm -o $@
	$(Q) $(STRIP) $@
	$(Q) arm-none-eabi-size $@

check: $(DIST)
	$(Q) $(NWLINK) nwa-bin --ram-length 153676 $< $(BUILD_DIR)/check.bin
	@echo "BIN     $(BUILD_DIR)/check.bin: $$(wc -c < $(BUILD_DIR)/check.bin) bytes"

$(BUILD_DIR) $(DIST_DIR):
	$(Q) mkdir -p $@

clean:
	$(Q) rm -rf $(BUILD_DIR) $(DIST)
