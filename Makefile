# Короткие команды для Linux/WSL. Вся логика сборки в CMakeLists.txt и CMakePresets.json.
#   make            собрать (debug)
#   make test       собрать и прогнать тесты
#   make run        собрать и запустить sandbox
#   make release    собрать release; PRESET=release работает и с остальными целями
#   make clean      удалить результаты сборки текущего пресета
#   make distclean  удалить папку build целиком, вместе со скачанными зависимостями

PRESET ?= debug
BUILD_DIR := build/$(PRESET)

.PHONY: all build test run release clean distclean

all: build

# Конфигурируем только при первом запуске, дальше Ninja сам перезапускает CMake.
$(BUILD_DIR)/CMakeCache.txt:
	cmake --preset $(PRESET)

build: $(BUILD_DIR)/CMakeCache.txt
	cmake --build --preset $(PRESET)

test: build
	ctest --preset $(PRESET)

run: build
	./$(BUILD_DIR)/bin/sandbox

release:
	$(MAKE) build PRESET=release

clean:
	cmake --build --preset $(PRESET) --target clean

distclean:
	rm -rf build
