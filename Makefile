
APP		:= ./daisyff.exe
SOURCE_DIR	:= ./src
OBJECT_DIR	:= ./object

INCLUDE		:= -I./ -I./include
CFLAGS		:= -std=c11 -lm -g
CFLAGS		+= -fno-strict-aliasing
CFLAGS		+= -W -Wall -Wextra
CFLAGS		+= -Werror
CFLAGS		+= -Wno-unused-parameter
CFLAGS		+= -Wno-sign-compare

SOURCES		:= $(wildcard $(SOURCE_DIR)/*.c)
OBJECTS		:= $(subst $(SOURCE_DIR),$(OBJECT_DIR),$(SOURCES:.c=.o))
DEPENDS		:= $(OBJECTS:.o=.d)

.PHONY: all run test clean
.PHONY: dump com

run: $(APP)
	$(APP) daisy-min

all: run test dump

gdb: $(APP)
	gdb --args $(APP) daisy-min

test: ./test.exe
	make clean
	./test.exe
	make
	make dump
	./daisydump.exe daisy-min.otf -t cmap

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
	./daisydump.exe daisy-min.otf

com:
	file daisy-min.otf
	ttx daisy-min.otf

$(APP): src/daisyff.c src/*.h include/*.h
	gcc $< \
		$(CFLAGS) $(INCLUDE) \
		-o $(APP)

clean:
	rm -rf $(APP) *.otf
	rm -rf *.d

-include $(DEPENDS)

