SRC = $(shell find . -name "*.cpp" | grep -v "test")

OBJ_DIR = ./obj/

SRC_DIR = ./

OBJ = $(addprefix $(OBJ_DIR), $(SRC:.cpp=.o))

CC = c++

INCD = $(shell find . -name "*.hpp" | grep -v "test")

INC = include/

CFLAGS = -std=c++98 -Wall -Wextra -fsanitize=address -g -ggdb3 


NAME = webserv

all : $(NAME) run

$(NAME) : $(OBJ)
	$(CC)  $(CFLAGS)  $(OBJ)  -o $(NAME) 

$(OBJ_DIR)%.o: $(SRC_DIR)%.cpp $(INCD)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I$(INC)  -c $< -o $@

clean :
	rm -rf $(OBJ_DIR)

fclean : clean
	make -f MakeTest fclean
	rm -f $(NAME)

re : fclean all

test:
	make -f MakeTest
	@./serverTest
run :
	@./$(NAME)


.PHONY: all clean fclean re test
