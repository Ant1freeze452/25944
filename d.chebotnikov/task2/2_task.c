#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(void)
{
    time_t now;
    struct tm *california;
    char result[80];
    char timezone[] = "TZ=PST8";

    if (putenv(timezone) != 0) {
        perror("putenv");
        return EXIT_FAILURE;
    }
    tzset();

    if (time(&now) == (time_t)-1) {
        perror("time");
        return EXIT_FAILURE;
    }

    california = localtime(&now);
    if (california == NULL ||
        strftime(result, sizeof result, "%d.%m.%Y %H:%M:%S %Z", california) == 0) {
        fprintf(stderr, "Cannot convert time\n");
        return EXIT_FAILURE;
    }

    printf("California time: %s\n", result);
    return EXIT_SUCCESS;
}
