OBJS = physical_layer.o datalink_layer.o application_layer.o program.o
PROGRAM = ./execute

executable: $(PROGRAM)
$(PROGRAM) : $(OBJS)
	$(CC) -o $(PROGRAM) -Wall $(OBJS)

transmitter: $(PROGRAM)
	$(PROGRAM) /dev/ttyS0 w pinguim.gif 100 3 4

receiver: $(PROGRAM)
	$(PROGRAM) /dev/ttyS0 r 3 4

clear: 
	rm -f $(PROGRAM) $(OBJS)