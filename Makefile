BUILD_DIR ?= build
export GENERATOR ?= Unix Makefiles
export JOBS ?= 8
export GINO_INSTALL_DIR ?= $(shell realpath ./install)
export NOELLE_INSTALL_DIR = $(shell noelle-config --prefix)
export MAKEFLAGS += --no-print-directory

all: install

install: check_noelle compile
	cmake --install $(BUILD_DIR) 

compile: $(BUILD_DIR)
	cmake --build $(BUILD_DIR) -j$(JOBS) 

$(BUILD_DIR):
	cmake -S . -B $(BUILD_DIR) -G "$(GENERATOR)" \
		-DCMAKE_INSTALL_MESSAGE=LAZY \
		-DCMAKE_INSTALL_PREFIX=$(GINO_INSTALL_DIR) \
		-DNOELLE_INSTALL_DIR=$(NOELLE_INSTALL_DIR)

check_noelle:
	@if ! noelle-config > /dev/null 2>&1; then \
		echo -e "\e[1;1mNOELLE not found\e[0m. \e[32mDid you forget to source it?\e[0m" ;\
		exit 1; \
	fi

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

.PHONY: all install compile check_noelle external tests format clean uninstall
