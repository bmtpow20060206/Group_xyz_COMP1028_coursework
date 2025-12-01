#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "content.h"
#include "tool.h"

// Global variables
static WordNode* word_array[MAX_WORDS];
static int wordcount = 0;

// toxic phrases stuff
static ToxicPhrase toxic_phrases[MAX_TOXIC_PHRASES];
static int toxic_phrase_count = 0;

static char stopwords[MAX_STOPWORDS][MAX_WORD_LEN];
static int stopword_count = 0;

// ==================== UTILITY FUNCTIONS ====================

// Load stop words from file
int load_stopwords(const char* filename, char stopwords[][MAX_WORD_LEN], int max_count) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf(" Error: Cannot open stopwords file '%s'\n", filename);
        return 0;
    }
    
    int count = 0;
    char word[MAX_WORD_LEN];
    while (count < max_count && fscanf(file, "%s", word) == 1) {
        strcpy(stopwords[count], word);
        count++;
    }
    
    fclose(file);
    printf(" Loaded %d stopwords from %s\n", count, filename);
    return count;
}

void init_stopwords() {
    stopword_count = load_stopwords("stopwords.txt", stopwords, MAX_STOPWORDS);
    if (stopword_count == 0) {
        printf(" Warning: No stopwords loaded, using default set\n");
    }
}

int is_stop_word(const char* word) {
    for (int i = 0; i < stopword_count; i++) {
        if (strcmp(word, stopwords[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

const char* get_severity_name(ToxicitySeverity severity) {
    switch (severity) {
        case SEVERITY_SEVERE: return "SEVERE";
        case SEVERITY_MODERATE: return "MODERATE";
        case SEVERITY_MILD: return "MILD";
        default: return "UNKNOWN";
    }
}

// ==================== TOXIC DICTIONARY MANAGEMENT ====================

void add_toxic_phrase(const char* phrase, ToxicitySeverity severity) {
    if (toxic_phrase_count >= MAX_TOXIC_PHRASES) {
        printf(" Warning: Toxic phrase dictionary full, cannot add '%s'\n", phrase);
        return;
    }
    
    if (strlen(phrase) == 0 || strlen(phrase) >= MAX_PHRASE_LEN) {
        printf(" Warning: Invalid phrase length for '%s'\n", phrase);
        return;
    }
    
    // Check for duplicates
    for (int i = 0; i < toxic_phrase_count; i++) {
        if (strcasecmp(toxic_phrases[i].text, phrase) == 0) {
            printf("  Warning: Duplicate toxic phrase '%s', skipping\n", phrase);
            return;
        }
    }
    
    strncpy(toxic_phrases[toxic_phrase_count].text, phrase, MAX_PHRASE_LEN - 1);
    toxic_phrases[toxic_phrase_count].text[MAX_PHRASE_LEN - 1] = '\0';
    toxic_phrases[toxic_phrase_count].severity = severity;
    toxic_phrase_count++;
}

// Unified toxic dictionary loading
int load_toxic_dictionary(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf(" Error: Cannot open toxic dictionary file '%s'\n", filename);
        printf("Please create 'toxicwords.txt' with format: phrase,SEVERITY\n");
        return 0;
    }

    char line[256];
    toxic_phrase_count = 0;
    int loaded_count = 0;

    printf(" Loading toxic dictionary from %s...\n", filename);
    
    while (fgets(line, sizeof(line), file) && toxic_phrase_count < MAX_TOXIC_PHRASES) {
        line[strcspn(line, "\n")] = '\0';
        line[strcspn(line, "\r")] = '\0';
        
        if (strlen(line) == 0 || line[0] == '#' || line[0] == ';') {
            continue;
        }
        
        char* comma = strchr(line, ',');
        if (comma != NULL) {
            *comma = '\0';
            char* phrase = line;
            char* severity_str = comma + 1;
            
            ToxicitySeverity severity = SEVERITY_MILD;
            
            if (strcasecmp(severity_str, "SEVERE") == 0) {
                severity = SEVERITY_SEVERE;
            } else if (strcasecmp(severity_str, "MODERATE") == 0) {
                severity = SEVERITY_MODERATE;
            } else if (strcasecmp(severity_str, "MILD") == 0) {
                severity = SEVERITY_MILD;
            } else {
                printf("  Warning: Invalid severity '%s' for phrase '%s', using MILD\n", 
                       severity_str, phrase);
            }
            
            add_toxic_phrase(phrase, severity);
            loaded_count++;
        } else {
            add_toxic_phrase(line, SEVERITY_MILD);
            loaded_count++;
        }
    }
    
    fclose(file);
    printf(" Successfully loaded %d toxic phrases\n", loaded_count);
    return (loaded_count > 0) ? 1 : 0;
}

// ==================== TOXICITY DETECTION ====================

int detect_toxic_phrases(const char* text, AnalysisResult* result) {
    if (text == NULL || result == NULL) {
        return 0;
    }
    
    result->toxic_phrase_count = 0;
    result->toxic_word_count = 0;
    memset(result->severity_counts, 0, sizeof(result->severity_counts));
    
    char text_lower[100000];
    strncpy(text_lower, text, sizeof(text_lower) - 1);
    text_lower[sizeof(text_lower) - 1] = '\0';
    to_lower_case(text_lower);
    
    int total_detected = 0;
    
    for (int i = 0; i < toxic_phrase_count && result->toxic_phrase_count < 50; i++) {
        char phrase_lower[MAX_PHRASE_LEN];
        strncpy(phrase_lower, toxic_phrases[i].text, sizeof(phrase_lower) - 1);
        phrase_lower[sizeof(phrase_lower) - 1] = '\0';
        to_lower_case(phrase_lower);
        
        char* pos = (char*)text_lower;
        int phrase_count = 0;
        
        while ((pos = strstr(pos, phrase_lower)) != NULL) {
            if ((pos == text_lower || !isalnum((unsigned char)*(pos-1))) &&
                (!isalnum((unsigned char)*(pos + strlen(phrase_lower))))) {
                
                phrase_count++;
                result->toxic_word_count++;
            }
            pos += strlen(phrase_lower);
        }
        
        if (phrase_count > 0 && result->toxic_phrase_count < 50) {
            strncpy(result->detected_toxic_phrases[result->toxic_phrase_count].text, 
                   toxic_phrases[i].text, MAX_PHRASE_LEN - 1);
            result->detected_toxic_phrases[result->toxic_phrase_count].text[MAX_PHRASE_LEN - 1] = '\0';
            result->detected_toxic_phrases[result->toxic_phrase_count].severity = toxic_phrases[i].severity;
            result->detected_toxic_phrases[result->toxic_phrase_count].count = phrase_count;
            
            result->severity_counts[toxic_phrases[i].severity]++;
            result->toxic_phrase_count++;
            total_detected += phrase_count;
        }
    }
    
    return total_detected;
}

int calculate_toxicity_score(const AnalysisResult* result) {
    if (result->word_count == 0) return 0;
    

    int base_score = result->severity_counts[SEVERITY_SEVERE] * 3 +
                    result->severity_counts[SEVERITY_MODERATE] * 2 +
                    result->severity_counts[SEVERITY_MILD] * 1;
    
    double density = (double)result->toxic_phrase_count / result->word_count * 100;
    
    if (density > 5.0) base_score += 40;
    else if (density > 2.0) base_score += 25;
    else if (density > 1.0) base_score += 15;
    else if (density > 0.5) base_score += 5;
    
    return (base_score > 100) ? 100 : base_score;
}

const char* get_toxicity_level(int score) {
    if (score >= 70) return "VERY HIGH";
    if (score >= 50) return "HIGH";
    if (score >= 30) return "MEDIUM";
    if (score >= 15) return "LOW";
    return "VERY LOW";
}

// ==================== HASH TABLE IMPLEMENTATION ====================

unsigned int hash_function(const char* word) {
    unsigned int hash = 0;
    for (int i = 0; word[i] != '\0'; i++) {
        hash = hash * 31 + (unsigned char)word[i];
    }
    return hash % HASH_TABLE_SIZE;
}

void hash_table_init(HashTable* ht) {
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        ht->table[i] = NULL;
    }
    ht->size = 0;
}

void hash_table_insert(HashTable* ht, const char* word) {
    unsigned int index = hash_function(word);
    WordNode* current = ht->table[index];
    
    while (current != NULL) {
        if (strcmp(current->word, word) == 0) {
            current->frequency++;
            return;
        }
        current = current->next;
    }
    
    WordNode* new_node = (WordNode*)malloc(sizeof(WordNode));
    strncpy(new_node->word, word, MAX_WORD_LEN - 1);
    new_node->word[MAX_WORD_LEN - 1] = '\0';
    new_node->frequency = 1;
    new_node->next = ht->table[index];
    ht->table[index] = new_node;
    ht->size++;
}

void hash_table_to_array(HashTable* ht, WordNode*** array, int* size) {
    *size = 0;
    *array = (WordNode**)malloc(ht->size * sizeof(WordNode*));
    
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        WordNode* current = ht->table[i];
        while (current != NULL) {
            (*array)[(*size)++] = current;
            current = current->next;
        }
    }
}

void hash_table_free(HashTable* ht) {
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        WordNode* current = ht->table[i];
        while (current != NULL) {
            WordNode* temp = current;
            current = current->next;
            free(temp);
        }
        ht->table[i] = NULL;
    }
    ht->size = 0;
}

// ==================== SORTING ALGORITHMS ====================

int partition(WordNode** array, int low, int high) {
    int pivot = array[high]->frequency;
    int i = low - 1;
    
    for (int j = low; j < high; j++) {
        if (array[j]->frequency >= pivot) {
            i++;
            WordNode* temp = array[i];
            array[i] = array[j];
            array[j] = temp;
        }
    }
    
    WordNode* temp = array[i + 1];
    array[i + 1] = array[high];
    array[high] = temp;
    
    return i + 1;
}

void quick_sort(WordNode** array, int low, int high) {
    if (low < high) {
        int pi = partition(array, low, high);
        quick_sort(array, low, pi - 1);
        quick_sort(array, pi + 1, high);
    }
}

void bubble_sort(WordNode** array, int n) {
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            if (array[j]->frequency < array[j + 1]->frequency) {
                WordNode* temp = array[j];
                array[j] = array[j + 1];
                array[j + 1] = temp;
            }
        }
    }
}

