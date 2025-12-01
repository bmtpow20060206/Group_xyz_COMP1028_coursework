#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "content.h"
#include "file.h"
#include "tool.h"
#include "error.h"

// App configuration settings
typedef struct {
    int show_charts;
    int autosave;
    int compare_mode;
    int maxwords;
    char report_format[10];
} Config;

Config app_config = {1, 0, 0, 20, "both"};

// Store analysis results here
char* global_text = NULL;
AnalysisResult global_result = {0};
int analysis_done = 0;

// Print main menu
void show_menu() {
    printf("\n=== Cyberbullying Text Analyzer ===\n");
    printf("1. Load and analyze single file\n");
    printf("2. Load and analyze multiple files\n");
    printf("3. Show analysis report\n");
    printf("4. Show high frequency words\n");
    printf("5. Save analysis results\n");
    printf("6. Save toxicity report only\n");
    printf("7. Show structured results table\n");
    printf("8. Save comprehensive reports (all formats)\n");
    printf("9. Compare two files\n");
    printf("10. Show ASCII charts\n");
    printf("11. Configuration\n");
    printf("12. Exit program\n");
    printf("Please choose an option: ");
}

// Settings menu
void show_config_menu() {
    printf("\n  CONFIGURATION MENU\n");
    printf("1. %s ASCII Charts\n", app_config.show_charts ? "Disable" : "Enable");
    printf("2. %s Auto-save Reports\n", app_config.autosave ? "Disable" : "Enable");
    printf("3. Set Max Words Display (Current: %d)\n", app_config.maxwords);
    printf("4. Toggle Report Format (Current: %s)\n", app_config.report_format);
    printf("5. Back to Main Menu\n");
    printf("Choose option: ");
}

void handle_config_menu() {
    int choice;
    while (1) {
        show_config_menu();
        if (scanf("%d", &choice) != 1) {
            printf(" Invalid input\n");
            while (getchar() != '\n');
            continue;
        }
        getchar();
        
        switch (choice) {
            case 1:
                app_config.show_charts = !app_config.show_charts;
                printf(" ASCII Charts %s\n", app_config.show_charts ? "Enabled" : "Disabled");
                break;
            case 2:
                app_config.autosave = !app_config.autosave;
                printf(" Auto-save Reports %s\n", app_config.autosave ? "Enabled" : "Disabled");
                break;
            case 3:
                printf("Enter maximum words to display: ");
                if (scanf("%d", &app_config.maxwords) == 1 && app_config.maxwords > 0) {
                    printf(" Max words display set to %d\n", app_config.maxwords);
                } else {
                    printf(" Invalid number\n");
                }
                getchar();
                break;
            case 4:
                if (strcmp(app_config.report_format, "both") == 0) {
                    strcpy(app_config.report_format, "text");
                } else if (strcmp(app_config.report_format, "text") == 0) {
                    strcpy(app_config.report_format, "csv");
                } else {
                    strcpy(app_config.report_format, "both");
                }
                printf(" Report format set to: %s\n", app_config.report_format);
                break;
            case 5:
                return;
            default:
                printf(" Invalid option\n");
        }
    }
}

// List txt and csv files in current folder
void check_current_files() {
    printf("Available text files in current directory:\n");
    system("dir *.txt 2>nul");
    system("dir *.csv 2>nul");
}

// Split space-separated filenames into array
int parse_filenames(const char* input, char*** filenames) {
    if (input == NULL || strlen(input) == 0) {
        return 0;
    }

    char input_copy[1024];
    strcpy(input_copy, input);
    
    int count = 0;
    char* token = strtok(input_copy, " ");
    while (token != NULL) {
        count++;
        token = strtok(NULL, " ");
    }

    if (count == 0) {
        return 0;
    }

    *filenames = malloc(count * sizeof(char*));
    strcpy(input_copy, input);
    
    count = 0;
    token = strtok(input_copy, " ");
    while (token != NULL) {
        (*filenames)[count] = malloc(strlen(token) + 1);
        strcpy((*filenames)[count], token);
        count++;
        token = strtok(NULL, " ");
    }

    return count;
}


