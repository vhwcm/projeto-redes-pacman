all:
	$(MAKE) -C "Trabalho_1(Client_side)"
	$(MAKE) -C servidor

clean:
	$(MAKE) -C "Trabalho_1(Client_side)" clean
	$(MAKE) -C servidor clean

tar: clean all
	tar --exclude='*.o' \
	    --exclude='Trabalho_1(Client_side)/Readme.txt' \
	    --exclude='Trabalho_1(Client_side)/*.png' \
	    --exclude='Trabalho_1(Client_side)/*.mp4' \
	    --exclude='Trabalho_1(Client_side)/*.jpg' \
	    --exclude='Trabalho_1(Client_side)/*.txt' \
	    -czvf 20245275_20211771.tgz \
	    servidor "Trabalho_1(Client_side)" rede.c rede.h README.md relatorio.pdf Makefile