void merge(WordNode** array, int left, int mid, int right) {
    int n1 = mid - left + 1;
    int n2 = right - mid;
    
    WordNode** left_arr = (WordNode**)malloc(n1 * sizeof(WordNode*));
    WordNode** right_arr = (WordNode**)malloc(n2 * sizeof(WordNode*));
    
    for (int i = 0; i < n1; i++)
        left_arr[i] = array[left + i];
    for (int j = 0; j < n2; j++)
        right_arr[j] = array[mid + 1 + j];
    
    int i = 0, j = 0, k = left;
    
    while (i < n1 && j < n2) {
        if (left_arr[i]->frequency >= right_arr[j]->frequency) {
            array[k] = left_arr[i];
            i++;
        } else {
            array[k] = right_arr[j];
            j++;
        }
        k++;
    }
    
    while (i < n1) {
        array[k] = left_arr[i];
        i++;
        k++;
    }
    
    while (j < n2) {
        array[k] = right_arr[j];
        j++;
        k++;
    }
    
    free(left_arr);
    free(right_arr);
}

void merge_sort(WordNode** array, int left, int right) {
    if (left < right) {
        int mid = left + (right - left) / 2;
        merge_sort(array, left, mid);
        merge_sort(array, mid + 1, right);
        merge(array, left, mid, right);
    }
}

void sort_words(WordNode** array, int n, SortAlgorithm algorithm) {
    switch (algorithm) {
        case SORT_BUBBLE:
            bubble_sort(array, n);
            break;
        case SORT_QUICK:
            quick_sort(array, 0, n - 1);
            break;
        case SORT_MERGE:
            merge_sort(array, 0, n - 1);
            break;
        default:
            quick_sort(array, 0, n - 1);
            break;
    }
}

// ==================== ANALYSIS CORE FUNCTIONS ====================

void init_analyzer(void) {
    wordcount = 0;
    memset(word_array, 0, sizeof(word_array));
    
    init_stopwords();
    
    if (!load_toxic_dictionary("toxicwords.txt")) {
        printf("  Toxicity detection disabled. Create 'toxicwords.txt' to enable.\n");
    }
}

void process_word(const char* word, AnalysisResult* result) {
    if (strlen(word) == 0) return;
    
    result->word_count++;
    
    char normalized[MAX_WORD_LEN];
    normalize_word(word, normalized);
    
    if (strlen(normalized) == 0) return;
    
    if (is_stop_word(normalized)) {
        return;
    }
    
    hash_table_insert(&result->hash_table, normalized);
}

void calculate_advanced_stats(const char* text, AdvancedStats* stats) {
    if (text == NULL || stats == NULL) return;
    
    memset(stats, 0, sizeof(AdvancedStats));
    
    // Count paragraphs based on empty lines
    int empty_lines = 0;
    const char* ptr = text;
    while (*ptr) {
        if (*ptr == '\n') {
            empty_lines++;
        }
        ptr++;
    }
    stats->total_paragraphs = empty_lines > 0 ? empty_lines + 1 : 1;
    
    // Count sentences
    stats->total_sentences = 0;
    stats->longest_sentence = 0;
    stats->shortest_sentence = 10000;
    int current_sentence_words = 0;
    
    ptr = text;
    while (*ptr) {
        if (isalnum((unsigned char)*ptr)) {
            current_sentence_words++;
            while (*ptr && isalnum((unsigned char)*ptr)) ptr++;
        }
        
        if (*ptr == '.' || *ptr == '!' || *ptr == '?') {
            stats->total_sentences++;
            stats->total_words += current_sentence_words;
            
            if (current_sentence_words > stats->longest_sentence) {
                stats->longest_sentence = current_sentence_words;
            }
            if (current_sentence_words < stats->shortest_sentence && current_sentence_words > 0) {
                stats->shortest_sentence = current_sentence_words;
            }
            current_sentence_words = 0;
        }
        if (*ptr) ptr++;
    }
    
    // Calculate averages
    if (stats->total_sentences > 0) {
        stats->avg_sentence_length = (double)stats->total_words / stats->total_sentences;
    }
}

