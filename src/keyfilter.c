#define NDEBUG

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define ITEM_SIZE 100
#define ITEM_ARRAY_SIZE ITEM_SIZE + 1
#define ITEM_SEPARATOR '\n'
#define BOOL_MAP_NODES 32
#define BOOL_MAP_NODE_SIZE (int)(8 * sizeof(char))

#define IsLowercase(c) (c > 96 && c < 123)

#ifndef NDEBUG
#define loginfo(s, ...) fprintf(stderr, __FILE__ ":%u: " s "\n", __LINE__, __VA_ARGS__)
#else
#define loginfo(s, ...)
#endif

bool is_empty(char *string)
{
    return strlen(string) == 0;
}

bool is_printable(char c)
{
    return (int)c > 32 || (int)c < 127;
}

bool logging_enabled()
{
    char *logging = getenv("KEYFILTER_LOGGING");
    return logging != NULL && strcmp(logging, "1") == 0;
}

// Stores which characters can be unputted next.
// Each bit in the array of nodes represents a character
// with values:
// 0 - can't be inputted next
// 1 - can be inputted next
//
// I do acknowledge that this structure can store 256 chars,
// even though only 127 exists and only ~96 of them are printable,
// but since it does not increase the algorithminc complexity of the solution
// (it's a constant memory usage), I decided to keep it simple.
typedef struct
{
    char index[BOOL_MAP_NODES];
    int chars_counter;
    int matched_items_counter;
} char_bool_map;

char_bool_map new_char_bool_map()
{
    char_bool_map chars_idx;
    chars_idx.chars_counter = 0;
    chars_idx.matched_items_counter = 0;

    for (int i = 0; i < BOOL_MAP_NODES; i++)
    {
        chars_idx.index[i] = 0;
    }

    return chars_idx;
}

bool allow_char(char_bool_map *idx, char c)
{
    if (!is_printable(c))
    {
        loginfo("Received an invalid char `%c(%i)`", c, c);
        return false;
    }

    loginfo("Attemting to allow char %c(%i)", c, c);
    int node_idx = c / BOOL_MAP_NODE_SIZE;
    int char_pos = 1 << (c % BOOL_MAP_NODE_SIZE);

    bool is_char_allowed = (idx->index[node_idx] & char_pos) == char_pos;
    if (!is_char_allowed)
    {
        loginfo("Setting char %c(%i) as allowed", c, c);
        idx->index[node_idx] |= char_pos;
        idx->chars_counter++;
    }
    else
    {
        loginfo("Char `%c` is already allowed", c);
    }
    idx->matched_items_counter++;

    return true;
}

char get_char_from_bool_map(int node_idx, int item_idx)
{
    return (node_idx * BOOL_MAP_NODE_SIZE) + item_idx;
}

void print_found_item(char item[ITEM_ARRAY_SIZE])
{
    char upper_item[100];
    strcpy(upper_item, item);

    for (int i = 0; i < (int)strlen(upper_item); i++)
    {
        upper_item[i] = toupper(upper_item[i]);
    }

    printf("Found: %s\n", upper_item);
}

void print_node_chars(char_bool_map *idx, int node_idx, int *print_counter)
{
    for (int shift = 0; shift < BOOL_MAP_NODE_SIZE; shift++)
    {
        int char_pos = 0b00000001 << shift;
        bool should_print = (idx->index[node_idx] & char_pos) == char_pos;

        if (should_print)
        {
            int char_modulo = shift;
            char c = get_char_from_bool_map(node_idx, char_modulo);
            printf("%c", toupper(c));
            (*print_counter)++;
        }
    }
}

void print_chars(char_bool_map *idx)
{
    int printed_chars = 0;

    printf("Enable: ");
    for (int node_idx = 0; node_idx < BOOL_MAP_NODES; node_idx++)
    {
        print_node_chars(idx, node_idx, &printed_chars);

        if (printed_chars == idx->chars_counter)
        {
            printf("\n");
            return;
        }
    }
}

bool has_partial_matches(char_bool_map *idx)
{
    return idx->matched_items_counter != 0;
}

bool has_multiple_matched_items(char_bool_map *idx)
{
    return idx->matched_items_counter > 1;
}

bool has_single_partial_match(char_bool_map *idx)
{
    return idx->matched_items_counter == 1;
}

typedef enum compare_result
{
    FullMatch,
    NoMatch,
    PartialMatch
} compare_result;

