/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 *
 * contact: https://github.com/vcave
 */

#include <stdio.h>
#include <string.h>

enum {
    OPT_NONE, OPT_NPROC, OPT_BIND, OPT_VERSION, OPT_HELP
};

static struct options {
    char *flag;
    char *env_flag;
    int option;
    char *help;
} hclib_optarray[] = {
    {
        "nproc", "HCLIB_NPROC", OPT_NPROC, "--nproc <n> : the number of processors"
    },
    {
        "bind", "HCLIB_BIND", OPT_BIND, "--bind <bind-file> : thread binding map of worker to cpu"
    },
    {
        "version", "", OPT_VERSION, "--version : print version of HCLIB"
    },
    {
        "help", "", OPT_HELP, "--help : print this message"
    },
    {
        NULL, NULL, 0, NULL
    }
};

static void print_help(void) {
    struct options *p;

    fprintf(stderr, "Usage: program [<HCLIB options>] [<program options>]\n");
    fprintf(stderr, "HCLIB options:\n");

    for (p = hclib_optarray; p->flag; ++p)
        if (p->help)
            fprintf(stderr, "    %s, env: %s\n", p->help, p->env_flag);

    fprintf(stderr, "\n");
    fprintf(stderr, "Contacts: https://github.com/vcave\n");
}

static void print_version(void) {
    fprintf(stderr, "HCLIB Runtime 0.4\n");
}

static struct options *parse_option(char *s) {
    struct options *p = NULL;
    for (p = hclib_optarray; p->flag; ++p) {
        if (strcmp(s, p->flag) == 0) {
            break;
        }
    }
    return p;
}

static void read_bind_file(hclib_options * options, char * bind_file) {
    FILE *fp;
    int temp_map[1024];
    int cpu, n = 0;

    fp=fopen(bind_file, "r");
    while(fscanf(fp, "%d", &cpu) != EOF) {
        temp_map[n] = cpu;
        n++;
        check_log_die((n >= 1024), "cpu set limit reached. Ignoring thread binding.");
    }
    options->bind_map_size = n;
    options->bind_map = (int *)malloc(sizeof(int) * n);
    memcpy(options->bind_map, temp_map, sizeof(int) * n);
}

void read_options_from_env(hclib_options * options) {
    struct options  *p;
    for (p = hclib_optarray; p->flag; ++p) {
        char * opt = getenv(p->env_flag);
        /* Check if the option is defined and has a value */
        if ((opt != NULL) && (strcmp(opt, "") != 0)) {
            switch (p->option) {
            case OPT_NPROC:
                options->nproc = atoi(opt);
                break;
            case OPT_BIND:
            {
                char * bind_file = opt;
                struct stat st;
                if (stat(bind_file, &st) != 0) {
                    fprintf(stderr, "cannot find bind spec file: %s. Using default round-robin binding\n", bind_file);
                    options->bind_map_size = 0;
                } else {
                    read_bind_file(options, bind_file);
                }
            }
            break;
            default: /* The first non-HCLIB option means that we are starting the user program option */
                break;
            }
        }
    }
}

/**
 * Check there's something after an option that takes an argument
 */
static void check_argument(int i, int argc, struct options * p) {
    if (i >= argc) {
      printf("argument missing for %s\n", p->flag);
      exit(1);
    } 
}

/**
 * Check the program command line for HCLIB options.
 * Print a warning if HCLIB options are found after
 * the first non-HCLIB option
 */
static void check_program_command_line(int argc, char *argv[]) {
    int i;
    struct options  *p;
    char * s;
    for (i = 0; i < argc; ++i) {
        if (argv[i][1] == '-')
            s = argv[i] + 2;
        else {
            /* accept deprecated old-style form -X instead of --X */
            s = argv[i] + 1;
        }
        p = parse_option(s);
        if (p->flag != NULL) {
            printf("warning: HCLIB option '%s' detected in arguments will be ignored\n", p->flag);
        }
    }
}

void parse_command_line(hclib_options * options, int * argc_ptr, char *argv[]) {
    int i, j, argc;
    struct options  *p;
    char * s;
    argc = *argc_ptr;
    for (i = 1; i < argc; ++i) {
        if (argv[i][1] == '-')
            s = argv[i] + 2;
        else {
            /* accept deprecated old-style form -X instead of --X */
            s = argv[i] + 1;
        }
        p = parse_option(s);
        switch (p->option) {
        case OPT_NPROC:
            ++i;
            check_argument(i, argc, p);
            options->nproc = atoi(argv[i]);
            check_log_die(options->nproc < 0, "non-positive number of processors");
            break;
        case OPT_BIND:
        {
            ++i;
            if (i > argc) {
                fprintf(stderr, "No bind spec file provided. Using default round-robin binding\n");
                options->bind_map_size = 0;
            } else {
                char * bind_file = argv[i];
                struct stat st;
                if (stat(bind_file, &st) != 0) {
                    fprintf(stderr, "cannot find bind spec file: %s. Using default round-robin binding\n", bind_file);
                    options->bind_map_size = 0;
                    --i;
                } else {
                    read_bind_file(options, bind_file);
                }
            }
            break;
        }
        case OPT_HELP:
            print_help();
            exit(0);
            break;
        case OPT_VERSION:
            print_version();
            exit(0);
            break;
        default:  /* The first non-HCLIB option means that we are starting the user program option */
            goto stop;
        }
    }

    stop:
    /* remove hc command-line arguments from argv */
    for (j = i; j < argc; j++) {
        argv[j - (i - 1)] = argv[j];
    }
    *argc_ptr -= i - 1;

    /* Check the remaining arguments (should not contain HCLIB options anymore) */
    check_program_command_line(*argc_ptr, argv);
}

void read_options (hclib_options * options, int * argc, char *argv[]){
    /* default options */
    hclib_options opt = hc_defaultOptions;
    *options = opt;
    read_options_from_env(options);
    if (argc != NULL && *argc != 0 && argv != NULL) parse_command_line(options, argc, argv);
}
