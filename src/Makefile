
all: move.exe

# Watcom has different flag for output file name at compile vs link
_CFLAGS = $(subst -fe=,-fo=,$(CFLAGS))

kitten.obj: ../kitten/kitten.c
	$(CC) $(_CFLAGS)$@ -c $^

tnyprntf.obj: ../tnyprntf/tnyprntf.c
	$(CC) $(_CFLAGS)$@ -c $^

move.exe move.com: move.c movedir.c misc.c $(EXTRA_OBJS)
	$(CC) $(CFLAGS)$@ $^ $(LDFLAGS)
	$(UPXARGS) $@

clean:
	$(RM) move.exe
	$(RM) move.com
	$(RM) *.o
	$(RM) *.obj
	$(RM) *.map
