#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// `#define` was used to set the array size at compile-time
#define _CHARACTER_BOOL_MAP_NODES 32
const int CHARACTER_BOOL_MAP_NODES = _CHARACTER_BOOL_MAP_NODES;
const int CHARACTER_BOOL_MAP_NODE_SIZE = 8;

// helper functions

bool is_lowercase(char c)
{
    return (c > 96 && c < 123);
}

bool is_uppercase(char c)
{
    return (c > 64 && c < 91);
}

char to_uppercase(char c)
{
    if (is_lowercase(c))
    {
        return c & 0b11011111;
    }
    return c;
}

// Stores which characters can be unputted next.
// Each bit in the array of nodes represents a character
// with values: 0 - can't, and 1 - can be included
typedef struct char_bool_map
{
    char index[_CHARACTER_BOOL_MAP_NODES];
    int amount_of_true;
} char_bool_map;

char_bool_map new_char_bool_map()
{
    char_bool_map chars_idx;
    chars_idx.amount_of_true = 0;

    for (int i = 0; i < CHARACTER_BOOL_MAP_NODES; i++)
    {
        chars_idx.index[i] = 0;
    }

    return chars_idx;
}

void allow_char(char_bool_map **idx, char c)
{
    int array_idx = c / CHARACTER_BOOL_MAP_NODE_SIZE;
    int shift = c % CHARACTER_BOOL_MAP_NODE_SIZE;

    int pos = 1 << shift;

    bool already_set = ((**idx).index[array_idx] & pos) == pos;
    if (!already_set)
    {
        (**idx).index[array_idx] |= pos;
        (**idx).amount_of_true++;
    }
}

char get_node_char(int node_idx, int item_idx)
{
    return (node_idx * CHARACTER_BOOL_MAP_NODE_SIZE) + item_idx;
}

void print_next_chars(char_bool_map idx)
{
    int printed_chars = 0;

    printf("Enable: ");
    for (int node_idx = 0; node_idx < CHARACTER_BOOL_MAP_NODES; node_idx++)
    {
        for (int item_idx = 0; item_idx < CHARACTER_BOOL_MAP_NODE_SIZE; item_idx++)
        {
            // optimization: Do not traverse further if there is
            // definetly no chars left to print.
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
    printf("\n");
}

typedef enum compare_result
{
    FullMatch,
    NoMatch,
    PartialMatch
} compare_result;

compare_result compare_to_key(char *key, char *value)
{
    int key_len = strlen(key);

    for (int i = 0; i < key_len; i++)
    {
        if (to_uppercase(key[i]) != to_uppercase(value[i]))
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

typedef struct keyfilter_result
{
    bool no_results;
    char *found_address;
    char_bool_map *next_chars_bool_map;
} keyfilter_result;

keyfilter_result no_match_keyfilter_result(char_bool_map **idx)
{
    keyfilter_result result;
    result.no_results = true;
    result.found_address = NULL;
    result.next_chars_bool_map = *idx;
    return result;
}

keyfilter_result full_match_keyfilter_result(char_bool_map **idx, char *address)
{
    keyfilter_result result;
    result.no_results = false;
    result.found_address = address;
    result.next_chars_bool_map = *idx;
    return result;
}

keyfilter_result partial_match_keyfilter_result(char_bool_map **idx)
{
    keyfilter_result result;
    result.no_results = false;
    result.found_address = NULL;
    result.next_chars_bool_map = *idx;
    return result;
}

keyfilter_result keyfilter(char_bool_map **idx, char *key, int items_amount, char *items[])
{
    int key_len = strlen(key);

    for (int i = 0; i < items_amount; i++)
    {
        char *item = items[i];
        compare_result match_result = compare_to_key(key, item);

        if (match_result == NoMatch)
        {
            continue;
        }
        else if (match_result == PartialMatch)
        {
            char next_letter = item[key_len];
            allow_char(idx, to_uppercase(next_letter));
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

bool logging_enabled()
{
    char *logging = getenv("KEYFILTER_LOG");
    return logging != NULL && strcmp(logging, "1") == 0;
}

void log_parsed_input(char key[], char *addresses[], int addresses_amount)
{
    printf("Key: %s\n", key);

    printf("Addresses: ");
    for (int i = 0; i < addresses_amount; i++)
    {
        printf("%s, ", addresses[i]);
    }
    printf("\n");
}

void print_result(keyfilter_result *result)
{
    if (result->no_results)
    {
        printf("Not found\n");
    }
    else if (result->found_address != NULL)
    {
        printf("Found: %s\n", result->found_address);
    }
    else
    {
        print_next_chars(*result->next_chars_bool_map);
    }
}

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        printf("At least one argument must be provided.\n");
        return 1;
    }

    char_bool_map chars_map = new_char_bool_map();
    char_bool_map *chars_map_ptr = &chars_map;
    keyfilter_result result;

    char first_letter = argv[1][0];

    if (is_lowercase(first_letter))
    {
        char *key = argv[1];

        int addresses_amount = argc - 2;
        char *addresses[addresses_amount];

        for (int i = 0; i < addresses_amount; i++)
        {
            addresses[i] = argv[i + 2];
        }
        if (logging_enabled())
        {
            log_parsed_input(key, addresses, addresses_amount);
        }

        result = keyfilter(&chars_map_ptr, key, addresses_amount, addresses);
    }
    else
    {
        char *key = "";

        int addresses_amount = argc - 1;
        char *addresses[addresses_amount];

        for (int i = 0; i < addresses_amount; i++)
        {
            addresses[i] = argv[i + 1];
        }
        if (logging_enabled())
        {
            log_parsed_input(key, addresses, addresses_amount);
        }

        result = keyfilter(&chars_map_ptr, key, addresses_amount, addresses);
    }

    print_result(&result);

    return 0;
}
