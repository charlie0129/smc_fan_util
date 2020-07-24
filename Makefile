CC = gcc
CXXFLAGS = -O3 -framework IOKit -Wall

all: smc_fan_util

smc_fan_util: smc_fan_util.o
	$(CC) $(CXXFLAGS) -o smc_fan_util smc_fan_util.o smc.o

smc_fan_util.o: smc.h smc_fan_util.c smc.c
	$(CC) $(CXXFLAGS) -c smc_fan_util.c smc.c


clean:
	-rm -f smc_fan_util.o