compare_result compare_to_key(char *key, char *value)
{
    loginfo("Comparing key `%s` to item `%s` with first char `%i`", key, value, value[0]);
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
    read_item_result result = {.item_too_long = false, .read_all_items = false};

    while ((c = fgetc(stream)) != ITEM_SEPARATOR)
    {
        if (c == EOF)
        {
            result.read_all_items = true;
            break;
        }
        else if (next_char_idx == (ITEM_ARRAY_SIZE - 1))
        {
            result.item_too_long = true;
            break;
        }

        item[next_char_idx] = c;
        next_char_idx++;
    }
    item[next_char_idx] = '\0';

    return result;
}

// Represents a result of using a keyfilter.
typedef struct keyfilter_result
{
    char invalid_item[ITEM_ARRAY_SIZE];
    char found_item[ITEM_ARRAY_SIZE];
    char_bool_map *next_chars_bool_map;
} keyfilter_result;

keyfilter_result invalid_item_keyfilter_result(char_bool_map *idx, char invalid_item[ITEM_ARRAY_SIZE])
{
    keyfilter_result result = {"", "", idx};
    strcpy(result.invalid_item, invalid_item);
    return result;
}

keyfilter_result keyfilter(char_bool_map *idx, char *key, FILE *stream)
{
    char latest_partial_match_item[ITEM_ARRAY_SIZE] = "";
    char found_item[ITEM_ARRAY_SIZE] = "";
    char current_item[ITEM_ARRAY_SIZE];

    while (true)
    {
        read_item_result item_result = read_item(&current_item[0], stream);

        if (item_result.item_too_long)
            return invalid_item_keyfilter_result(idx, current_item);
        if (item_result.read_all_items)
            break;

        compare_result match_result = compare_to_key(key, current_item);
        if (match_result == NoMatch)
        {
            continue;
        }
        else if (match_result == FullMatch)
        {
            strcpy(found_item, current_item);
        }
        else if (match_result == PartialMatch)
        {
            char next_char = current_item[strlen(key)];
            bool char_is_valid = allow_char(idx, toupper(next_char));
            if (!char_is_valid)
            {
                return invalid_item_keyfilter_result(idx, current_item);
            }
            strcpy(latest_partial_match_item, current_item);
        }
    }

    loginfo("latest_partial_match_item: `%s`", latest_partial_match_item);
    loginfo("found_item: `%s`", found_item);
    loginfo("chars_counter: `%i`", idx->chars_counter);
    loginfo("matched_items_counter: `%i`", idx->matched_items_counter);

    keyfilter_result result = {.invalid_item = "", .found_item = "", .next_chars_bool_map = idx};

    if (!is_empty(found_item))
    {
        strcpy(result.found_item, found_item);
    }
    else if (has_single_partial_match(idx))
    {
        strcpy(result.found_item, latest_partial_match_item);
    }

    return result;
}

// I have decided to keep the result resolvement separate from
// printing it to keep functions with a single responsibility.
void print_keyfilter_result(keyfilter_result *result)
{
    bool full_match = !is_empty(result->found_item);
    bool partial_matches = has_partial_matches(result->next_chars_bool_map);

    if (full_match)
    {
        print_found_item(result->found_item);

        if (result->next_chars_bool_map->matched_items_counter > 1)
        {
            print_chars(result->next_chars_bool_map);
        }
    }
    else if (partial_matches)
    {
        print_chars(result->next_chars_bool_map);
    }
    else
    {
        printf("Not found\n");
    }
}

int main(int argc, char *argv[])
{
    if (argc > 2)
    {
        fprintf(stderr, "No more than 1 argument must be provided. It must be a key to make a search for.\n");
        return 1;
    }

    char key[ITEM_ARRAY_SIZE] = "";
    if (argc == 2)
    {
        if (strlen(argv[1]) > ITEM_SIZE)
        {
            fprintf(stderr, "Key `%s` is too long\n", argv[1]);
            return 1;
        }

        strcpy(key, argv[1]);
    }

    char_bool_map chars_map = new_char_bool_map();
    keyfilter_result result = keyfilter(&chars_map, key, stdin);

    if (!is_empty(result.invalid_item))
    {
        fprintf(stderr, "Received an invalid item `%s`.\n", result.invalid_item);
        return 1;
    }

    print_keyfilter_result(&result);

    return 0;
}
