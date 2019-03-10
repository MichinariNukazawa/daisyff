
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

.PHONY: gdb
gdb: $(APP)
	gdb --args $(APP) DaisyMini

.PHONY: utest
utest: ./test.exe
	./test.exe

test:
	make clean
	make
	make dump
	make utest
	./test/test.sh
	# strict check DaisyMini.otf
	./daisydump.exe DaisyMini.otf --strict > /dev/null

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
	rm -rf $(OBJECT_DIR)
	rm -rf *.d
	rm -rf $(APP)
	rm -rf *.exe
	rm -rf *.otf

clean-ttx:
	- find . -name "*\.ttx" -type f | xargs rm
	- rm -f *.ttf
	- rm -f example/*.ttf
	git checkout example/*.ttf

-include $(DEPENDS)

