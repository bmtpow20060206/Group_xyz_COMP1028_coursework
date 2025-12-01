#ifndef TEXT_ANALYZER_H
#define TEXT_ANALYZER_H
#include <time.h>
#define MAX_WORDS 10000
#define MAX_WORD_LEN 50
#define MAX_PHRASE_LEN 100
#define MAX_SEVERITY_LEVELS 3
#define MAX_TOXIC_PHRASES 1000
#define HASH_TABLE_SIZE 10007  // prime number reduce hash collisions
#define MAX_STOPWORDS 1000
#define MAX_WORD_LEN 50

void init_stopwords();
// sorting algorithm types
typedef enum {
    SORT_BUBBLE = 0,
    SORT_QUICK,
    SORT_MERGE
} SortAlgorithm;

// word info struct
typedef struct WordNode {
    char word[MAX_WORD_LEN];
    int frequency;
    struct WordNode* next;  
} WordNode;

// hash structure
typedef struct {
    WordNode* table[HASH_TABLE_SIZE];
    int size;
} HashTable;

// asvanced stats struct
typedef struct {
    int total_words;
    int unique_words;
    int total_sentences;
    int total_paragraphs;
    double lexical_diversity;
    double avg_sentence_length;
    double avg_word_length;
    int longest_sentence;
    int shortest_sentence;
    double toxic_ratio;          
    double clean_ratio;            
    int clean_word_count; 
} AdvancedStats;

// toxicity severity levels
typedef enum {
    SEVERITY_MILD = 0,
    SEVERITY_MODERATE,
    SEVERITY_SEVERE
} ToxicitySeverity;

// toxic phrase struct
typedef struct {
    char text[MAX_PHRASE_LEN];
    ToxicitySeverity severity;
    int count;
} ToxicPhrase;

// analysis result st
typedef struct {
    int word_count;
    int char_count;
    int line_count;
    int sentence_count;
    int unique_words;
    double avg_word_length;
    double reading_level;
    WordNode** word_freq;  // change to array for sort
    HashTable hash_table;  // new hash table
    
    // advanced stats
    AdvancedStats advanced_stats;
    
    // toxicity stats
    int toxic_phrase_count;
    int severity_counts[MAX_SEVERITY_LEVELS];
    ToxicPhrase detected_toxic_phrases[50];
    int toxic_word_count;           // toxic word count
    double lexical_diversity;       // diversity
    double avg_sentence_length;     // average length
    double sentiment_score;         // mood score
} AnalysisResult;

// basic function
void init_analyzer(void);
AnalysisResult analyze_text(const char* text);
void print_analysis_report(const AnalysisResult* result);
void get_top_words(int n, const AnalysisResult* result);
void save_analysis_report(const char* filename, AnalysisResult* result);
int is_stop_word(const char* word);
void process_word(const char* word, AnalysisResult* result);
void cleanup_analyzer(AnalysisResult* result);

// advanced stats function
void calculate_advanced_stats(const char* text, AdvancedStats* stats);
void print_advanced_stats(const AdvancedStats* stats);


// toxicity detection function
int load_toxic_dictionary(const char* filename);
void add_toxic_phrase(const char* phrase, ToxicitySeverity severity);
int detect_toxic_phrases(const char* text, AnalysisResult* result);
void print_toxicity_report(const AnalysisResult* result);
void save_toxicity_report(const char* filename, const AnalysisResult* result);
const char* get_severity_name(ToxicitySeverity severity);
int calculate_toxicity_score(const AnalysisResult* result);
const char* get_toxicity_level(int score);

// sorting algorithm
void quick_sort(WordNode** array, int low, int high);
void merge_sort(WordNode** array, int left, int right);
void bubble_sort(WordNode** array, int n);
int partition(WordNode** array, int low, int high);
void merge(WordNode** array, int left, int mid, int right);
void sort_words(WordNode** array, int n, SortAlgorithm algorithm);
void compare_sorting_algorithms(WordNode** array, int n);

//hash table function
unsigned int hash_function(const char* word);
void hash_table_init(HashTable* ht);
void hash_table_insert(HashTable* ht, const char* word);
void hash_table_to_array(HashTable* ht, WordNode*** array, int* size);
void hash_table_free(HashTable* ht);

// toxic word sort
void show_most_toxic_words(const AnalysisResult* result, int n);

// structured output
void save_analysis_csv(const char* filename, const AnalysisResult* result);
void save_toxicity_csv(const char* filename, const AnalysisResult* result);
void save_word_frequency_csv(const char* filename, WordNode** words, int count);
void print_results_table(const AnalysisResult* result);
void save_comprehensive_report(const char* base_filename, const AnalysisResult* result);

// enhanced feature 
void print_word_frequency_chart(const AnalysisResult* result, int top_n);
void print_toxicity_pie_chart(const AnalysisResult* result);

// compare
void compare_results(const AnalysisResult* result1, const AnalysisResult* result2, 
                             const char* filename1, const char* filename2);
void save_comparison_report(const char* filename, const AnalysisResult* result1, 
                           const AnalysisResult* result2, const char* name1, const char* name2);

#endif