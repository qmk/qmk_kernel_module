CFLAGS_ALL=-I../lib/libusbgx/build/include -I../lib/libqmk/include -I../include -L../lib/libusbgx/build/lib -lz -lpthread -lm -ldl

qmk_helper: CFLAGS+=-static $(CFLAGS_ALL) -lusbgx -lconfig
qmk_helper: qmk_helper.c qmk_gadget.c qmk_socket_listener.c	
	@echo "  CC [M]  $@"
	@$(CC)  $^ $(CFLAGS) -o $@

clean:
	@rm qmk_helper
