INC=./includes
SRC=./src
OUT=./bin

$(OUT)/lab: $(OUT)/main.o $(OUT)/select.o $(OUT)/signals.o
	gcc -o $(OUT)/lab $(OUT)/main.o $(OUT)/select.o $(OUT)/signals.o

$(OUT)/main.o: $(SRC)/main.c $(INC)/labHeader.h
	gcc -I$(INC) -o $(OUT)/main.o -c $(SRC)/main.c

$(OUT)/select.o: $(SRC)/select.c $(INC)/labHeader.h
	gcc -I$(INC) -o $(OUT)/select.o -c $(SRC)/select.c

$(OUT)/signals.o: $(SRC)/signals.c $(INC)/labHeader.h
	gcc -I$(INC) -o $(OUT)/signals.o -c $(SRC)/signals.c

clean:
	rm $(OUT)/*.o $(OUT)/lab
