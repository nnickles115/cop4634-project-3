# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -g -Wall -std=c++11 -lpthread

# Source files
SOURCE = lizards.cpp
UNI_SOURCE = lizardsUni.cpp

# Object files
OBJECT = $(SOURCE:.cpp=.o)
UNI_OBJECT = $(UNI_SOURCE:.cpp=.o)

# Targets
TARGET = lizards
UNI_TARGET = lizardsUni

# Default rule
all: $(TARGET)

# Rule for the original lizards program
$(TARGET): $(OBJECT)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECT)
	rm -f $(OBJECT)

# Rule for the unidirectional version
$(UNI_TARGET): $(UNI_OBJECT)
	$(CXX) $(CXXFLAGS) -o $(UNI_TARGET) $(UNI_OBJECT)
	rm -f $(UNI_OBJECT)

# Compile .cpp files into .o files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean rule
clean:
	rm -f *.o $(TARGET) $(UNI_TARGET)

# Unidirectional rule
uni: $(UNI_TARGET)