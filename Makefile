.ONESHELL:

CFLAGS += -g -DLOG_USE_COLOR -lSDL2 -lSDL2main
CFLAGS += -Ofast -Wno-overflow -Wall -pedantic

PLATFORM := $(shell uname -s)

ifeq ($(PLATFORM),Darwin)
	CFLAGS += -mmacosx-version-min=10.9 -Wno-newline-eof
endif

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
		$(CFLAGS)

clean:
	rm -rf "bin"
