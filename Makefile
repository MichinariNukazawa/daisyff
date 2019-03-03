
APP		:= ./daisyff.exe
SOURCE_DIR	:= ./src
OBJECT_DIR	:= ./object

INCLUDE		:= -I./ -I./include
CFLAGS		:= -std=c11 -lm -g
CFLAGS		+= -fno-strict-aliasing
CFLAGS		+= -W -Wall -Wextra
CFLAGS		+= -Werror
CFLAGS		+= -Wno-unused-parameter
CFLAGS		+= -Wunused -Wimplicit-function-declaration \
		 -Wincompatible-pointer-types \
		 -Wbad-function-cast -Wcast-align \
		 -Wdisabled-optimization -Wdouble-promotion \
		 -Wformat-y2k -Wuninitialized -Winit-self \
		 -Wlogical-op -Wmissing-include-dirs \
		 -Wshadow -Wswitch-default -Wundef \
		 -Wwrite-strings -Wunused-macros
CFLAGS		+= -Wno-sign-compare # @todo
CFLAGS		+= -Wno-bad-function-cast # @todo

SOURCES		:= $(wildcard $(SOURCE_DIR)/*.c)
OBJECTS		:= $(subst $(SOURCE_DIR),$(OBJECT_DIR),$(SOURCES:.c=.o))
DEPENDS		:= $(OBJECTS:.o=.d)

.PHONY: all run test clean
.PHONY: dump com

run: $(APP)
	$(APP) DaisyMini

all: run test dump

gdb: $(APP)
	gdb --args $(APP) DaisyMini

test: ./test.exe
	make clean
	./test.exe
	make
	make dump
	./daisydump.exe DaisyMini.otf -t cmap

./test.exe: test/test.c src/*.h include/*.h
	gcc $< \
		$(CFLAGS) \
		$(INCLUDE) \
		-o ./test.exe

dump: src/daisydump.c src/*.h include/*.h
	mkdir -p $(OBJECT_DIR)
	bash ./version.sh $(OBJECT_DIR)
	gcc \
		$< \
		$(OBJECT_DIR)/version.c \
		$(CFLAGS) \
		$(INCLUDE) \
		-o ./daisydump.exe
	./daisydump.exe DaisyMini.otf

com:
	file DaisyMini.otf
	ttx DaisyMini.otf

$(APP): src/daisyff.c src/*.h include/*.h
	gcc $< \
		$(CFLAGS) $(INCLUDE) \
		-o $(APP)

clean:
	rm -rf $(APP) *.otf
	rm -rf *.d

-include $(DEPENDS)

