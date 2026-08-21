.PHONY: all clean run

TARGET := triangles-rs
PROFILE := release
ELF := target/powerpc-unknown-eabi/$(PROFILE)/$(TARGET)
DOL := $(TARGET).dol

all:
	cargo +nightly build --$(PROFILE)
	elf2dol $(ELF) $(DOL)

clean:
	cargo clean
	$(RM) $(DOL)

run: all
	wiiload $(DOL)
