/*
 * pw-hash.c
 *
 *  Created on: 1 nov 2023
 */

#include "common.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#define A 1103515245 // The multiplier for the LCG
#define C 12345 // The increment for the LCG
#define M 2147483648 // The modulus for the LCG

int string_to_number(char* s)
{
    // A function that converts a string into a numerical value
    int num = 0; // The resulting number
    int len = strlen(s); // The length of the string
    int i; // A loop variable

    // Loop through each character in the string
    for (i = 0; i < len; i++)
    {
        // Shift the previous digits left by one byte
        num = num << 8;

        // Add the ASCII code of the current character
        num = num + s[i];
    }

    return num;
}

int hash_pw(char* s)
{
    // A function that hashes a string using an LCG
    int seed = string_to_number(s); // The seed for the LCG
    int hash; // The hash output

    // Generate one value from the LCG using the formula
    hash = (A * seed + C) % M;

    return hash;
}

bool check_pw(char* s, int h)
{
    // Returns true if password is correct
    int hash; // The hash of the input string

    // Hash the input string using the same LCG function
    hash = hash_pw(s);

    // Compare the hashes using bitwise XOR
    // If they are equal, the result will be zero
    return (hash ^ h) == 0;
}
