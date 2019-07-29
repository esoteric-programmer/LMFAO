lmfao: bin/lmfao
	cp bin/lmfao .

all: lmfao doc

doc: Doxyfile src/*.c src/*.h
	doxygen Doxyfile

bin/lmfao: src/*.c src/*.h src/lmfao.y src/lmfao.l
	make -C src/

install: lmfao
	install -Dm755 ./lmfao $(DESTDIR)/usr/bin/lmfao

uninstall:
	rm -f $(DESTDIR)/usr/bin/lmfao

clean:
	make -C src/ clean
	rm -f ./lmfao
	rm -rf doc/

