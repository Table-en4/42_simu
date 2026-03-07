#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <glob.h>
#include <libgen.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <stdarg.h>
#include <readline/readline.h>
#include <readline/history.h>

typedef enum {
    STATE_CHOOSE_RANK,
    STATE_CHOOSE_MODE,
    STATE_CUSTOM_CHOOSE_EXO,
    STATE_IN_EXAM
} ProgramState;

int     current_rank = 0;
int     current_level = 0;
int     total_levels = 0;
int     score = 0;
int     points_per_level = 0;
int     is_exam_mode = 0;
char    current_exo_name[256] = "";
char    current_exo_path[512] = "";
char    exam_exos[16][512];
char    exam_exo_names[16][256];
char    g_level_names[16][64];


int get_term_width() {
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1)
        return 80;
    return w.ws_col;
}

size_t visible_len(const char *str) {
    size_t len = 0;
    int in_ansi = 0;
    for (size_t i = 0; str[i]; i++) {
        if (str[i] == '\001' || str[i] == '\002') {
            continue;
        } else if (str[i] == '\033' && str[i+1] == '[') {
            in_ansi = 1;
        } else if (in_ansi && ((str[i] >= 'a' && str[i] <= 'z') || (str[i] >= 'A' && str[i] <= 'Z'))) {
            in_ansi = 0;
        } else if (!in_ansi) {
            if ((str[i] & 0xC0) != 0x80) len++; 
        }
    }
    return len;
}

void print_centered(const char *format, ...) {
    char buffer[8192];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    int term_width = get_term_width();
    char *line = buffer;
    char *next_line;

    while ((next_line = strchr(line, '\n')) != NULL) {
        *next_line = '\0';
        int v_len = visible_len(line);
        int padding = (term_width > v_len) ? (term_width - v_len) / 2 : 0;
        if (padding > 0) printf("%*s", padding, "");
        printf("%s\n", line);
        line = next_line + 1;
    }
    if (*line) {
        int v_len = visible_len(line);
        int padding = (term_width > v_len) ? (term_width - v_len) / 2 : 0;
        if (padding > 0) printf("%*s", padding, "");
        printf("%s", line);
    }
}


int get_level_names(int rank, char lnames[16][64]) {
    glob_t globbuf;
    char pattern[128];
    int result;
    int count = 0;
    const char *prefixes[] = {".secret/", "simu/.secret/", "42_simu/.secret/"};

    for (int i = 0; i < 3; i++) {
        sprintf(pattern, "%srank%02d/level*/", prefixes[i], rank);
        result = glob(pattern, GLOB_NOSORT, NULL, &globbuf);
        if (result == 0 && globbuf.gl_pathc > 0) {
            for (size_t j = 0; j < globbuf.gl_pathc && j < 16; j++) {
                char tmp[512];
                strcpy(tmp, globbuf.gl_pathv[j]);
                size_t len = strlen(tmp);
                if (len > 0 && tmp[len - 1] == '/')
                    tmp[len - 1] = '\0';
                strcpy(lnames[j], basename(tmp));
                count++;
            }
            globfree(&globbuf);
            return count;
        }
        globfree(&globbuf);
    }
    return 0;
}

int get_exercises_for_level_name(int rank, char *level_name, glob_t *globbuf) {
    char pattern[256];
    int result;
    const char *prefixes[] = {".secret/", "simu/.secret/", "42_simu/.secret/"};

    for (int i = 0; i < 3; i++) {
        sprintf(pattern, "%srank%02d/%s/*/", prefixes[i], rank, level_name);
        result = glob(pattern, GLOB_NOSORT, NULL, globbuf);
        if (result == 0 && globbuf->gl_pathc > 0) {
            for (size_t j = 0; j < globbuf->gl_pathc; j++) {
                size_t len = strlen(globbuf->gl_pathv[j]);
                if (len > 0 && globbuf->gl_pathv[j][len - 1] == '/')
                    globbuf->gl_pathv[j][len - 1] = '\0';
            }
            return 1;
        }
        globfree(globbuf);
    }
    return 0;
}

int get_exercises(int rank, glob_t *globbuf) {
    char pattern[128];
    int result;
    const char *prefixes[] = {".secret/", "simu/.secret/", "42_simu/.secret/"};

    for (int i = 0; i < 3; i++) {
        sprintf(pattern, "%srank%02d/level*/*/", prefixes[i], rank);
        result = glob(pattern, GLOB_NOSORT, NULL, globbuf);
        if (result == 0 && globbuf->gl_pathc > 0) {
            for (size_t j = 0; j < globbuf->gl_pathc; j++) {
                size_t len = strlen(globbuf->gl_pathv[j]);
                if (len > 0 && globbuf->gl_pathv[j][len - 1] == '/')
                    globbuf->gl_pathv[j][len - 1] = '\0';
            }
            return 1;
        }
        globfree(globbuf);
    }
    return 0;
}

