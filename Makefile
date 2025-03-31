
BUILD_DIR ?= build
INSTALL_DIR ?= install

all: hooks src

src:
	mkdir -p $(BUILD_DIR)
	mkdir -p $(INSTALL_DIR)
	cmake \
	  -DCMAKE_C_COMPILER=$(shell which clang) \
	  -DCMAKE_CXX_COMPILER=$(shell which clang++) \
	  -DCMAKE_INSTALL_PREFIX=$(INSTALL_DIR) \
	  -S . -B $(BUILD_DIR)
	make -j16 -C $(BUILD_DIR) install

tests: src
	cd tests ; make ;

hooks:
	make -C .githooks

# format:
# 	cd src ; ./scripts/format_source_code.sh

clean:
	rm -rf $(BUILD_DIR)
	cd tests ; make clean ;
	find ./ -name .clangd -exec rm -rv {} +
	find ./ -name .cache -exec rm -rv {} +

uninstall: clean
	rm -rf $(BUILD_DIR)
	rm -f enable ;
	rm -rf install ;
	if test -d .githooks ; then cd .githooks ; make clean ; fi;

# .PHONY: src tests hooks format clean uninstall
.PHONY: src tests hooks clean uninstall
