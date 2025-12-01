#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "error.h"


int extract_readable_text(const char* ifile, const char* output_file) {
    printf(" Extracting readable text from %s to %s\n", ifile, output_file);
    
    FILE* in = fopen(ifile, "rb");
    if (!in) {
        printf(" Cannot open input file: %s\n", ifile);
        return 0;
    }
    
    FILE* out = fopen(output_file, "w");
    if (!out) {
        printf(" Cannot create output file: %s\n", output_file);
        fclose(in);
        return 0;
    }
    
    int recovered_chars = 0;
    int ch;
    
    // Go through the file and grab printable stuff only
    while ((ch = fgetc(in)) != EOF) {
        if (ch == '\n' || ch == '\t' || ch == '\r') {
            fputc(ch, out);
            recovered_chars++;
        }
         // Keep regular ASCII characters (32-126)
        else if (ch >= 32 && ch <= 126) {
             fputc(ch, out);
              recovered_chars++;
        }
        // Save common punctuation
        else if (ch == ' ' || ch == '.' || ch == ',' || ch == '!' || ch == '?') {
            fputc(ch, out);
            recovered_chars++;
        }
    }
    
    fclose(in);
    fclose(out);
    
    printf(" Recovered %d readable characters to %s\n", recovered_chars, output_file);
    return recovered_chars > 0;
}

// fix messed up CSV 
int fix_csv_format(const char* input_file, const char* output_file) {
    printf(" Fixing CSV format for %s\n", input_file);
    
    FILE* in = fopen(input_file, "r");
    if (!in) {
        printf(" Cannot open CSV file: %s\n", input_file);
        return 0;
    }
    
    FILE* out = fopen(output_file, "w");
    if (!out) {
        printf(" Cannot create output file: %s\n", output_file);
        fclose(in);
        return 0;
    }
    
    char line[1024];
    int line_count = 0;
    int recoveredlines = 0;
    
    while (fgets(line, sizeof(line), in)) {
        line_count++;
        line[strcspn(line, "\r\n")] = '\0';
        
        //CSV repair
        int quote_count = 0;
        int comma_count = 0;
        char fixed_line[1024] = "";
        int fixed_pos = 0;
        
        for (int i = 0; line[i] != '\0' && fixed_pos < sizeof(fixed_line) - 1; i++) {
            if (line[i] == '"') {
                quote_count++;
                fixed_line[fixed_pos++] = line[i];
            }
            else if (line[i] == ',') {
                comma_count++;
                fixed_line[fixed_pos++] = line[i];
            }
            
            else if (isalnum((unsigned char)line[i]) || 
                     line[i] == ' ' || line[i] == '.' || line[i] == '!' || line[i] == '?') {
                fixed_line[fixed_pos++] = line[i];
            }
            
            else {
                fixed_line[fixed_pos++] = ' ';
            }
        }
        fixed_line[fixed_pos] = '\0';
        
          // Only save lines that have real content
        if (strlen(fixed_line) > 10) { // Need at least 10 chars to be useful
            fprintf(out, "%s\n", fixed_line);
            recoveredlines++;
        }
    }
    
    fclose(in);
    fclose(out);
    
    printf(" Recovered %d/%d lines to %s\n", recoveredlines, line_count, output_file);
    return recoveredlines > 0;
}

int try_different_encodings(const char* input_file, const char* output_file) {
    printf(" Trying different encodings for %s\n", input_file);
    
    FILE* in = fopen(input_file, "rb");
    if (!in) {
        printf(" Cannot open file: %s\n", input_file);
        return 0;
    }
    
    FILE* out = fopen(output_file, "w");
    if (!out) {
        printf(" Cannot create output file: %s\n", output_file);
        fclose(in);
        return 0;
    }
    
   //encoding detection and conversion
    int converted_chars = 0;
    int ch;
    
    while ((ch = fgetc(in)) != EOF) {
        
        if (ch < 0) {
            // Deal with extended ASCII chars
            unsigned char uc = (unsigned char)ch;
            if (uc >= 128 && uc <= 255) {
                
                fputc('?', out);
            }
        }
        else if (ch >= 32 && ch <= 126) {
           
            fputc(ch, out);
            converted_chars++;
        }
        else if (ch == '\n' || ch == '\r' || ch == '\t') {
           
            fputc(ch, out);
            converted_chars++;
        }
         // Ignore other control chars
    }
    
    fclose(in);
    fclose(out);
    
    printf(" Converted %d characters with encoding fix to %s\n", converted_chars, output_file);
    return converted_chars > 0;
}

