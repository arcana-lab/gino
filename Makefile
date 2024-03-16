BUILD_DIR ?= build
export GENERATOR ?= Unix Makefiles
export JOBS ?= 16
export NOELLE_INSTALL_DIR = $(shell noelle-config --prefix)
export MAKEFLAGS += --no-print-directory
export KCONFIG_CONFIG = .config
export MENUCONFIG_STYLE = aquatic

all: install

install: check_noelle compile
	cmake --install $(BUILD_DIR) 

compile: $(BUILD_DIR)
	cmake --build $(BUILD_DIR) -j$(JOBS) 

build:
	cmake -S . -B $(BUILD_DIR) -G "$(GENERATOR)" \
		-DCMAKE_INSTALL_MESSAGE=LAZY \
		-DCMAKE_INSTALL_PREFIX=install \
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

menuconfig:
	@python3 bin/menuconfig.py

clean: uninstall
	rm -rf $(BUILD_DIR)
	rm -rf .config
	$(MAKE) -C tests clean > /dev/null
	rm -f compile_commands.json
	rm -f config.cmake

uninstall:
	-cat $(BUILD_DIR)/install_manifest.txt | xargs rm -f
	rm -f enable
	rm -f .git/hooks/pre-commit

.PHONY: all build install compile check_noelle external tests format clean uninstall
