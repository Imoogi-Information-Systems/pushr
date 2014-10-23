CC=gcc
CFLAGS=-g -Wall -Wextra -pedantic
OBJS=stringbuffer.o stringlist.o singleton.o pushr.o
PROG=pushr

CURL_FLAGS=`curl-config --cflags`
CURL_LIBS=`curl-config --libs`
MYSQL_FLAGS=`mysql_config --cflags`
MYSQL_LIBS=`mysql_config --libs`

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(PROG) $(CURL_LIBS) $(MYSQL_LIBS)

pushr.o: pushr.h pushr.c settings.h
	$(CC) $(CFLAGS) -c pushr.c $(CURL_FLAGS) $(MYSQL_FLAGS)

stringbuffer.o: stringbuffer.h stringbuffer.c
	$(CC) $(CFLAGS) -c stringbuffer.c 

stringlist.o: stringlist.h stringlist.c
	$(CC) $(CFLAGS) -c stringlist.c

singleton.o: singleton.h singleton.c
	$(CC) $(CFLAGS) -c singleton.c

clean:
	$(RM) $(PROG) $(OBJS)

