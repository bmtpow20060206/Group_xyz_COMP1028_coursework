#ifndef FILE_PROCESSOR_H
#define FILE_PROCESSOR_H


char* read_text_file(const char* filename);
char* csv_column_to_text(const char* filename, int column_index);
char* csv_all_columns_to_text(const char* filename);
void preview_csv_columns(const char* filename);
int file_exists(const char* filename);
int extract_csv_column_to_file(const char* csv_filename, int column_index, const char* output_txt_filename);
int extract_csv_all_to_file(const char* csv_filename, const char* output_txt_filename);


char* process_multiple_files(const char** filenames, int file_count);
char* read_large_file(const char* filename);
int is_file_empty(const char* filename);
long get_file_size(const char* filename);

#endif