// quick summary of toxic findings
void show_toxicity_summary(const AnalysisResult* result) {
    if (result->toxic_phrase_count > 0) {
        printf("\n Toxicity Summary: %d toxic phrases detected", result->toxic_phrase_count);
        if (result->severity_counts[SEVERITY_SEVERE] > 0) {
            printf(" ( %d severe)", result->severity_counts[SEVERITY_SEVERE]);
        }
        if (result->severity_counts[SEVERITY_MODERATE] > 0) {
            printf(" ( %d moderate)", result->severity_counts[SEVERITY_MODERATE]);
        }
        if (result->severity_counts[SEVERITY_MILD] > 0) {
            printf(" ( %d mild)", result->severity_counts[SEVERITY_MILD]);
        }
        printf("\n");
    } else {
        printf("\n No toxic content detected.\n");
    }
}

// File comparison feature
void compare_files() {
    char filename1[100], filename2[100];
    char* text1 = NULL, *text2 = NULL;
    AnalysisResult result1, result2;
    
    printf("Enter first filename: ");
    fgets(filename1, sizeof(filename1), stdin);
    filename1[strcspn(filename1, "\n")] = '\0';
    
    printf("Enter second filename: ");
    fgets(filename2, sizeof(filename2), stdin);
    filename2[strcspn(filename2, "\n")] = '\0';
    
    // First file
    if (!file_exists(filename1)) {
        printf(" Error: File '%s' does not exist!\n", filename1);
        return;
    }
    text1 = read_text_file(filename1);
    if (!text1) return;
    result1 = analyze_text(text1);
    
    // Second file
    if (!file_exists(filename2)) {
        printf(" Error: File '%s' does not exist!\n", filename2);
        free(text1);
        return;
    }
    text2 = read_text_file(filename2);
    if (!text2) {
        free(text1);
        return;
    }
    result2 = analyze_text(text2);
    
    // Show diff
    compare_results(&result1, &result2, filename1, filename2);
    
    // Cleanup
    free(text1);
    free(text2);
    cleanup_analyzer(&result1);
    cleanup_analyzer(&result2);
}

// Prompt user for filename with retry logic
int get_valid_filename(char* filename, int max_size) {
    int retry_count = 0;
    const int max_retries = 3;
    
    while (retry_count < max_retries) {
        printf("Enter filename: ");
        if (fgets(filename, max_size, stdin) == NULL) {
            return 0; 
        }
        filename[strcspn(filename, "\n")] = '\0';
        
        // Check if file exists
        if (file_exists(filename)) {
            return 1; 
        }
        
        // File missing, give help
        printf("\n ERROR: File not found - \"%s\"\n", filename);
        
        retry_count++;
        if (retry_count < max_retries) {
            printf("\n RECOVERY OPTIONS:\n");
            printf("1. Check filename spelling\n");
            printf("2. View files in current directory\n");
            printf("3. Try a different filename\n");
            printf("Choose option (1-3): ");
            
            char option[10];
            if (fgets(option, sizeof(option), stdin)) {
                int choice = atoi(option);
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
                        // Let user try again
                        break;
                    default:
                        printf("Invalid choice. ");
                }
            }
            
            printf("\n Would you like to try again? (y/n): ");
            char response[10];
            if (fgets(response, sizeof(response), stdin)) {
                if (response[0] != 'y' && response[0] != 'Y') {
                    return 0; 
                }
            }
        }
    }
    
    printf(" Maximum retry attempts reached. Returning to main menu.\n");
    return 0;
}

