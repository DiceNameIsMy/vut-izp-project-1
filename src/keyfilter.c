#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_ITEM_SIZE 100
#define ITEM_SEPARATOR '\n'
#define BOOL_MAP_NODES 32
#define BOOL_MAP_NODE_SIZE (int)(8 * sizeof(char))

#define IsLowercase(c) (c > 96 && c < 123)

bool is_empty(char *string)
{
    return strlen(string) == 0;
}

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
    int allowed_chars_counter;
} char_bool_map;

char_bool_map new_char_bool_map()
{
    char_bool_map chars_idx;
    chars_idx.chars_counter = 0;
    chars_idx.allowed_chars_counter = 0;

    for (int i = 0; i < BOOL_MAP_NODES; i++)
    {
        chars_idx.index[i] = 0;
    }

    return chars_idx;
}

bool is_printable(char c)
{
    return (int)c > 32 || (int)c < 127;
}

bool allow_char(char_bool_map *idx, char c)
{
    if (!is_printable(c))
    {
        if (logging_enabled())
        {
            printf("LOG: Received an invalid char `%c(%i)`\n", c, c);
        }
        return false;
    }

    if (logging_enabled())
    {
        printf("LOG: Attemting to allow char %c(%i).\n", c, c);
    }

    int node_idx = c / BOOL_MAP_NODE_SIZE;
    int char_pos = 1 << (c % BOOL_MAP_NODE_SIZE);

    bool is_char_allowed = (idx->index[node_idx] & char_pos) == char_pos;
    if (!is_char_allowed)
    {
        if (logging_enabled())
        {
            printf("LOG: Setting char %c(%i) as allowed.\n", c, c);
        }
        idx->index[node_idx] |= char_pos;
        idx->chars_counter++;
    }
    else
    {
        if (logging_enabled())
        {
            printf("LOG: Char `%c` is already allowed.\n", c);
        }
    }

    idx->allowed_chars_counter++;

    return true;
}

char get_char_from_bool_map(int node_idx, int item_idx)
{
    return (node_idx * BOOL_MAP_NODE_SIZE) + item_idx;
}

void print_node_chars(char_bool_map *idx, int node_idx, int *print_counter)
{
    for (int item_idx = 0; item_idx < BOOL_MAP_NODE_SIZE; item_idx++)
    {
        int n = 0b0000000000000001 << item_idx;
        bool should_print = (idx->index[node_idx] & n) == n;

        if (should_print)
        {
            printf("%c", get_char_from_bool_map(node_idx, item_idx));
            (*print_counter)++;
        }
    }
}

void print_next_chars(char_bool_map *idx)
{
    int printed_chars_counter = 0;

    printf("Enable: ");
    for (int node_idx = 0; node_idx < BOOL_MAP_NODES; node_idx++)
    {
        print_node_chars(idx, node_idx, &printed_chars_counter);

        if (printed_chars_counter == idx->chars_counter)
        {
            printf("\n");
            return;
        }
    }
}

typedef enum compare_result
{
    FullMatch,
    NoMatch,
    PartialMatch
} compare_result;

compare_result compare_to_key(char *key, char *value)
{
    if (logging_enabled())
    {
        printf("LOG: Comparing key `%s` to item `%s` with first letter `%i`\n", key, value, value[0]);
    }

    int key_len = strlen(key);

    for (int char_idx = 0; char_idx < key_len; char_idx++)
    {
        if (toupper(key[char_idx]) != toupper(value[char_idx]))
        {
            return NoMatch;
        }
    }

    int value_len = strlen(value);
    if (key_len == value_len)
    {
        return FullMatch;
    }

    return PartialMatch;
}

typedef struct read_item_result
{
    bool read_all_items;
    bool item_too_long;
} read_item_result;

