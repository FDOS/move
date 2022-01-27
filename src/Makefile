
all: move.exe

move.exe move.com: move.c movedir.c misc.c $(EXTRA_SRCS)
	$(CC) $(CFLAGS)$@ $^ $(LDFLAGS)
	$(UPXARGS) $@

clean:
	$(RM) move.exe
	$(RM) move.com
	$(RM) *.o
	$(RM) *.obj
	$(RM) *.map