AnalysisResult analyze_text(const char* text) {
    AnalysisResult result = {0};
    if (text == NULL) return result;
    
    hash_table_init(&result.hash_table);
    
    char buffer[MAX_WORD_LEN];
    const char* p = text;
    int in_word = 0;
    int word_len = 0;
    
    while (*p) {
        result.char_count++;
        
        if (*p == '.' || *p == '!' || *p == '?') {
            result.sentence_count++;
        }
        
        if (*p == '\n') {
            result.line_count++;
        }
        
        if (isalnum((unsigned char)*p)) {
            if (!in_word) {
                in_word = 1;
                word_len = 0;
            }
            if (word_len < MAX_WORD_LEN - 1) {
                buffer[word_len++] = tolower((unsigned char)*p);
            }
        } else {
            if (*p == '\'' && in_word && isalnum((unsigned char)*(p+1))) {
                if (word_len < MAX_WORD_LEN - 1) {
                    buffer[word_len++] = tolower((unsigned char)*p);
                }
            } else {
                if (in_word && word_len > 0) {
                    buffer[word_len] = '\0';
                    process_word(buffer, &result);
                }
                in_word = 0;
                word_len = 0;
            }
        }
        p++;
    }
    
    if (in_word && word_len > 0) {
        buffer[word_len] = '\0';
        process_word(buffer, &result);
    }
    
    hash_table_to_array(&result.hash_table, &result.word_freq, &result.unique_words);
    wordcount = result.unique_words;
    
    sort_words(result.word_freq, wordcount, SORT_QUICK);
    
    for (int i = 0; i < wordcount && i < MAX_WORDS; i++) {
        word_array[i] = result.word_freq[i];
    }
    
    if (result.word_count > 0) {
        result.avg_word_length = (double)result.char_count / result.word_count;
        if (result.sentence_count > 0) {
            result.reading_level = (0.39 * ((double)result.word_count / result.sentence_count)) + 
                                  (11.8 * result.avg_word_length) - 15.59;
        }
    }
    
    result.line_count++;
    
    calculate_advanced_stats(text, &result.advanced_stats);
    result.advanced_stats.unique_words = result.unique_words;
    
    if (toxic_phrase_count > 0) {
        detect_toxic_phrases(text, &result);
    }
    
    // Calculate toxicity ratios
    if (result.word_count > 0) {
        result.advanced_stats.clean_word_count = result.word_count - result.toxic_word_count;
        result.advanced_stats.toxic_ratio = (double)result.toxic_word_count / result.word_count * 100;
        result.advanced_stats.clean_ratio = (double)result.advanced_stats.clean_word_count / result.word_count * 100;
    }
    result.advanced_stats.total_words = result.word_count;
    
    if (result.word_count > 0) {
        result.advanced_stats.lexical_diversity = (double)result.unique_words / result.word_count;
    } else {
        result.advanced_stats.lexical_diversity = 0.0;
    }
    
    return result;
}

// ==================== DISPLAY FUNCTIONS ====================

void show_most_toxic_words(const AnalysisResult* result, int n) {
    if (result->toxic_phrase_count == 0) {
        printf("No toxic words detected.\n");
        return;
    }
    
    ToxicPhrase sorted_toxic[50];
    int total_count = result->toxic_phrase_count;
    if (total_count > 50) total_count = 50;
    
    for (int i = 0; i < total_count; i++) {
        sorted_toxic[i] = result->detected_toxic_phrases[i];
    }
    
    for (int i = 0; i < total_count - 1; i++) {
        for (int j = 0; j < total_count - i - 1; j++) {
            if (sorted_toxic[j].count < sorted_toxic[j + 1].count) {
                ToxicPhrase temp = sorted_toxic[j];
                sorted_toxic[j] = sorted_toxic[j + 1];
                sorted_toxic[j + 1] = temp;
            }
        }
    }
    
    int show_count = (total_count < n) ? total_count : n;
    
    printf("\n=== TOP %d MOST TOXIC WORDS/PHRASES ===\n", show_count);
    printf("Rank  Phrase              Severity    Count\n");
    printf("-----------------------------------------------\n");
    
    for (int i = 0; i < show_count; i++) {
        printf("%-5d %-18s %-11s %d\n", 
               i + 1, 
               sorted_toxic[i].text,
               get_severity_name(sorted_toxic[i].severity),
               sorted_toxic[i].count);
    }
    printf("-----------------------------------------------\n");
}

void print_toxicity_report(const AnalysisResult* result) {
    printf("\n=== TOXICITY ANALYSIS REPORT ===\n");
    
    if (result->toxic_phrase_count == 0) {
        printf(" NO TOXIC CONTENT DETECTED\n");
        return;
    }
    
    printf("Total toxic phrases: %d\n", result->toxic_phrase_count);
    printf("Severe: %d, Moderate: %d, Mild: %d\n", 
           result->severity_counts[SEVERITY_SEVERE],
           result->severity_counts[SEVERITY_MODERATE],
           result->severity_counts[SEVERITY_MILD]);
    
    double toxicity_ratio = (double)result->toxic_phrase_count / result->word_count * 100;
    printf("Toxicity density: %.2f%%\n", toxicity_ratio);
    
    printf("Word Composition:   %d toxic (%.1f%%) / %d clean (%.1f%%)\n",
           result->toxic_word_count, result->advanced_stats.toxic_ratio,
           result->advanced_stats.clean_word_count, result->advanced_stats.clean_ratio);
    
    int toxicity_score = calculate_toxicity_score(result);
    printf("Toxicity score: %d/100 (%s)\n", toxicity_score, get_toxicity_level(toxicity_score));
    
    printf("\nDetected toxic phrases:\n");
    for (int i = 0; i < result->toxic_phrase_count; i++) {
        printf("  %s [%s]\n", 
               result->detected_toxic_phrases[i].text,
               get_severity_name(result->detected_toxic_phrases[i].severity));
    }
    
    printf("\nRISK ASSESSMENT: ");
    if (result->severity_counts[SEVERITY_SEVERE] > 0) {
        printf(" HIGH RISK - Contains severe toxic content\n");
    } else if (result->severity_counts[SEVERITY_MODERATE] >= 3) {
        printf(" MEDIUM RISK - Multiple toxic phrases detected\n");
    } else {
        printf(" LOW RISK - Minor toxic content\n");
    }
}

void print_advanced_stats(const AdvancedStats* stats) {
    printf("\n=== ADVANCED STATISTICS ===\n");
    
    if (stats->total_words > 0) {
        double diversity = (double)stats->unique_words / stats->total_words;
        printf("Vocabulary Diversity: %.1f%%\n", diversity * 100);
    } else {
        printf("Vocabulary Diversity: 0.000\n");
    }
    
    printf("Unique Words:         %d / %d\n", stats->unique_words, stats->total_words);
    printf("Average Sentence:     %.1f words\n", stats->avg_sentence_length);
    printf("Longest Sentence:     %d words\n", stats->longest_sentence);
    printf("Shortest Sentence:    %d words\n", stats->shortest_sentence);
    printf("Paragraphs:           %d\n", stats->total_paragraphs);
}

