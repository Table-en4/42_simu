#include <stdio.h>
#include <stdlib.h>

int is_safe(int *pos, int ca, int lp)
{
  for (int i = 0; i < ca; i++)
  {
    int cp = pos[i];
    if (cp == lp || cp + i == lp + ca || cp - i == lp + ca)
      return 1;
  }
  return 0;
}

void backtrack(int *pos, int colone, int size)
{
  if (colone == size)
  {
    for (int i = 0; i < size; i++)
    {
      if (i > 0)
        printf(" ");
      printf("%d", pos[i]);
    }
    printf("\n");
    return ;
  }
  for (int j = 0; j < size; j++)
  {
    if (is_safe(pos, colone, j))
    {
      pos[colone] = j;
      backtrack(pos, colone + 1, size);
    }
  }
}

int main(int ac, char **av)
{
  if (ac < 2)
    return 1;
  int size = atoi(av[1]);
  if (size <= 0)
    return 1;
  int pos[size];
  backtrack(pos, 0, size);
}