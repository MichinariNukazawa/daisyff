
APP=./daisyf.exe
CFLAGS		:= -std=c11 -lm
INCLUDE		:= -I./

.PHONY: all run test clean

all: run

run: $(APP)
	$(APP) daisy-min.otf

test: test/test.c
	gcc $< $(CFLAGS) $(INCLUDE) -o ./test.exe
	./test.exe

com:
	file daisy-min.otf
	ttx daisy-min.otf

$(APP): src/main.c
	gcc $< $(CFLAGS) $(INCLUDE) -o $(APP)

clean:
	rm -rf $(APP) *.otf

