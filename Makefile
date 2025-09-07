NAME = webserv

# base directories
INC_DIR = include
OBJ_DIR = .objects

# base flags
# the MMD flag is used to track changes in header files
CXXFLAGS =  -Wall -Wextra -Werror -std=c++98 -MMD
CXXFLAGS += -I$(INC_DIR)/utils -I$(INC_DIR)/Config -g3

# project files.
# todo: remove the wildcard functions
MAIN = src/main.cpp
PARSING = $(wildcard src/Config/*.cpp)
UTILS = $(wildcard src/utils/*.cpp)


SRC = $(MAIN) $(UTILS) $(PARSING)
OBJ = $(SRC:%.cpp=$(OBJ_DIR)/%.o)

# track header files too
-include $(OBJ:.o=.d)

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(OBJ) $(CXXFLAGS) -o $@

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -rf $(NAME)

re: fclean all

.PHONY: all clean fclean re
.SECONDARY: $(OBJ)