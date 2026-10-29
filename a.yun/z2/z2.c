#include <stdio.h>
#include <time.h>

int main(void)
{
    time_t now = time(NULL);
    struct tm *california;

    if (now == (time_t)-1) {
        perror("time");
        return 1;
    }
    now -= 8 * 60 * 60;
    california = gmtime(&now);
    if (california == NULL) {
        fprintf(stderr, "gmtime failed\n");
        return 1;
    }
    printf("%04d-%02d-%02d %02d:%02d:%02d PST (UTC-8)\n",
           california->tm_year + 1900, california->tm_mon + 1,
           california->tm_mday, california->tm_hour,
           california->tm_min, california->tm_sec);
    return 0;
}