// Handle analyzing one file
void handle_single_file_analysis() {
    char filename[100];
    
    if (!get_valid_filename(filename, sizeof(filename))) {
        return; 
    }
    
   // empty file check
    if (is_file_empty(filename)) {
        handle_error("main", ERROR_FILE_EMPTY, filename);
        return;
    }
    
    const char* extension = get_file_extension(filename);
    char* text = NULL;
    
if (strcmp(extension, ".csv") == 0) {
    // Handle CSV files
    char txt_filename[100];
    
    // same name
    strcpy(txt_filename, filename);
    char* dot = strrchr(txt_filename, '.');
    if (dot != NULL) {
        strcpy(dot, ".txt");
    } else {
        strcat(txt_filename, ".txt");
    }
    
    printf("Converting CSV to TXT: %s\n", txt_filename);
    
    if (extract_csv_all_to_file(filename, txt_filename)) {
        // Directly read and analyze the converted txt
        text = read_text_file(txt_filename);
        if (text) {
            printf("✓ CSV converted to %s and loaded for analysis\n", txt_filename);
            
        }
    }
} else if (strcmp(extension, ".txt") == 0) {
     // handle TXT files differently based on size
    long size = get_file_size(filename);
    if (size > 5 * 1024 * 1024) {
        text = read_large_file(filename);
    } else {
        text = read_text_file(filename);
    }
} else {
    printf(" Error: Unsupported file format\n");
    return;
}


//
    if (text) {
        printf(" Analyzing text for toxic content...\n");
        
        // free old data
        if (global_text) {
            free(global_text);
        }
        if (global_result.word_count > 0) {
            cleanup_analyzer(&global_result);
        }
        
        global_result = analyze_text(text);
        global_text = text;
        analysis_done = 1;
        
        printf(" File analysis completed!\n");
        
        // Show quick results
        show_toxicity_summary(&global_result);
        
         // Basic stats
        printf("\n BASIC STATISTICS:\n");
        printf("----------------------------------------\n");
        printf("Total words:       %d\n", global_result.word_count);
        printf("Unique words:      %d\n", global_result.unique_words);
        printf("Sentences:         %d\n", global_result.sentence_count);
        printf("Characters:        %d\n", global_result.char_count);
        printf("Avg word length:   %.1f\n", global_result.avg_word_length);
        if (global_result.word_count > 0) {
            double toxic_percentage = (double)global_result.toxic_phrase_count / global_result.word_count * 100;
            printf("Toxicity level:    %.1f%%\n", toxic_percentage);
        }
        
        printf("Lexical diversity: %.1f%%\n", 
        ((double)global_result.unique_words / global_result.word_count) * 100);
        
        // Auto-save if enabled
        if (app_config.autosave) {
            save_comprehensive_report("auto_save", &global_result);
            printf(" Reports auto-saved to files\n");
        }
    } else {
        printf(" Failed to load file content\n");
    }
}

// Handle multiple files at once
void handle_multiple_file_analysis() {
    char input[1024];
    printf("Enter filenames (separated by spaces): ");
    fgets(input, sizeof(input), stdin);
    input[strcspn(input, "\n")] = '\0';
    
    char** filenames = NULL;
    int file_count = parse_filenames(input, &filenames);
    
    if (file_count > 0) {
        char* text = process_multiple_files((const char**)filenames, file_count);
        
        for (int i = 0; i < file_count; i++) {
            free(filenames[i]);
        }
        free(filenames);
        
        if (text) {
            printf(" Analyzing multiple files for toxic content...\n");
            
            // Clean up previous results
            if (global_text) {
                free(global_text);
            }
            if (global_result.word_count > 0) {
                cleanup_analyzer(&global_result);
            }
            
            global_result = analyze_text(text);
            global_text = text;
            analysis_done = 1;
            
            printf(" Multi-file analysis completed!\n");
            
            // Quick summary
            show_toxicity_summary(&global_result);
            
            //  stats
            printf("\n BASIC STATISTICS:\n");
            printf("----------------------------------------\n");
            printf("Total words:       %d\n", global_result.word_count);
            printf("Unique words:      %d\n", global_result.unique_words);
            printf("Sentences:         %d\n", global_result.sentence_count);
            printf("Files processed:   %d\n", file_count);
            printf("Lexical diversity: %.2f%%\n", global_result.advanced_stats.lexical_diversity * 100);
            
            if (app_config.autosave) {
                save_comprehensive_report("multi_file", &global_result);
                printf(" Reports auto-saved to files\n");
            }
        }
    } else {
        printf(" Error: No valid files specified\n");
    }
}