void print_analysis_report(const AnalysisResult* result) {
    printf("\n=== TEXT ANALYSIS ===\n");
    printf("Total characters: %d\n", result->char_count);
    printf("Total words:      %d\n", result->word_count);
    printf("Total sentences:  %d\n", result->sentence_count);
    printf("Unique words:     %d\n", result->unique_words);
    printf("Reading level:    %.2f\n", result->reading_level);
    
    if (wordcount > 0) {
        printf("\nTop 3 words: ");
        int top_n = (wordcount < 3) ? wordcount : 3;
        for (int i = 0; i < top_n; i++) {
            printf("%s(%d) ", result->word_freq[i]->word, result->word_freq[i]->frequency);
        }
        printf("\n");
    }
    
    print_advanced_stats(&result->advanced_stats);
    
    if (toxic_phrase_count > 0) {
        print_toxicity_report(result);
        if (result->toxic_phrase_count > 0) {
            show_most_toxic_words(result, 5);
        }
    } else {
        printf("\n  Toxicity detection disabled\n");
    }
}

// ==================== COMPARISON AND UTILITY FUNCTIONS ====================

int compare_arrays(WordNode** arr1, WordNode** arr2, int n) {
    for (int i = 0; i < n; i++) {
        if (strcmp(arr1[i]->word, arr2[i]->word) != 0 || 
            arr1[i]->frequency != arr2[i]->frequency) {
            return 0;
        }
    }
    return 1;
}

void print_top_n_words(WordNode** array, int count, int total) {
    int show_count = (count < total) ? count : total;
    for (int i = 0; i < show_count; i++) {
        printf("%s(%d) ", array[i]->word, array[i]->frequency);
    }
    printf("\n");
}

void compare_sorting_algorithms(WordNode** original_array, int n) {
    if (n <= 1) {
        printf("Not enough words to compare sorting algorithms.\n");
        return;
    }
    
    printf("\n=== SORTING ALGORITHM OUTPUT COMPARISON ===\n");
    
    WordNode** bubble_array = (WordNode**)malloc(n * sizeof(WordNode*));
    WordNode** quick_array = (WordNode**)malloc(n * sizeof(WordNode*));
    WordNode** merge_array = (WordNode**)malloc(n * sizeof(WordNode*));
    
    for (int i = 0; i < n; i++) {
        bubble_array[i] = original_array[i];
        quick_array[i] = original_array[i];
        merge_array[i] = original_array[i];
    }
    
    bubble_sort(bubble_array, n);
    quick_sort(quick_array, 0, n - 1);
    merge_sort(merge_array, 0, n - 1);
    
    printf("Comparison Results:\n");
    printf("Bubble vs Quick:  %s\n", compare_arrays(bubble_array, quick_array, n) ? "IDENTICAL" : "DIFFERENT");
    printf("Bubble vs Merge:  %s\n", compare_arrays(bubble_array, merge_array, n) ? "IDENTICAL" : "DIFFERENT");
    printf("Quick vs Merge:   %s\n", compare_arrays(quick_array, merge_array, n) ? "IDENTICAL" : "DIFFERENT");
    
    printf("\nTop 5 words by each algorithm:\n");
    printf("Bubble Sort: "); print_top_n_words(bubble_array, 5, n);
    printf("Quick Sort:  "); print_top_n_words(quick_array, 5, n);
    printf("Merge Sort:  "); print_top_n_words(merge_array, 5, n);
    
    free(bubble_array);
    free(quick_array);
    free(merge_array);
    printf("==========================================\n");
}

void get_top_words(int n, const AnalysisResult* result) {
    if (n <= 0) {
        printf("Error: Please enter a positive number\n");
        return;
    }
    
    if (result == NULL || result->unique_words == 0 || result->word_freq == NULL) {
        printf("No words available for analysis. Please analyze a text file first.\n");
        return;
    }
    
    printf("\nChoose sorting algorithm:\n");
    printf("1. Bubble Sort\n");
    printf("2. Quick Sort\n");
    printf("3. Merge Sort\n");
    printf("4. Compare All Algorithms\n");
    printf("Enter your choice (1-4): ");
    
    int choice;
    if (scanf("%d", &choice) != 1) {
        printf(" Invalid input\n");
        while (getchar() != '\n');
        return;
    }
    getchar();
    
    int display_count = (n > result->unique_words) ? result->unique_words : n;
    
    WordNode** temp_array = (WordNode**)malloc(result->unique_words * sizeof(WordNode*));
    if (temp_array == NULL) {
        printf(" Memory allocation failed\n");
        return;
    }
    
    for (int i = 0; i < result->unique_words; i++) {
        temp_array[i] = result->word_freq[i];
    }
    
    switch (choice) {
        case 1:
            bubble_sort(temp_array, result->unique_words);
            printf("\n=== TOP %d WORDS (Bubble Sort) ===\n", display_count);
            break;
        case 2:
            quick_sort(temp_array, 0, result->unique_words - 1);
            printf("\n=== TOP %d WORDS (Quick Sort) ===\n", display_count);
            break;
        case 3:
            merge_sort(temp_array, 0, result->unique_words - 1);
            printf("\n=== TOP %d WORDS (Merge Sort) ===\n", display_count);
            break;
        case 4:
            compare_sorting_algorithms(temp_array, result->unique_words);
            quick_sort(temp_array, 0, result->unique_words - 1);
            printf("\n=== TOP %d WORDS (After Comparison) ===\n", display_count);
            break;
        default:
            printf("Invalid choice. Using Quick Sort.\n");
            quick_sort(temp_array, 0, result->unique_words - 1);
            printf("\n=== TOP %d WORDS (Quick Sort) ===\n", display_count);
            break;
    }
    
    printf("Rank  Word                Frequency\n");
    printf("------------------------------------\n");
    
    for (int i = 0; i < display_count; i++) {
        printf("%-5d %-18s %d\n", 
               i + 1, 
               temp_array[i]->word, 
               temp_array[i]->frequency);
    }
    printf("------------------------------------\n");
    
    int total_occurrences = 0;
    for (int i = 0; i < result->unique_words; i++) {
        total_occurrences += result->word_freq[i]->frequency;
    }
    
    if (display_count > 0 && total_occurrences > 0) {
        double coverage = 0.0;
        int top_words_occurrences = 0;
        for (int i = 0; i < display_count; i++) {
            top_words_occurrences += temp_array[i]->frequency;
        }
        coverage = (double)top_words_occurrences / total_occurrences * 100;
        
        printf("These %d words represent %.1f%% of all word occurrences\n", 
               display_count, coverage);
    }
    
    free(temp_array);
}

// ==================== FILE OUTPUT FUNCTIONS ====================

