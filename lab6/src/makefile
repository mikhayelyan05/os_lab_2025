# Компилятор
CC = gcc

# Флаги компиляции
CFLAGS = -Wall -Wextra -Werror -std=c99 -pedantic -g
LDFLAGS = -pthread

# Цели
TARGETS = client server
SOURCES_CLIENT = client.c common.c
SOURCES_SERVER = server.c common.c
OBJECTS_CLIENT = client.o common.o
OBJECTS_SERVER = server.o common.o

# Основная цель
all: $(TARGETS)

# Компиляция клиента
client: $(OBJECTS_CLIENT)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Компиляция сервера
server: $(OBJECTS_SERVER)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Компиляция объектных файлов
%.o: %.c common.h
	$(CC) $(CFLAGS) -c $< -o $@

# Очистка
clean:
	rm -f $(TARGETS) *.o

# Перекомпиляция
rebuild: clean all

# Запуск тестового примера
test: all
	@echo "Запуск тестового примера..."
	@echo "Создание файла servers.txt..."
	@echo "127.0.0.1:20001" > servers.txt
	@echo "127.0.0.1:20002" >> servers.txt
	@echo "127.0.0.1:20003" >> servers.txt
	@echo "Запуск серверов в фоне..."
	@./server --port 20001 --tnum 2 &
	@./server --port 20002 --tnum 2 &
	@./server --port 20003 --tnum 2 &
	@sleep 2
	@echo "Запуск клиента..."
	@./client --k 10 --mod 1000 --servers servers.txt
	@echo "Остановка серверов..."
	@pkill server || true

# Отладочная сборка с дополнительными флагами
debug: CFLAGS += -DDEBUG -O0
debug: rebuild

# Профилировочная сборка
profile: CFLAGS += -pg
profile: LDFLAGS += -pg
profile: rebuild

# Установка (если нужно куда-то копировать)
PREFIX = /usr/local
install: all
	install -d $(PREFIX)/bin/
	install -m 755 client server $(PREFIX)/bin/

# Создание статической библиотеки
libcommon.a: common.o
	ar rcs $@ $^

# Статическая линковка
static: libcommon.a
	$(CC) $(CFLAGS) -o client client.c -L. -lcommon $(LDFLAGS)
	$(CC) $(CFLAGS) -o server server.c -L. -lcommon $(LDFLAGS)

# Помощь
help:
	@echo "Доступные цели:"
	@echo "  all      - компиляция клиента и сервера (по умолчанию)"
	@echo "  client   - компиляция только клиента"
	@echo "  server   - компиляция только сервера"
	@echo "  clean    - удаление скомпилированных файлов"
	@echo "  rebuild  - перекомпиляция"
	@echo "  test     - запуск тестового примера"
	@echo "  debug    - отладочная сборка"
	@echo "  profile  - сборка для профилирования"
	@echo "  install  - установка в систему"
	@echo "  static   - сборка со статической библиотекой"
	@echo "  help     - эта справка"

# Служебные цели
.PHONY: all clean rebuild test debug profile install static help