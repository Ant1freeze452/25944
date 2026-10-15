#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Node {
    char *text;
    struct Node *next;
} Node;

/* 1: строка прочитана; 0: конец ввода; -1: ошибка. */
static int read_line(char **result)
{
    char buffer[256];
    char *line = NULL;
    size_t length = 0;
    size_t capacity = 0;

    *result = NULL;
    while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        size_t part = strlen(buffer);
        size_t needed;
        int complete = part > 0 && buffer[part - 1] == '\n';

        if (complete)
            --part;

        if (part > SIZE_MAX - length - 1) {
            fprintf(stderr, "Строка слишком длинная\n");
            free(line);
            return -1;
        }

        needed = length + part + 1;
        if (needed > capacity) {
            size_t new_capacity = capacity == 0 ? sizeof(buffer) : capacity;
            char *new_line;

            while (new_capacity < needed) {
                if (new_capacity > SIZE_MAX / 2) {
                    new_capacity = needed;
                    break;
                }
                new_capacity *= 2;
            }

            new_line = realloc(line, new_capacity);
            if (new_line == NULL) {
                fprintf(stderr, "Недостаточно памяти\n");
                free(line);
                return -1;
            }
            line = new_line;
            capacity = new_capacity;
        }

        memcpy(line + length, buffer, part);
        length += part;
        line[length] = '\0';

        if (complete) {
            *result = line;
            return 1;
        }
    }

    if (ferror(stdin)) {
        fprintf(stderr, "Ошибка чтения\n");
        free(line);
        return -1;
    }

    *result = line;
    return line != NULL;
}

/* Убираем ANSI-коды и управляющие символы, сохраняя байты UTF-8. */
static void clean_input(char *text)
{
    size_t r = 0;
    size_t w = 0;

    while (text[r] != '\0') {
        unsigned char ch = (unsigned char)text[r++];

        if (ch == 27) {
            if (text[r] == '[' || text[r] == 'O') {
                ++r;
                while (text[r] != '\0') {
                    unsigned char code = (unsigned char)text[r++];
                    if (code >= 0x40 && code <= 0x7e)
                        break;
                }
            } else if (text[r] == ']' || text[r] == 'P' ||
                       text[r] == 'X' || text[r] == '^' || text[r] == '_') {
                ++r;
                while (text[r] != '\0') {
                    if ((unsigned char)text[r] == 7) {
                        ++r;
                        break;
                    }
                    if ((unsigned char)text[r] == 27 && text[r + 1] == '\\') {
                        r += 2;
                        break;
                    }
                    ++r;
                }
            } else {
                while ((unsigned char)text[r] >= 0x20 &&
                       (unsigned char)text[r] <= 0x2f)
                    ++r;
                if ((unsigned char)text[r] >= 0x30 &&
                    (unsigned char)text[r] <= 0x7e)
                    ++r;
            }
            continue;
        }

        if ((ch < 32 && ch != '\t') || ch == 127)
            continue;

        text[w++] = (char)ch;
    }
    text[w] = '\0';
}

static void free_list(Node *head)
{
    while (head != NULL) {
        Node *next = head->next;
        free(head->text);
        free(head);
        head = next;
    }
}

int main(void)
{
    Node *head = NULL;
    Node *tail = NULL;
    int status = EXIT_SUCCESS;

    for (;;) {
        char *line;
        Node *node;
        int result = read_line(&line);

        if (result <= 0) {
            if (result < 0)
                status = EXIT_FAILURE;
            break;
        }

        clean_input(line);
        if (line[0] == '.') {
            free(line);
            break;
        }

        node = malloc(sizeof(*node));
        if (node == NULL) {
            fprintf(stderr, "Недостаточно памяти\n");
            free(line);
            status = EXIT_FAILURE;
            break;
        }
        node->text = line;
        node->next = NULL;

        if (tail == NULL)
            head = node;
        else
            tail->next = node;
        tail = node;
    }

    if (status == EXIT_SUCCESS) {
        for (Node *node = head; node != NULL; node = node->next) {
            if (puts(node->text) == EOF) {
                status = EXIT_FAILURE;
                break;
            }
        }
        if (fflush(stdout) == EOF)
            status = EXIT_FAILURE;
    }

    free_list(head);
    return status;
}
