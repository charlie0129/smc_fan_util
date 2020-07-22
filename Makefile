CC = g++
CFLAGS = -Wall -O3 -framework IOKit
# CPPFLAGS = -DCMD_TOOL_BUILD

all: smc_fan_util

smc_fan_util: smc_fan_util.o
	$(CC) $(CFLAGS) -o smc_fan_util smc_fan_util.o
smc_fan_util.o: smc.h smc_fan_util.c smc.cpp
	$(CC) $(CPPFLAGS) -c smc_fan_util.c smc.cpp


clean:
	-rm -f smc_fan_util.o
