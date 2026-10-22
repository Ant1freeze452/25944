#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct ListItem {
    char *value;
    struct ListItem *next;
} ListItem;

int input_string(char **out) {
    char chunk[256];
    char *data = NULL;
    size_t used = 0;
    size_t allocated = 0;

    *out = NULL;

    while (fgets(chunk, sizeof(chunk), stdin)) {
        size_t count = strlen(chunk);
        int end = count > 0 && chunk[count - 1] == '\n';

        if (end)
            count--;

        if (count > SIZE_MAX - used - 1) {
            free(data);
            return -1;
        }

        size_t required = used + count + 1;

        if (required > allocated) {
            size_t size = allocated ? allocated : sizeof(chunk);

            while (size < required) {
                if (size > SIZE_MAX / 2) {
                    size = required;
                    break;
                }
                size *= 2;
            }

            char *temp = realloc(data, size);

            if (!temp) {
                free(data);
                return -1;
            }

            data = temp;
            allocated = size;
        }

        memcpy(data + used, chunk, count);
        used += count;
        data[used] = '\0';

        if (end) {
            *out = data;
            return 1;
        }
    }

    if (ferror(stdin)) {
        free(data);
        return -1;
    }

    *out = data;
    return data != NULL;
}

static int is_arrow(char c) {
    return c == 'A' || c == 'B' || c == 'C' || c == 'D';
}

void filter_string(char *s) {
    size_t src = 0;
    size_t dst = 0;

    while (s[src]) {
        if ((unsigned char)s[src] == 27) {
            if (s[src + 1] == '[' || s[src + 1] == 'O') {
                if (is_arrow(s[src + 2]))
                    src += 3;
                else
                    src += 2;
            } else {
                src++;
            }

            continue;
        }

        s[dst++] = s[src++];
    }

    s[dst] = '\0';
}

int append_item(ListItem ***position, char *value) {
    ListItem *item = malloc(sizeof(ListItem));

    if (!item)
        return 0;

    item->value = value;
    item->next = NULL;

    **position = item;
    *position = &item->next;

    return 1;
}

void destroy_list(ListItem *item) {
    while (item) {
        ListItem *following = item->next;
        free(item->value);
        free(item);
        item = following;
    }
}

int main(void) {
    ListItem *first = NULL;
    ListItem **end = &first;
    int exit_code = EXIT_SUCCESS;

    while (1) {
        char *str = NULL;
        int state = input_string(&str);

        if (state == 0)
            break;

        if (state < 0) {
            fprintf(stderr, "Ошибка чтения или выделения памяти\n");
            exit_code = EXIT_FAILURE;
            break;
        }

        filter_string(str);

        if (*str == '.') {
            free(str);
            break;
        }

        if (*str == '\0') {
            free(str);
            continue;
        }

        if (!append_item(&end, str)) {
            fprintf(stderr, "Не удалось создать элемент списка\n");
            free(str);
            exit_code = EXIT_FAILURE;
            break;
        }
    }

    if (exit_code == EXIT_SUCCESS) {
        for (ListItem *current = first;
             current != NULL;
             current = current->next) {
            if (puts(current->value) == EOF) {
                exit_code = EXIT_FAILURE;
                break;
            }
        }

        if (fflush(stdout) == EOF)
            exit_code = EXIT_FAILURE;
    }

    destroy_list(first);
    return exit_code;
}