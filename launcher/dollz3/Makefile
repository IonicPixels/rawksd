ifeq ($(OS),Windows_NT)
	DOL2ELF := dol2elf.exe
else
	DOL2ELF := dol2elf
endif

dol2elf: dol2elf.c
	$(CXX) -O2 -fwhole-program -o $@ $<

clean:
	rm -f $(DOL2ELF)

.PHONY: all clean
.DELETE_ON_ERROR:
