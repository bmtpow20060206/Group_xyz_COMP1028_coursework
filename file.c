#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "file.h"
#include "error.h"
#include "tool.h"

// Check if file exists
int file_exists(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file) {
        fclose(file);
        return 1;
    }
    return 0;
}

// See if file is empty
int is_file_empty(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) return 1;
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fclose(file);
    
    return size == 0;
}

// Get file size in bytes
long get_file_size(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) 
    return -1;
        fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fclose(file);
    
    return size;
}

// Read a text file and return its content
char* read_text_file(const char* fname) {
    FILE* f = fopen(fname, "r");
    if (!f) {
        printf("Can't open %s\n", fname);  
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);  
    fseek(f, 0, SEEK_SET);

    if (sz <= 0) {
        printf("File %s is empty\n", fname); 
        fclose(f);
        return NULL;
    }

    // check if readable
    if (is_file_corrupted(fname)) {
        printf("File %s might be corrupted\n", fname);  
        fclose(f);
        return NULL;
    }    
    int enc = detect_file_encoding(fname);
    if (enc == 2) { 
        printf("UTF-16 not supported for %s\n", fname); 
        fclose(f);
        return NULL;
    }

     // Allocate memory for file content
    char* content = (char*)malloc(sz + 1);
    if (content == NULL) {
        handle_error("read_text_file", ERROR_MEMORY_ALLOCATION, fname);
        fclose(f);
        return NULL;
    }

    // Read the file
    size_t bytes_read = fread(content, 1, sz, f);
    content[bytes_read] = '\0';
    fclose(f);    
    printf("File %s loaded successfully (%ld bytes)\n", fname, bytes_read);
    return content;
}

// chunk by chunk read large files
char* read_large_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error: Cannot open file %s\n", filename);
        return NULL;
    }
    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (file_size <= 0) {
        printf("Error: File is empty\n");
        fclose(file);
        return NULL;
    }

    // Warn if file is huge (over 100MB)
    if (file_size > 100 * 1024 * 1024) {
        printf("Warning: File is very large (%ld MB). Processing may take time.\n", file_size / (1024 * 1024));
    }

    // Allocate memory
    char* content = (char*)malloc(file_size + 1);
    if (content == NULL) {
        printf("Error: Memory allocation failed for large file\n");
        fclose(file);
        return NULL;
    }

    // read file in chunk
    size_t total_read = 0;
    size_t chunk_size = 8192; // 8KB 

    while (total_read < file_size) {
        size_t to_read = (file_size - total_read < chunk_size) ? 
                        (file_size - total_read) : chunk_size;
        
        size_t bytes_read = fread(content + total_read, 1, to_read, file);
        if (bytes_read == 0) {
            break; // error or EOF
        }
        total_read += bytes_read;
        
        // show process for big file
        if (file_size > 10 * 1024 * 1024) {
            int progress = (int)((total_read * 100) / file_size);
            printf("\rReading file: %d%% complete", progress);
            fflush(stdout);
        }
    }

    if (file_size > 10 * 1024 * 1024) {
        printf("\n"); // show done process
    }
    content[total_read] = '\0';
    fclose(file);
    printf("File %s loaded successfully (%ld bytes)\n", filename, total_read);
    return content;
}

// Extract specific column from CSV and convert to text
char* csv_column_to_text(const char* filename, int column_index) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error: Cannot open CSV file %s\n", filename);
        return NULL;
    }

    char line[2048];
    char* result_text = malloc(1);
    if (result_text == NULL) {
        printf("Error: Memory allocation failed\n");
        fclose(file);
        return NULL;
    }
    result_text[0] = '\0';
    
    int line_count = 0;
    int valid_columns = 0;

    printf("Processing CSV file column %d...\n", column_index);
    
    while (fgets(line, sizeof(line), file)) {
        line_count++;
        
        // Remove line endings
        line[strcspn(line, "\n")] = '\0';
        line[strcspn(line, "\r")] = '\0';
        
        // Skip empty lines
        if (strlen(line) == 0) {
            continue;
        }
        
        char* token;
        char* rest = line;
        int current_column = 0;
        char* column_content = NULL;
        
        // Split by comma and find target column
        while ((token = strtok(rest, ",")) != NULL) {
            if (current_column == column_index) {
                column_content = token;
                break;
            }
            current_column++;
            rest = NULL;
        }
        
        if (column_content != NULL && strlen(column_content) > 0) {
            // Remove surrounding quotes
            char cleaned_content[1024];
            strcpy(cleaned_content, column_content);
            
            if (cleaned_content[0] == '"' && cleaned_content[strlen(cleaned_content)-1] == '"') {
                memmove(cleaned_content, cleaned_content + 1, strlen(cleaned_content) - 2);
                cleaned_content[strlen(cleaned_content) - 2] = '\0';
            }
            
            // Use safe string concatenation
            safe_strcat(&result_text, cleaned_content);
            safe_strcat(&result_text, " ");
            valid_columns++;
        }
    }
    
    fclose(file);
    
    if (valid_columns == 0) {
        printf("Warning: No valid data found in column %d\n", column_index);
        free(result_text);
        return NULL;
    }
    
    printf("CSV column %d processed successfully: %d lines, %d valid entries\n", 
           column_index, line_count, valid_columns);
    return result_text;
}

// all text columns
char* csv_all_columns_to_text(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error: Cannot open CSV file %s\n", filename);
        return NULL;
    }

    char line[2048];
    char* result_text = malloc(1);
    if (result_text == NULL) {
        printf("Error: Memory allocation failed\n");
        fclose(file);
        return NULL;
    }
    result_text[0] = '\0';
    
    int line_count = 0;
    int total_columns = 0;

    printf("Processing entire CSV file (all columns)...\n");
    
    while (fgets(line, sizeof(line), file)) {
        line_count++;
        
        // Remove line endings
        line[strcspn(line, "\n")] = '\0';
        line[strcspn(line, "\r")] = '\0';
        
        // Skip empty lines
        if (strlen(line) == 0) {
            continue;
        }
        
        char* token;
        char* rest = line;
        int columns_in_line = 0;
        
        // Process all columns
        while ((token = strtok(rest, ",")) != NULL) {
            char cleaned_content[1024];
            strcpy(cleaned_content, token);
            
            // Remove surrounding quotes
            if (cleaned_content[0] == '"' && cleaned_content[strlen(cleaned_content)-1] == '"') {
                memmove(cleaned_content, cleaned_content + 1, strlen(cleaned_content) - 2);
                cleaned_content[strlen(cleaned_content) - 2] = '\0';
            }
            
            // Only add non-empty content
            if (strlen(cleaned_content) > 0) {
                safe_strcat(&result_text, cleaned_content);
                safe_strcat(&result_text, " ");
                total_columns++;
                columns_in_line++;
            }
            rest = NULL;
        }
        
        // Add newline after each row (maintain paragraph structure)
        safe_strcat(&result_text, "\n");
    }
    
    fclose(file);
    
    printf("CSV file processed successfully: %d lines, %d total text entries\n", 
           line_count, total_columns);
    return result_text;
}

// Preview CSV file structure
void preview_csv_columns(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error: Cannot open CSV file %s\n", filename);
        return;
    }

    char line[2048];
    printf("\n=== CSV File Preview ===\n");
    
        for (int i = 0; i < 3 && fgets(line, sizeof(line), file); i++) {
        line[strcspn(line, "\n")] = '\0';
        line[strcspn(line, "\r")] = '\0';
        
        if (i == 0) {
            printf("Column Headers: %s\n", line);
            printf("Column Structure:\n");
            
            char* token;
            char* rest = line;
            int col_index = 0;
            
            while ((token = strtok(rest, ",")) != NULL) {
                printf("  Column %d: %s\n", col_index, token);
                col_index++;
                rest = NULL;
            }
            printf("\nData Preview:\n");
        } else {
            printf("Row %d: %s\n", i, line);
        }
    }
    
    fclose(file);
    printf("===================\n");
}

// Extract CSV column and save to TXT file
int extract_csv_column_to_file(const char* csv_filename, int column_index, const char* output_txt_filename) {
    FILE* csv_file = fopen(csv_filename, "r");
    FILE* txt_file = fopen(output_txt_filename, "w");
    
    if (csv_file == NULL) {
        printf("Error: Cannot open CSV file %s\n", csv_filename);
        return 0;
    }
    if (txt_file == NULL) {
        printf("Error: Cannot create output file %s\n", output_txt_filename);
        fclose(csv_file);
        return 0;
    }

    char line[2048];
    int line_count = 0;
    int saved_entries = 0;

    printf("Extracting column %d from %s to %s...\n", column_index, csv_filename, output_txt_filename);
    
    while (fgets(line, sizeof(line), csv_file)) {
        line_count++;
        
        // Remove line endings
        line[strcspn(line, "\n")] = '\0';
        line[strcspn(line, "\r")] = '\0';
        
        // Skip empty lines
        if (strlen(line) == 0) {
            continue;
        }
        
        char* token;
        char* rest = line;
        int current_column = 0;
        char* column_content = NULL;
        
        // Split by comma and find target column
        while ((token = strtok(rest, ",")) != NULL) {
            if (current_column == column_index) {
                column_content = token;
                break;
            }
            current_column++;
            rest = NULL;
        }
        
        if (column_content != NULL && strlen(column_content) > 0) {
            // Remove surrounding quotes
            char cleaned_content[1024];
            strcpy(cleaned_content, column_content);
            
            if (cleaned_content[0] == '"' && cleaned_content[strlen(cleaned_content)-1] == '"') {
                memmove(cleaned_content, cleaned_content + 1, strlen(cleaned_content) - 2);
                cleaned_content[strlen(cleaned_content) - 2] = '\0';
            }
            
            // Write to TXT file
            fprintf(txt_file, "%s\n", cleaned_content);
            saved_entries++;
        }
    }
    
    fclose(csv_file);
    fclose(txt_file);
    
    printf("Extraction completed: %d lines processed, %d entries saved\n", line_count, saved_entries);
    return 1;
}

// all to save
int extract_csv_all_to_file(const char* csv_filename, const char* output_txt_filename) {
    FILE* csv_file = fopen(csv_filename, "r");
    FILE* txt_file = fopen(output_txt_filename, "w");
    
    if (csv_file == NULL) {
        printf("Error: Cannot open CSV file %s\n", csv_filename);
        return 0;
    }
    if (txt_file == NULL) {
        printf("Error: Cannot create output file %s\n", output_txt_filename);
        fclose(csv_file);
        return 0;
    }

    char line[2048];
    int line_count = 0;
    int total_entries = 0;

    printf("Extracting text from %s to %s...\n", csv_filename, output_txt_filename);
    
    while (fgets(line, sizeof(line), csv_file)) {
        line_count++;
        
        // Remove line endings
        line[strcspn(line, "\n")] = '\0';
        line[strcspn(line, "\r")] = '\0';
        
        // Skip empty lines
        if (strlen(line) == 0) {
            continue;
        }
        
        if (strchr(line, ',') == NULL) {
            fprintf(txt_file, "%s\n", line);
            total_entries++;
        } else {
            // Multiple columns, extract all content
            char* token;
            char* rest = line;
            int columns_in_line = 0;
            char row_text[2048] = "";
            
            // Process all columns
            while ((token = strtok(rest, ",")) != NULL) {
                char cleaned_content[1024];
                strcpy(cleaned_content, token);
                
                // Remove surrounding quotes
                if (cleaned_content[0] == '"' && cleaned_content[strlen(cleaned_content)-1] == '"') {
                    memmove(cleaned_content, cleaned_content + 1, strlen(cleaned_content) - 2);
                    cleaned_content[strlen(cleaned_content) - 2] = '\0';
                }
                
                // Remove extra spaces
                char trimmed_content[1024];
                int start = 0, end = strlen(cleaned_content) - 1;
                while (cleaned_content[start] == ' ') start++;
                while (end > start && cleaned_content[end] == ' ') end--;
                strncpy(trimmed_content, cleaned_content + start, end - start + 1);
                trimmed_content[end - start + 1] = '\0';
                
                // Only add non-empty content
                if (strlen(trimmed_content) > 0) {
                    if (columns_in_line > 0) {
                        strcat(row_text, " ");
                    }
                    strcat(row_text, trimmed_content);
                    columns_in_line++;
                }
                rest = NULL;
            }
            
            // Write the combined row text to file
            if (strlen(row_text) > 0) {
                fprintf(txt_file, "%s\n", row_text);
                total_entries++;
            }
        }
    }
    
    fclose(csv_file);
    fclose(txt_file);
    
    printf("Extraction completed: %d lines processed, %d text entries saved\n", line_count, total_entries);
    return 1;
}

// Process multiple files
char* process_multiple_files(const char** filenames, int file_count) {
    if (filenames == NULL || file_count <= 0) {
        printf("Error: No files specified\n");
        return NULL;
    }

    char* combined_text = malloc(1);
    if (combined_text == NULL) {
        printf("Error: Memory allocation failed\n");
        return NULL;
    }
    combined_text[0] = '\0';

    int successful_files = 0;
    long total_size = 0;

    printf("Processing %d files...\n", file_count);

    for (int i = 0; i < file_count; i++) {
        const char* filename = filenames[i];
        
        printf("\nFile %d/%d: %s\n", i + 1, file_count, filename);
        
       // Check if file exists
        if (!file_exists(filename)) {
            printf("  Warning: File does not exist, skipping\n");
            continue;
        }

        // Check if file is empty
        if (is_file_empty(filename)) {
            printf("  Warning: File is empty, skipping\n");
            continue;
        }

        // Get file size
        long size = get_file_size(filename);
        if (size > 0) {
            printf("  Size: %ld bytes\n", size);
            total_size += size;
        }

        // Handle based on file type
        const char* extension = get_file_extension(filename);
        char* file_content = NULL;

        if (strcmp(extension, ".csv") == 0) {
            // Handle CSV file
            file_content = csv_all_columns_to_text(filename);
        } else if (strcmp(extension, ".txt") == 0) {
            // Handle text file
            if (size > 5 * 1024 * 1024) { // Use large file handler for >5MB
                file_content = read_large_file(filename);
            } else {
                file_content = read_text_file(filename);
            }
        } else {
            printf("  Warning: Unsupported file type '%s', skipping\n", extension);
            continue;
        }

        if (file_content != NULL) {
            
            safe_strcat(&combined_text, "\n--- File: ");
            safe_strcat(&combined_text, filename);
            safe_strcat(&combined_text, " ---\n");
            
            // Add file content
            safe_strcat(&combined_text, file_content);
            
            free(file_content);
            successful_files++;
            printf("   Processed successfully\n");
        } else {
            printf("   Failed to process file\n");
        }
    }

    if (successful_files == 0) {
        printf("\nError: No files were successfully processed\n");
        free(combined_text);
        return NULL;
    }

    printf("\nProcessing completed: %d/%d files successful, total size: %ld bytes\n", 
           successful_files, file_count, total_size);
    
    return combined_text;
}