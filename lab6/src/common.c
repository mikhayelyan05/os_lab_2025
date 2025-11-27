#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

uint64_t MultModulo(uint64_t a, uint64_t b, uint64_t mod) {
    uint64_t result = 0;
    a = a % mod;
    while (b > 0) {
        if (b % 2 == 1)
            result = (result + a) % mod;
        a = (a * 2) % mod;
        b /= 2;
    }
    return result % mod;
}

bool ConvertStringToUI64(const char *str, uint64_t *val) {
    char *end = NULL;
    unsigned long long i = strtoull(str, &end, 10);
    if (errno == ERANGE) {
        fprintf(stderr, "Out of uint64_t range: %s\n", str);
        return false;
    }

    if (errno != 0)
        return false;

    *val = i;
    return true;
}

int read_servers(const char* filename, struct Server* servers, int max_servers) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Ошибка открытия файла серверов");
        return -1;
    }
    
    char line[256];
    int count = 0;
    
    while (fgets(line, sizeof(line), file) && count < max_servers) {
        line[strcspn(line, "\n")] = 0;
        
        char* colon = strchr(line, ':');
        if (colon) {
            *colon = '\0';
            strncpy(servers[count].ip, line, 254);
            servers[count].ip[254] = '\0';
            servers[count].port = atoi(colon + 1);
            count++;
        }
    }
    
    fclose(file);
    return count;
}