int get_level_count(int rank) {
    glob_t globbuf;
    char pattern[128];
    int result;
    int count = 0;
    const char *prefixes[] = {".secret/", "simu/.secret/", "42_simu/.secret/"};

    for (int i = 0; i < 3; i++) {
        sprintf(pattern, "%srank%02d/level*/", prefixes[i], rank);
        result = glob(pattern, GLOB_NOSORT, NULL, &globbuf);
        if (result == 0 && globbuf.gl_pathc > 0) {
            count = globbuf.gl_pathc;
            globfree(&globbuf);
            return count;
        }
        globfree(&globbuf);
    }
    return 0;
}

void build_exam(int rank, int nb_levels) {
    get_level_names(rank, g_level_names);
    for (int lvl = 0; lvl < nb_levels; lvl++) {
        glob_t globbuf;
        if (get_exercises_for_level_name(rank, g_level_names[lvl], &globbuf) && globbuf.gl_pathc > 0) {
            int idx = rand() % globbuf.gl_pathc;
            strcpy(exam_exos[lvl], globbuf.gl_pathv[idx]);
            char tmp[512];
            strcpy(tmp, globbuf.gl_pathv[idx]);
            strcpy(exam_exo_names[lvl], basename(tmp));
        } else {
            exam_exos[lvl][0] = '\0';
            exam_exo_names[lvl][0] = '\0';
        }
        globfree(&globbuf);
    }
}

void start_level(int lvl) {
    strcpy(current_exo_path, exam_exos[lvl]);
    strcpy(current_exo_name, exam_exo_names[lvl]);
    printf("\n\033[1;36m╭──────────────────────────────────────────────╮\033[0m\n");
    printf("\033[1;36m│\033[0m  NIVEAU %-2d | EXERCICE : \033[1;32m%-20s\033[0m\033[1;36m │\033[0m\n", lvl, current_exo_name);
    printf("\033[1;36m│\033[0m  SCORE     | ACTUEL   : \033[1;33m%-3d\033[0m/100              \033[1;36m│\033[0m\n", score);
    printf("\033[1;36m╰──────────────────────────────────────────────╯\033[0m\n");
    printf(" 💡 Commandes : \033[33msubject\033[0m : afficher le sujet de l'exam\n                \033[33mtest\033[0m : tester votre code\n                \033[33mcheat\033[0m : validé l'exo pour passer au suivant\n                \033[33mfinish\033[0m : quitter l'exam\n\n");
}

void start_random_exam(void) {
    score = 0;
    current_level = 0;
    points_per_level = (total_levels > 0) ? 100 / total_levels : 0;
    is_exam_mode = 1;
    build_exam(current_rank, total_levels);
    start_level(current_level);
}

void show_exam_end(void) {
    print_centered("\n\033[1;32m╔══════════════════════════════════════════════╗\033[0m\n");
    print_centered("\033[1;32m║               EXAMEN TERMINÉ !               ║\033[0m\n");
    print_centered("\033[1;32m║               Score final : %-3d/100          ║\033[0m\n", score);
    print_centered("\033[1;32m╚══════════════════════════════════════════════╝\033[0m\n");
    system("rm -rf rendu/* 2>/dev/null; mkdir -p rendu");
    print_centered("\nQue souhaitez-vous faire ?\n");
    print_centered(" ├─ \033[1mretry\033[0m  : Recommencer l'examen\n");
    print_centered(" ├─ \033[1mback\033[0m   : Choisir un autre rang\n");
    print_centered(" └─ \033[1mfinish\033[0m : Quitter\n\n");
}

void print_custom_list(int rank) {
    glob_t globbuf;
    if (get_exercises(rank, &globbuf) && globbuf.gl_pathc > 0) {
        printf("\nExercices disponibles pour le rang %d :\n", rank);
        for (size_t i = 0; i < globbuf.gl_pathc; i++) {
            char temp_path[512];
            strcpy(temp_path, globbuf.gl_pathv[i]);
            printf(" • %s\n", basename(temp_path));
        }
        printf("\nTapez le nom de l'exercice :\n");
    }
    globfree(&globbuf);
}


