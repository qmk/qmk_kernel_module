CFLAGS_ALL=-I../lib/libusbgx/build/include -L../lib/libusbgx/build/lib -lz -lpthread -lm -ldl

qmk_helper: CFLAGS+=-static $(CFLAGS_ALL) -lusbgx -lconfig
qmk_helper: qmk_helper.c	
	@echo "  CC [M]  $@"
	@$(CC)  $^ $(CFLAGS) -o $@

clean:
	-rm qmk_helper
