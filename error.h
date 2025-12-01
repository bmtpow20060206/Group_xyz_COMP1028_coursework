#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

typedef enum {
    ERROR_NONE = 0,
    ERROR_FILE_NOT_FOUND,
    ERROR_FILE_EMPTY,
    ERROR_FILE_CORRUPTED,
    ERROR_FILE_ENCODING,
    ERROR_FILE_PERMISSION,
    ERROR_FILE_TOO_LARGE,
    ERROR_MEMORY_ALLOCATION,
    ERROR_CSV_FORMAT,
    ERROR_NETWORK,
    ERROR_UNKNOWN
} ErrorCode;

// Handle errors when they happen
void handle_error(const char* context, ErrorCode error_code, const char* details);
void show_error_recovery_options(const char* filename, ErrorCode error_code);
int try_alternative_processing(const char* filename, ErrorCode error_code);

// Check if files are messed up
int is_file_corrupted(const char* filename);
int detect_file_encoding(const char* filename);

// Better file checking stuff
int check_file_exists(const char* filename);
void handle_file_not_found_error(const char* filename);

// File recovery functions
int try_alternative_processing(const char* filename, ErrorCode error_code);
int extract_readable_text(const char* input_file, const char* output_file);
int fix_csv_format(const char* input_file, const char* output_file);
int try_different_encodings(const char* input_file, const char* output_file);

#endif