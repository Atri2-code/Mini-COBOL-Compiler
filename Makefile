CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -g
SRCS    = src/lexer.c src/ast.c src/parser.c src/semantic.c src/codegen.c src/main.c
TARGET  = minicobol

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^

test: $(TARGET)
	@echo "=== Running tests ==="
	@for f in tests/samples/*.cbl; do \
	    echo "Testing $$f ..."; \
	    ./$(TARGET) $$f -o /tmp/test_out.asm && \
	    nasm -f elf64 /tmp/test_out.asm -o /tmp/test_out.o && \
	    ld /tmp/test_out.o -o /tmp/test_bin && \
	    echo "  PASS"; \
	done

tokens: $(TARGET)
	./$(TARGET) samples/hello.cbl --tokens

ast: $(TARGET)
	./$(TARGET) samples/hello.cbl --ast

clean:
	rm -f $(TARGET) *.o *.asm

.PHONY: all test tokens ast clean
