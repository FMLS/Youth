#!Makefile
# ------------------------------------------------------------------------
#
# 项目的 Makefile 示例，对于此结构代码，该 Makefile 均通用。
#
# ------------------------------------------------------------------------
C_SOURCES = $(shell find . -name "*.c")
C_OBJECTS = $(patsubst %.c, %.o, $(C_SOURCES))

CC = gcc
C_FLAGS = -D_REENTRANT -DSERVER_DEBUG -c -Wall -Iinclude -g "-fno-stack-protector"
LINK_FLAGS = -lpthread
PROGRAM = out.youth

all: $(C_OBJECTS) 
	@echo Link ...
	$(CC) $(C_OBJECTS) $(LINK_FLAGS) -o $(PROGRAM)

.c.o:
	@echo Compiler $< ...
	$(CC) $(C_FLAGS) $< -o $@

.PHONY:clean
clean:
	@echo Clean all ...
	$(RM) $(C_OBJECTS) $(PROGRAM)
