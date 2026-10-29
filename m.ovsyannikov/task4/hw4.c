#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE 4096

typedef struct ListItem {
    char *text;
    struct ListItem *next;
} ListItem;

static void strip_escapes(char *s)
{
    char *src = s;
    char *dst = s;

    while (*src != '\0') {
        if (*src == '\033') {
            src++;
            if (*src == '[') {
                src++;
                while (*src != '\0' && !(*src >= 0x40 && *src <= 0x7E))
                    src++;
                if (*src != '\0')
                    src++;
            } else if (*src == 'O') {
                src++;
                if (*src != '\0')
                    src++;
            }
            continue;
        }
        *dst++ = *src++;
    }
    *dst = '\0';
}

static ListItem *make_item(const char *line)
{
    size_t n = strlen(line);
    ListItem *item = malloc(sizeof(*item));

    if (item == NULL)
        return NULL;

    item->text = malloc(n + 1);          
    if (item->text == NULL) {
        free(item);
        return NULL;
    }

    memcpy(item->text, line, n + 1);
    item->next = NULL;
    return item;
}

static void print_items(const ListItem *p)
{
    for (; p != NULL; p = p->next)
        fputs(p->text, stdout);
}

static void destroy_items(ListItem *p)
{
    while (p != NULL) {
        ListItem *next = p->next;
        free(p->text);
        free(p);
        p = next;
    }
}

int main(void)
{
    char line[MAX_LINE];
    ListItem *first = NULL;
    ListItem **last_link = &first;   

    while (fgets(line, sizeof(line), stdin) != NULL) {
        strip_escapes(line);

        if (line[0] == '.')
            break;

        if (line[0] == '\0' || strcmp(line, "\n") == 0)
            continue;

        ListItem *item = make_item(line);
        if (item == NULL) {
            perror("malloc");
            destroy_items(first);
            return EXIT_FAILURE;
        }

        *last_link = item;
        last_link = &item->next;
    }

    print_items(first);
    destroy_items(first);

    return EXIT_SUCCESS;
}