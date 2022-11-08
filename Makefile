# Makefile pro program Simulator cislicovych obvodu, IMS, 2011/2012

CPP=g++
CPPFLAGS= -pedantic -W

# Compile the program.
main : main.cpp
	$(CPP) $(CPPFLAGS) -o gatesim main.cpp

# command to be executed.
clean:
	rm -f main
run:
	./gatesim -t 10 -f syncJK.xml -o text

