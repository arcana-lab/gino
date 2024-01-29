all: hooks src

src:
	cd src ; make ; 

tests: src
	cd tests ; make ;

hooks:
	make -C .githooks

format:
	cd src ; ./scripts/format_source_code.sh

clean:
	cd src ; make $@ ; 
	cd tests ; make $@ ;
	find ./ -name .clangd -exec rm -rv {} +
	find ./ -name .cache -exec rm -rv {} +

uninstall: clean
	cd src ; make $@ ;
	rm -f enable ;
	rm -rf install ;
	if test -d .githooks ; then cd .githooks ; make clean ; fi;

.PHONY: src tests hooks format clean uninstall
