CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -Isrc
BUILD = build
BIN = f1code

OBJS = $(BUILD)/ast.o $(BUILD)/symtab.o $(BUILD)/semantico.o $(BUILD)/codegen_andante.o \
       $(BUILD)/main.o $(BUILD)/f1code.tab.o $(BUILD)/lex.yy.o

.PHONY: all clean test

all: $(BIN)

$(BUILD):
	mkdir -p $(BUILD)

# Los archivos de Flex y Bison se regeneran desde src/; nunca se editan a mano
$(BUILD)/f1code.tab.c $(BUILD)/f1code.tab.h: src/f1code.y | $(BUILD)
	bison -d -Wall -o $(BUILD)/f1code.tab.c src/f1code.y

$(BUILD)/lex.yy.c: src/f1code.l $(BUILD)/f1code.tab.h | $(BUILD)
	flex -o $(BUILD)/lex.yy.c src/f1code.l

$(BUILD)/%.o: src/%.c include/*.h $(BUILD)/f1code.tab.h | $(BUILD)
	$(CC) $(CFLAGS) -I$(BUILD) -c $< -o $@

$(BUILD)/f1code.tab.o: $(BUILD)/f1code.tab.c | $(BUILD)
	$(CC) $(CFLAGS) -I$(BUILD) -c $(BUILD)/f1code.tab.c -o $(BUILD)/f1code.tab.o

$(BUILD)/lex.yy.o: $(BUILD)/lex.yy.c | $(BUILD)
	$(CC) $(CFLAGS) -Wno-unused-function -I$(BUILD) -c $(BUILD)/lex.yy.c -o $(BUILD)/lex.yy.o

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $(BIN) $^

# Validos: deben traducirse igual a tests/esperado/. Invalidos: deben rechazarse sin generar .and
test: $(BIN)
	sh tests/run_tests.sh

clean:
	rm -rf $(BUILD) $(BIN) $(BIN).exe tests/salida
