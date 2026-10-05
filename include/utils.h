#ifndef WINERUNNER_UTILS_H
#define WINERUNNER_UTILS_H

#include <stdbool.h>
#include <stddef.h>

/* ANSI Colors and Text Formatting */
#define ANSI_RESET       "\033[0m"
#define ANSI_BOLD        "\033[1m"
#define ANSI_DIM         "\033[2m"
#define ANSI_ITALIC      "\033[3m"
#define ANSI_UNDERLINE   "\033[4m"

#define ANSI_RED         "\033[31m"
#define ANSI_GREEN       "\033[32m"
#define ANSI_YELLOW      "\033[33m"
#define ANSI_BLUE        "\033[34m"
#define ANSI_MAGENTA     "\033[35m"
#define ANSI_CYAN        "\033[36m"
#define ANSI_WHITE       "\033[37m"

#define ANSI_BRIGHT_RED     "\033[91m"
#define ANSI_BRIGHT_GREEN   "\033[92m"
#define ANSI_BRIGHT_YELLOW  "\033[93m"
#define ANSI_BRIGHT_BLUE    "\033[94m"
#define ANSI_BRIGHT_MAGENTA "\033[95m"
#define ANSI_BRIGHT_CYAN    "\033[96m"
#define ANSI_BRIGHT_WHITE   "\033[97m"

/* Path and String Utilities */
char *expand_path(const char *path);
bool file_exists(const char *path);
bool dir_exists(const char *path);
int ensure_dir(const char *path);
char *get_directory_of_file(const char *filepath);
const char *get_filename_from_path(const char *filepath);
void trim_whitespace(char *str);

/* Display and Hardware Detection */
void detect_screen_resolution(char *out_res, size_t max_len);

/* User Interactive Prompts */
void prompt_string(const char *prompt, const char *default_val, char *buffer, size_t max_len);
bool prompt_yes_no(const char *prompt, bool default_val);

#endif /* WINERUNNER_UTILS_H */