void save_toxicity_report(const char* filename, const AnalysisResult* result) {
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        printf(" Error: Cannot create toxicity report file\n");
        return;
    }
    
    fprintf(file, "TOXICITY ANALYSIS REPORT\n");
    fprintf(file, "Generated on: %s", ctime(&(time_t){time(NULL)}));
    fprintf(file, "============================================\n\n");
    
    fprintf(file, "TEXT STATISTICS:\n");
    fprintf(file, "Total words:        %d\n", result->word_count);
    fprintf(file, "Total sentences:    %d\n", result->sentence_count);
    fprintf(file, "Text length:        %d characters\n\n", result->char_count);
    
    fprintf(file, "TOXICITY FINDINGS:\n");
    fprintf(file, "Total toxic phrases: %d\n", result->toxic_phrase_count);
    fprintf(file, "Severe: %d, Moderate: %d, Mild: %d\n", 
            result->severity_counts[SEVERITY_SEVERE],
            result->severity_counts[SEVERITY_MODERATE],
            result->severity_counts[SEVERITY_MILD]);
    
    double toxicity_ratio = (double)result->toxic_phrase_count / result->word_count * 100;
    fprintf(file, "Toxicity density:    %.2f%%\n\n", toxicity_ratio);
    
    fprintf(file, "DETECTED TOXIC PHRASES:\n");
    for (int i = 0; i < result->toxic_phrase_count; i++) {
        fprintf(file, "%2d. %s [%s]\n", 
                i + 1,
                result->detected_toxic_phrases[i].text,
                get_severity_name(result->detected_toxic_phrases[i].severity));
    }
    
    fclose(file);
    printf(" Toxicity report saved to: %s\n", filename);
}

void save_analysis_report(const char* filename, AnalysisResult* result) {
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        printf(" Error: Cannot create analysis report\n");
        return;
    }
    
    fprintf(file, "TEXT ANALYSIS REPORT\n");
    fprintf(file, "====================\n\n");
    
    fprintf(file, "BASIC STATISTICS:\n");
    fprintf(file, "Total characters: %d\n", result->char_count);
    fprintf(file, "Total words:      %d\n", result->word_count);
    fprintf(file, "Total sentences:  %d\n", result->sentence_count);
    fprintf(file, "Unique words:     %d\n", result->unique_words);
    fprintf(file, "Reading level:    %.2f\n\n", result->reading_level);

    fprintf(file, "TOXICITY RATIO ANALYSIS:\n");
    fprintf(file, "Toxic words:      %d (%.1f%%)\n", 
            result->toxic_word_count, result->advanced_stats.toxic_ratio);
    fprintf(file, "Clean words:      %d (%.1f%%)\n",
            result->advanced_stats.clean_word_count, result->advanced_stats.clean_ratio);
    fprintf(file, "Total words:      %d\n\n", result->word_count);
    
    fprintf(file, "TOP 10 WORDS:\n");
    int top_n = (result->unique_words < 10) ? result->unique_words : 10;
    for (int i = 0; i < top_n; i++) {
        fprintf(file, "%2d. %-15s (%d)\n", i+1, result->word_freq[i]->word, result->word_freq[i]->frequency);
    }
    
    fclose(file);
    printf(" Analysis report saved to: %s\n", filename);
}

// ==================== ADDITIONAL OUTPUT FORMATS ====================

void save_analysis_csv(const char* filename, const AnalysisResult* result) {
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        printf(" Error: Cannot create CSV file: %s\n", filename);
        return;
    }
    
    fprintf(file, "Metric,Value\n");
    fprintf(file, "Total Characters,%d\n", result->char_count);
    fprintf(file, "Total Words,%d\n", result->word_count);
    fprintf(file, "Total Sentences,%d\n", result->sentence_count);
    fprintf(file, "Total Lines,%d\n", result->line_count);
    fprintf(file, "Unique Words,%d\n", result->unique_words);
    fprintf(file, "Average Word Length,%.2f\n", result->avg_word_length);
    fprintf(file, "Reading Level,%.2f\n", result->reading_level);
    fprintf(file, "Lexical Diversity,%.3f\n", result->advanced_stats.lexical_diversity);
    fprintf(file, "Average Sentence Length,%.1f\n", result->advanced_stats.avg_sentence_length);
    fprintf(file, "Paragraph Count,%d\n", result->advanced_stats.total_paragraphs);
    
    if (result->toxic_phrase_count > 0) {
        fprintf(file, "Total Toxic Phrases,%d\n", result->toxic_phrase_count);
        fprintf(file, "Severe Toxic Phrases,%d\n", result->severity_counts[SEVERITY_SEVERE]);
        fprintf(file, "Moderate Toxic Phrases,%d\n", result->severity_counts[SEVERITY_MODERATE]);
        fprintf(file, "Mild Toxic Phrases,%d\n", result->severity_counts[SEVERITY_MILD]);
        fprintf(file, "Toxicity Score,%d\n", calculate_toxicity_score(result));
        fprintf(file, "Toxicity Density,%.2f%%\n", (double)result->toxic_phrase_count / result->word_count * 100);
    }
    
    fclose(file);
    printf(" Analysis CSV saved to: %s\n", filename);
}

void save_word_frequency_csv(const char* filename, WordNode** words, int count) {
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        printf(" Error: Cannot create CSV file: %s\n", filename);
        return;
    }
    
    fprintf(file, "Rank,Word,Frequency,Percentage\n");
    
    int total_occurrences = 0;
    for (int i = 0; i < count; i++) {
        total_occurrences += words[i]->frequency;
    }
    
    for (int i = 0; i < count && i < 100; i++) {
        double percentage = (double)words[i]->frequency / total_occurrences * 100;
        fprintf(file, "%d,%s,%d,%.2f%%\n", 
                i + 1, 
                words[i]->word, 
                words[i]->frequency, 
                percentage);
    }
    
    fclose(file);
    printf(" Word frequency CSV saved to: %s\n", filename);
}

void save_toxicity_csv(const char* filename, const AnalysisResult* result) {
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        printf(" Error: Cannot create CSV file: %s\n", filename);
        return;
    }
    
    fprintf(file, "Rank,Toxic Phrase,Severity\n");
    
    for (int i = 0; i < result->toxic_phrase_count; i++) {
        fprintf(file, "%d,%s,%s\n", 
                i + 1,
                result->detected_toxic_phrases[i].text,
                get_severity_name(result->detected_toxic_phrases[i].severity));
    }
    
    fclose(file);
    printf(" Toxicity CSV saved to: %s\n", filename);
}

// ==================== CLEANUP FUNCTION ====================

void cleanup_analyzer(AnalysisResult* result) {
    if (result && result->word_freq) {
        free(result->word_freq);
    }
    if (result) {
        hash_table_free(&result->hash_table);
    }
}
// ==================== ADDITIONAL OUTPUT AND COMPARISON FUNCTIONS ====================

