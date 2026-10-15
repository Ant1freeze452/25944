#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

int main(void)
{
    time_t now;
    struct tm *sp;

    // получаем текущее время
    (void) time(&now);

    now -= 7 * 60 * 60;

    // преобразуем полученное время как UTC
    sp = gmtime(&now);

    printf("%02d/%02d/%04d %02d:%02d:%02d PDT\n",
           sp->tm_mon + 1,
           sp->tm_mday,
           sp->tm_year + 1900,
           sp->tm_hour,
           sp->tm_min,
           sp->tm_sec);

    time_t pst = now + 7 * 60 * 60;

    pst -= 8 * 60 * 60;

    sp = gmtime(&pst);

    printf("\n%02d/%02d/%04d %02d:%02d:%02d PST\n",
           sp->tm_mon + 1,
           sp->tm_mday,
           sp->tm_year + 1900,
           sp->tm_hour,
           sp->tm_min,
           sp->tm_sec);

    exit(0);
}