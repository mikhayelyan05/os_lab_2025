#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>

// Общие структуры данных
struct Server {
    char ip[255];
    int port;
};

struct FactorialArgs {
    uint64_t begin;
    uint64_t end;
    uint64_t mod;
};

// Общие функции
uint64_t MultModulo(uint64_t a, uint64_t b, uint64_t mod);
bool ConvertStringToUI64(const char *str, uint64_t *val);
int read_servers(const char* filename, struct Server* servers, int max_servers);

#endif