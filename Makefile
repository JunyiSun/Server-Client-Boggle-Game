PORT=53516
CFLAGS = -DPORT=$(PORT) -g -Werror -std=c99

all: game_server game_client

game_client: game_client.o game.o read_line.o board_generator.o dictionary.o construct_word_dict.o word_checker.o
	gcc ${CFLAGS} -o game_client game_client.o game.o read_line.o board_generator.o dictionary.o construct_word_dict.o word_checker.o

game_server: game_server.o game.o read_line.o board_generator.o dictionary.o construct_word_dict.o word_checker.o
	gcc ${CFLAGS} -o game_server game_server.o game.o read_line.o board_generator.o dictionary.o construct_word_dict.o word_checker.o

game.o: game.c game.h
	gcc ${CFLAGS} -c game.c

game_client.o: game_client.c
	gcc ${CFLAGS} -c game_client.c

game_server.o: game_server.c
	gcc ${CFLAGS} -c game_server.c

read_line.o: read_line.c read_line.h
	gcc ${CFLAGS} -c read_line.c

board_generator.o: board_generator.c board_generator.h
	gcc ${CFLAGS} -c board_generator.c

dictionary.o: dictionary.c dictionary.h
	gcc ${CFLAGS} -c dictionary.c

construct_word_dict.o: construct_word_dict.c construct_word_dict.h
	gcc ${CFLAGS} -c construct_word_dict.c

word_checker.o: word_checker.c word_checker.h
	gcc ${CFLAGS} -c word_checker.c

clean:
	rm *.o game_server game_client