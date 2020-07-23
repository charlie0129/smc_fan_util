CC = g++
CXXFLAGS = -Wall -O3 -framework IOKit

all: smc_fan_util

smc_fan_util: smc_fan_util.o
	$(CC) $(CXXFLAGS) -o smc_fan_util smc_fan_util.o

smc_fan_util.o: smc.h smc_fan_util.cpp smc.cpp
	$(CC) $(CXXFLAGS) -c smc_fan_util.cpp smc.cpp


clean:
	-rm -f smc_fan_util.o
