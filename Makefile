OUTDIR ?= out
ADDR ?= 127.0.0.1
PORT ?= 43337

ANSI_COLOR_RED     = \033[0;31m
ANSI_COLOR_GREEN   = \033[0;32m
ANSI_COLOR_YELLOW  = \033[0;33m
ANSI_COLOR_BLUE    = \033[0;34m
ANSI_COLOR_MAGENTA = \033[0;35m
ANSI_COLOR_CYAN    = \033[0;36m
ANSI_COLOR_RESET   = \033[0m

server: senddata
	@printf "$(ANSI_COLOR_YELLOW)--> Running server (receive)$(ANSI_COLOR_RESET)\n"
	./senddata receive
client: senddata
	@printf "$(ANSI_COLOR_YELLOW)--> Running client (send)$(ANSI_COLOR_RESET)\n"
	./senddata send $(ADDR)
clean:
	@printf "$(ANSI_COLOR_RED)--> Cleaning$(ANSI_COLOR_RESET)\n"
	rm senddata
senddata: main.c
	@printf "$(ANSI_COLOR_GREEN)--> Compile client$(ANSI_COLOR_RESET)\n"
	clang -o senddata main.c
