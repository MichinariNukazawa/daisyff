
APP=./daisyf.exe
CFLAGS := -lm
INCLUDE		:= -I./

.PHONY: all run clean

all: run

run: $(APP)
	$(APP) daisy-min.otf

com:
	file daisy-min.otf
	ttx daisy-min.otf

$(APP): src/main.c
	gcc $< $(CFLAGS) $(INCLUDE) -o $(APP)

clean:
	rm -rf $(APP) *.otf

