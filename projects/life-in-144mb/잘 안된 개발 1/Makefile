CC ?= clang
PYTHON ?= /opt/homebrew/Caskroom/miniforge/base/envs/min/bin/python3
CFLAGS = -Wall -Wextra -std=c99 -O2 -I/opt/homebrew/include -Isrc
LDFLAGS = -L/opt/homebrew/lib -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -framework CoreAudio

RELEASE_CFLAGS = -Wall -Wextra -std=c99 -Os -flto -s -DNDEBUG -I/opt/homebrew/include -Isrc
RELEASE_LDFLAGS = -L/opt/homebrew/lib -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -framework CoreAudio

SRC = $(wildcard src/*.c)
TARGET = bin/game

all: $(TARGET)

$(TARGET): $(SRC)
	@mkdir -p bin
	$(CC) $(CFLAGS) $(SRC) $(LDFLAGS) -o $(TARGET)
	@$(PYTHON) scripts/check_size.py $(TARGET)

release: $(SRC)
	@mkdir -p bin
	$(CC) $(RELEASE_CFLAGS) $(SRC) $(RELEASE_LDFLAGS) -o $(TARGET)
	@strip $(TARGET) 2>/dev/null || true
	@$(PYTHON) scripts/check_size.py $(TARGET)

clean:
	rm -rf bin src/*.o

test: tests/test_audio.c src/audio_synth.c
	@mkdir -p bin
	$(CC) $(CFLAGS) tests/test_audio.c src/audio_synth.c $(LDFLAGS) -o bin/test_audio
	./bin/test_audio

run: $(TARGET)
	./$(TARGET)

.PHONY: all release clean run test
