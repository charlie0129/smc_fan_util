CC = gcc
CFLAGS = -mmacosx-version-min=10.4  -Wall -O3 -framework IOKit
# CPPFLAGS = -DCMD_TOOL_BUILD

all: sfc_manual smc


smc: smc.o
	$(CC) $(CFLAGS) -o smc smc.o
smc.o: smc.h smc.c
	$(CC) $(CPPFLAGS) -c smc.c


sfc_manual: sfc_manual.o
	$(CC) $(CFLAGS) -o sfc_manual sfc_manual.o
sfc_manual.o: smc.h sfc_manual.c
	$(CC) $(CPPFLAGS) -c sfc_manual.c


clean:
	-rm -f smc.o sfc_manual.o
