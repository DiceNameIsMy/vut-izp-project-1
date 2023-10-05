#include <stdio.h>
#include <string.h>
#include <stdbool.h>

typedef enum address_matches
{
    Yes,
    No,
    Partly
} address_matches;

typedef struct character_index
{
    int index[16];
    int amount_of_true;
} character_index;

character_index new_character_index()
{
    character_index chars_idx;
    chars_idx.amount_of_true = 0;

    for (int i = 0; i < 16; i++)
    {
        chars_idx.index[i] = 0;
    }

    return chars_idx;
}

character_index allow_char(character_index idx, char c)
{
    int array_idx = c / 16;
    int shift = c % 16;

    int pos = 0b0000000000000001 << shift;

    bool already_set = (idx.index[array_idx] & pos) == pos;
    if (!already_set)
    {
        idx.index[array_idx] |= pos;
        idx.amount_of_true++;
    }

    return idx;
}

void print_chars(character_index idx)
{
    int printed_chars = 0;

    printf("ENABLE: ");
    for (int i = 0; i < 16; i++)
    {
        // printf("Byte: %i\n", i);
        for (int j = 0; j < 16; j++)
        {
            if (printed_chars == idx.amount_of_true)
            {
                printf("\n");
                return;
            }
            int n = 0b0000000000000001 << j;
            // printf("Bit: %i, Value: %i\n", n, idx.index[i]);
            bool should_print = (idx.index[i] & n) == n;

            if (should_print)
            {
                // printf("Char to print: %i\n", ((i * 16) + j));
                printf("%c", (char)((i * 16) + j));
                printed_chars++;
            }
        }
    }
    printf("\n");
}

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
        return c & 223;
    }
    return c;
}

bool is_address_matching(char *key, char *address)
{
    int key_len = strlen(key);

    for (int i = 0; i < key_len; i++)
    {
        if (to_uppercase(key[i]) != to_uppercase(address[i]))
        {
            return false;
        }
    }
    return true;
}

character_index poppulate_next_chars(character_index idx, char *key, int addresses_amount, char *addresses[])
{
    int key_len = strlen(key);

    for (int i = 0; i < addresses_amount; i++)
    {
        char *address = addresses[i];
        bool is_match = is_address_matching(key, address);

        if (is_match)
        {
            // printf("%c is a match\n", address[key_len]);
            char next_letter = to_uppercase(address[key_len]);

            idx = allow_char(idx, next_letter);
        }
    }
    return idx;
}

void print_parsed_input(char key[], char *addresses[], int addresses_amount)
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

    character_index chars_index = new_character_index();

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

        print_parsed_input(key, addresses, addresses_amount);

        chars_index = poppulate_next_chars(chars_index, key, addresses_amount, addresses);
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

        print_parsed_input(key, addresses, addresses_amount);

        chars_index = poppulate_next_chars(chars_index, key, addresses_amount, addresses);
    }

    print_chars(chars_index);

    return 0;
}
