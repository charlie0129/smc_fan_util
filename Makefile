CC = gcc
CFLAGS = -O3 -Wall
LDFLAGS = -framework IOKit
MACROS = # -DDEBUG

all: build build/smc_fan_util

build/smc_fan_util: build/smc_fan_util.o build/smc.o
	$(CC) $(LDFLAGS) $(MACROS) -o ./build/smc_fan_util ./build/smc_fan_util.o ./build/smc.o

build/smc.o: smc.h smc.c
	$(CC) $(CFLAGS) $(MACROS) -c smc.c -o ./build/smc.o

build/smc_fan_util.o: smc_fan_util.c
	$(CC) $(CFLAGS) $(MACROS) -c smc_fan_util.c -o ./build/smc_fan_util.o

run: build/smc_fan_util
	sudo ./build/smc_fan_util --SMC-enhanced

build:
	mkdir -p build

clean:
	rm -rf ./build
