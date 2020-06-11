VERSION = 0.1
NAME = VTSPS2-EXEC

EE_BIN = $(NAME).ELF
EE_BIN_PACKED = $(NAME)-packed.ELF
EE_BIN_STRIPPED = $(NAME)-stripped.ELF
EE_OBJS = $(NAME).o pad.o loader_elf.o
EE_LIBS = -lc -lpatches -ldebug -lpad

all:
	@echo "======================================="
	@echo "=== Building $(NAME) v$(VERSION) ==="
	@echo "======================================="
	$(MAKE) $(EE_BIN_PACKED)

clean:
	@echo "================"
	@echo "=== Cleaning ==="
	@echo "================"
	rm -f *.elf *.o *.s *.bak loader/*.o loader/*.elf

#wLaunchELF's loader.elf

loader/loader.elf: loader
	$(MAKE) -C $<

loader_elf.s: loader/loader.elf
	bin2s $< $@ loader_elf
	
run: $(EE_BIN)
	ps2client execee host:$(EE_BIN)

reset:
	ps2client reset

$(EE_BIN_STRIPPED): $(EE_BIN)
	@echo "================="
	@echo "=== Stripping ==="
	@echo "================="
	$(EE_STRIP) -o $@ $<
	
$(EE_BIN_PACKED): $(EE_BIN_STRIPPED)
# Uncomment to compress ELF. Adjust path to match your environment
	@echo "==================="
	@echo "=== Compressing ==="
	@echo "==================="
	~/ps2homebrew/ps2-packer/ps2-packer -v $< $@
	
include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal