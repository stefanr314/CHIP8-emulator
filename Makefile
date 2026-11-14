CC = gcc
DEBUG ?= 0

ifeq ($(DEBUG), 1)
	#debug mode
	MODE := debug
	CFLAGS := -g3 -O0 -DDEBUG
else
	MODE := release
	CFLAGS := -O2 -DNDEBUG
endif

BUILDDIR := build/$(MODE)
OBJDIR := $(BUILDDIR)/obj
BINDIR := $(BUILDDIR)/bin

SRCDIR := src

TARGET := $(BINDIR)/chip8_emu

override CFLAGS += -Wall -Wextra -std=c11 -pedantic
override CPPFLAGS += -MMD -MP
SDL_CFLAGS := $(shell pkgconf --cflags sdl2)
SDL_LIBS := $(shell pkgconf --libs sdl2)

LDLIBS := $(SDL_LIBS)
LDFLAGS :=

ALL_CFLAGS := $(CFLAGS) $(SDL_CFLAGS)

SRCS := $(wildcard $(SRCDIR)/*.c)
OBJS := $(SRCS:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
DEPS := $(OBJS:.o=.d)

.PHONY: all clean

all: $(TARGET)

# let's write rules for linking phase
$(TARGET): $(OBJS) | $(BINDIR)
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)

# let's write implicit rule for compiling
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CPPFLAGS) $(ALL_CFLAGS) -c $< -o $@

$(OBJDIR) $(BINDIR):
	mkdir -p $@ 

clean:
	rm -rf build 

-include $(DEPS)
