BUILD_DIR ?= build

.PHONY: all configure build clean

all: build

configure:
	@test -n "$$VITASDK" || (echo "VITASDK is not set"; exit 1)
	cmake -S . -B "$(BUILD_DIR)" \
		-DCMAKE_TOOLCHAIN_FILE="$$VITASDK/share/vita.toolchain.cmake"

build: configure
	cmake --build "$(BUILD_DIR)" --parallel

clean:
	rm -rf "$(BUILD_DIR)"
