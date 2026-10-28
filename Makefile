# Короткие команды для Linux/WSL. Вся логика сборки в CMakeLists.txt и CMakePresets.json.
#   make            собрать (debug)
#   make test       собрать и прогнать тесты
#   make run        собрать и запустить движок с редактором
#   make release    собрать release; PRESET=release работает и с остальными целями
#   make format     отформатировать код по .clang-format
#   make hooks      включить git-хук: форматирование и тесты перед коммитом
#   make clean      удалить результаты сборки текущего пресета
#   make distclean  удалить папку build целиком, вместе со скачанными зависимостями

PRESET ?= debug
BUILD_DIR := build/$(PRESET)
CLANG_FORMAT ?= clang-format
SOURCES = $(shell find engine editor backends apps tests -name '*.cpp' -o -name '*.hpp')

.PHONY: all build test run release format format-check hooks clean distclean

all: build

# Конфигурируем только при первом запуске, дальше Ninja сам перезапускает CMake.
$(BUILD_DIR)/CMakeCache.txt:
	cmake --preset $(PRESET)

build: $(BUILD_DIR)/CMakeCache.txt
	cmake --build --preset $(PRESET)

test: build
	ctest --preset $(PRESET)

run: build
	./$(BUILD_DIR)/bin/yqlora

release:
	$(MAKE) build PRESET=release

format:
	$(CLANG_FORMAT) -i $(SOURCES)

# Для проверки в CI: ошибка, если что-то не отформатировано.
format-check:
	$(CLANG_FORMAT) --dry-run -Werror $(SOURCES)

hooks:
	git config core.hooksPath .githooks

clean:
	cmake --build --preset $(PRESET) --target clean

distclean:
	rm -rf build
