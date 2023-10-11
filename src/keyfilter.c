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
    int amount_of_true;
} char_bool_map;

char_bool_map new_char_bool_map()
{
    char_bool_map chars_idx;
    chars_idx.amount_of_true = 0;

    for (int i = 0; i < BOOL_MAP_NODES; i++)
    {
        chars_idx.index[i] = 0;
    }

    return chars_idx;
}

bool allow_char(char_bool_map **idx, char c)
{
    if ((int)c < 33 || (int)c > 126)
    {
        return false;
    }

    int array_idx = c / BOOL_MAP_NODE_SIZE;
    int shift = c % BOOL_MAP_NODE_SIZE;

    int pos = 1 << shift;

    bool already_allowed = ((**idx).index[array_idx] & pos) == pos;
    if (!already_allowed)
    {
        (**idx).index[array_idx] |= pos;
        (**idx).amount_of_true++;
    }

    return true;
}

char get_node_char(int node_idx, int item_idx)
{
    return (node_idx * BOOL_MAP_NODE_SIZE) + item_idx;
}

void print_next_chars(char_bool_map idx)
{
    int printed_chars = 0;

    printf("Enable: ");
    for (int node_idx = 0; node_idx < BOOL_MAP_NODES; node_idx++)
    {
        for (int item_idx = 0; item_idx < BOOL_MAP_NODE_SIZE; item_idx++)
        {
            if (printed_chars == idx.amount_of_true)
            {
                printf("\n");
                return;
            }

            int n = 0b0000000000000001 << item_idx;
            bool should_print = (idx.index[node_idx] & n) == n;

            if (should_print)
            {
                printf("%c", get_node_char(node_idx, item_idx));
                printed_chars++;
            }
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
        printf("DEBUG: Comparing key `%s` to item `%s` with first letter `%i`\n", key, value, value[0]);
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
    bool has_invalid_item;
    bool no_results;
    char found_item[MAX_ITEM_SIZE];
    char_bool_map *next_chars_bool_map;
} keyfilter_result;

keyfilter_result invalid_item_keyfilter_result(char_bool_map **idx)
{
    keyfilter_result result = {true, true, "", *idx};
    return result;
}

keyfilter_result no_match_keyfilter_result(char_bool_map **idx)
{
    keyfilter_result result = {false, true, "", *idx};
    return result;
}

keyfilter_result full_match_keyfilter_result(char_bool_map **idx, char item[MAX_ITEM_SIZE])
{
    keyfilter_result result = {false, false, "", *idx};
    strcpy(result.found_item, item);
    return result;
}

keyfilter_result partial_match_keyfilter_result(char_bool_map **idx)
{
    keyfilter_result result = {false, false, "", *idx};
    return result;
}

keyfilter_result keyfilter(char_bool_map **idx, char *key, FILE *stream)
{
    while (true)
    {
        char item[MAX_ITEM_SIZE] = "";
        read_item_result item_result = read_item(&item[0], stream);

        if (item_result.item_too_long)
        {
            return invalid_item_keyfilter_result(idx);
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
                return invalid_item_keyfilter_result(idx);
            }
        }
        else if (match_result == FullMatch)
        {
            return full_match_keyfilter_result(idx, item);
        }
    }

    if ((**idx).amount_of_true == 0)
    {
        return no_match_keyfilter_result(idx);
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
    char_bool_map *chars_map_ptr = &chars_map;
    keyfilter_result result = keyfilter(&chars_map_ptr, key, stdin);

    if (result.has_invalid_item)
    {
        printf("Some of the items is invalid.\n");
    }
    else if (result.no_results)
    {
        printf("Not found\n");
    }
    else if (strlen(result.found_item) != 0)
    {
        printf("Found: %s\n", result.found_item);
    }
    else
    {
        print_next_chars(chars_map);
    }

    return 0;
}
