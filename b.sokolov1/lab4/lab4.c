#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <readline/readline.h>
#include <readline/history.h>

typedef struct Node {
    char *str;
    struct Node *next;
} Node;

void free_list(Node *head)
{
    while (head != NULL) {
        Node *next = head->next;
        free(head->str);
        free(head);
        head = next;
    }
}

int main(void)
{
    setlocale(LC_ALL, "");

    Node *head = NULL;
    Node *tail = NULL;
    char *line;

    printf("Введите строки (точка в начале строки — конец):\n");

    while ((line = readline("> ")) != NULL) {
        if (line[0] == '.') {
            free(line);
            break;
        }

        if (line[0] != '\0') {
            add_history(line);
        }

        Node *new_node = malloc(sizeof(Node));
        if (new_node == NULL) {
            perror("malloc");
            free(line);
            free_list(head);
            clear_history();
            return 1;
        }

        new_node->str = line;
        new_node->next = NULL;

        if (head == NULL) {
            head = new_node;
        } else {
            tail->next = new_node;
        }
        tail = new_node;
    }

    printf("\nВведённые строки:\n");
    for (Node *current = head; current != NULL; current = current->next) {
        puts(current->str);
    }

    free_list(head);
    clear_history();
    return 0;
}
