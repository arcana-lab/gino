all: 
	./scripts/check_noelle.sh
	cd core ; make ;
	cd tools ; make ;
	./scripts/generate_enable_file.sh

clean:
	cd core ; make clean ;
	cd tools ; make clean ; 
	find ./ -name .ycm_extra_conf.py -delete ;
	find ./ -name compile_commands.json -delete ;
	find ./ -name .clangd -exec rm -rv {} +

uninstall: clean
	rm -f scripts/DependencesCMake.txt ;

.PHONY: clean uninstall
