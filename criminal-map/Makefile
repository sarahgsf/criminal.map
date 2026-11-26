CC     ?= gcc
CFLAGS  = -std=c99 -Wall -Wextra -pedantic -O2
NUCLEO  = src/hipergrafo.c src/vertices.c src/hiperarestas.c src/consultas.c \
          src/analise.c src/exportacao.c src/persistencia.c src/demo.c src/util.c

ifeq ($(OS),Windows_NT)
  LIBS = -lws2_32
endif

criminalmap: $(NUCLEO) src/servidor.c src/main.c src/*.h
	$(CC) $(CFLAGS) $(NUCLEO) src/servidor.c src/main.c -o $@ $(LIBS)

run: criminalmap
	./criminalmap

teste: $(NUCLEO) tests/teste_hipergrafo.c src/*.h
	$(CC) $(CFLAGS) -Isrc $(NUCLEO) tests/teste_hipergrafo.c -o teste_hipergrafo
	./teste_hipergrafo

clean:
	rm -f criminalmap criminalmap.exe teste_hipergrafo
