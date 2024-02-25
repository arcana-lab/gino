BUILD_DIR ?= build
export GENERATOR ?= Unix Makefiles
export JOBS ?= 8
export GINO_INSTALL_DIR ?= $(shell realpath ./install)
export NOELLE_INSTALL_DIR = $(shell noelle-config --prefix)
export MAKEFLAGS += --no-print-directory

all: install

install: compile
	cmake --install $(BUILD_DIR) 

compile: $(BUILD_DIR)
	cmake --build $(BUILD_DIR) -j$(JOBS) 

$(BUILD_DIR):
	cmake -S . -B $(BUILD_DIR) -G "$(GENERATOR)" \
		-DCMAKE_INSTALL_MESSAGE=LAZY \
		-DCMAKE_INSTALL_PREFIX=$(GINO_INSTALL_DIR) \
		-DNOELLE_INSTALL_DIR=$(NOELLE_INSTALL_DIR)

tests: install
	$(MAKE) -C tests

format:
	find ./src -regex '.*\.[c|h]pp' | xargs clang-format -i

clean:
	rm -rf $(BUILD_DIR)
	$(MAKE) -C tests clean
	rm -f compile_commands.json

uninstall:
	-cat $(BUILD_DIR)/install_manifest.txt | xargs rm -f
	rm -f enable
	rm -f .git/hooks/pre-commit

.PHONY: all install compile external tests format clean uninstall