read_item_result read_item(char *item, FILE *stream)
{
    int next_char_idx = 0;

    char c;
    read_item_result result;

    while ((c = fgetc(stream)) != ITEM_SEPARATOR)
    {
        item[next_char_idx] = c;

        result.read_all_items = (c == EOF);
        result.item_too_long = (next_char_idx == MAX_ITEM_SIZE);
        if (result.read_all_items || result.item_too_long)
        {
            return result;
        }

        next_char_idx++;
    }
    return result;
}

typedef struct keyfilter_result
{
    char invalid_item[MAX_ITEM_SIZE];
    bool no_results;
    char found_item[MAX_ITEM_SIZE];
    char_bool_map *next_chars_bool_map;
} keyfilter_result;

keyfilter_result invalid_item_keyfilter_result(char_bool_map *idx, char invalid_item[MAX_ITEM_SIZE])
{
    keyfilter_result result = {"", true, "", idx};
    strcpy(result.invalid_item, invalid_item);
    return result;
}

keyfilter_result no_match_keyfilter_result(char_bool_map *idx)
{
    keyfilter_result result = {"", true, "", idx};
    return result;
}

keyfilter_result full_match_keyfilter_result(char_bool_map *idx, char item[MAX_ITEM_SIZE])
{
    keyfilter_result result = {"", false, "", idx};
    strcpy(result.found_item, item);
    return result;
}

keyfilter_result partial_match_keyfilter_result(char_bool_map *idx)
{
    keyfilter_result result = {"", false, "", idx};
    return result;
}

keyfilter_result keyfilter(char_bool_map *idx, char *key, FILE *stream)
{
    char latest_item[MAX_ITEM_SIZE] = "";
    char found_item[MAX_ITEM_SIZE] = "";

    while (true)
    {
        char item[MAX_ITEM_SIZE] = "";
        read_item_result item_result = read_item(&item[0], stream);

        if (item_result.item_too_long)
        {
            return invalid_item_keyfilter_result(idx, item);
        }
        if (item_result.read_all_items)
        {
            break;
        }

        compare_result match_result = compare_to_key(key, item);

        if (match_result == NoMatch)
        {
            continue;
        }
        else if (match_result == PartialMatch)
        {
            char next_letter = item[strlen(key)];
            bool valid_char = allow_char(idx, toupper(next_letter));
            if (!valid_char)
            {
                return invalid_item_keyfilter_result(idx, item);
            }

            strcpy(latest_item, item);
        }
        else if (match_result == FullMatch)
        {
            strcpy(found_item, item);
        }
    }

    if (logging_enabled())
    {
        printf("LOG: Finished filtering\n");
        printf("LOG: Returning results with following data:\n");
        printf("LOG: latest_item: `%s`\n", latest_item);
        printf("LOG: found_item: `%s`\n", found_item);
        printf("LOG: amount_of_chars_allowed: `%i`\n", idx->chars_counter);
        printf("LOG: amount_of_items_matched: `%i`\n", idx->allowed_chars_counter);
    }

    if (!is_empty(found_item))
    {
        return full_match_keyfilter_result(idx, found_item);
    }
    else if (idx->chars_counter == 0)
    {
        return no_match_keyfilter_result(idx);
    }
    else if (idx->chars_counter == 1 && idx->allowed_chars_counter == 1)
    {
        return full_match_keyfilter_result(idx, latest_item);
    }

    return partial_match_keyfilter_result(idx);
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

    char_bool_map chars_map = new_char_bool_map();
    keyfilter_result result = keyfilter(&chars_map, key, stdin);

    if (!is_empty(result.invalid_item))
    {
        printf("Received an invalid item `%s`.\n", result.invalid_item);
    }
    else if (result.no_results)
    {
        printf("Not found\n");
    }
    else if (!is_empty(result.found_item))
    {
        printf("Found: %s\n", result.found_item);

        if (chars_map.chars_counter > 1)
        {
            print_next_chars(&chars_map);
        }
    }
    else
    {
        print_next_chars(&chars_map);
    }

    return 0;
}
