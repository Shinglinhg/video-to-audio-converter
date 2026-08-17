CC := x86_64-w64-mingw32-gcc
INPUT := main.c
OUTPUT := conversordeaudio.exe
WIN_LIBRARIES := \
	-lraylib \
	-lgdi32 \
	-lwinmm \
	-lopengl32 \
	-lshell32 \
	-luser32 \
	-lkernel32 \
	-lcomdlg32

all: $(OUTPUT) run

.PHONY: all run clean

$(OUTPUT):
	$(CC) \
		main.c \
		-o $@ \
		-I3rdparty/raylib/include \
		-L. \
		$(WIN_LIBRARIES)

run: $(OUTPUT)
	wine ./$^

clean: $(OUTPUT)
	rm $^
