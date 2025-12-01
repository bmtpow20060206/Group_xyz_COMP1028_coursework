#ifndef UTILS_H
#define UTILS_H

#define MAX_WORD_LEN 50


void init_console_encoding(void);
void to_lower_case(char* str);


void normalize_word(const char* input, char* output);


void safe_strcat(char** dest, const char* src);


const char* get_file_extension(const char* filename);
char** split_string(const char* str, const char* delimiter, int* count);
void free_split_string(char** array, int count);

#endif