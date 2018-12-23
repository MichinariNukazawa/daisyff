
APP=./daisyf.exe

.PHONY: all run clean

all: run

run: $(APP)
	$(APP) daisy-min.otf

$(APP): src/main.c
	gcc $< -o $(APP)

clean:
	rm -rf $(APP) *.otf