void print_results_table(const AnalysisResult* result) {
    printf("\n=== STRUCTURED ANALYSIS RESULTS ===\n");
    
    // Basic stats table
    printf("\n BASIC TEXT STATISTICS\n");
    printf("┌────────────────────────────┬──────────┐\n");
    printf("│ Metric                     │ Value    │\n");
    printf("├────────────────────────────┼──────────┤\n");
    printf("│ Total Characters           │ %8d │\n", result->char_count);
    printf("│ Total Words                │ %8d │\n", result->word_count);
    printf("│ Total Sentences            │ %8d │\n", result->sentence_count);
    printf("│ Unique Words               │ %8d │\n", result->unique_words);
    printf("│ Average Word Length        │ %8.2f │\n", result->avg_word_length);
    printf("│ Reading Level              │ %8.2f │\n", result->reading_level);
    printf("└────────────────────────────┴──────────┘\n");
    
    // Advanced stats table
    printf("\n ADVANCED STATISTICS\n");
    printf("┌────────────────────────────┬──────────┐\n");
    printf("│ Metric                     │ Value    │\n");
    printf("├────────────────────────────┼──────────┤\n");
    printf("│ Lexical Diversity          │ %8.3f │\n", result->advanced_stats.lexical_diversity);
    printf("│ Avg Sentence Length        │ %8.1f │\n", result->advanced_stats.avg_sentence_length);
    printf("│ Longest Sentence           │ %8d │\n", result->advanced_stats.longest_sentence);
    printf("│ Shortest Sentence          │ %8d │\n", result->advanced_stats.shortest_sentence);
    printf("│ Paragraph Count            │ %8d │\n", result->advanced_stats.total_paragraphs);
    printf("└────────────────────────────┴──────────┘\n");
    
    // Toxicity stats table
    if (result->toxic_phrase_count > 0) {
        printf("\n  TOXICITY ANALYSIS\n");
        printf("┌────────────────────────────┬──────────┐\n");
        printf("│ Metric                     │ Value    │\n");
        printf("├────────────────────────────┼──────────┤\n");
        printf("│ Total Toxic Phrases        │ %8d │\n", result->toxic_phrase_count);
        printf("│ Severe Phrases             │ %8d │\n", result->severity_counts[SEVERITY_SEVERE]);
        printf("│ Moderate Phrases           │ %8d │\n", result->severity_counts[SEVERITY_MODERATE]);
        printf("│ Mild Phrases               │ %8d │\n", result->severity_counts[SEVERITY_MILD]);
        printf("│ Toxicity Score             │ %8d │\n", calculate_toxicity_score(result));
        printf("│ Toxicity Density           │ %7.2f%% │\n", 
               (double)result->toxic_phrase_count / result->word_count * 100);
        printf("└────────────────────────────┴──────────┘\n");
        
        // Toxic phrases table
        if (result->toxic_phrase_count > 0) {
            printf("\n DETECTED TOXIC PHRASES\n");
            printf("┌──────┬────────────────────┬──────────┐\n");
            printf("│ Rank │ Phrase             │ Severity │\n");
            printf("├──────┼────────────────────┼──────────┤\n");
            for (int i = 0; i < result->toxic_phrase_count && i < 10; i++) {
                printf("│ %4d │ %-18s │ %-8s │\n", 
                       i + 1,
                       result->detected_toxic_phrases[i].text,
                       get_severity_name(result->detected_toxic_phrases[i].severity));
            }
            printf("└──────┴────────────────────┴──────────┘\n");
        }
    } else {
        printf("\n TOXICITY ANALYSIS: No toxic content detected\n");
    }
}

void save_comprehensive_report(const char* base_filename, const AnalysisResult* result) {
    char analysis_csv[100], word_csv[100], toxicity_csv[100], full_report[100];
    
    // Generate filenames
    snprintf(analysis_csv, sizeof(analysis_csv), "%s_analysis.csv", base_filename);
    snprintf(word_csv, sizeof(word_csv), "%s_words.csv", base_filename);
    snprintf(toxicity_csv, sizeof(toxicity_csv), "%s_toxicity.csv", base_filename);
    snprintf(full_report, sizeof(full_report), "%s_full.txt", base_filename);
    
    // Save reports in various formats
    save_analysis_csv(analysis_csv, result);
    save_word_frequency_csv(word_csv, result->word_freq, result->unique_words);
    
    if (result->toxic_phrase_count > 0) {
        save_toxicity_csv(toxicity_csv, result);
    }
    
    // Save full text report
    FILE* file = fopen(full_report, "w");
    if (file) {
        time_t now = time(NULL);
        fprintf(file, "COMPREHENSIVE TEXT ANALYSIS REPORT\n");
        fprintf(file, "Generated on: %s", ctime(&now));
        fprintf(file, "Analysis ID: %s\n\n", base_filename);
        
        fprintf(file, "Output Files:\n");
        fprintf(file, "- Analysis CSV: %s\n", analysis_csv);
        fprintf(file, "- Word Frequency CSV: %s\n", word_csv);
        if (result->toxic_phrase_count > 0) {
            fprintf(file, "- Toxicity CSV: %s\n", toxicity_csv);
        }
        fprintf(file, "\n");
        
        // Basic stats
        fprintf(file, "BASIC STATISTICS:\n");
        fprintf(file, "Total characters: %d\n", result->char_count);
        fprintf(file, "Total words: %d\n", result->word_count);
        fprintf(file, "Total sentences: %d\n", result->sentence_count);
        fprintf(file, "Unique words: %d\n", result->unique_words);
        fprintf(file, "Average word length: %.2f\n", result->avg_word_length);
        fprintf(file, "Reading level: %.2f\n\n", result->reading_level);
        
        // Advanced stats
        fprintf(file, "ADVANCED STATISTICS:\n");
        fprintf(file, "Lexical diversity: %.1f%%\n", result->advanced_stats.lexical_diversity * 100);
        fprintf(file, "Average sentence length: %.1f\n", result->advanced_stats.avg_sentence_length);
        fprintf(file, "Paragraph count: %d\n\n", result->advanced_stats.total_paragraphs);
        
        // Word frequency
        fprintf(file, "TOP 20 WORDS:\n");
        int top_n = (result->unique_words < 20) ? result->unique_words : 20;
        for (int i = 0; i < top_n; i++) {
            fprintf(file, "%2d. %-15s (%d occurrences)\n", 
                    i + 1, result->word_freq[i]->word, result->word_freq[i]->frequency);
        }
        fprintf(file, "\n");
        
        // Toxicity stats
        if (result->toxic_phrase_count > 0) {
            fprintf(file, "TOXICITY ANALYSIS:\n");
            fprintf(file, "Total toxic phrases: %d\n", result->toxic_phrase_count);
            fprintf(file, "Severe: %d, Moderate: %d, Mild: %d\n", 
                    result->severity_counts[SEVERITY_SEVERE],
                    result->severity_counts[SEVERITY_MODERATE],
                    result->severity_counts[SEVERITY_MILD]);
            fprintf(file, "Toxicity score: %d/100\n\n", calculate_toxicity_score(result));
            
            fprintf(file, "DETECTED TOXIC PHRASES:\n");
            for (int i = 0; i < result->toxic_phrase_count; i++) {
                fprintf(file, "%2d. %s [%s]\n", 
                        i + 1,
                        result->detected_toxic_phrases[i].text,
                        get_severity_name(result->detected_toxic_phrases[i].severity));
            }
        } else {
            fprintf(file, "TOXICITY ANALYSIS: No toxic content detected\n");
        }
        
        fclose(file);
        printf(" Comprehensive report saved to: %s\n", full_report);
    }
    
    printf("\n Generated structured outputs:\n");
    printf("   - %s (Analysis CSV)\n", analysis_csv);
    printf("   - %s (Word frequency CSV)\n", word_csv);
    if (result->toxic_phrase_count > 0) {
        printf("   - %s (Toxicity CSV)\n", toxicity_csv);
    }
    printf("   - %s (Full report)\n", full_report);
}

