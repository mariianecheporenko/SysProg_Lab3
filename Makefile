CXX ?= g++
FLEX ?= flex
BISON ?= bison
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2

TARGET = parser
GENERATED = parser.tab.cpp parser.tab.h lex.yy.cpp

.PHONY: all clean
all: $(TARGET)

parser.tab.cpp: parser.y ast.h
	$(BISON) -d --defines=parser.tab.h -o parser.tab.cpp parser.y

parser.tab.h: parser.tab.cpp
	@test -f $@ || $(BISON) -d --defines=parser.tab.h -o parser.tab.cpp parser.y

lex.yy.cpp: lexer.l parser.tab.h
	$(FLEX) -o $@ lexer.l

$(TARGET): parser.tab.cpp lex.yy.cpp ast.cpp ast.h
	$(CXX) $(CXXFLAGS) -o $@ parser.tab.cpp lex.yy.cpp ast.cpp

clean:
	$(RM) $(TARGET) mini_c $(GENERATED)
