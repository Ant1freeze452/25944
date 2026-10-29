#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MAX_LEN 256

typedef struct node
{
    char *text;
    struct node *next;
} node_t;

static void free_list(node_t *head)
{
    node_t *next;

    while (head != NULL)
    {
        next = head->next;
        free(head->text);
        free(head);
        head = next;
    }
}

int main(void)
{
    char *line;
    node_t *head = NULL, *tail = NULL, *item;

    while ((line = readline("")) != NULL)
    {
        if (line[0] == '.')
        {
            free(line);
            break;
        }

        if (strlen(line) > MAX_LEN)
        {
            printf("Too long. Maximum: %d characters\n", MAX_LEN);
            free(line);
            continue;
        }

        if (line[0] != '\0')
            add_history(line);

        item = malloc(sizeof *item);
        if (item == NULL)
        {
            perror("malloc");
            free(line);
            free_list(head);
            return EXIT_FAILURE;
        }

        item->text = line;
        item->next = NULL;

        if (head == NULL)
            head = item;
        else
            tail->next = item;
        tail = item;
    }

    puts("List:");
    for (item = head; item != NULL; item = item->next)
        puts(item->text);

    free_list(head);
    return EXIT_SUCCESS;
}
