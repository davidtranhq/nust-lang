CXX = g++
GTEST_DIR = /opt/homebrew/Cellar/googletest/1.16.0

CXXFLAGS = -std=c++17 -I${GTEST_DIR}/include -Iinclude -I/opt/homebrew/include -g
LDFLAGS = -L${GTEST_DIR}/lib -lgtest -lgtest_main -pthread

SRC_DIR = src
OBJ_DIR = build
TEST_DIR = test

# Main program sources (excluding main.cpp)
LIB_SRCS = $(filter-out $(SRC_DIR)/main.cpp, $(wildcard $(SRC_DIR)/*.cpp)) $(wildcard $(SRC_DIR)/*/*.cpp)
LIB_OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(LIB_SRCS))

# Main program executable sources
MAIN_SRC = $(SRC_DIR)/main.cpp
MAIN_OBJ = $(OBJ_DIR)/main.o

# Test sources
TEST_SRCS = $(wildcard $(TEST_DIR)/*.cpp)
TEST_OBJS = $(patsubst $(TEST_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(TEST_SRCS))

TARGET = nust
TEST_TARGET = nust_test

.PHONY: all clean test

all: $(TARGET)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TARGET): $(LIB_OBJS) $(MAIN_OBJ)
	$(CXX) $^ -o $@

$(TEST_TARGET): $(LIB_OBJS) $(TEST_OBJS)
	$(CXX) $^ -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(TEST_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(TARGET) $(TEST_TARGET) 