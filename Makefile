NAME    = examshell
CC      = cc
CFLAGS  = -Wall -Wextra -Werror

SRC     = exam.c
OBJ     = $(SRC:.c=.o)

LIBS    = -lreadline

all: check_readline $(NAME)
	./$(NAME)


# ---------------------------------------------------------
# Vérifie si readline est installé, sinon l'installe
# ---------------------------------------------------------
check_readline:
	@echo "Checking for readline..."
	@if ! ldconfig -p | grep -q readline; then \
		echo "Readline not found. Installing..."; \
		sudo apt-get update && sudo apt-get install -y libreadline-dev; \
	else \
		echo "Readline is already installed."; \
	fi

# ---------------------------------------------------------
# Compilation
# ---------------------------------------------------------
$(NAME): $(OBJ)
	$(CC) $(CFLAGS) -o $(NAME) $(OBJ) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# ---------------------------------------------------------
# Divers
# ---------------------------------------------------------
clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re check_readline