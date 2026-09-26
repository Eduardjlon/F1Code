CC = gcc
CFLAGS = -Wall -Iinclude -Isrc
BUILD = build
BIN = f1code

.PHONY: all clean test

all: $(BIN)

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/f1code.tab.c $(BUILD)/f1code.tab.h: src/f1code.y | $(BUILD)
	bison -d -o $(BUILD)/f1code.tab.c src/f1code.y

$(BUILD)/lex.yy.c: src/f1code.l $(BUILD)/f1code.tab.h | $(BUILD)
	flex -o $(BUILD)/lex.yy.c src/f1code.l

$(BUILD)/ast.o: src/ast.c include/ast.h | $(BUILD)
	$(CC) $(CFLAGS) -I$(BUILD) -c src/ast.c -o $(BUILD)/ast.o

$(BUILD)/symtab.o: src/symtab.c include/symtab.h | $(BUILD)
	$(CC) $(CFLAGS) -I$(BUILD) -c src/symtab.c -o $(BUILD)/symtab.o

$(BUILD)/main.o: src/main.c | $(BUILD)
	$(CC) $(CFLAGS) -I$(BUILD) -c src/main.c -o $(BUILD)/main.o

$(BUILD)/f1code.tab.o: $(BUILD)/f1code.tab.c | $(BUILD)
	$(CC) $(CFLAGS) -I$(BUILD) -c $(BUILD)/f1code.tab.c -o $(BUILD)/f1code.tab.o

$(BUILD)/lex.yy.o: $(BUILD)/lex.yy.c | $(BUILD)
	$(CC) $(CFLAGS) -I$(BUILD) -c $(BUILD)/lex.yy.c -o $(BUILD)/lex.yy.o

$(BIN): $(BUILD)/ast.o $(BUILD)/symtab.o $(BUILD)/main.o $(BUILD)/f1code.tab.o $(BUILD)/lex.yy.o
	$(CC) $(CFLAGS) -o $(BIN) $^

test: $(BIN)
	@echo ">>> Prueba con programa VALIDO:"
	./$(BIN) tests/valido.f1
	@echo ""
	@echo ">>> Prueba con programa INVALIDO:"
	./$(BIN) tests/invalido.f1

clean:
	rm -rf $(BUILD) $(BIN)
