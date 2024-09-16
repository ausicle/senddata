ADDR ?= 127.0.0.1

TARGET ?= senddata.out
OBJDIR = .obj
SRCS := $(wildcard *.c)
OBJS := $(SRCS:%.c=$(OBJDIR)/%.o)
DEPS := $(OBJS:.o=.d)
FORMATS := $(SRCS) $(wildcard */*.h)

CC := clang
CFLAGS += -Wall -Wextra -pedantic -pedantic-errors -MMD -MP

ANSI_COLOR_RED     := \033[0;31m
ANSI_COLOR_GREEN   := \033[0;32m
ANSI_COLOR_YELLOW  := \033[0;33m
ANSI_COLOR_BLUE    := \033[0;34m
ANSI_COLOR_MAGENTA := \033[0;35m
ANSI_COLOR_CYAN    := \033[0;36m
ANSI_COLOR_RESET   := \033[0m

.PHONY: compile clean format

$(TARGET): $(OBJS)
	@printf "$(ANSI_COLOR_GREEN)--> Linking $@$(ANSI_COLOR_RESET)\n"
	$(CC) -o $@ $^
$(OBJDIR)/%.o: %.c | $(OBJDIR)
	@printf "$(ANSI_COLOR_GREEN)--> Compiling $@$(ANSI_COLOR_RESET)\n"
	$(CC) $(CFLAGS) -c -o $@ $<
$(OBJDIR):
	mkdir -p $(OBJDIR)
format: $(FORMATS)
	@printf "$(ANSI_COLOR_RED)--> Formatting$(ANSI_COLOR_RESET)\n"
	clang-format --verbose -i $^
clean:
	@printf "$(ANSI_COLOR_RED)--> Cleaning$(ANSI_COLOR_RESET)\n"
	$(RM) $(TARGET) $(OBJS)

-include $(DEPS)
