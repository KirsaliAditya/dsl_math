# Makefile for hybrid DSL interpreter (LLVM + AST)

# Files
LEXER = scanner.l
PARSER = parser.y
AST = ast.h
MAIN = main.cpp

# Output files
PARSER_CPP = parser.tab.cpp
PARSER_HPP = parser.tab.hpp
LEXER_CPP = lexer.yy.cpp
OBJS = main.o $(PARSER_CPP:.cpp=.o) $(LEXER_CPP:.cpp=.o)

# Compiler and flags
CXX = clang++
CXXFLAGS = -std=c++17 -fexceptions -g
LLVM_CFLAGS = `llvm-config --cxxflags`
LLVM_LDFLAGS = `llvm-config --ldflags --system-libs --libs core executionengine native`

# Executable name
TARGET = dsl

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LLVM_LDFLAGS)

main.o: main.cpp $(AST) parser.tab.hpp
	$(CXX) $(CXXFLAGS) $(LLVM_CFLAGS) -fexceptions -c $<

parser.tab.cpp parser.tab.hpp: $(PARSER)
	bison -d -o $(PARSER_CPP) $(PARSER)

parser.tab.o: parser.tab.cpp parser.tab.hpp $(AST)
	$(CXX) $(CXXFLAGS) $(LLVM_CFLAGS) -fexceptions -c parser.tab.cpp

lexer.yy.cpp: $(LEXER) parser.tab.hpp
	flex -o $@ $(LEXER)

lexer.yy.o: lexer.yy.cpp parser.tab.hpp
	$(CXX) $(CXXFLAGS) $(LLVM_CFLAGS) -fexceptions -c $<

clean:
	rm -f $(TARGET) *.o parser.tab.* lexer.yy.cpp
