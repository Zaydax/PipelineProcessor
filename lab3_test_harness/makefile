all: lc3bsim 

lc3bsim: lc3bsim3.c
	gcc -Wall -Wno-unused-variable lc3bsim3.c -lm -o lc3bsim

grade: lc3bsim
	./run.sh



clean:
	rm -f A B lc3bsim test/lc3bsim test/dumpsim test/out
