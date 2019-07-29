CC=gcc
CFLAGS=-O3 -Wall -c -I.
LEX=flex
YACC=bison -d -v
OBJ=../bin/bison.o ../bin/lex.o ../bin/initialize.o ../bin/highlevel_initializer.o ../bin/main.o ../bin/malbolge.o

all: lmfao

../bin:
	mkdir ../bin/

../bin/lmfao.tab.c: lmfao.y
	$(YACC) -o ../bin/lmfao.tab.c lmfao.y

../bin/bison.o: ../bin/lmfao.tab.c
	$(CC) $(CFLAGS) -o ../bin/bison.o ../bin/lmfao.tab.c

../bin/lex.yy.c: lmfao.l ../bin/lmfao.tab.c
	$(LEX) -o ../bin/lex.yy.c lmfao.l

../bin/lex.o: ../bin/lex.yy.c
	$(CC) $(CFLAGS) -o ../bin/lex.o ../bin/lex.yy.c

../bin/%.o: %.c *.h
	$(CC) $(CFLAGS) -o $@ $<

lmfao: ../bin $(OBJ)
	gcc $(OBJ) -o ../bin/lmfao -lfl

clean:
	rm -rf ../bin/

