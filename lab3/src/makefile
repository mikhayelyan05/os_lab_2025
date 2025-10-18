CC=gcc
CFLAGS=-I.

# Основная цель - сборка всех программ
all: sequential_min_max parallel_min_max run_sequential

# sequential_min_max - последовательная версия
sequential_min_max : utils.o find_min_max.o utils.h find_min_max.h
	$(CC) -o sequential_min_max sequential_min_max.c find_min_max.o utils.o $(CFLAGS)

# parallel_min_max - параллельная версия  
parallel_min_max : utils.o find_min_max.o utils.h find_min_max.h
	$(CC) -o parallel_min_max parallel_min_max.c utils.o find_min_max.o $(CFLAGS)

# run_sequential - программа для запуска sequential_min_max в отдельном процессе
run_sequential : 
	$(CC) -o run_sequential run_sequential.c $(CFLAGS)

# Цели для компиляции объектных файлов:

# utils.o - объектный файл утилит
utils.o : utils.h
	$(CC) -o utils.o -c utils.c $(CFLAGS)

# find_min_max.o - объектный файл функций поиска min/max
find_min_max.o : utils.h find_min_max.h
	$(CC) -o find_min_max.o -c find_min_max.c $(CFLAGS)

# Очистка - удаление всех сгенерированных файлов
clean :
	rm -f utils.o find_min_max.o sequential_min_max parallel_min_max run_sequential
