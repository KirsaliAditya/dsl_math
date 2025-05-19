# Makefile for hybrid DSL interpreter (LLVM + AST)

# Files
LEXER = scanner.l
PARSER = parser.y
AST_H = ast.h
AST_CPP = ast.cpp
MAIN = main.cpp

# Output files
PARSER_CPP = parser.tab.cpp
PARSER_HPP = parser.tab.hpp
LEXER_CPP = lexer.yy.cpp

OBJS = main.o parser.tab.o lexer.yy.o ast.o

# Compiler and flags
CXX = clang++
CXXFLAGS = -std=c++17 -fPIC
LLVM_CFLAGS = `llvm-config --cxxflags`
LLVM_LDFLAGS = `llvm-config --ldflags --system-libs --libs core executionengine native`

# Executable name
TARGET = dsl

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LLVM_LDFLAGS)

main.o: $(MAIN) $(AST_H) $(PARSER_HPP)
	$(CXX) $(CXXFLAGS) $(LLVM_CFLAGS) -fexceptions -c $(MAIN)

parser.tab.cpp parser.tab.hpp: $(PARSER)
	bison -d -o $(PARSER_CPP) $(PARSER)

parser.tab.o: $(PARSER_CPP) $(PARSER_HPP) $(AST_H)
	$(CXX) $(CXXFLAGS) $(LLVM_CFLAGS) -fexceptions -c $(PARSER_CPP)

lexer.yy.cpp: $(LEXER) $(PARSER_HPP)
	flex -o $@ $(LEXER)

lexer.yy.o: $(LEXER_CPP) $(PARSER_HPP)
	$(CXX) $(CXXFLAGS) $(LLVM_CFLAGS) -fexceptions -c $(LEXER_CPP)

ast.o: $(AST_CPP) $(AST_H)
	$(CXX) $(CXXFLAGS) $(LLVM_CFLAGS) -fexceptions -c $(AST_CPP)

clean:
	rm -f $(TARGET) $(OBJS) parser.tab.cpp parser.tab.hpp lexer.yy.cpp

.PHONY: all clean
