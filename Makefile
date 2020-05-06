NAME = main
EXEC = $(NAME)
sources = $(NAME).c

CXX = gcc
RM = rm -f

CFLAGS = -g -std=gnu99 -Wall -Wextra -pedantic
LDFLAGS = -lrt -lpthread

OBJFILES = $(sources:.c=.o)

.PHONY : all

all: $(EXEC)

%.o : %.c
	$(CXX) $(CFLAGS) -c $< -o $@

$(EXEC) : $(OBJFILES)
	$(CXX) $(CLFAGS) -o $@ $(OBJFILES) $(LDFLAGS)

clean:
	$(RM) *.o core *.out

cleanshm:
	$(RM) /dev/shm/xdohna45*

cleanall: clean cleanshm
	$(RM) $(EXEC)

