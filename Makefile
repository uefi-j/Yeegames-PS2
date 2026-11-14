EE_BIN = YEEWARE.ELF
EE_OBJS = src/main_ps2.o src/game.o src/font.o audsrv_irx.o freesd_irx.o

EE_INCS += -Isrc -I$(PS2DEV)/gsKit/include -I$(PS2SDK)/ports/include
EE_LDFLAGS += -L$(PS2DEV)/gsKit/lib -L$(PS2SDK)/ports/lib
EE_CFLAGS += -std=gnu99 -O2
EE_LIBS = -lgskit -ldmakit -lpad -laudsrv -lkernel -lc
EE_CLEAN = audsrv_irx.c freesd_irx.c

all: $(EE_BIN)

audsrv_irx.c: $(PS2SDK)/iop/irx/audsrv.irx
	bin2c $< $@ audsrv_irx
freesd_irx.c: $(PS2SDK)/iop/irx/freesd.irx
	bin2c $< $@ freesd_irx

clean:
	rm -f $(EE_BIN) $(EE_OBJS) $(EE_CLEAN)

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
