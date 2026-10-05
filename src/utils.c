#define _GNU_SOURCE
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <ctype.h>
#include <limits.h>

char *expand_path(const char *path) {
    if (!path || *path == '\0') {
        return NULL;
    }

    char temp[PATH_MAX];
    if (path[0] == '~') {
        const char *home = getenv("HOME");
        if (!home) {
            struct passwd *pw = getpwuid(getuid());
            if (pw) {
                home = pw->pw_dir;
            }
        }
        if (!home) {
            home = "";
        }

        if (path[1] == '/' || path[1] == '\0') {
            snprintf(temp, sizeof(temp), "%s%s", home, path + 1);
        } else {
            // ~username syntax (rarely needed, fallback)
            snprintf(temp, sizeof(temp), "%s", path);
        }
    } else {
        snprintf(temp, sizeof(temp), "%s", path);
    }

    char resolved[PATH_MAX];
    if (realpath(temp, resolved)) {
        return strdup(resolved);
    }

    return strdup(temp);
}

bool file_exists(const char *path) {
    if (!path || *path == '\0') return false;
    struct stat st;
    if (stat(path, &st) == 0) {
        return S_ISREG(st.st_mode);
    }
    return false;
}

bool dir_exists(const char *path) {
    if (!path || *path == '\0') return false;
    struct stat st;
    if (stat(path, &st) == 0) {
        return S_ISDIR(st.st_mode);
    }
    return false;
}

int ensure_dir(const char *path) {
    if (!path || *path == '\0') return -1;
    char temp[PATH_MAX];
    char *p = NULL;
    size_t len;

    snprintf(temp, sizeof(temp), "%s", path);
    len = strlen(temp);
    if (temp[len - 1] == '/') {
        temp[len - 1] = '\0';
    }

    for (p = temp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(temp, 0755) != 0 && !dir_exists(temp)) {
                return -1;
            }
            *p = '/';
        }
    }

    if (mkdir(temp, 0755) != 0 && !dir_exists(temp)) {
        return -1;
    }

    return 0;
}

char *get_directory_of_file(const char *filepath) {
    if (!filepath) return NULL;
    char *dup = strdup(filepath);
    char *last_slash = strrchr(dup, '/');
    if (last_slash) {
        if (last_slash == dup) {
            *(last_slash + 1) = '\0';
        } else {
            *last_slash = '\0';
        }
        return dup;
    }
    free(dup);
    return strdup(".");
}

const char *get_filename_from_path(const char *filepath) {
    if (!filepath) return "";
    const char *last_slash = strrchr(filepath, '/');
    if (last_slash) {
        return last_slash + 1;
    }
    return filepath;
}

void trim_whitespace(char *str) {
    if (!str) return;
    char *start = str;
    while (isspace((unsigned char)*start)) {
        start++;
    }

    if (*start == '\0') {
        *str = '\0';
        return;
    }

    char *end = start + strlen(start) - 1;
    while (end > start && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }

    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
}

void detect_screen_resolution(char *out_res, size_t max_len) {
    if (!out_res || max_len == 0) return;

    /* Default fallback */
    strncpy(out_res, "1920x1080", max_len - 1);
    out_res[max_len - 1] = '\0';

    /* Try detecting via xrandr */
    FILE *fp = popen("xrandr --current 2>/dev/null", "r");
    if (fp) {
        char line[512];
        while (fgets(line, sizeof(line), fp)) {
            /* Try matching 'Screen 0: ... current 1920 x 1080' */
            char *cur = strstr(line, "current ");
            if (cur) {
                unsigned int w = 0, h = 0;
                if (sscanf(cur, "current %u x %u", &w, &h) == 2 && w > 0 && h > 0) {
                    snprintf(out_res, max_len, "%ux%u", w, h);
                    pclose(fp);
                    return;
                }
            }

            /* Try matching connected primary/active line like 'connected primary 1920x1080' */
            if (strstr(line, " connected")) {
                char *conn = line;
                while (*conn) {
                    unsigned int w = 0, h = 0;
                    if (sscanf(conn, "%ux%u+", &w, &h) == 2 && w > 0 && h > 0) {
                        snprintf(out_res, max_len, "%ux%u", w, h);
                        pclose(fp);
                        return;
                    }
                    conn++;
                }
            }
        }
        pclose(fp);
    }
}

void prompt_string(const char *prompt, const char *default_val, char *buffer, size_t max_len) {
    if (!prompt || !buffer || max_len == 0) return;

    if (default_val && *default_val) {
        printf("%s%s%s [%s%s%s]: ", ANSI_BOLD, prompt, ANSI_RESET, ANSI_CYAN, default_val, ANSI_RESET);
    } else {
        printf("%s%s%s: ", ANSI_BOLD, prompt, ANSI_RESET);
    }
    fflush(stdout);

    char line[1024];
    if (fgets(line, sizeof(line), stdin)) {
        trim_whitespace(line);
        if (*line == '\0' && default_val) {
            strncpy(buffer, default_val, max_len - 1);
            buffer[max_len - 1] = '\0';
        } else {
            strncpy(buffer, line, max_len - 1);
            buffer[max_len - 1] = '\0';
        }
    } else if (default_val) {
        strncpy(buffer, default_val, max_len - 1);
        buffer[max_len - 1] = '\0';
    } else {
        buffer[0] = '\0';
    }
}

bool prompt_yes_no(const char *prompt, bool default_val) {
    printf("%s%s%s [%s]: ", ANSI_BOLD, prompt, ANSI_RESET,
           default_val ? ANSI_CYAN "Y/n" ANSI_RESET : ANSI_CYAN "y/N" ANSI_RESET);
    fflush(stdout);

    char line[128];
    if (fgets(line, sizeof(line), stdin)) {
        trim_whitespace(line);
        if (*line == '\0') {
            return default_val;
        }
        if (line[0] == 'y' || line[0] == 'Y') {
            return true;
        }
        if (line[0] == 'n' || line[0] == 'N') {
            return false;
        }
    }
    return default_val;
}
