#!/bin/bash
source ../../../main/colors.sh
file1=microshell.c
file2=../../../../rendu/microshell/microshell.c

run_test() {
    local desc="$1"
    shift
    local expected_out="$1"
    shift
    local expected_err="$1"
    shift

    actual_out=$(./out2 "$@" 2>/dev/null)
    actual_err=$(./out2 "$@" 2>&1 >/dev/null)

    if [ "$actual_out" != "$expected_out" ] || [ "$actual_err" != "$expected_err" ]; then
        echo "$(tput setaf 1)$(tput bold)FAIL$(tput sgr 0) - $desc"
        echo "${GREEN}Expected stdout:${RESET} \"$expected_out\""
        echo "${RED}Your stdout:${RESET}     \"$actual_out\""
        echo "${GREEN}Expected stderr:${RESET} \"$expected_err\""
        echo "${RED}Your stderr:${RESET}     \"$actual_err\""
        rm -f out1 out2
        exit 1
    fi
}

gcc -Werror -Wall -Wextra -o out1 "$file1" 2>/dev/null
gcc -Werror -Wall -Wextra -o out2 "$file2" 2>/dev/null
if [ $? -ne 0 ]; then
    echo "$(tput setaf 1)$(tput bold)FAIL$(tput sgr 0) - Compilation échouée"
    rm -f out1 out2
    exit 1
fi

# 1. commande simple
run_test "commande simple" "hello" "" /bin/echo hello

# 2. pipe simple
run_test "pipe simple" "hello" "" /bin/echo hello "|" /bin/cat

# 3. double pipe
run_test "double pipe" "hello" "" /bin/echo hello "|" /bin/cat "|" /bin/cat

# 4. point-virgule
run_test "point-virgule" "hello
world" "" /bin/echo hello ";" /bin/echo world

# 5. pipe + point-virgule
run_test "pipe + point-virgule" "foo
bar" "" /bin/echo foo "|" /bin/cat ";" /bin/echo bar

# 6. cd valide
printf '/bin/echo before\ncd /tmp\n/bin/pwd\n' | xargs -d '\n' ./out2 > /tmp/ms_out.txt 2>/tmp/ms_err.txt

# 7. cd sans argument
run_test "cd sans argument" "" "error: cd: bad arguments" cd

# 8. cd trop d'arguments
run_test "cd trop d'arguments" "" "error: cd: bad arguments" cd /tmp /var

# 9. cd dossier inexistant
run_test "cd inexistant" "" "error: cd: cannot change directory to /doesnotexist123" cd /doesnotexist123

# 10. commande inexistante
run_test "commande inexistante" "" "error: cannot execute /bin/doesnotexist" /bin/doesnotexist

# 11. pipe vers commande inexistante
run_test "pipe vers inexistant" "" "error: cannot execute /bin/doesnotexist" /bin/echo test "|" /bin/doesnotexist

rm -f out1 out2
echo "$(tput setaf 2)$(tput bold)PASSED 🎉$(tput sgr 0)"
exit 0