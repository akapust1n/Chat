#include "logger.h"
#include "string.h"
#include <time.h>
#include <unistd.h>

void writeLog(FILE* logfile, int author, const char* message)
{
    time_t rawtime;
    struct tm* timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    int len = strlen(message) + 30;
    char buf[len];
    bzero(buf, len);

    snprintf(buf, len, "\n%s %d : %s ", asctime(timeinfo), author, message);
    write(logfile, buf, len);
}
