#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>

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
// Each bit in the array of integers represents a character
// with values: 0 - can't, and 1 - can be included
typedef struct character_bool_map
{
    char index[_CHARACTER_BOOL_MAP_NODES];
    int amount_of_true;
} character_bool_map;

character_bool_map new_character_bool_map()
{
    character_bool_map chars_idx;
    chars_idx.amount_of_true = 0;

    for (int i = 0; i < CHARACTER_BOOL_MAP_NODES; i++)
    {
        chars_idx.index[i] = 0;
    }

    return chars_idx;
}

void allow_char(character_bool_map **idx, char c)
{
    // printf("Letter to allow: %c(%u)\n", c, (unsigned int)c);

    int array_idx = c / CHARACTER_BOOL_MAP_NODE_SIZE;
    int shift = c % CHARACTER_BOOL_MAP_NODE_SIZE;

    int pos = 1 << shift;

    // printf("Idx: %i, Pos: %i\n", array_idx, pos);

    bool already_set = ((**idx).index[array_idx] & pos) == pos;
    if (!already_set)
    {
        (**idx).index[array_idx] |= pos;
        (**idx).amount_of_true++;

        // printf("Alowed char: %c, Shift: %i, Idx value: %hi\n", c, shift, (**idx).index[array_idx]);
    }
}

void print_next_letters(character_bool_map idx)
{
    int printed_chars = 0;

    printf("Enable: ");
    for (int i = 0; i < CHARACTER_BOOL_MAP_NODES; i++)
    {
        // printf("Byte: %i, Value: %hi\n", i, idx.index[i]);
        for (int j = 0; j < CHARACTER_BOOL_MAP_NODE_SIZE; j++)
        {
            // printf("Printed chars: %i, Amount of true: %i\n", printed_chars, idx.amount_of_true);
            if (printed_chars == idx.amount_of_true)
            {
                printf("\n");
                return;
            }
            int n = 0b0000000000000001 << j;
            bool should_print = (idx.index[i] & n) == n;
            // printf("Bit: %i\n", n);

            if (should_print)
            {
                // printf("Printing bit: %u\n", n);
                // printf("Char to print: %i\n", ((i * CHARACTER_INDEX_ITEM_BITS_AMOUNT) + j));
                printf("%c", (char)((i * CHARACTER_BOOL_MAP_NODE_SIZE) + j));
                printed_chars++;
            }
        }
    }
    printf("\n");
}

typedef enum address_match_result
{
    FullMatch,
    DoesNotMatch,
    PartlyMatch
} address_match_result;

address_match_result check_address(char *key, char *address)
{
    int key_len = strlen(key);

    for (int i = 0; i < key_len; i++)
    {
        if (to_uppercase(key[i]) != to_uppercase(address[i]))
        {
            return DoesNotMatch;
        }
    }

    int address_len = strlen(address);
    if (key_len == address_len)
    {
        return FullMatch;
    }

    return PartlyMatch;
}

typedef struct autocomplete_result
{
    bool no_results;
    char *found_address;
    character_bool_map *next_chars_bool_map;
} autocomplete_result;

autocomplete_result no_match_autocomplete_result(character_bool_map **idx)
{
    autocomplete_result result;
    result.no_results = true;
    result.found_address = NULL;
    result.next_chars_bool_map = *idx;
    return result;
}

autocomplete_result full_match_autocomplete_result(character_bool_map **idx, char *address)
{
    autocomplete_result result;
    result.no_results = false;
    result.found_address = address;
    result.next_chars_bool_map = *idx;
    return result;
}

autocomplete_result partial_match_autocomplete_result(character_bool_map **idx)
{
    autocomplete_result result;
    result.no_results = false;
    result.found_address = NULL;
    result.next_chars_bool_map = *idx;
    return result;
}

autocomplete_result autocomplete(character_bool_map **idx, char *key, int addresses_amount, char *addresses[])
{
    int key_len = strlen(key);

    for (int i = 0; i < addresses_amount; i++)
    {
        char *address = addresses[i];
        address_match_result match_result = check_address(key, address);

        if (match_result == DoesNotMatch)
        {
            continue;
        }
        else if (match_result == PartlyMatch)
        {
            // printf("%c is a match\n", address[key_len]);
            char next_letter = address[key_len];
            allow_char(idx, to_uppercase(next_letter));
        }
        else if (match_result == FullMatch)
        {
            return full_match_autocomplete_result(idx, address);
        }
    }

    if ((**idx).amount_of_true == 0)
    {
        return no_match_autocomplete_result(idx);
    }

    return partial_match_autocomplete_result(idx);
}

bool logging_enabled()
{
    return true;
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

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        printf("At least one argument must be provided.\n");
        return 1;
    }

    character_bool_map chars_index = new_character_bool_map();
    character_bool_map *chars_index_ptr = &chars_index;
    autocomplete_result result;

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

        result = autocomplete(&chars_index_ptr, key, addresses_amount, addresses);
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

        result = autocomplete(&chars_index_ptr, key, addresses_amount, addresses);
    }

    if (result.no_results)
    {
        printf("Not found\n");
    }
    else if (result.found_address != NULL)
    {
        printf("Found: %s\n", result.found_address);
    }
    else
    {
        print_next_letters(*result.next_chars_bool_map);
    }

    return 0;
}
