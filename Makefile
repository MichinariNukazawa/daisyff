
APP=./daisyff.exe
CFLAGS		:= -std=c11 -lm -g
INCLUDE		:= -I./ -I./include
SOURCE_DIR	:= ./src
OBJECT_DIR		:= ./object

SOURCES		:= $(wildcard $(SOURCE_DIR)/*.c)
OBJECTS		:= $(subst $(SOURCE_DIR),$(OBJECT_DIR),$(SOURCES:.c=.o))
DEPENDS		:= $(OBJECTS:.o=.d)

.PHONY: all run test clean
.PHONY: dump com

run: $(APP)
	$(APP) daisy-min

all: run dump

gdb: $(APP)
	gdb --args $(APP) daisy-min

test: test/test.c src/*.c  src/*.h include/*.h
	gcc $< $(CFLAGS) $(INCLUDE) -o ./test.exe
	./test.exe

dump: src/daisydump.c src/*.h include/*.h
	mkdir -p $(OBJECT_DIR)
	bash ./version.sh $(OBJECT_DIR)
	gcc \
		$< \
		$(OBJECT_DIR)/version.c \
		$(CFLAGS) $(INCLUDE) -o ./daisydump.exe
	./daisydump.exe daisy-min.otf

com:
	file daisy-min.otf
	ttx daisy-min.otf

$(APP): src/daisyff.c src/*.h
	gcc $< $(CFLAGS) $(INCLUDE) -o $(APP)

clean:
	rm -rf $(APP) *.otf
	rm -rf *.d

-include $(DEPENDS)

