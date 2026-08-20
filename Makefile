CXX = g++
CXXFLAGS = -std=c++17 -O3 -march=native -flto -Wall -pthread
SRC = src/board.cpp src/eval.cpp src/search.cpp src/main.cpp
OBJ = $(SRC:.cpp=.o)
TARGET = chess-engine

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJ)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
