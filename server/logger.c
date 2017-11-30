#include "logger.h"
#include "string.h"
#include "structs.h"
#include <time.h>
#include <unistd.h>
static int logger;
void openLog()
{
    logger = open(LOGFILE, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
}

void writeLog(int author, const char* message)
{
    time_t rawtime;
    struct tm* timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    int len = strlen(message) + 30;
    char buf[len];
    bzero(buf, len);

    snprintf(buf, len, "\n%s %d : %s ", asctime(timeinfo), author, message);
    write(LOGFILE, buf, len);
}
void closeLog()
{
    close(logger);
}
