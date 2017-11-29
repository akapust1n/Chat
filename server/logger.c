#include "logger.h"
#include "string.h"
#include <unistd.h>

void writeLog(int logfile, int author, const char* message)
{
    int len = strlen(message) + 50;
    char buf[len];

    snprintf(buf, len, "%d : %s \n", author, message);
    write(logfile, buf, len);
}