int main() {
    init_console_encoding();
    init_stopwords(); 
    printf("=== Cyberbullying Text Analyzer ===\n");
    printf("Supports: Single files, Multiple files, Large files\n");
    printf("Toxicity detection with multi-word phrase support\n");
    
    check_current_files();
    init_analyzer();
    
    int choice;
    
    while (1) {
        show_menu();
        if (scanf("%d", &choice) != 1) {
            printf(" Invalid input, please try again!\n");
            while (getchar() != '\n');
            continue;
        }
        getchar();
        
        switch (choice) {
            case 1:
                handle_single_file_analysis();
                break;
                
            case 2:
                handle_multiple_file_analysis();
                break;
                
            case 3:
                if (analysis_done && global_result.word_count > 0) {
                    print_analysis_report(&global_result);
                } else {
                    printf(" Please load and analyze files first!\n");
                }
                break;
                
            case 4:
                if (analysis_done && global_result.word_count > 0) {
                    int n;
                    printf("Show top how many high frequency words: ");
                    scanf("%d", &n);
                    getchar();
                    get_top_words(n, &global_result);
                } else {
                    printf(" Please load and analyze files first!\n");
                }
                break;
                
            case 5:
                if (analysis_done && global_result.word_count > 0) {
                    save_analysis_report("analysis_report.txt", &global_result);
                    save_toxicity_report("toxicity_report.txt", &global_result);
                    printf(" Results saved to analysis_report.txt\n");
                    printf(" Toxicity report saved to toxicity_report.txt\n");
                } else {
                    printf(" Please load and analyze files first!\n");
                }
                break;
                
            case 6:
                if (analysis_done && global_result.word_count > 0) {
                    save_toxicity_report("toxicity_report.txt", &global_result);
                    printf(" Toxicity report saved to toxicity_report.txt\n");
                } else {
                    printf(" Please load and analyze files first!\n");
                }
                break;

            case 7:
                if (analysis_done && global_result.word_count > 0) {
                    print_results_table(&global_result);
                } else {
                    printf(" Please load and analyze files first!\n");
                }
                break;
                
            case 8:
                if (analysis_done && global_result.word_count > 0) {
                    save_comprehensive_report("analysis", &global_result);
                } else {
                    printf(" Please load and analyze files first!\n");
                }
                break;
                
            case 9:
                compare_files();
                break;
                
            case 10:
                if (analysis_done && global_result.word_count > 0) {
                   
                    printf("\n ASCII VISUALIZATION CHARTS\n");
                    printf("===========================================\n");
                    print_word_frequency_chart(&global_result, app_config.maxwords);
                    print_toxicity_pie_chart(&global_result);
                } else {
                    printf(" Please load and analyze files first!\n");
                }
                break;
                
            case 11:
                handle_config_menu();
                break;
                
            case 12:
                if (global_text) {
                    free(global_text);
                }
                if (global_result.word_count > 0) {
                    cleanup_analyzer(&global_result);
                }
                printf(" Goodbye! Thank you for using Cyberbullying Text Analyzer.\n");
                return 0;
            case 13: //compare sort
    if (analysis_done && global_result.word_count > 0) {
        compare_sorting_algorithms(global_result.word_freq, global_result.unique_words);
    } else {
        printf("Please load and analyze files first!\n");
    }
    break;
            default:
                printf(" Invalid option, please choose again!\n");
        }
    }
}