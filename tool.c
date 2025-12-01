#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "tool.h"

#define MAX_WORD_LEN 50

// Initialize console encoding
void init_console_encoding(void) {
    #ifdef _WIN32
        system("chcp 65001 > nul");
    #endif
}

// Convert string to lowercase
void to_lower_case(char* str) {
    for (int i = 0; str[i]; i++) {
        str[i] = tolower((unsigned char)str[i]);
    }
}

// Normalize word by removing non-alphanumeric characters and converting to lowercase
void normalize_word(const char* input, char* output) {
    int j = 0;
    for (int i = 0; input[i] && j < MAX_WORD_LEN - 1; i++) {
        if (isalnum((unsigned char)input[i])) {
            output[j++] = tolower((unsigned char)input[i]);
        }
    }
    output[j] = '\0';
}

// Safe string concatenation with dynamic memory allocation
void safe_strcat(char** dest, const char* src) {
    if (src == NULL) return;
    
    size_t dest_len = (*dest == NULL) ? 0 : strlen(*dest);
    size_t src_len = strlen(src);
    
    char* new_dest = realloc(*dest, dest_len + src_len + 1);
    if (new_dest == NULL) {
        printf(" Error: Memory reallocation failed in safe_strcat\n");
        return;
    }
    
    *dest = new_dest;
    strcpy(*dest + dest_len, src);
}

// Get file extension
const char* get_file_extension(const char* filename) {
    const char* dot = strrchr(filename, '.');
    return (dot == NULL) ? "" : dot;
}

// Split string by delimiter
char** split_string(const char* str, const char* delimiter, int* count) {
    if (str == NULL || delimiter == NULL) {
        *count = 0;
        return NULL;
    }
    
    // Count number of tokens
    char* str_copy = strdup(str);
    if (str_copy == NULL) return NULL;
    
    int token_count = 0;
    char* token = strtok(str_copy, delimiter);
    while (token != NULL) {
        token_count++;
        token = strtok(NULL, delimiter);
    }
    free(str_copy);
    
    if (token_count == 0) {
        *count = 0;
        return NULL;
    }
    
    // Allocate memory for split results
    char** result = malloc(token_count * sizeof(char*));
    if (result == NULL) return NULL;
    
    str_copy = strdup(str);
    token_count = 0;
    token = strtok(str_copy, delimiter);
    while (token != NULL) {
        result[token_count] = strdup(token);
        token_count++;
        token = strtok(NULL, delimiter);
    }
    free(str_copy);
    
    *count = token_count;
    return result;
}

// Free split string array
void free_split_string(char** array, int count) {
    if (array == NULL) return;
    
    for (int i = 0; i < count; i++) {
        free(array[i]);
    }
    free(array);
}