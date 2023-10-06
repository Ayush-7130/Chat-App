#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_MESSAGE_SIZE 1024
#define N 256 // 2^8

int messageSize = sizeof(char) * MAX_MESSAGE_SIZE;
int S[N];

void string_to_bits(char *str, int *bits)
{
    int bit_index = 0;
    int len = strlen(str);

    for (int i = 0; i < len; i++)
    {
        char c = str[i];
        for (int j = 7; j >= 0; j--)
        {
            bits[bit_index++] = ((c >> j) & 1);
        }
    }
}

void bits_to_string(int *bits, char *str, int len)
{
    int bit_index = 0;
    int str_index = 0;

    while (bit_index < len)
    {
        char c = 0;
        for (int j = 7; j >= 0; j--)
        {
            c = (c << 1) | (bits[bit_index++]);
        }
        str[str_index++] = c;
    }
    str[str_index] = '\0';
}

void string_to_int_array(char *str, int *array, int length)
{
    for (int i = 0; i < length; i++)
    {
        array[i] = str[i] - '0';
    }
}

void int_array_to_string(int *array, int length, char *str)
{
    for (int i = 0; i < length; i++)
    {
        str[i] = array[i] + '0';
    }
    str[length] = '\0';
}

void swap(int *a, int *b)
{
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

void init_state()
{
    for (int i = 0; i < N; i++)
    {
        S[i] = i;
    }
}

void KSA(int *key, int len)
{
    int j = 0;

    for (int i = 0; i < N; i++)
    {
        j = (j + S[i] + key[i % len]) % N;
        swap(&S[i], &S[j]);
    }
}

void PRGA(int len)
{
    int i = 0;
    int j = 0;

    for (int n = 0; n < len; n++)
    {
        i = (i + 1) % N;
        j = (j + S[i]) % N;

        swap(&S[i], &S[j]);
        int rnd = S[(S[i] + S[j]) % N];

        S[n] = rnd;
    }
}

void do_xor(int *ciphertext, int *decrpt, int len)
{
    for (int n = 0; n < len; n++)
    {
        decrpt[n] = S[n] ^ ciphertext[n];
    }
}

void RC4(int *key, int *plaintext, int *ciphertext, int len)
{
    init_state();
    KSA(key, len);

    PRGA(len);
    do_xor(plaintext, ciphertext, len);
}

char *encryption(char *plaintext, char *keytext)
{
    int len = strlen(plaintext);
    int klen = strlen(keytext);

    int *plain_bits = (int *)malloc(messageSize);
    string_to_bits(plaintext, plain_bits);

    int *key_bits = (int *)malloc(messageSize);
    string_to_bits(keytext, key_bits);

    int *cipher_bits = (int *)malloc(messageSize);
    RC4(key_bits, plain_bits, cipher_bits, 8 * len);

    char *ciphertext = (char *)malloc(messageSize);
    int_array_to_string(cipher_bits, 8 * len, ciphertext);

    return ciphertext;
}

char *decryption(char *ciphertext, char *keytext)
{
    int len = strlen(ciphertext);
    int klen = strlen(keytext);

    int *cipher_bits = (int *)malloc(messageSize);
    string_to_int_array(ciphertext, cipher_bits, len);

    int *key_bits = (int *)malloc(messageSize);
    string_to_bits(keytext, key_bits);

    int *plain_bits = (int *)malloc(messageSize);
    RC4(key_bits, cipher_bits, plain_bits, len);

    char *plaintext = (char *)malloc(messageSize);
    bits_to_string(plain_bits, plaintext, len);

    return plaintext;
}