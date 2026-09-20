#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int is_sep(char c){
	return( c == ' ' || c == '\n' || c == '\t');
}


char **ft_split(char *str){
	char	**tab;
	int	n;
	int	j;
	int	k;
	int	i;

	i = 0;
	k = 0;
	tab = malloc(sizeof(char *) * 10000);
	while(str[i]){
		while(is_sep(str[i]))
			i++;
		j = i;
		while(str[i] && !is_sep(str[i]))
			i++;
		if(i > j){
			
			tab[k] = malloc(i - j + 1);
			n = 0;
			while(j + n < i){
				tab[k][n] = str[j + n];
				n++;
				
			}
			tab[k++][n] = '\0';
		}

	}
	tab[k] = NULL;
	return(tab);
}
/*int main() {
	printf("%s\n", ft_split("je test une phrase"));
}*/
