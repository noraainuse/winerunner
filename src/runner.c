#define _GNU_SOURCE
#include "runner.h"
#include "utils.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <dirent.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <limits.h>

static bool is_wine_installed(void) {
    const char *path_env = getenv("PATH");
    if (!path_env) return false;

    char *path_copy = strdup(path_env);
    char *token = strtok(path_copy, ":");
    while (token) {
        char fullpath[PATH_MAX];
        snprintf(fullpath, sizeof(fullpath), "%s/wine", token);
        if (access(fullpath, X_OK) == 0) {
            free(path_copy);
            return true;
        }
        token = strtok(NULL, ":");
    }
    free(path_copy);
    return false;
}

static void apply_env_vars(const char *env_str) {
    if (!env_str || *env_str == '\0') return;

    char *dup = strdup(env_str);
    char *token = strtok(dup, " ");
    while (token) {
        char *eq = strchr(token, '=');
        if (eq) {
            *eq = '\0';
            setenv(token, eq + 1, 1);
        }
        token = strtok(NULL, " ");
    }
    free(dup);
}

bool runner_is_running(const GameConfig *game, pid_t *out_pid) {
    if (!game || game->exe_path[0] == '\0') return false;

    const char *exe_name = get_filename_from_path(game->exe_path);
    if (!exe_name || *exe_name == '\0') return false;

    DIR *dir = opendir("/proc");
    if (!dir) return false;

    struct dirent *ent;
    pid_t my_pid = getpid();

    while ((ent = readdir(dir)) != NULL) {
        if (!isdigit((unsigned char)ent->d_name[0])) continue;

        pid_t pid = (pid_t)atoi(ent->d_name);
        if (pid == my_pid) continue;

        char cmdline_path[PATH_MAX];
        snprintf(cmdline_path, sizeof(cmdline_path), "/proc/%s/cmdline", ent->d_name);

        FILE *fp = fopen(cmdline_path, "r");
        if (!fp) continue;

        char buf[2048];
        size_t n = fread(buf, 1, sizeof(buf) - 1, fp);
        fclose(fp);

        if (n == 0) continue;
        buf[n] = '\0';

        /* Scan null-delimited arguments */
        for (size_t i = 0; i < n;) {
            const char *arg = buf + i;
            size_t arg_len = strlen(arg);

            const char *arg_base = get_filename_from_path(arg);
            if (strcasecmp(arg_base, exe_name) == 0) {
                if (out_pid) *out_pid = pid;
                closedir(dir);
                return true;
            }

            i += arg_len + 1;
        }
    }

    closedir(dir);
    return false;
}

int runner_kill(const GameConfig *game) {
    if (!game) return -1;

    const char *exe_name = get_filename_from_path(game->exe_path);
    printf("%sChecking for running instances of '%s' (%s)...%s\n",
           ANSI_DIM, game->name, exe_name, ANSI_RESET);

    DIR *dir = opendir("/proc");
    if (!dir) {
        perror("Failed to open /proc");
        return -1;
    }

    struct dirent *ent;
    pid_t pids[64];
    size_t num_pids = 0;
    pid_t my_pid = getpid();

    while ((ent = readdir(dir)) != NULL && num_pids < 64) {
        if (!isdigit((unsigned char)ent->d_name[0])) continue;

        pid_t pid = (pid_t)atoi(ent->d_name);
        if (pid == my_pid) continue;

        char cmdline_path[PATH_MAX];
        snprintf(cmdline_path, sizeof(cmdline_path), "/proc/%s/cmdline", ent->d_name);

        FILE *fp = fopen(cmdline_path, "r");
        if (!fp) continue;

        char buf[2048];
        size_t n = fread(buf, 1, sizeof(buf) - 1, fp);
        fclose(fp);

        if (n == 0) continue;
        buf[n] = '\0';

        for (size_t i = 0; i < n;) {
            const char *arg = buf + i;
            size_t arg_len = strlen(arg);

            const char *arg_base = get_filename_from_path(arg);
            if (strcasecmp(arg_base, exe_name) == 0) {
                pids[num_pids++] = pid;
                break;
            }

            i += arg_len + 1;
        }
    }
    closedir(dir);

    if (num_pids == 0) {
        printf("%s○ No running instances found for '%s'.%s\n",
               ANSI_YELLOW, game->name, ANSI_RESET);
        return 0;
    }

    for (size_t i = 0; i < num_pids; i++) {
        printf("%sTerminating PID %d...%s\n", ANSI_YELLOW, pids[i], ANSI_RESET);
        kill(pids[i], SIGTERM);
    }

    /* Wait up to 1.5 seconds */
    usleep(500000);

    /* Check if still alive, send SIGKILL */
    for (size_t i = 0; i < num_pids; i++) {
        if (kill(pids[i], 0) == 0) {
            printf("%sForce killing PID %d (SIGKILL)...%s\n", ANSI_RED, pids[i], ANSI_RESET);
            kill(pids[i], SIGKILL);
        }
    }

    printf("%s✓ Successfully stopped %s.%s\n", ANSI_GREEN, game->name, ANSI_RESET);
    return 0;
}

