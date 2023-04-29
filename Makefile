CC    = g++
STD   = -std=c++17
SRC   = ./src/*.cpp
LIBS  = -lSDL2 -lSDL2main -lSDL2_ttf
IPATH = 
LPATH = 
ARGS  = -Wall

ifeq ($(OS), Windows_NT)
IPATH += -I./include
LPATH += -L./lib
ARGS  += -static-libstdc++
else
CC    = clang++
endif

all:
	$(CC) $(STD) $(SRC) $(IPATH) $(LPATH) $(LIBS) $(ARGS) -o rouge