#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

extern char *tzname[];

int main(void)
{
    time_t now;
    struct tm *sp;
    char tz[] = "TZ=PST8PDT";

    (void)time(&now);

    putenv(tz);
    tzset();

    printf("%s", ctime(&now));
    
    sp = localtime(&now);
    printf("%d/%d/%02d %d:%02d:%d PST\n",
        sp->tm_mon + 1, sp->tm_mday,
        sp->tm_year+1900, sp->tm_hour - 1,
        sp->tm_min, sp->tm_sec, tzname[sp->tm_isdst]);
    exit(0);
}
