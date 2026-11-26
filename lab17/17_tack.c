#include <termios.h>
#include <unistd.h>

#define WIDTH 40
#define CTRL_D 4
#define CTRL_W 23

void erase_symbol(void) {
    write(1, "\b \b", 3);
}

void beep(void) {
    write(1, "\a", 1);
}

int main(void) {
    struct termios old_s;
    struct termios new_s;

    char line[WIDTH];
    char symbol;
    int length = 0;

    tcgetattr(0, &old_s);

    new_s = old_s;
    new_s.c_lflag &= ~(ECHO | ICANON | ISIG);
    new_s.c_cc[VMIN] = 1;
    new_s.c_cc[VTIME] = 0;

    tcsetattr(0, TCSANOW, &new_s);

    while (read(0, &symbol, 1) == 1) {
        if (symbol == old_s.c_cc[VERASE]) {
            if (length > 0) {
                length--;
                erase_symbol();
            } else {
                beep();
            }
        }

        else if (symbol == old_s.c_cc[VKILL]) {
            while (length > 0) {
                length--;
                erase_symbol();
            }
        }

        else if (symbol == CTRL_W) {
            if (length == 0) {
                beep();
                continue;
            }

            while (length > 0 && line[length - 1] == ' ') {
                length--;
                erase_symbol();
            }

            while (length > 0 && line[length - 1] != ' ') {
                length--;
                erase_symbol();
            }
        }

        else if (symbol == CTRL_D) {
            if (length == 0)
                break;

            beep();
        }

        else if (symbol < 32 || symbol > 126) {
            beep();
        }

        else {
            if (length == WIDTH) {
                int word_start = length;

                while (word_start > 0 &&
                       line[word_start - 1] != ' ') {
                    word_start--;
                }

                if (word_start > 0 && symbol != ' ') {
                    int word_length = length - word_start;

                    while (length > word_start) {
                        length--;
                        erase_symbol();
                    }

                    write(1, "\n", 1);

                    for (int i = 0; i < word_length; i++)
                        line[i] = line[word_start + i];

                    length = word_length;
                    write(1, line, length);
                } else {
                    write(1, "\n", 1);
                    length = 0;
                }
            }

            line[length] = symbol;
            length++;

            write(1, &symbol, 1);
        }
    }

    tcsetattr(0, TCSANOW, &old_s);
    write(1, "\n", 1);

    return 0;
}