# Finance — basic finance calculators for NumWorks.
#   make          build ../../build/apps/Finance.nwa
#   make check    link the app like the calculator does (output/finance.bin)
Q ?= @
CC = arm-none-eabi-gcc
NWLINK ?= npx --yes -- nwlink@1.0.0
BUILD_DIR = output
DIST_DIR = ../../build/apps
DIST = $(DIST_DIR)/Interest.nwa
SRC = src/main.c
HEADERS = ../common/epsilon_app.h

CFLAGS = -std=c11 $(shell $(NWLINK) eadk-cflags-device)
CFLAGS += -Os -Wall -Wextra -Wno-unused-parameter -fno-math-errno -fsingle-precision-constant
CFLAGS += -fno-tree-loop-distribute-patterns -flto -fno-fat-lto-objects -fwhole-program -fvisibility=internal
LDFLAGS = -Wl,--relocatable -nostartfiles --specs=nano.specs
LDFLAGS += -Wl,-e,main -Wl,-u,eadk_app_name -Wl,-u,eadk_app_icon -Wl,-u,eadk_api_level
LDFLAGS += -Wl,--gc-sections -flinker-output=nolto-rel

.PHONY: build check clean
build: $(DIST)

check: $(DIST)
	$(Q) $(NWLINK) nwa-bin $< $(BUILD_DIR)/finance.bin
	@echo "BIN     $(BUILD_DIR)/finance.bin: $$(wc -c < $(BUILD_DIR)/finance.bin) bytes"

$(BUILD_DIR)/main.o: $(SRC) $(HEADERS) | $(BUILD_DIR)
	@echo "CC      $<"
	$(Q) $(CC) $(CFLAGS) -c $(SRC) -o $@

$(BUILD_DIR)/finance.nwa: $(BUILD_DIR)/main.o $(BUILD_DIR)/icon.o
	@echo "LD      $@"
	$(Q) $(CC) $(CFLAGS) $(LDFLAGS) $^ -lm -o $@
	$(Q) arm-none-eabi-strip --strip-unneeded $@

$(DIST): $(BUILD_DIR)/finance.nwa | $(DIST_DIR)
	@echo "CP      $@"
	$(Q) cp $< $@

$(BUILD_DIR)/icon.o: src/icon.png | $(BUILD_DIR)
	@echo "ICON    $<"
	$(Q) $(NWLINK) png-icon-o $< $@

$(BUILD_DIR):
	$(Q) mkdir -p $@ $(DIST_DIR)

$(DIST_DIR):
	$(Q) mkdir -p $@ $(BUILD_DIR)

clean:
	$(Q) rm -rf $(BUILD_DIR) $(DIST)
