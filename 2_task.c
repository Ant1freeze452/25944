#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
extern char *tzname[];
main()
{
    time_t now;
    struct tm *sp;
    char tz[] = "TZ=PST8PDT";

    (void) time(&now);

    printf("PC time: %s", ctime(&now));

    putenv(tz);
    tzset();

    sp = localtime(&now);
    printf("California time: %d/%d/%02d %d:%02d:%02d %s\n",
        sp->tm_mon + 1, sp->tm_mday,
        sp->tm_year + 1900, sp->tm_hour,
        sp->tm_min, sp->tm_sec, 
        sp->tm_isdst ? "PDT" : "PST");
    exit(0);
}