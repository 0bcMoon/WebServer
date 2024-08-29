SRC = $(shell find . -name "*.cpp" | grep -v "test")

OBJ_DIR = ./obj/

SRC_DIR = ./

OBJ = $(addprefix $(OBJ_DIR), $(SRC:.cpp=.o))

CC = c++

INC = $(shell find . -name "*.hpp" | grep -v "test")

CFLAGS = -std=c++98 -Wall -Wextra -Werror


NAME = webserv

all : $(NAME)

$(NAME) : $(OBJ)
	$(CC)  $(CFLAGS)  $(OBJ)  -o $(NAME) 

$(OBJ_DIR)%.o: $(SRC_DIR)%.cpp $(INC)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I$(INC)  -c $< -o $@

clean :
	rm -rf $(OBJ_DIR)

fclean : clean
	rm -f $(NAME)

re : fclean all

test:
	make -f MakeTest


.PHONY: all clean fclean re