int main(void) {
    char *input;
    ProgramState current_state = STATE_CHOOSE_RANK;

    srand(time(NULL));

    printf("\033[2J\033[H"); // Nettoyage initial

    // Centré au lancement
    print_centered("\n\033[1;36m╔══════════════════════════════════════════════╗\033[0m\n");
    print_centered("\033[1;36m║              42 EXAM SIMULATOR               ║\033[0m\n");
    print_centered("\033[1;36m╚══════════════════════════════════════════════╝\033[0m\n\n");
    print_centered("Veuillez choisir un rang (2, 3, 4, 5, 6) :\n");

    while (1) {
        char raw_prompt[512];

        // Formatage des prompts
        if (current_state == STATE_CHOOSE_RANK)
            snprintf(raw_prompt, sizeof(raw_prompt), "\001\033[1;36m\002examshell\001\033[0m\002> ");
        else if (current_state == STATE_CHOOSE_MODE || current_state == STATE_CUSTOM_CHOOSE_EXO)
            snprintf(raw_prompt, sizeof(raw_prompt), "\001\033[1;36m\002examshell\001\033[0m\002/rank%02d> ", current_rank);
        else if (current_state == STATE_IN_EXAM) {
            if (is_exam_mode)
                snprintf(raw_prompt, sizeof(raw_prompt), "\001\033[1;32m\002%.100s\001\033[0m\002 [\001\033[1;33m\002%d pts\001\033[0m\002]> ", current_exo_name, score);
            else
                snprintf(raw_prompt, sizeof(raw_prompt), "\001\033[1;32m\002%.100s\001\033[0m\002> ", current_exo_name);
        }

        // On centre dynamiquement le prompt Readline SEULEMENT pour les menus initiaux
        if (current_state == STATE_CHOOSE_RANK || current_state == STATE_CHOOSE_MODE) {
            int term_width = get_term_width();
            int v_len = visible_len(raw_prompt);
            int padding = (term_width > v_len) ? (term_width - v_len) / 2 : 0;
            
            char centered_prompt[1024] = "";
            if (padding > 0) {
                memset(centered_prompt, ' ', padding);
                centered_prompt[padding] = '\0';
            }
            strcat(centered_prompt, raw_prompt);
            input = readline(centered_prompt);
        } else {
            // Affichage normal (gauche) pour les autres états
            input = readline(raw_prompt);
        }

        if (!input) {
            printf("\nFermeture du simulateur...\n");
            break;
        }
        
        char *trimmed_input = input;
        while (*trimmed_input == ' ') trimmed_input++;

        if (strlen(trimmed_input) > 0)
            add_history(trimmed_input);

        if (strcmp(trimmed_input, "finish") == 0) {
            system("rm -rf rendu/* 2>/dev/null; mkdir -p rendu");
            print_centered("\n\033[1;34mClean de l'exam, merci pour votre participation !!!\033[0m\n");
            free(input);
            break;
        }

        if (current_state == STATE_CHOOSE_RANK) {
            current_rank = atoi(trimmed_input);
            if (current_rank >= 2 && current_rank <= 6) {
                total_levels = get_level_count(current_rank);
                points_per_level = (total_levels > 0) ? 100 / total_levels : 0;
                score = 0;
                
                print_centered("\nRang %d sélectionné.\n", current_rank);
                if (total_levels > 0)
                    print_centered("\033[1;33mCet examen contient %d niveaux.\033[0m\n", total_levels);
                else
                    print_centered("\033[1;31mAttention : Aucun niveau trouvé.\033[0m\n");
                
                print_centered("\nModes disponibles :\n");
                print_centered(" - \033[1mrandom\033[0m (Un exercice au hasard par niveau)\n");
                print_centered(" - \033[1mcustom\033[0m (Choisir un exercice spécifique)\n\n");
                current_state = STATE_CHOOSE_MODE;
            } else {
                print_centered("\033[1;31mRang invalide. Veuillez choisir un rang entre 2 et 6.\033[0m\n\n");
            }
        }
        else if (current_state == STATE_CHOOSE_MODE) {
            if (strcmp(trimmed_input, "random") == 0 || strcmp(trimmed_input, "retry") == 0) {
                if (total_levels > 0) {
                    start_random_exam();
                    current_state = STATE_IN_EXAM;
                } else
                    print_centered("\033[1;31mAucun exercice trouvé pour le rang %d.\033[0m\n\n", current_rank);
            }
            else if (strcmp(trimmed_input, "custom") == 0) {
                print_custom_list(current_rank);
                current_state = STATE_CUSTOM_CHOOSE_EXO;
            }
            else if (strcmp(trimmed_input, "back") == 0) {
                current_state = STATE_CHOOSE_RANK;
                print_centered("\nVeuillez choisir un rang (2, 3, 4, 5, 6) :\n");
            }
            else {
                print_centered("\033[1;31mCommande inconnue. Tapez 'random', 'custom', 'retry' ou 'back'.\033[0m\n\n");
            }
        }
        else if (current_state == STATE_CUSTOM_CHOOSE_EXO) {
            glob_t globbuf;
            int found = 0;
            if (get_exercises(current_rank, &globbuf)) {
                for (size_t i = 0; i < globbuf.gl_pathc; i++) {
                    char temp_path[512];
                    strcpy(temp_path, globbuf.gl_pathv[i]);
                    char *exo_name = basename(temp_path);
                    if (strcmp(trimmed_input, exo_name) == 0) {
                        strcpy(current_exo_name, exo_name);
                        strcpy(current_exo_path, globbuf.gl_pathv[i]);
                        found = 1;
                        break;
                    }
                }
            }
            globfree(&globbuf);
            if (found) {
                is_exam_mode = 0;
                score = 0;
                printf("\n\033[1;36m╭──────────────────────────────────────────────╮\033[0m\n");
                printf("\033[1;36m│\033[0m ASSIGNATION DE L'EXERCICE : \033[1;32m%-16s\033[0m\033[1;36m│\033[0m\n", current_exo_name);
                printf("\033[1;36m╰──────────────────────────────────────────────╯\033[0m\n");
                printf(" 💡 Commandes : 'subject' : afficher le sujet de l'exam\n 'test' : tester votre code\n 'cheat' : validé l'exo pour passer au suivant\n 'back': revenir au menu\n\n");
                current_state = STATE_IN_EXAM;
            }
            else if (strcmp(trimmed_input, "back") == 0) {
                current_state = STATE_CHOOSE_MODE;
                print_centered("\nModes : 'random' ou 'custom'\n\n");
            }
            else {
                printf("\033[1;31mExercice introuvable. Nom exact requis, ou 'back'.\033[0m\n\n");
            }
        }
        else if (current_state == STATE_IN_EXAM) {
            if (strcmp(trimmed_input, "test") == 0 || strcmp(trimmed_input, "grademe") == 0) {
                char cmd[1024];
                printf("\033[1;33m=> Lancement du testeur pour %s...\033[0m\n", current_exo_name);
                snprintf(cmd, sizeof(cmd), "cd %s && bash tester.sh", current_exo_path);
                int ret = system(cmd);
                if (WIFEXITED(ret) && WEXITSTATUS(ret) == 0) {
                    if (is_exam_mode) {
                        score += points_per_level;
                        printf("\033[1;32m✓ Succès ! +%d pts → Score : %d/100\033[0m\n", points_per_level, score);
                        current_level++;
                        if (current_level >= total_levels) {
                            show_exam_end();
                            is_exam_mode = 0;
                            current_state = STATE_CHOOSE_MODE;
                            // Ré-affiche le menu au centre pour une transition propre
                            print_centered("\nModes : 'random' ou 'custom'\n\n");
                            free(input);
                            continue;
                        }
                        start_level(current_level);
                    }
                    else {
                        printf("\033[1;32m✓ PASSED ! Tapez 'back' pour choisir un autre exercice.\033[0m\n\n");
                    }
                }
                else {
                    printf("\033[1;31m✖ Échec des tests. Réessayez !\033[0m\n\n");
                }
            }
            else if (strcmp(trimmed_input, "cheat") == 0) {
                if (is_exam_mode) {
                    score += points_per_level;
                    printf("\033[1;35m🤫 [CHEAT] +%d pts → Score : %d/100\033[0m\n", points_per_level, score);
                    current_level++;
                    if (current_level >= total_levels) {
                        show_exam_end();
                        is_exam_mode = 0;
                        current_state = STATE_CHOOSE_MODE;
                        print_centered("\nModes : 'random' ou 'custom'\n\n");
                        free(input);
                        continue;
                    }
                    start_level(current_level);
                }
                else {
                    printf("\033[1;35m🤫 [CHEAT] Exercice validé !\033[0m\n\n");
                    is_exam_mode = 0;
                    current_state = STATE_CUSTOM_CHOOSE_EXO;
                    print_custom_list(current_rank);
                    free(input);
                    continue;
                }
            }
            else if (strcmp(trimmed_input, "subject") == 0) {
                char cmd[1024];
                printf("\n\033[1;34m╭─── SUJET : %s ──────────────────────────────╮\033[0m\n", current_exo_name);
                snprintf(cmd, sizeof(cmd), "cat %s/sub.txt", current_exo_path);
                system(cmd);
                printf("\033[1;34m╰──────────────────────────────────────────────╯\033[0m\n\n");
            }
            else if (strcmp(trimmed_input, "back") == 0 && !is_exam_mode) {
                is_exam_mode = 0;
                current_state = STATE_CUSTOM_CHOOSE_EXO;
                print_custom_list(current_rank);
                free(input);
                continue;
            }
            else {
                printf("\033[1mCommandes disponibles :\033[0m\n");
                printf(" subject - Affiche le sujet\n");
                printf(" test    - Lance le testeur\n");
                printf(" cheat   - Valide le niveau\n");
                if (is_exam_mode)
                    printf(" finish  - Vide rendu/ et quitte\n\n");
                else {
                    printf(" back    - Choisir un autre exercice\n");
                    printf(" finish  - Vide rendu/ et quitte\n\n");
                }
            }
        }
        free(input);
    }
    return 0;
}