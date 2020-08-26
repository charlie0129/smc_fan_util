CC = gcc
CXXFLAGS = -O3 -framework IOKit -Wall
MACROS = -DDAEMON

all: smc_fan_util

smc_fan_util: smc_fan_util.o
	$(CC) $(CXXFLAGS) $(MACROS) -o ./build/smc_fan_util ./build/smc_fan_util.o ./build/smc.o

smc_fan_util.o: smc.h smc_fan_util.c smc.c builddir
	$(CC) $(CXXFLAGS) $(MACROS) -c smc_fan_util.c -o ./build/smc_fan_util.o
	$(CC) $(CXXFLAGS) $(MACROS) -c smc.c -o ./build/smc.o

builddir:
	mkdir build


clean:
	rm -rf ./build
