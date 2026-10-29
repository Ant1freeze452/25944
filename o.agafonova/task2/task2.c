#include <stdio.h>
#include <stdlib.h>
#include <time.h>

extern char *tzname[];

int main()
{
    time_t now;
    struct tm *sp;

    time(&now);

    // PST: всегда UTC-8
    setenv("TZ", "PST8", 1);
    tzset();
    sp = localtime(&now);

    printf("%02d/%02d/%d %02d:%02d %s\n",
           sp->tm_mon + 1,
           sp->tm_mday,
           sp->tm_year + 1900,
           sp->tm_hour,
           sp->tm_min,
           tzname[0]);

    // PDT: всегда UTC-7
    setenv("TZ", "PDT7", 1);
    tzset();
    sp = localtime(&now);

    printf("%02d/%02d/%d %02d:%02d %s\n",
           sp->tm_mon + 1,
           sp->tm_mday,
           sp->tm_year + 1900,
           sp->tm_hour,
           sp->tm_min,
           tzname[0]);

    return 0;
}