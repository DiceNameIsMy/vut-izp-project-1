#include <stdio.h>
#include <string.h>
#include <stdbool.h>

bool is_lowercase_letter(char c)
{
    return (c >= 97 && c <= 122);
}

bool is_address_matching(char *key, char *address)
{
    int key_len = strlen(key);

    for (int i = 0; i < key_len; i++)
    {
        if ((key[i] & ~32) != (address[i] & ~32))
        {
            return false;
        }
    }
    return true;
}

void poppulate_next_chars(char *key, int addresses_amount, char *addresses[], bool chars[26])
{
    int key_len = strlen(key);

    for (int i = 0; i < addresses_amount; i++)
    {
        char *address = addresses[i];
        char uppercase_letter = address[key_len] & ~32;

        chars[uppercase_letter - 65] = is_address_matching(key, address);
    }
    return;
}

void print_chars(bool chars_index[26])
{
    printf("Chars: ");

    for (int i = 0; i < 26; i++)
    {
        if (chars_index[i])
        {
            printf("%c", (char)(i + 65));
        }
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

    char first_letter = argv[1][0];

    char *key;
    bool chars_index[26];

    if (is_lowercase_letter(first_letter))
    {
        int addresses_amount = argc - 2;
        char *addresses[addresses_amount];
        int l;
        for (l = 0; l < addresses_amount; l++)
        {
            addresses[l] = argv[l + 2];
        }

        key = argv[1];

        poppulate_next_chars(key, addresses_amount, addresses, chars_index);

        print_chars(chars_index);
    }
    else
    {
        int addresses_amount = argc - 1;
        char *addresses[addresses_amount];
        int l;
        for (l = 0; l < addresses_amount; l++)
        {
            addresses[l] = argv[l + 1];
        }

        key = "";

        poppulate_next_chars(key, addresses_amount, addresses, chars_index);

        print_chars(chars_index);
    }

    return 0;
}
