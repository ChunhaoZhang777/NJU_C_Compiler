CC = gcc
FLEX = flex
BISON = bison
CFLAGS ?= -std=c99 -O2 -g -Wall -Wextra
CPPFLAGS ?=
LDFLAGS ?=
LDLIBS ?=

BUILD := build
SOURCES := $(wildcard *.c)
OBJECTS := $(addprefix $(BUILD)/,$(SOURCES:.c=.o)) $(BUILD)/parser.tab.o $(BUILD)/lexer.yy.o
INCLUDES := -I. -I$(BUILD)

.PHONY: all clean test test-mips
all: parser

parser: $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(BUILD):
	mkdir -p $@

# Bison writes both files; recreate the header if it was deleted separately.
$(BUILD)/parser.tab.c: parser.y | $(BUILD)
	$(BISON) -d -v -o $@ $<
$(BUILD)/parser.tab.h: $(BUILD)/parser.tab.c
	@test -f $@ || $(BISON) -d -v -o $(BUILD)/parser.tab.c parser.y
$(BUILD)/lexer.yy.c: lexer.l $(BUILD)/parser.tab.h
	$(FLEX) -o $@ $<

$(BUILD)/parser.tab.o: $(BUILD)/parser.tab.c $(BUILD)/parser.tab.h
	$(CC) $(CPPFLAGS) $(INCLUDES) $(CFLAGS) -MMD -MP -c -o $@ $<
$(BUILD)/lexer.yy.o: $(BUILD)/lexer.yy.c $(BUILD)/parser.tab.h
	$(CC) $(CPPFLAGS) $(INCLUDES) $(CFLAGS) -MMD -MP -c -o $@ $<
$(BUILD)/%.o: %.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(INCLUDES) $(CFLAGS) -MMD -MP -c -o $@ $<

-include $(OBJECTS:.o=.d)

test: parser
	python3 ../test/run_tests.py

test-mips: parser
	python3 ../test/run_tests.py --require-spim

clean:
	rm -rf $(BUILD) ../test/__pycache__
	rm -f parser *.o *.d lex.yy.c syntax.tab.c syntax.tab.h syntax.output