int runner_show_logs(const GameConfig *game, int num_lines) {
    if (!game) return -1;

    char log_file[PATH_MAX];
    snprintf(log_file, sizeof(log_file), "%s/%s.log", config_get_logs_dir(), game->name);

    if (!file_exists(log_file)) {
        printf("%s○ No logs found for '%s' yet (%s).%s\n",
               ANSI_YELLOW, game->name, log_file, ANSI_RESET);
        return 0;
    }

    printf("%s═══════════════════════════════════════════════════════════════%s\n", ANSI_DIM, ANSI_RESET);
    printf("%s Logs for %s%s%s (%s)%s\n",
           ANSI_BOLD, ANSI_CYAN, game->name, ANSI_RESET, log_file, ANSI_RESET);
    printf("%s═══════════════════════════════════════════════════════════════%s\n", ANSI_DIM, ANSI_RESET);

    char cmd[PATH_MAX + 64];
    snprintf(cmd, sizeof(cmd), "tail -n %d \"%s\"", num_lines > 0 ? num_lines : 40, log_file);
    int ret = system(cmd);
    (void)ret;

    printf("%s═══════════════════════════════════════════════════════════════%s\n", ANSI_DIM, ANSI_RESET);
    return 0;
}

int runner_launch(const GameConfig *game, const RunOptions *opts) {
    if (!game) return -1;

    if (!is_wine_installed()) {
        fprintf(stderr, "%sError: 'wine' is not installed or not in your PATH.%s\n",
                ANSI_BOLD ANSI_RED, ANSI_RESET);
        return -1;
    }

    if (!file_exists(game->exe_path)) {
        fprintf(stderr, "%sError: Executable not found:%s %s\n",
                ANSI_BOLD ANSI_RED, ANSI_RESET, game->exe_path);
        return -1;
    }

    /* Working directory */
    char workdir[PATH_MAX];
    if (game->workdir[0] != '\0') {
        strncpy(workdir, game->workdir, sizeof(workdir) - 1);
        workdir[sizeof(workdir) - 1] = '\0';
    } else {
        char *dir = get_directory_of_file(game->exe_path);
        if (dir) {
            strncpy(workdir, dir, sizeof(workdir) - 1);
            workdir[sizeof(workdir) - 1] = '\0';
            free(dir);
        } else {
            strcpy(workdir, ".");
        }
    }

    /* Determine resolution */
    char res[MAX_RES_LEN];
    if (opts && opts->override_res[0] != '\0') {
        strncpy(res, opts->override_res, sizeof(res) - 1);
    } else if (game->resolution[0] != '\0') {
        strncpy(res, game->resolution, sizeof(res) - 1);
    } else {
        detect_screen_resolution(res, sizeof(res));
    }
    res[sizeof(res) - 1] = '\0';

    /* Determine desktop name */
    const char *desktop_name = (game->desktop_name[0] != '\0') ? game->desktop_name : game->name;

    /* Virtual desktop flag */
    bool use_desktop = game->virtual_desktop;
    if (opts && opts->force_no_desktop) {
        use_desktop = false;
    }

    /* Check if already running */
    pid_t existing_pid;
    if (runner_is_running(game, &existing_pid)) {
        printf("%s⚠️  Notice: '%s' is already running (PID: %d).%s\n",
               ANSI_YELLOW, game->name, existing_pid, ANSI_RESET);
        printf("Launch another instance anyway? ");
        if (!prompt_yes_no("", false)) {
            printf("Aborted.\n");
            return 0;
        }
    }

    /* Prepare log path */
    ensure_dir(config_get_logs_dir());
    char log_file[PATH_MAX];
    snprintf(log_file, sizeof(log_file), "%s/%s.log", config_get_logs_dir(), game->name);

    /* Construct arguments */
    const char *exe_filename = get_filename_from_path(game->exe_path);
    char desktop_arg[128];
    snprintf(desktop_arg, sizeof(desktop_arg), "/desktop=%s,%s", desktop_name, res);

    char *argv[32];
    int argc = 0;
    argv[argc++] = "wine";

    if (use_desktop) {
        argv[argc++] = "explorer";
        argv[argc++] = desktop_arg;
    }

    /* Pass executable name (we cd into workdir first) */
    argv[argc++] = (char *)exe_filename;

    /* Extra args if present */
    char extra_buf[MAX_ARGS_LEN];
    if (game->extra_args[0] != '\0') {
        strncpy(extra_buf, game->extra_args, sizeof(extra_buf) - 1);
        extra_buf[sizeof(extra_buf) - 1] = '\0';
        char *tok = strtok(extra_buf, " ");
        while (tok && argc < 30) {
            argv[argc++] = tok;
            tok = strtok(NULL, " ");
        }
    }
    argv[argc] = NULL;

    bool is_foreground = (opts && opts->foreground);

    if (is_foreground) {
        printf("\n%s🚀 Launching %s%s%s in FOREGROUND...%s\n",
               ANSI_BOLD, ANSI_CYAN, game->name, ANSI_RESET ANSI_BOLD, ANSI_RESET);
        printf("   %s•%s Executable : %s%s%s\n", ANSI_DIM, ANSI_RESET, ANSI_BOLD, game->exe_path, ANSI_RESET);
        printf("   %s•%s Working Dir: %s\n", ANSI_DIM, ANSI_RESET, workdir);
        if (use_desktop) {
            printf("   %s•%s Display    : Virtual Desktop (%s, %s)\n", ANSI_DIM, ANSI_RESET, desktop_name, res);
        } else {
            printf("   %s•%s Display    : Direct Window (No Virtual Desktop)\n", ANSI_DIM, ANSI_RESET);
        }
        if (game->wineprefix[0] != '\0') {
            printf("   %s•%s Prefix     : %s\n", ANSI_DIM, ANSI_RESET, game->wineprefix);
        }
        printf("%s(Logs will stream to terminal. Press Ctrl+C to exit)%s\n\n", ANSI_DIM, ANSI_RESET);

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork failed");
            return -1;
        }

        if (pid == 0) {
            /* Child */
            if (chdir(workdir) != 0) {
                perror("chdir failed");
                _exit(1);
            }

            if (game->wineprefix[0] != '\0') {
                char *exp_prefix = expand_path(game->wineprefix);
                if (exp_prefix) {
                    setenv("WINEPREFIX", exp_prefix, 1);
                    free(exp_prefix);
                }
            }

            if (!opts || !opts->debug_wine) {
                /* Suppress wine spam unless requested */
                if (!getenv("WINEDEBUG")) {
                    setenv("WINEDEBUG", "-all", 1);
                }
            }

            apply_env_vars(game->env_vars);

            execvp("wine", argv);
            perror("execvp wine failed");
            _exit(127);
        }

        /* Parent */
        int status;
        waitpid(pid, &status, 0);
        return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    } else {
        /* Detached background mode */
        printf("\n%s🚀 Launching %s%s%s%s\n",
               ANSI_BOLD, ANSI_CYAN, game->name, ANSI_RESET, ANSI_BOLD);
        printf("   %s•%s Executable : %s%s%s\n", ANSI_DIM, ANSI_RESET, ANSI_BOLD, game->exe_path, ANSI_RESET);
        printf("   %s•%s Working Dir: %s\n", ANSI_DIM, ANSI_RESET, workdir);
        if (use_desktop) {
            printf("   %s•%s Display    : Virtual Desktop (%s, %s)\n", ANSI_DIM, ANSI_RESET, desktop_name, res);
        } else {
            printf("   %s•%s Display    : Direct Window\n", ANSI_DIM, ANSI_RESET);
        }
        if (game->wineprefix[0] != '\0') {
            printf("   %s•%s Prefix     : %s\n", ANSI_DIM, ANSI_RESET, game->wineprefix);
        }
        printf("   %s•%s Log File   : %s%s%s\n", ANSI_DIM, ANSI_RESET, ANSI_CYAN, log_file, ANSI_RESET);
        printf("   %s•%s Mode       : Background / Detached\n", ANSI_DIM, ANSI_RESET);

        int log_fd = open(log_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (log_fd < 0) {
            perror("Failed to open log file");
            return -1;
        }

        /* Log launch timestamp header */
        time_t now = time(NULL);
        char time_str[64];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&now));
        dprintf(log_fd, "\n=======================================================\n");
        dprintf(log_fd, "=== Launch: %s at %s ===\n", game->name, time_str);
        dprintf(log_fd, "=== Exe: %s\n", game->exe_path);
        dprintf(log_fd, "=== Desktop: %s (%s)\n", use_desktop ? desktop_name : "none", res);
        dprintf(log_fd, "=======================================================\n");

        /* Double fork daemonization */
        pid_t pid1 = fork();
        if (pid1 < 0) {
            perror("fork failed");
            close(log_fd);
            return -1;
        }

        if (pid1 > 0) {
            /* First parent waits for immediate child */
            int status;
            waitpid(pid1, &status, 0);
            close(log_fd);

            printf("\n%s✨ '%s' launched successfully!%s\n", ANSI_BOLD ANSI_GREEN, game->name, ANSI_RESET);
            printf("%s💡 View logs with:   %swinerunner logs %s%s\n",
                   ANSI_DIM, ANSI_WHITE, game->name, ANSI_RESET);
            printf("%s🛑 Stop game with:   %swinerunner kill %s%s\n\n",
                   ANSI_DIM, ANSI_WHITE, game->name, ANSI_RESET);
            return 0;
        }

        /* Child 1 */
        setsid();

        pid_t pid2 = fork();
        if (pid2 < 0) {
            _exit(1);
        }

        if (pid2 > 0) {
            /* Child 1 exits so Child 2 is adopted by init */
            _exit(0);
        }

        /* Grandchild (Child 2) */
        if (chdir(workdir) != 0) {
            dprintf(log_fd, "Error: chdir to %s failed\n", workdir);
            _exit(1);
        }

        /* Redirect stdin from /dev/null */
        int devnull = open("/dev/null", O_RDONLY);
        if (devnull >= 0) {
            dup2(devnull, STDIN_FILENO);
            close(devnull);
        }

        /* Redirect stdout & stderr to log file */
        dup2(log_fd, STDOUT_FILENO);
        dup2(log_fd, STDERR_FILENO);
        close(log_fd);

        if (game->wineprefix[0] != '\0') {
            char *exp_prefix = expand_path(game->wineprefix);
            if (exp_prefix) {
                setenv("WINEPREFIX", exp_prefix, 1);
                free(exp_prefix);
            }
        }

        if (!opts || !opts->debug_wine) {
            if (!getenv("WINEDEBUG")) {
                setenv("WINEDEBUG", "-all", 1);
            }
        }

        apply_env_vars(game->env_vars);

        execvp("wine", argv);
        fprintf(stderr, "Fatal error executing wine: %s\n", strerror(errno));
        _exit(127);
    }

    return 0;
}
