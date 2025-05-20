CC      = gcc
CFLAGS  = -Wall -g
INCLUDES = -I. -Icommands
SRCS    = client.c \
          commands/commands.c \
          commands/admin_commands.c \
          commands/user_commands.c \
          commands/movie_commands.c \
          commands/collection_commands.c \
          helpers.c \
          http_requests.c \
          state.c \
          parson.c
OBJS    = $(SRCS:.c=.o)
TARGET  = client

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
