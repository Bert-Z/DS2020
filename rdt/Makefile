# NOTE: Feel free to change the makefile to suit your own need.

# compile and link flags
CCFLAGS = -Wall -g -std=c++14
LDFLAGS = -Wall -g -std=c++14

# make rules
TARGETS = rdt_sim 

all: $(TARGETS)

debug:	CCFLAGS += -D DEBUG
debug:	LDFLAGS += -D DEBUG
debug:	$(TARGETS)

.cc.o:
	g++ $(CCFLAGS) -c -o $@ $<

rdt_check.o:	rdt_struct.h rdt_parameters.h rdt_check.h 

rdt_sender.o: 	rdt_struct.h rdt_sender.h

rdt_receiver.o:	rdt_struct.h rdt_receiver.h rdt_parameters.h rdt_check.h

rdt_sim.o: 	rdt_struct.h

rdt_sim: rdt_sim.o rdt_check.o rdt_receiver.o rdt_sender.o 
	g++ $(LDFLAGS) -o $@ $^

clean:
	rm -f *~ *.o $(TARGETS)
