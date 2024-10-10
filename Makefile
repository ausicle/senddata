ANSI_COLOR_RED     := \033[0;31m
ANSI_COLOR_GREEN   := \033[0;32m
ANSI_COLOR_YELLOW  := \033[0;33m
ANSI_COLOR_BLUE    := \033[0;34m
ANSI_COLOR_MAGENTA := \033[0;35m
ANSI_COLOR_CYAN    := \033[0;36m
ANSI_COLOR_RESET   := \033[0m

ADDR ?= 127.0.0.1

TARGET ?= senddata.out
OBJDIR = .obj
SRCS := args.c encryption.c help.c main.c networking.c
HEADS := args.h encryption.h help.h networking.h

CC := clang
CSTD := c11
CFLAGS += -Wall -Wextra -pedantic -pedantic-errors -MMD -MP -std=$(CSTD)
CFLAGS += $(shell pkg-config --cflags openssl)
LDFLAGS += -lssl -lcrypto

ifeq ($(GUI_METHOD), gtk4)
SRCS += gtk.c
HEADS += gtk.h
CFLAGS += -DUSE_GTK=4
CFLAGS += $(shell pkg-config --cflags glib-2.0 gtk4 gio-2.0)
LDFLAGS += $(shell pkg-config --libs glib-2.0 gtk4 gio-2.0)
endif

OBJS := $(SRCS:%.c=$(OBJDIR)/%.o)
DEPS := $(OBJS:.o=.d)

.PHONY: all clean format

all: pre-build build

pre-build:
ifneq ($(wildcard $(TARGET)),"")
	$(RM) $(OBJDIR)/main.d $(OBJDIR)/main.o
endif
build: $(TARGET)

$(TARGET): $(OBJS)
	@printf "$(ANSI_COLOR_GREEN)--> Linking $@$(ANSI_COLOR_RESET)\n"
	$(CC) $(LDFLAGS) -o $@ $^
$(OBJDIR)/%.o: %.c | $(OBJDIR)
	@printf "$(ANSI_COLOR_GREEN)--> Compiling $@$(ANSI_COLOR_RESET)\n"
	$(CC) $(CFLAGS) -c -o $@ $<
$(OBJDIR):
	mkdir -p $(OBJDIR)
format: $(SRCS) $(HEADS)
	@printf "$(ANSI_COLOR_RED)--> Formatting$(ANSI_COLOR_RESET)\n"
	clang-tidy --fix $^ -- $(CFLAGS)
	clang-format --verbose -i $^
clean:
	@printf "$(ANSI_COLOR_RED)--> Cleaning$(ANSI_COLOR_RESET)\n"
	$(RM) $(TARGET) $(OBJS)

-include $(DEPS)
