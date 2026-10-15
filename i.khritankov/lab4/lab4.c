#include <stdio.h> // fgets, printf, perror
#include <stdlib.h> // malloc, realloc, free
#include <string.h> // strlen, memcpy
#include <stdint.h> // SIZE_MAX
#include <unistd.h> // isatty, STDIN_FILENO

extern char *readline(const char *prompt); // редактор строки из libreadline
extern void add_history(const char *line); // история для стрелок вверх и вниз

struct Node
{
    char *text; // указатель на динамически выделенную строку
    struct Node *next;
};

static char *read_text(int terminal, int *status) // возвращаем целую строку в динамической памяти
{
    char buffer[256]; // длинные строки читаем по частям
    char *text = NULL; // пока строка не выделена
    char *grown; // новый адрес строки после realloc
    size_t length = 0; // число уже сохранённых байт строки
    size_t part; // длина очередной порции из fgets

    if (terminal)
    {
        text = readline(NULL); // стрелки, Backspace и Delete обрабатывает редактор
        if (text == NULL)
        {
            return NULL;
        }
        if (text[0] != '\0' && text[0] != '.')
        {
            add_history(text);
        }
        length = strlen(text); // получаем длину строки без завершающего нуля
        if (length > SIZE_MAX - 2) // проверяем переполнение перед добавлением перевода строки и нуля
        {
            fprintf(stderr, "Line is too long\n");
            free(text);
            *status = 1;
            return NULL;
        }
        grown = realloc(text, length + 2); // выделяем место для двух завершающих символов
        if (grown == NULL)
        {
            perror("realloc");
            free(text);
            *status = 1;
            return NULL;
        }
        grown[length] = '\n'; // readline возвращает строку без перевода строки
        grown[length + 1] = '\0'; // добавляем завершающий нулевой символ
        return grown;
    }

    while (fgets(buffer, sizeof(buffer), stdin) != NULL)
    {
        part = strlen(buffer); // узнаём длину прочитанной порции
        if (part == 0 || length > SIZE_MAX - part - 1) // проверяем текстовый ввод и переполнение размера памяти
        {
            fprintf(stderr, "Invalid or too long text line\n");
            free(text);
            *status = 1;
            return NULL;
        }
        grown = realloc(text, length + part + 1); // длинная строка читается частями
        if (grown == NULL)
        {
            perror("realloc");
            free(text);
            *status = 1;
            return NULL;
        }
        text = grown; // сохраняем новый адрес строки
        memcpy(text + length, buffer, part + 1); // добавляем порцию вместе с завершающим нулём
        length += part; // увеличиваем общую длину строки
        if (buffer[part - 1] == '\n') // строка прочитана полностью
        {
            break;
        }
    }
    if (ferror(stdin))
    {
        perror("fgets");
        free(text);
        *status = 1;
        return NULL;
    }
    return text;
}

int main(void)
{
    struct Node *head = NULL; // первый узел списка
    struct Node *tail = NULL; // последний узел для добавления в конец
    struct Node *node; // узел, с которым сейчас работаем
    char *text; // указатель на динамически выделенную строку
    int terminal = isatty(STDIN_FILENO); // отличаем клавиатуру от файла или конвейера
    int status = 0; // пока ошибок нет

    while ((text = read_text(terminal, &status)) != NULL) // читаем строки до точки, EOF или ошибки
    {
        if (text[0] == '.') // только точка в начале строки завершает ввод
        {
            free(text);
            break;
        }
        node = malloc(sizeof(*node)); // выделяем память для нового узла списка
        if (node == NULL)
        {
            perror("malloc");
            free(text);
            status = 1;
            break;
        }
        node->text = text; // передаём узлу прочитанную строку
        node->next = NULL; // после нового последнего узла ничего нет
        if (head == NULL)
        {
            head = node; // создаём начало ранее пустого списка
        }
        else
        {
            tail->next = node; // сохраняем порядок ввода
        }
        tail = node; // обновляем указатель на последний узел
    }

    while (head != NULL) // проходим список для вывода и освобождения
    {
        node = head; // запоминаем узел перед его удалением
        if (status == 0)
        {
            printf("%s", node->text);
        }
        head = head->next; // переходим дальше до освобождения узла
        free(node->text); // освобождаем память строки
        free(node); // освобождаем память самого узла
    }
    return status;
}
