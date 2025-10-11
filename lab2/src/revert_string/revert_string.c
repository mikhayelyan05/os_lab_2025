#include "revert_string.h"

void RevertString(char *str)
{
    int length = strlen(str) - 1; // Длина строки минус завершающий ноль
    int i = 0;
    char temp;

    while (i < length - i) {
        temp = str[i];
        str[i] = str[length - i];
        str[length - i] = temp;
        i++;
    }
}