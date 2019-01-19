
APP=./daisyf.exe
CFLAGS := -lm

.PHONY: all run clean

all: run

run: $(APP)
	$(APP) daisy-min.otf

com:
	file daisy-min.otf
	ttx daisy-min.otf

$(APP): src/main.c
	gcc $< $(CFLAGS) -o $(APP)

clean:
	rm -rf $(APP) *.otf

