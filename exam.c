#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <glob.h>
#include <libgen.h>
#include <sys/wait.h>
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

int get_level_names(int rank, char lnames[16][64])
{
    glob_t globbuf;
    char pattern[128];
    int result;
    int count = 0;
    const char *prefixes[] = {".secret/", "simu/.secret/", "42_simu/.secret/"};

    for (int i = 0; i < 3; i++)
    {
        sprintf(pattern, "%srank%02d/level*/", prefixes[i], rank);
        result = glob(pattern, GLOB_NOSORT, NULL, &globbuf);
        if (result == 0 && globbuf.gl_pathc > 0)
        {
            for (size_t j = 0; j < globbuf.gl_pathc && j < 16; j++)
            {
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

int get_exercises_for_level_name(int rank, char *level_name, glob_t *globbuf)
{
    char pattern[256];
    int result;
    const char *prefixes[] = {".secret/", "simu/.secret/", "42_simu/.secret/"};

    for (int i = 0; i < 3; i++)
    {
        sprintf(pattern, "%srank%02d/%s/*/", prefixes[i], rank, level_name);
        result = glob(pattern, GLOB_NOSORT, NULL, globbuf);
        if (result == 0 && globbuf->gl_pathc > 0)
        {
            for (size_t j = 0; j < globbuf->gl_pathc; j++)
            {
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

int get_exercises(int rank, glob_t *globbuf)
{
    char pattern[128];
    int result;
    const char *prefixes[] = {".secret/", "simu/.secret/", "42_simu/.secret/"};

    for (int i = 0; i < 3; i++)
    {
        sprintf(pattern, "%srank%02d/level*/*/", prefixes[i], rank);
        result = glob(pattern, GLOB_NOSORT, NULL, globbuf);
        if (result == 0 && globbuf->gl_pathc > 0)
        {
            for (size_t j = 0; j < globbuf->gl_pathc; j++)
            {
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

int get_level_count(int rank)
{
    glob_t globbuf;
    char pattern[128];
    int result;
    int count = 0;
    const char *prefixes[] = {".secret/", "simu/.secret/", "42_simu/.secret/"};

    for (int i = 0; i < 3; i++)
    {
        sprintf(pattern, "%srank%02d/level*/", prefixes[i], rank);
        result = glob(pattern, GLOB_NOSORT, NULL, &globbuf);
        if (result == 0 && globbuf.gl_pathc > 0)
        {
            count = globbuf.gl_pathc;
            globfree(&globbuf);
            return count;
        }
        globfree(&globbuf);
    }
    return 0;
}

void build_exam(int rank, int nb_levels)
{
    get_level_names(rank, g_level_names);
    for (int lvl = 0; lvl < nb_levels; lvl++)
    {
        glob_t globbuf;
        if (get_exercises_for_level_name(rank, g_level_names[lvl], &globbuf) && globbuf.gl_pathc > 0)
        {
            int idx = rand() % globbuf.gl_pathc;
            strcpy(exam_exos[lvl], globbuf.gl_pathv[idx]);
            char tmp[512];
            strcpy(tmp, globbuf.gl_pathv[idx]);
            strcpy(exam_exo_names[lvl], basename(tmp));
        }
        else
        {
            exam_exos[lvl][0] = '\0';
            exam_exo_names[lvl][0] = '\0';
        }
        globfree(&globbuf);
    }
}

void start_level(int lvl)
{
    strcpy(current_exo_path, exam_exos[lvl]);
    strcpy(current_exo_name, exam_exo_names[lvl]);
    printf("\n┌──────────────────────────────────────────────┐\n");
    printf("│  NIVEAU %-2d | EXERCICE : \033[1;32m%-20s\033[0m│\n", lvl, current_exo_name);
    printf("│  SCORE     | ACTUEL   : \033[1;33m%-3d\033[0m/100            │\n", score);
    printf("└──────────────────────────────────────────────┘\n");
    printf(" 💡 Commandes : 'subject', 'test', 'cheat', 'finish'\n\n");
}

void start_random_exam(void)
{
    score = 0;
    current_level = 0;
    points_per_level = (total_levels > 0) ? 100 / total_levels : 0;
    is_exam_mode = 1;
    build_exam(current_rank, total_levels);
    start_level(current_level);
}

void show_exam_end(void)
{
    printf("\n\033[1;32m╔══════════════════════════════════════════════╗\033[0m\n");
    printf("\033[1;32m║               EXAMEN TERMINÉ !               ║\033[0m\n");
    printf("\033[1;32m║               Score final : %-3d/100          ║\033[0m\n", score);
    printf("\033[1;32m╚══════════════════════════════════════════════╝\033[0m\n");
    system("rm -rf rendu/* 2>/dev/null; mkdir -p rendu");
    printf(" Que souhaitez-vous faire ?\n");
    printf("  ├─ \033[1mretry\033[0m  : Recommencer l'examen\n");
    printf("  ├─ \033[1mback\033[0m   : Choisir un autre rang\n");
    printf("  └─ \033[1mfinish\033[0m : Quitter\n");
}

void print_custom_list(int rank)
{
    glob_t globbuf;
    if (get_exercises(rank, &globbuf) && globbuf.gl_pathc > 0)
    {
        printf("Exercices disponibles pour le rang %d :\n", rank);
        for (size_t i = 0; i < globbuf.gl_pathc; i++)
        {
            char temp_path[512];
            strcpy(temp_path, globbuf.gl_pathv[i]);
            printf("  • %s\n", basename(temp_path));
        }
        printf("\nTapez le nom de l'exercice :\n");
    }
    globfree(&globbuf);
}

int main(void)
{
    char *input;
    ProgramState current_state = STATE_CHOOSE_RANK;

    srand(time(NULL));

    printf("==========================================\n");
    printf("        42 EXAM SIMULATOR                 \n");
    printf("==========================================\n\n");
    printf("Veuillez choisir un rang (2, 3, 4, 5, 6) :\n");

    while (1)
    {
        char prompt[512];

        if (current_state == STATE_CHOOSE_RANK)
            snprintf(prompt, sizeof(prompt), "\033[1;36mexamshell\033[0m> ");
        else if (current_state == STATE_CHOOSE_MODE || current_state == STATE_CUSTOM_CHOOSE_EXO)
            snprintf(prompt, sizeof(prompt), "\033[1;36mexamshell\033[0m/rank%02d> ", current_rank);
        else if (current_state == STATE_IN_EXAM)
        {
            if (is_exam_mode)
                snprintf(prompt, sizeof(prompt), "\033[1;32m%.100s\033[0m [\033[1;33m%d pts\033[0m]> ", current_exo_name, score);
            else
                snprintf(prompt, sizeof(prompt), "\033[1;32m%.100s\033[0m> ", current_exo_name);
        }

        input = readline(prompt);
        if (!input)
        {
            printf("\nFermeture du simulateur...\n");
            break;
        }
        if (strlen(input) > 0)
            add_history(input);

        if (strcmp(input, "finish") == 0)
        {
            system("rm -rf rendu/* 2>/dev/null; mkdir -p rendu");
            printf("Dossier rendu vidé. Fermeture du simulateur...\n");
            free(input);
            break;
        }

        if (current_state == STATE_CHOOSE_RANK)
        {
            current_rank = atoi(input);
            if (current_rank >= 2 && current_rank <= 6)
            {
                total_levels = get_level_count(current_rank);
                points_per_level = (total_levels > 0) ? 100 / total_levels : 0;
                score = 0;
                printf("\nRang %d sélectionné.\n", current_rank);
                if (total_levels > 0)
                    printf("\033[1;33mCet examen contient %d niveaux.\033[0m\n", total_levels);
                else
                    printf("\033[1;31mAttention : Aucun niveau trouvé.\033[0m\n");
                printf("\nModes disponibles :\n  - \033[1mrandom\033[0m (Un exercice au hasard par niveau)\n  - \033[1mcustom\033[0m (Choisir un exercice spécifique)\n");
                current_state = STATE_CHOOSE_MODE;
            }
            else
                printf("Rang invalide. Veuillez choisir un rang entre 2 et 6.\n");
        }
        else if (current_state == STATE_CHOOSE_MODE)
        {
            if (strcmp(input, "random") == 0 || strcmp(input, "retry") == 0)
            {
                if (total_levels > 0)
                {
                    start_random_exam();
                    current_state = STATE_IN_EXAM;
                }
                else
                    printf("Aucun exercice trouvé pour le rang %d.\n", current_rank);
            }
            else if (strcmp(input, "custom") == 0)
            {
                print_custom_list(current_rank);
                current_state = STATE_CUSTOM_CHOOSE_EXO;
            }
            else if (strcmp(input, "back") == 0)
            {
                current_state = STATE_CHOOSE_RANK;
                printf("Veuillez choisir un rang (2, 3, 4, 5, 6) :\n");
            }
            else
                printf("Commande inconnue. Tapez 'random', 'custom', 'retry' ou 'back'.\n");
        }
        else if (current_state == STATE_CUSTOM_CHOOSE_EXO)
        {
            glob_t globbuf;
            int found = 0;
            if (get_exercises(current_rank, &globbuf))
            {
                for (size_t i = 0; i < globbuf.gl_pathc; i++)
                {
                    char temp_path[512];
                    strcpy(temp_path, globbuf.gl_pathv[i]);
                    char *exo_name = basename(temp_path);
                    if (strcmp(input, exo_name) == 0)
                    {
                        strcpy(current_exo_name, exo_name);
                        strcpy(current_exo_path, globbuf.gl_pathv[i]);
                        found = 1;
                        break;
                    }
                }
            }
            globfree(&globbuf);
            if (found)
            {
                is_exam_mode = 0;
                score = 0;
                printf("\n┌──────────────────────────────────────────────┐\n");
                printf("│ ASSIGNATION DE L'EXERCICE : \033[1;32m%-16s\033[0m│\n", current_exo_name);
                printf("└──────────────────────────────────────────────┘\n");
                printf(" 💡 Commandes : 'subject', 'test', 'cheat', 'back'\n\n");
                current_state = STATE_IN_EXAM;
            }
            else if (strcmp(input, "back") == 0)
            {
                current_state = STATE_CHOOSE_MODE;
                printf("Modes : 'random' ou 'custom'\n");
            }
            else
                printf("Exercice introuvable. Nom exact requis, ou 'back'.\n");
        }
        else if (current_state == STATE_IN_EXAM)
        {
            if (strcmp(input, "test") == 0 || strcmp(input, "grademe") == 0)
            {
                char cmd[1024];
                printf("\033[1;33m=> Lancement du testeur pour %s...\033[0m\n", current_exo_name);
                snprintf(cmd, sizeof(cmd), "cd %s && bash tester.sh", current_exo_path);
                int ret = system(cmd);
                if (WIFEXITED(ret) && WEXITSTATUS(ret) == 0)
                {
                    if (is_exam_mode)
                    {
                        score += points_per_level;
                        printf("\033[1;32m✓ Succès ! +%d pts → Score : %d/100\033[0m\n", points_per_level, score);
                        current_level++;
                        if (current_level >= total_levels)
                        {
                            show_exam_end();
                            is_exam_mode = 0;
                            current_state = STATE_CHOOSE_MODE;
                            free(input);
                            continue;
                        }
                        start_level(current_level);
                    }
                    else
                        printf("\033[1;32m✓ PASSED ! Tapez 'back' pour choisir un autre exercice.\033[0m\n");
                }
                else
                    printf("\033[1;31m✖ Échec des tests. Réessayez !\033[0m\n");
            }
            else if (strcmp(input, "cheat") == 0)
            {
                if (is_exam_mode)
                {
                    score += points_per_level;
                    printf("\033[1;35m🤫 [CHEAT] +%d pts → Score : %d/100\033[0m\n", points_per_level, score);
                    current_level++;
                    if (current_level >= total_levels)
                    {
                        show_exam_end();
                        is_exam_mode = 0;
                        current_state = STATE_CHOOSE_MODE;
                        free(input);
                        continue;
                    }
                    start_level(current_level);
                }
                else
                {
                    printf("\033[1;35m🤫 [CHEAT] Exercice validé !\033[0m\n");
                    is_exam_mode = 0;
                    current_state = STATE_CUSTOM_CHOOSE_EXO;
                    print_custom_list(current_rank);
                    free(input);
                    continue;
                }
            }
            else if (strcmp(input, "subject") == 0)
            {
                char cmd[1024];
                printf("\n\033[1;34m╭─── SUJET : %s ──────────────────────────────╮\033[0m\n", current_exo_name);
                snprintf(cmd, sizeof(cmd), "cat %s/sub.txt", current_exo_path);
                system(cmd);
                printf("\033[1;34m╰──────────────────────────────────────────────╯\033[0m\n");
            }
            else if (strcmp(input, "back") == 0 && !is_exam_mode)
            {
                is_exam_mode = 0;
                current_state = STATE_CUSTOM_CHOOSE_EXO;
                print_custom_list(current_rank);
                free(input);
                continue;
            }
            else
            {
                printf("Commandes disponibles :\n");
                printf("  subject - Affiche le sujet\n");
                printf("  test    - Lance le testeur\n");
                printf("  cheat   - Valide le niveau\n");
                if (is_exam_mode)
                    printf("  finish  - Vide rendu/ et quitte\n");
                else
                    printf("  back    - Choisir un autre exercice\n  finish  - Vide rendu/ et quitte\n");
            }
        }
        free(input);
    }
    return 0;
}