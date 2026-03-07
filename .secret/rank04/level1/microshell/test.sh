#!/bin/bash

# 1. commande simple
./a.out /bin/echo hello
echo "---"

# 2. pipe simple
./a.out /bin/echo hello "|" /bin/cat
echo "---"

# 3. double pipe
./a.out /bin/echo hello "|" /bin/cat "|" /bin/cat
echo "---"

# 4. point-virgule
./a.out /bin/echo hello ";" /bin/echo world
echo "---"

# 5. pipe + point-virgule
./a.out /bin/echo foo "|" /bin/cat ";" /bin/echo bar
echo "---"

# 6. cd valide
./a.out /bin/echo before ";" cd /tmp ";" /bin/pwd
echo "---"

# 7. cd sans argument
./a.out cd 2>&1
echo "---"

# 8. cd trop d'arguments
./a.out cd /tmp /var 2>&1
echo "---"

# 9. cd dossier inexistant
./a.out cd /doesnotexist123 2>&1
echo "---"

# 10. commande inexistante
./a.out /bin/doesnotexist 2>&1
echo "---"

# 11. separateur seul
./a.out ";"
echo "---"

# 12. pipe vers commande inexistante
./a.out /bin/echo test "|" /bin/doesnotexist 2>&1
echo "---"