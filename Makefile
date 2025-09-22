NAME = webserv

# base directories
INC_DIR = include
OBJ_DIR = .objects

# base flags
# the MMD flag is used to track changes in header files
CXXFLAGS =  -Wall -Wextra -Werror -std=c++98 -MMD
CXXFLAGS += -I$(INC_DIR)/utils \
			-I$(INC_DIR)/error_pages \
			-I$(INC_DIR)/Config \
			-I$(INC_DIR)/Routing \
			-I$(INC_DIR)/http \
			-g3
# project files.
# todo: remove the wildcard functions
MAIN = src/main.cpp
PARSING = $(wildcard src/Config/*.cpp)
ROUTING = $(wildcard src/Routing/*.cpp)
ERRORS = $(wildcard src/error_pages/*.cpp)
HTTP = $(wildcard src/http/*.cpp)
UTILS = $(wildcard src/utils/*.cpp)


SRC = $(MAIN) $(UTILS) $(ERRORS) $(PARSING) $(ROUTING) $(HTTP)
OBJ = $(SRC:%.cpp=$(OBJ_DIR)/%.o)


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

# track dependancies
-include $(OBJ:.o=.d)

.PHONY: all clean fclean re
.SECONDARY: $(OBJ)