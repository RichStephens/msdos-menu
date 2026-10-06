WCC ?= wcc
WLINK ?= wlink
CFLAGS ?= -0 -bt=dos -os
MENU_TITLE ?= Program Menu
MENU_PROGRAM ?= dosmenu

TARGET = $(MENU_PROGRAM).exe
OBJECT = build/menu.obj
DEMO_NAMES ?= DEMO01 DEMO02 DEMO03 DEMO04 DEMO05 DEMO06 \
	DEMO07 DEMO08 DEMO09 DEMO10 DEMO11 DEMO12 \
	DEMO13 DEMO14 DEMO15 DEMO16 DEMO17 DEMO18 \
	DEMO19 DEMO20 DEMO21 DEMO22 DEMO23 DEMO24
DEMO_BINS = $(addprefix build/,$(addsuffix .exe,$(DEMO_NAMES)))
TEST_DISK = testmenu-test.img

.PHONY: all clean test testdisk FORCE

all: $(TARGET)

$(OBJECT): menu.c FORCE
	mkdir -p build
	$(WCC) $(CFLAGS) -d'MENU_TITLE="$(MENU_TITLE)"' \
		-d'MENU_EXE_NAME="$(notdir $(TARGET))"' -fo=$@ $<

$(TARGET): $(OBJECT) tools/limit_memory.py
	$(WLINK) OPTION quiet SYSTEM dos name $@ file {$<}
	python3 tools/limit_memory.py $@

build/%.obj: tests/demo.c | build
	$(WCC) $(CFLAGS) -d'DEMO_NAME="$*"' -fo=$@ $<

build/%.exe: build/%.obj
	$(WLINK) OPTION quiet SYSTEM dos name $@ file {$<}

build:
	mkdir -p $@

test: $(TEST_DISK)

testdisk: test

$(TEST_DISK): $(TARGET) $(DEMO_BINS)
	dd if=/dev/zero of=$@ bs=1024 count=360
	mformat -i $@ -f 360
	mcopy -i $@ $(TARGET) $(DEMO_BINS) ::/

clean:
	rm -rf build $(TARGET) $(TEST_DISK)
