#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define LIMIT 4096

typedef struct StringNode{
    char *string;
    struct StringNode *next;
} StringNode;


StringNode *createNode(char *input_str){
    StringNode *node = (StringNode*)malloc(sizeof(StringNode));
    if(node == NULL)
        return NULL;

    size_t len = strlen(input_str);
    node->string = (char*)malloc((len+1)*sizeof(char));
    if(node->string == NULL){
        free(node);
        return NULL;
    }

    strcpy(node->string, input_str);
    node->next = NULL;

    return node;
}

void deleteStringList(StringNode *head){
    while(head != NULL){
        StringNode *next = head->next;
        free(head->string);
        free(head);
        head = next;
    }
}

void printVisibleString(const char *text){
    const unsigned char *p = (const unsigned char *)text;

    unsigned char c = '\0';
    while(*p != '\0'){
        c = *p;
        if(c < 32 || c == 127){
            putchar('^');
            putchar(c == 127 ? '?' : c + 64);
        }
        else{
            putchar(c);
        }
        p++;
    }
    putchar('\n');
}

int main(void){

    StringNode *head = NULL;
    StringNode *tail = NULL;

    char input_str[LIMIT];

    while(fgets(input_str, LIMIT, stdin)){

        if(input_str[0] == '.')
            break;

        size_t len = strlen(input_str);

        if(len > 0 && input_str[len-1] == '\n')
            input_str[len-1] = '\0';
        else{
            int ch = getchar();
            if(ch != '\n' && ch != EOF){
                while((ch = getchar()) != '\n' && ch != EOF);
                fprintf(stderr, "String longer then limit, try again\n");
                continue;
            }
        }

        StringNode *node = createNode(input_str);
        if(node == NULL){
            deleteStringList(head);
            return 1;
        }

        if(head == NULL)
            head = node;
        else
            tail->next = node;

        tail = node;
    }

    for(StringNode *current = head; current != NULL; current = current->next){
        printVisibleString(current->string);
    }

    deleteStringList(head);

    return 0;
}