void print_word_frequency_chart(const AnalysisResult* result, int top_n) {
    if (result->unique_words == 0) {
        printf("No words available for chart.\n");
        return;
    }
    
    int display_count = (top_n > result->unique_words) ? result->unique_words : top_n;
    
    // Find max frequency for scaling
    int max_frequency = result->word_freq[0]->frequency;
    if (max_frequency == 0) max_frequency = 1; 
    
    printf("\n TOP %d WORDS FREQUENCY CHART\n", display_count);
    printf("┌────────────────────────────────────────────────────────┐\n");
    
    for (int i = 0; i < display_count; i++) {
        WordNode* word = result->word_freq[i];
        int bar_length = (word->frequency * 40) / max_frequency; 
        if (bar_length == 0 && word->frequency > 0) bar_length = 1; 
        
        printf("│ %-15s ", word->word);
        for (int j = 0; j < bar_length; j++) {
            printf("█");
        }
        printf(" %d\n", word->frequency);
    }
    
    printf("└────────────────────────────────────────────────────────┘\n");
}

void print_toxicity_pie_chart(const AnalysisResult* result) {
    if (result->toxic_phrase_count == 0) {
        return;
    }
    
    printf("\n  TOXICITY DISTRIBUTION\n");
    printf("┌────────────────────────────────────────────────────────┐\n");
    
    int total = result->toxic_phrase_count;
    if (total == 0) return;
    
    int severe = result->severity_counts[SEVERITY_SEVERE];
    int moderate = result->severity_counts[SEVERITY_MODERATE];
    int mild = result->severity_counts[SEVERITY_MILD];
    
    // Simple ASCII pie chart
    printf("│  Severe:   %2d phrases ", severe);
    for (int i = 0; i < (severe * 20 / total); i++) printf("█");
    printf("\n");
    
    printf("│  Moderate: %2d phrases ", moderate);
    for (int i = 0; i < (moderate * 20 / total); i++) printf("█");
    printf("\n");
    
    printf("│  Mild:     %2d phrases ", mild);
    for (int i = 0; i < (mild * 20 / total); i++) printf("█");
    printf("\n");
    
    printf("└────────────────────────────────────────────────────────┘\n");
}

void compare_results(const AnalysisResult* result1, const AnalysisResult* result2, 
                     const char* filename1, const char* filename2) {
    
    printf("        FILE COMPARISON REPORT\n");
    printf("===========================================\n");
    printf(" File 1: %s\n", filename1);
    printf(" File 2: %s\n", filename2);
    printf("===========================================\n\n");

    printf(" BASIC STATISTICS COMPARISON:\n");
    printf("-------------------------------------------\n");
    printf("%-25s %12s %12s %12s\n", "Metric", "File 1", "File 2", "Difference");
    printf("-------------------------------------------\n");
    
    int word_diff = result2->word_count - result1->word_count;
    printf("%-25s %12d %12d %12d\n", "Word Count", 
           result1->word_count, result2->word_count, word_diff);
    
    int unique_diff = result2->unique_words - result1->unique_words;
    printf("%-25s %12d %12d %12d\n", "Unique Words", 
           result1->unique_words, result2->unique_words, unique_diff);
    
    double lexical1 = result1->advanced_stats.lexical_diversity;
    double lexical2 = result2->advanced_stats.lexical_diversity;
    double lexical_diff = (lexical2 - lexical1) * 100;
    printf("%-25s %11.2f%% %11.2f%% %11.2f%%\n", "Lexical Diversity", 
           lexical1 * 100, lexical2 * 100, lexical_diff);
    
    double word_len1 = result1->avg_word_length;
    double word_len2 = result2->avg_word_length;
    double word_len_diff = word_len2 - word_len1;
    printf("%-25s %11.1f %11.1f %11.1f\n", "Avg Word Length", 
           word_len1, word_len2, word_len_diff);
    
    double sentence_len1 = result1->advanced_stats.avg_sentence_length;
    double sentence_len2 = result2->advanced_stats.avg_sentence_length;
    double sentence_len_diff = sentence_len2 - sentence_len1;
    printf("%-25s %11.1f %11.1f %11.1f\n", "Avg Sentence Length", 
           sentence_len1, sentence_len2, sentence_len_diff);
    
    printf("\n");
    
    // Toxicity analysis comparison
    printf(" TOXICITY ANALYSIS COMPARISON:\n");
    printf("-------------------------------------------\n");
    printf("%-25s %12s %12s %12s\n", "Toxicity Metric", "File 1", "File 2", "Difference");
    printf("-------------------------------------------\n");
    
    int toxic_diff = result2->toxic_phrase_count - result1->toxic_phrase_count;
    printf("%-25s %12d %12d %12d\n", "Toxic Content", 
           result1->toxic_phrase_count, result2->toxic_phrase_count, toxic_diff);
    
    // Calculate toxicity percentage
    double toxic_percent1 = result1->word_count > 0 ? 
        (double)result1->toxic_phrase_count / result1->word_count * 100 : 0;
    double toxic_percent2 = result2->word_count > 0 ? 
        (double)result2->toxic_phrase_count / result2->word_count * 100 : 0;
    double toxic_percent_diff = toxic_percent2 - toxic_percent1;
    
    printf("%-25s %11.2f%% %11.2f%% %11.2f%%\n", "Toxicity Percentage", 
           toxic_percent1, toxic_percent2, toxic_percent_diff);
    
    // Severity breakdown
    printf("\n TOXICITY SEVERITY BREAKDOWN:\n");
    printf("-------------------------------------------\n");
    printf("%-20s %8s %8s %8s\n", "Severity Level", "File 1", "File 2", "Diff");
    printf("-------------------------------------------\n");
    
    const char* severity_names[] = {"Mild", "Moderate", "Severe"};
    for (int i = 0; i < 3; i++) {
        int count1 = result1->severity_counts[i];
        int count2 = result2->severity_counts[i];
        int diff = count2 - count1;
        printf("%-20s %8d %8d %8d\n", severity_names[i], count1, count2, diff);
    }
    
    // Sentiment analysis comparison
    printf("\n SENTIMENT ANALYSIS COMPARISON:\n");
    printf("-------------------------------------------\n");
    printf("%-25s %12s %12s\n", "Sentiment", "File 1", "File 2");
    printf("-------------------------------------------\n");
    
    const char* sentiment1 = result1->toxic_phrase_count == 0 ? "Neutral" : "Negative";
    const char* sentiment2 = result2->toxic_phrase_count == 0 ? "Neutral" : "Negative";
    
    printf("%-25s %12s %12s\n", "Overall Sentiment", sentiment1, sentiment2);
    
    double sentiment_score1 = -toxic_percent1 / 10.0;  
    double sentiment_score2 = -toxic_percent2 / 10.0;
    printf("%-25s %12.3f %12.3f\n", "Sentiment Score", sentiment_score1, sentiment_score2);
    
    printf("\n COMPARISON CONCLUSION:\n");
    printf("-------------------------------------------\n");
    
    if (toxic_percent_diff > 2.0) {
        printf(" File 2 has HIGHER toxicity level than File 1\n");
        if (toxic_percent_diff > 5.0) {
            printf("     Significant difference detected!\n");
        }
    } else if (toxic_percent_diff < -2.0) {
        printf(" File 1 has HIGHER toxicity level than File 2\n");
        if (toxic_percent_diff < -5.0) {
            printf("     Significant difference detected!\n");
        }
    } else {
        printf(" Both files have similar toxicity levels\n");
    }
    
    if (lexical_diff > 1.0) {
        printf(" File 2 has better lexical diversity\n");
    } else if (lexical_diff < -1.0) {
        printf(" File 1 has better lexical diversity\n");
    } else {
        printf(" Both files have similar lexical diversity\n");
    }

    if (fabs(sentence_len_diff) > 0.5) {
        if (sentence_len_diff > 0) {
            printf(" File 2 has more complex sentence structure\n");
        } else {
            printf(" File 1 has more complex sentence structure\n");
        }
    } else {
        printf(" Both files have similar sentence complexity\n");
    }
    
    printf("===========================================\n\n");
    
    // Auto-save comparison report
    save_comparison_report("comparison_report.txt", result1, result2, filename1, filename2);
    printf(" Comparison report saved to: comparison_report.txt\n");
}

