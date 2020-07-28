CC = gcc
CXXFLAGS = -O3 -framework IOKit -Wall
MACROS = -DDAEMON

all: smc_fan_util

smc_fan_util: smc_fan_util.o
	$(CC) $(CXXFLAGS) $(MACROS) -o smc_fan_util smc_fan_util.o smc.o

smc_fan_util.o: smc.h smc_fan_util.c smc.c
	$(CC) $(CXXFLAGS) $(MACROS) -c smc_fan_util.c smc.c


clean:
	-rm -f smc_fan_util.o smc.o smc_fan_util
