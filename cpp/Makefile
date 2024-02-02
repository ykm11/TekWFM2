CXX=g++
SRC=./src
HDIR=./include
FLAG=-O3
TARGET=main

all: wfm.o
	$(CXX) -I $(HDIR) $(TARGET).cpp wfm.o -o $(TARGET) $(FLAG)
	
wfm.o: $(SRC)/wfm_read.cpp
	$(CXX) -c -I $(HDIR) $< -o $@ $(FLAG)

clean:
	$(RM) *.o $(TARGET)

.PHONY:
	clean
