
APP=./daisyff.exe
CFLAGS		:= -std=c11 -lm -g
INCLUDE		:= -I./ -I./include
OBJECT_DIR		:= ./object

.PHONY: all run test clean
.PHONY: dump com

run: $(APP)
	$(APP) daisy-min

all: run dump

gdb: $(APP)
	gdb --args $(APP) daisy-min

test: test/test.c
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

