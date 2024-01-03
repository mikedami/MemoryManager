CXX = g++
CXXFLAGS = -std=c++11 -Wall

TARGET_OBJ = MemoryManager.o

SRCS = MemoryManager.cpp

all: $(TARGET_OBJ)

$(TARGET_OBJ): $(SRCS)
	$(CXX) $(CXXFLAGS) -c $< -o $@