// Try backup processing methods
int try_alternative_processing(const char* filename, ErrorCode error_code) {
    printf(" Attempting alternative processing...\n");
    
    char output_file[100];
    int success = 0;
    
    switch(error_code) {
        case ERROR_FILE_CORRUPTED:
            printf(" Trying to extract readable text from corrupted file...\n");
            sprintf(output_file, "recovered_%s", filename);
            success = extract_readable_text(filename, output_file);
            if (success) {
                printf(" Try analyzing the recovered file: %s\n", output_file);
            }
            break;
            
        case ERROR_FILE_ENCODING:
            printf(" Trying different encodings...\n");
            sprintf(output_file, "encoded_fixed_%s", filename);
            success = try_different_encodings(filename, output_file);
            if (success) {
                printf(" Try analyzing the encoding-fixed file: %s\n", output_file);
            }
            break;
            
        case ERROR_CSV_FORMAT:
            printf(" Attempting to recover CSV data...\n");
            sprintf(output_file, "csv_fixed_%s.txt", filename);
            success = fix_csv_format(filename, output_file);
            if (success) {
                printf(" Try analyzing the CSV-fixed file: %s\n", output_file);
            }
            break;
            
        default:
            printf(" No alternative processing available for this error type.\n");
            success = 0;
    }
    
    return success;
}

void handle_error(const char* context, ErrorCode error_code, const char* details) {
    printf("\n ERROR in %s: ", context);
    
    switch(error_code) {
        case ERROR_FILE_NOT_FOUND:
            printf("File not found");
            break;
        case ERROR_FILE_EMPTY:
            printf("File is empty");
            break;
        case ERROR_FILE_CORRUPTED:
            printf("File appears corrupted");
            break;
        case ERROR_FILE_ENCODING:
            printf("Unsupported file encoding");
            break;
        case ERROR_FILE_PERMISSION:
            printf("Permission denied");
            break;
        case ERROR_FILE_TOO_LARGE:
            printf("File too large");
            break;
        case ERROR_MEMORY_ALLOCATION:
            printf("Memory allocation failed");
            break;
        case ERROR_CSV_FORMAT:
            printf("Invalid CSV format");
            break;
        default:
            printf("Unknown error");
    }
    
    if (details) {
        printf(" - %s", details);
    }
    printf("\n");
    
    // Show recovery options
    if (context && strstr(context, "file")) {
        show_error_recovery_options(details ? details : context, error_code);
    }
}

void show_error_recovery_options(const char* filename, ErrorCode error_code) {
    printf("\n RECOVERY OPTIONS:\n");
    
    switch(error_code) {
        case ERROR_FILE_NOT_FOUND:
            printf("1. Check filename spelling\n");
            printf("2. Browse current directory\n");
            break;
        case ERROR_FILE_CORRUPTED:
        case ERROR_FILE_ENCODING:
            printf("1. Try basic text extraction\n");
            printf("2. Skip problematic sections\n");
            printf("3. Use different file\n");
            break;
        case ERROR_CSV_FORMAT:
            printf("1. Attempt CSV recovery\n");
            printf("2. Process as plain text\n");
            break;
        default:
            printf("1. Try again\n");
            printf("2. Use different file\n");
    }
    
    printf("3. Return to main menu\n");
    printf("Choose option (1-3): ");
    
    int choice;
    if (scanf("%d", &choice) == 1) {
        getchar();
        if (choice == 1 || choice == 2) {
            try_alternative_processing(filename, error_code);
        }
        
    }
}

int is_file_corrupted(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) return 1;
    
       int binary_count = 0;
    int total_count = 0;
    int ch;
    
    while ((ch = fgetc(file)) != EOF && total_count < 1000) {
         total_count++;
          if ((ch < 32 || ch > 126) && ch != '\n' && ch != '\r' && ch != '\t') {
            binary_count++;
        }
    }
    
    fclose(file);
    
     // If over 30% chars are non-text, likely corrupted
    return (total_count > 0 && (binary_count * 100 / total_count) > 30);
}

int detect_file_encoding(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) return -1;
    
    unsigned char bom[3];
    size_t read = fread(bom, 1, 3, file);
    fclose(file);
    
    if (read >= 3 && bom[0] == 0xEF && bom[1] == 0xBB && bom[2] == 0xBF) {
        return 1; // UTF-8 BOM
    }
    if (read >= 2 && bom[0] == 0xFF && bom[1] == 0xFE) {
        return 2; // UTF-16 LE
    }
    
    return 0; // Assume UTF-8 without BOM or other encoding
}

// Better file existence check
int check_file_exists(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file) {
        fclose(file);
        return 1;
    }
    
    // File missing, show detailed error
    handle_file_not_found_error(filename);
    return 0;
}

void handle_file_not_found_error(const char* filename) {
    printf("\n ERROR: File not found - \"%s\"\n", filename);
    
    printf("\n RECOVERY OPTIONS:\n");
    printf("1. Check filename spelling\n");
    printf("2. View files in current directory\n");
    printf("3. Try a different filename\n");
    printf("Choose option (1-3): ");
    
    int choice;
    if (scanf("%d", &choice) == 1) {
        getchar();
        switch(choice) {
            case 1:
                printf(" Tip: Make sure the filename is spelled correctly and includes the extension (.txt, .csv)\n");
                printf("Current input: \"%s\"\n", filename);
                break;
            case 2:
                printf("\n Files in current directory:\n");
                system("dir *.txt *.csv 2>nul");
                break;
            case 3:
               
                printf(" Please enter a different filename when prompted\n");
                break;
            default:
                printf("Returning to main menu...\n");
        }
    } else {
       
        while (getchar() != '\n');
        printf("Invalid input. Returning to main menu.\n");
    }
}