#include "upe/upe_log.h"
#include <stdio.h>

void UpeLog_Info(const char *message) {
    printf("[INFO] %s\n", message);
}

void UpeLog_Error(const char *message) {
    fprintf(stderr, "[ERROR] %s\n", message);
}
