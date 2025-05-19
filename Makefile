CC      = gcc
CFLAGS  = -Wall -g
SRCS    = client.c commands.c helpers.c http_requests.c state.c parson.c
OBJS    = $(SRCS:.c=.o)
TARGET  = client

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
