CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Werror -pedantic

TARGET = main
SOURCES = main.cpp cpu.cpp assembler.cpp
TEST_TARGET = cpu_tests
TEST_SOURCES = tests.cpp cpu.cpp assembler.cpp

.PHONY: all clean rebuild test

all: $(TARGET)
	
$(TARGET): $(SOURCES) cpu.hpp assembler.hpp
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET)

$(TEST_TARGET): $(TEST_SOURCES) cpu.hpp assembler.hpp
	$(CXX) $(CXXFLAGS) $(TEST_SOURCES) -o $(TEST_TARGET)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

clean:
	rm -f $(TARGET) $(TEST_TARGET)

rebuild: clean all
