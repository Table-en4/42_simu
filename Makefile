NAME    = examshell
CC      = cc
CFLAGS  = -Wall -Wextra -Werror

SRC     = exam.c
OBJ     = $(SRC:.c=.o)

UNAME_S := $(shell uname -s)

# ---------------------------------------------------------
# Config système
# ---------------------------------------------------------
ifeq ($(UNAME_S),Linux)
    LIBS    = -lreadline
    INSTALL = sudo apt-get update && sudo apt-get install -y libreadline-dev
endif

ifeq ($(UNAME_S),Darwin)
    READLINE_PREFIX := $(shell brew --prefix readline 2>/dev/null)
    CFLAGS  += -I$(READLINE_PREFIX)/include
    LIBS    = -L$(READLINE_PREFIX)/lib -lreadline
    INSTALL = brew install readline
endif

# ---------------------------------------------------------
# Build
# ---------------------------------------------------------
all: check_readline $(NAME)
	./$(NAME)

# ---------------------------------------------------------
# Vérifie readline et l'installe si absent
# ---------------------------------------------------------
check_readline:
	@echo "Checking for readline..."
ifeq ($(UNAME_S),Linux)
	@if ! ldconfig -p 2>/dev/null | grep -q readline; then \
		echo "Readline not found. Installing..."; \
		$(INSTALL); \
	else \
		echo "Readline is already installed."; \
	fi
endif
ifeq ($(UNAME_S),Darwin)
	@if ! brew list readline >/dev/null 2>&1; then \
		echo "Readline not found. Installing..."; \
		$(INSTALL); \
	else \
		echo "Readline is already installed."; \
	fi
endif

# ---------------------------------------------------------
# Compilation
# ---------------------------------------------------------
$(NAME): $(OBJ)
	$(CC) $(CFLAGS) -o $(NAME) $(OBJ) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# ---------------------------------------------------------
# Clean
# ---------------------------------------------------------
clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re check_readline
