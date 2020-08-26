CC = gcc
CXXFLAGS = -O3 -framework IOKit -Wall
MACROS = -DDAEMON

all: build build/smc_fan_util

build/smc_fan_util: build/smc_fan_util.o build/smc.o
	$(CC) $(CXXFLAGS) $(MACROS) -o ./build/smc_fan_util ./build/smc_fan_util.o ./build/smc.o

build/smc.o: smc.h smc.c
	$(CC) $(CXXFLAGS) $(MACROS) -c smc.c -o ./build/smc.o

build/smc_fan_util.o: smc_fan_util.c
	$(CC) $(CXXFLAGS) $(MACROS) -c smc_fan_util.c -o ./build/smc_fan_util.o

build:
	mkdir -p build

clean:
	rm -rf ./build
