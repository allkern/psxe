.ONESHELL:

VERSION_TAG := $(shell git describe --always --tags --abbrev=0)
COMMIT_HASH := $(shell git rev-parse --short HEAD)
OS_INFO := $(shell uname -rmo)

SOURCES := $(wildcard psx/*.c)
SOURCES += $(wildcard psx/dev/*.c)
SOURCES += $(wildcard frontend/*.c)

bin/psxe frontend/main.c:
	mkdir -p bin

	gcc $(SOURCES) -o bin/psxe \
		-I"." \
		-DOS_INFO="$(OS_INFO)" \
		-DREP_VERSION="$(VERSION_TAG)" \
		-DREP_COMMIT_HASH="$(COMMIT_HASH)" \
		-g -DLOG_USE_COLOR `sdl-config --cflags --libs` \
		-Ofast -Wno-overflow -Wall -pedantic

clean:
	rm -rf "bin"
