#define _XOPEN_SOURCE 500 // включаем объявления функций POSIX

#include <stdio.h> // printf, perror
#include <stdlib.h> // putenv
#include <time.h> // time, localtime, tzname

int main(void)
{
    time_t now; // секунды с 1 января 1970 года
    struct tm *sp; // поля даты и времени
    char *zones[] =
    {
        "TZ=America/Los_Angeles", // текущее время Калифорнии с учётом сезона
        "TZ=PST8", // фиксированное UTC-8
        "TZ=PDT7" // фиксированное UTC-7
    };
    const char *labels[] =
    {
        "California",
        "PST",
        "PDT"
    };
    int i; // индекс часового пояса и подписи

    if (time(&now) == (time_t)-1) // получаем один текущий момент для всех трёх строк
    {
        perror("time");
        return 1;
    }

    for (i = 0; i < 3; i++) // один момент времени в трёх вариантах
    {
        if (putenv(zones[i]) != 0) // устанавливаем нужный часовой пояс через TZ
        {
            perror("putenv");
            return 1;
        }

        sp = localtime(&now); // пересчитываем дату и время по TZ
        if (sp == NULL)
        {
            perror("localtime");
            return 1;
        }

        printf("%s: %02d/%02d/%04d %02d:%02d:%02d %s\n",
               labels[i], sp->tm_mon + 1, sp->tm_mday, sp->tm_year + 1900, // месяцы с 0, годы с 1900
               sp->tm_hour, sp->tm_min, sp->tm_sec, tzname[sp->tm_isdst > 0]);
    }

    return 0;
}
