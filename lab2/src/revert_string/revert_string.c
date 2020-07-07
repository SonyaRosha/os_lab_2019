#include "revert_string.h"
#include <string.h>
#include <stdlib.h>

void RevertString(char *str)
{
    int medium = strlen(str)/2;
    for (int i = 0; i < medium; ++i) {
		char temp = str[i];
		str[i] = str[strlen(str) - i - 1];
		str[strlen(str) - i - 1] = temp;
    }
}