FLAG = -Wall -g3 -fsanitize=address -pthread
SSL = -L/usr/lib -lssl -lcrypto


compile:
	gcc $(FLAG) $(LBI) $(SSL) -ldl -lm -lpthread libs/sqlite/sqlite3.c src/servertest.c -o uchat_server
	gcc src/clienttest.c src/signal.c inc/header.h -o uchat  `pkg-config --cflags --libs gtk+-3.0` -Wl,-export-dynamic $(LBI) $(SSL)

client: src/clienttest.c src/signal.c inc/header.h
	gcc -o uchat src/clienttest.c src/signal.c inc/header.h $(LBI) $(SSL) `pkg-config --cflags --libs gtk+-3.0` -Wl,-export-dynamic

# Компилируем только серверную часть
server: src/servertest.c libs/sqlite/sqlite3.c inc/header.h
	gcc $(FLAG) -o uchat_server src/servertest.c libs/sqlite/sqlite3.c inc/header.h -ldl -lm -lpthread $(SSL)