void save_comparison_report(const char* filename, const AnalysisResult* result1, 
                           const AnalysisResult* result2, const char* name1, const char* name2) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        printf(" Error: Cannot create comparison report file\n");
        return;
    }
    
    fprintf(file, "===============================================\n");
    fprintf(file, "      CYBERBULLYING TEXT COMPARISON REPORT\n");
    fprintf(file, "===============================================\n\n");
    
    fprintf(file, "Compared Files:\n");
    fprintf(file, "  - File 1: %s\n", name1);
    fprintf(file, "  - File 2: %s\n", name2);
    fprintf(file, "  - Analysis Date: %s\n", __DATE__);
    fprintf(file, "\n");
    
    fprintf(file, "BASIC STATISTICS:\n");
    fprintf(file, "-------------------------------------------------\n");
    fprintf(file, "%-20s | %12s | %12s | %10s\n", "Metric", "File 1", "File 2", "Diff");
    fprintf(file, "-------------------------------------------------\n");
    
    fprintf(file, "%-20s | %12d | %12d | %10d\n", "Total Words", 
            result1->word_count, result2->word_count, 
            result2->word_count - result1->word_count);
    
    fprintf(file, "%-20s | %12d | %12d | %10d\n", "Unique Words", 
            result1->unique_words, result2->unique_words, 
            result2->unique_words - result1->unique_words);
    
    double diversity1 = (double)result1->unique_words / result1->word_count * 100;
    double diversity2 = (double)result2->unique_words / result2->word_count * 100;
    fprintf(file, "%-20s | %11.2f%% | %11.2f%% | %9.2f%%\n", "Lexical Diversity", 
            diversity1, diversity2, diversity2 - diversity1);
    
    fprintf(file, "%-20s | %12.1f | %12.1f | %10.1f\n", "Avg Word Length", 
            result1->avg_word_length, result2->avg_word_length,
            result2->avg_word_length - result1->avg_word_length);
    
    fprintf(file, "%-20s | %12.1f | %12.1f | %10.1f\n", "Avg Sentence Length", 
            result1->advanced_stats.avg_sentence_length, 
            result2->advanced_stats.avg_sentence_length,
            result2->advanced_stats.avg_sentence_length - result1->advanced_stats.avg_sentence_length);
    
    // Toxicity analysis
    fprintf(file, "\nTOXICITY ANALYSIS:\n");
    fprintf(file, "-------------------------------------------------\n");
    fprintf(file, "%-20s | %12s | %12s | %10s\n", "Toxicity Metric", "File 1", "File 2", "Diff");
    fprintf(file, "-------------------------------------------------\n");
    
    double toxic_percent1 = result1->word_count > 0 ? 
        (double)result1->toxic_phrase_count / result1->word_count * 100 : 0;
    double toxic_percent2 = result2->word_count > 0 ? 
        (double)result2->toxic_phrase_count / result2->word_count * 100 : 0;
    
    fprintf(file, "%-20s | %12d | %12d | %10d\n", "Toxic Content", 
            result1->toxic_phrase_count, result2->toxic_phrase_count,
            result2->toxic_phrase_count - result1->toxic_phrase_count);
    
    fprintf(file, "%-20s | %11.2f%% | %11.2f%% | %9.2f%%\n", "Toxicity Percentage", 
            toxic_percent1, toxic_percent2, toxic_percent2 - toxic_percent1);
    
    // Severity details
    fprintf(file, "\nTOXICITY SEVERITY BREAKDOWN:\n");
    fprintf(file, "-------------------------------------------------\n");
    fprintf(file, "%-15s | %8s | %8s | %8s\n", "Severity", "File 1", "File 2", "Diff");
    fprintf(file, "-------------------------------------------------\n");
    
    const char* severity_names[] = {"Mild", "Moderate", "Severe"};
    for (int i = 0; i < 3; i++) {
        fprintf(file, "%-15s | %8d | %8d | %8d\n", severity_names[i],
                result1->severity_counts[i], result2->severity_counts[i],
                result2->severity_counts[i] - result1->severity_counts[i]);
    }
    
    // Conclusion and recommendations
    fprintf(file, "\nCONCLUSION AND RECOMMENDATIONS:\n");
    fprintf(file, "-------------------------------------------------\n");
    
    double toxic_diff = toxic_percent2 - toxic_percent1;
    if (toxic_diff > 2.0) {
        fprintf(file, " File 2 shows significantly higher toxicity levels.\n");
        fprintf(file, " Recommendation: Review and moderate File 2 content.\n");
    } else if (toxic_diff < -2.0) {
        fprintf(file, " File 1 shows significantly higher toxicity levels.\n");
        fprintf(file, " Recommendation: Review and moderate File 1 content.\n");
    } else {
        fprintf(file, " Both files have comparable toxicity levels.\n");
        fprintf(file, " Recommendation: Continue monitoring both.\n");
    }
    
    if (result2->severity_counts[SEVERITY_SEVERE] > result1->severity_counts[SEVERITY_SEVERE]) {
        fprintf(file, "  File 2 contains more severe toxic content.\n");
    } else if (result1->severity_counts[SEVERITY_SEVERE] > result2->severity_counts[SEVERITY_SEVERE]) {
        fprintf(file, "  File 1 contains more severe toxic content.\n");
    }
    
    if (result2->advanced_stats.lexical_diversity > result1->advanced_stats.lexical_diversity + 0.02) {
        fprintf(file, " File 2 has better vocabulary richness.\n");
    } else if (result1->advanced_stats.lexical_diversity > result2->advanced_stats.lexical_diversity + 0.02) {
        fprintf(file, " File 1 has better vocabulary richness.\n");
    }
    
    fprintf(file, "\n===============================================\n");
    fprintf(file, "End of Comparison Report\n");
    fprintf(file, "===============================================\n");
    
    fclose(file);
}