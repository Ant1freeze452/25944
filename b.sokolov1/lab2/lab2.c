#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(void)
{
    time_t now;
    struct tm *sp;

    time(&now);

    setenv("TZ", "PST8", 1);
    tzset();
    sp = localtime(&now);
    printf("WINTER (PST): %02d/%02d/%d %02d:%02d:%02d\n",
        sp->tm_mon + 1, sp->tm_mday, sp->tm_year + 1900,
        sp->tm_hour, sp->tm_min, sp->tm_sec);

    printf("TIME IN CALIFORNIA: %d/%d/%02d %d:%02d %s\n",
        sp->tm_mon + 1, sp->tm_mday,
        sp->tm_year + 1900, sp->tm_hour,
        sp->tm_min, tzname[sp->tm_isdst]);

    setenv("TZ", "PDT7", 1);
    tzset();
    sp = localtime(&now);
    printf("SUMMER (PDT): %02d/%02d/%d %02d:%02d:%02d\n",
        sp->tm_mon + 1, sp->tm_mday, sp->tm_year + 1900,
        sp->tm_hour, sp->tm_min, sp->tm_sec);

    return 0;
}
