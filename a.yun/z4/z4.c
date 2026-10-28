#define _DEFAULT_SOURCE
#define __EXTENSIONS__ 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

struct node {
    char *text;
    struct node *next;
};

static size_t escape_controls(char *text, const char *source)
{
    size_t length = 0;
    while (*source != '\0') {
        unsigned char byte = (unsigned char)*source++;
        if ((byte < 32 && byte != '\n') || byte == 127) {
            text[length++] = '^';
            text[length++] = byte == 127 ? '?' : (char)(byte + 64);
        } else {
            text[length++] = (char)byte;
        }
    }
    text[length] = '\0';
    return length;
}

int main(void)
{
    struct node *head = NULL, *tail = NULL, *current;
    char buffer[4096];
    char text[2 * sizeof(buffer)];
    struct termios original, settings;
    int terminal_changed = 0;
    int status = 0;

    if (isatty(STDIN_FILENO)) {
        if (tcgetattr(STDIN_FILENO, &original) == -1) {
            perror("tcgetattr");
            return 1;
        }
        settings = original;
        settings.c_lflag |= ECHOCTL | IEXTEN;
        if (tcsetattr(STDIN_FILENO, TCSANOW, &settings) == -1) {
            perror("tcsetattr");
            return 1;
        }
        terminal_changed = 1;
    }
    puts("Enter lines; a dot at the beginning finishes input:");
    while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        size_t length;
        if (buffer[0] == '.')
            break;
        length = strlen(buffer);
        if (length == sizeof(buffer) - 1 && buffer[length - 1] != '\n') {
            fprintf(stderr, "Line must fit into the 4096-byte buffer\n");
            status = 1;
            break;
        }
        length = escape_controls(text, buffer);
        current = malloc(sizeof(*current));
        if (current == NULL) {
            perror("malloc");
            status = 1;
            break;
        }
        current->text = malloc(length + 1);
        if (current->text == NULL) {
            perror("malloc");
            free(current);
            status = 1;
            break;
        }
        strcpy(current->text, text);
        current->next = NULL;
        if (head == NULL)
            head = current;
        else
            tail->next = current;
        tail = current;
    }
    if (ferror(stdin)) {
        perror("fgets");
        status = 1;
    }
    puts("Saved lines:");
    for (current = head; current != NULL; current = current->next)
        fputs(current->text, stdout);
    while (head != NULL) {
        current = head->next;
        free(head->text);
        free(head);
        head = current;
    }
    if (terminal_changed)
        tcsetattr(STDIN_FILENO, TCSANOW, &original);
    return status;
}
