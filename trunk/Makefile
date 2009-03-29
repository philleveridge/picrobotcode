.c.o:
	sdcc --debug -V -mpic14 -p16f627 -c $<
	$(PRJ).hex: $(OBJS)
	gplink -m -s $(PRJ).lkr -o $(PRJ).hex $(OBJS) libsdcc.lib

