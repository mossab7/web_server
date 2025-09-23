NAME = webserv

# base directories
INC_DIR = include
OBJ_DIR = .objects

# base flags
# the MMD flag is used to track changes in header files
CXXFLAGS =  -Wall -Wextra -Werror -std=c++98 -MMD
CXXFLAGS += -I$(INC_DIR)/utils -I$(INC_DIR)/Config -I$(INC_DIR)/http -I$(INC_DIR)/server -I$(INC_DIR)/Routing -g3

# project files.
# todo: remove the wildcard functions
MAIN = src/main.cpp
PARSING = $(wildcard src/Config/*.cpp)
ROUTING = $(wildcard src/Routing/*.cpp)
HTTP = $(wildcard src/http/*.cpp)
SERVER = $(wildcard src/server/*.cpp)
UTILS = $(wildcard src/utils/*.cpp)


SRC = $(MAIN) $(UTILS) $(PARSING) $(ROUTING) $(HTTP) $(SERVER)
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