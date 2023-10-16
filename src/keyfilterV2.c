// TODO: Case when only 1 address is found -> Found: ADDRESS
// TODO: Case when key matches the address but is also a part of another one
// -> Found: ADDRESS\n Enable: CHARS

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_ITEM_CHARS 100
#define MAX_ITEM_SIZE MAX_ITEM_CHARS + 1 // last char is for '\0'
#define ITEM_SEPARATOR '\n'
#define BOOL_MAP_NODES 32
#define BOOL_MAP_NODE_SIZE (int)(8 * sizeof(char))

#define IsLowercase(c) (c > 96 && c < 123)

bool logging_enabled()
{
    char *logging = getenv("KEYFILTER_LOGGING");
    return logging != NULL && strcmp(logging, "1") == 0;
}

// Stores which characters can be unputted next.
// Each bit in the array of nodes represents a character
// with values: 0 - can't, and 1 - can be included
typedef struct char_bool_map
{
    char index[BOOL_MAP_NODES];
    int chars_counter;
} char_bool_map;

char_bool_map new_char_bool_map()
{
    char_bool_map chars_idx = {.chars_counter = 0};

    for (int i = 0; i < BOOL_MAP_NODES; i++)
    {
        chars_idx.index[i] = 0;
    }

    return chars_idx;
}

int get_char_node(char c)
{
    return c / BOOL_MAP_NODE_SIZE;
}

int get_char_node_position(char c)
{
    int shift = c % BOOL_MAP_NODE_SIZE;
    return 1 << shift;
}

bool allow_char(char_bool_map *idx, char c)
{
    if ((int)c < 33 || (int)c > 126)
    {
        return false;
    }

    int node_idx = get_char_node(c);
    int position = get_char_node_position(c);

    idx->index[node_idx] |= position;
    idx->chars_counter++;

    return true;
}

char get_char_from_bool_map(int node_idx, int item_idx)
{
    return (node_idx * BOOL_MAP_NODE_SIZE) + item_idx;
}

void print_next_chars(char_bool_map *idx)
{
    int printed_chars = 0;

    printf("Enable: ");
    for (int node_idx = 0; node_idx < BOOL_MAP_NODES; node_idx++)
    {
        for (int item_idx = 0; item_idx < BOOL_MAP_NODE_SIZE; item_idx++)
        {
            if (printed_chars == idx->chars_counter)
            {
                printf("\n");
                return;
            }

            int n = 0b0000000000000001 << item_idx;
            bool should_print = (idx->index[node_idx] & n) == n;

            if (should_print)
            {
                printf("%c", get_char_from_bool_map(node_idx, item_idx));
                printed_chars++;
            }
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc > 2)
    {
        printf("No more than 1 argument must be provided. It must be a key to make a search for.\n");
        return 1;
    }

    char key[MAX_ITEM_SIZE] = "";
    if (argc == 2)
    {
        strcpy(key, argv[1]);
    }

    if (logging_enabled())
    {
        printf("LOG: Key is `%s`\n", key);
    }

    int key_len = strlen(key);

    char latest_item[MAX_ITEM_SIZE];
    char_bool_map chars_map = new_char_bool_map();

    bool all_items_read = false;

    while (!all_items_read)
    {
        char item[MAX_ITEM_SIZE] = "";
        int char_idx = 0;

        bool skip_item = false;
        bool item_matches = true;
        while (true)
        {
            char c = fgetc(stdin);

            if (c == EOF)
            {
                all_items_read = true;
                break;
            }
            else if (c == ITEM_SEPARATOR)
            {
                if (item_matches)
                {
                    strcpy(latest_item, item);
                }
                break;
            }
            if (skip_item)
            {
                continue;
            }

            if (toupper(key[char_idx]) == toupper(c))
            {
                item[char_idx] = c;
            }
            else
            {
                skip_item = true;
            }
        }
        skip_item = false;
    }

    // char item[MAX_ITEM_SIZE] = "";
    // int item_next_char_idx = 0;
    // bool skip_item = false;
    // bool item_matching = true;

    // char c;
    // while (((c = fgetc(stdin)) != EOF) && !skip_item)
    // {
    //     if (logging_enabled())
    //     {
    //         printf("LOG: Read char `%c(%i)` from stdin\n", c, c);
    //     }

    //     if (c == ITEM_SEPARATOR)
    //     {
    //         strcpy(item, "");
    //         skip_item = false;
    //         continue;
    //     }

    //     if (item_next_char_idx == MAX_ITEM_SIZE)
    //     {
    //         printf("One of the items is too long. Each item should not be longer than 100 characters.\n");
    //         return 1;
    //     }

    //     if (item_next_char_idx < key_len)
    //     {
    //         char key_char = key[item_next_char_idx];
    //         bool chars_equal = toupper(key_char) == toupper(c);
    //         if (logging_enabled())
    //         {
    //             printf("LOG: comparing key char `%c` and item char `%c`\n", key_char, c);
    //         }

    //         if (!chars_equal)
    //         {
    //             skip_item = true;
    //             continue;
    //         }

    //         item[item_next_char_idx] = c;
    //     }

    //     strcpy(latest_item, item);
    //     allow_char(&chars_map, c);

    //     if (strcmp(key, item) == 0)
    //     {
    //         printf("Found: %s\n", item);
    //         return 0;
    //     }
    // }

    if (chars_map.chars_counter == 0)
    {
        printf("Not found.\n");
    }
    else if (chars_map.chars_counter == 1)
    {
        printf("Found: %s\n", latest_item);
    }
    else
    {
        print_next_chars(&chars_map);
    }
    return 0;
}
