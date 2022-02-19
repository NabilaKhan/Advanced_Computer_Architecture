CC = g++ -std=c++11
OPT = -O3
#OPT = -g
#WARN = -Wall
CFLAGS = $(OPT) $(WARN) $(INC) $(LIB)

# List all your .cc files here (source files, excluding header files)
SIM_SRC = BranchPredictor.cc

# List corresponding compiled object files here (.o files)
SIM_OBJ = BranchPredictor.o
 
#################################

# default rule

all: sim_cache
	@echo "my work is done here..."


# rule for making sim_cache

sim_cache: $(SIM_OBJ)
	$(CC) -o sim $(CFLAGS) $(SIM_OBJ) -lm
	@echo "-----------DONE WITH SIM-----------"


# generic rule for converting any .cc file to any .o file
 
.cc.o:
	$(CC) $(CFLAGS)  -c $*.cc


# type "make clean" to remove all .o files plus the sim_cache binary

clean:
	rm -f *.o sim


# type "make clobber" to remove all .o files (leaves sim_cache binary)

clobber:
	rm -f *.o


