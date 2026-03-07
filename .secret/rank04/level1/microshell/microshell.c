#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

size_t	ft_strlen(char *str)
{
	int	i;

	i = 0;
	if (!str)
		return (i);
	while (str[i])
		i++;
	return (i);
}

void	ft_putstr_err(char *str)
{
	write(2, str, ft_strlen(str));
}

int	ft_cd(char **av, int i)
{
	if (i != 2)
	{
		ft_putstr_err("error: cd: bad arguments\n");
		return (1);
	}
	if (chdir(av[1]) == -1)
	{
		ft_putstr_err("error: cd: cannot change directory to ");
		ft_putstr_err(av[1]);
		ft_putstr_err("\n");
		return (1);
	}
	return (0);
}

void	set_pipe(int has_pipe, int *fd, int end)
{
	if (has_pipe)
	{
		if (dup2(fd[end], end) == -1 || close(fd[0]) == -1 || close(fd[1]) == -1)
		{
			ft_putstr_err("error: fatal\n");
			exit(1);
		}
	}
}

int	exec(char **av, int i, char **envp)
{
	int	has_pipe;
	int	pipes_fd[2];
	int	pid;
	int	status;

	if (av[i] != NULL && strcmp(av[i], "|") == 0)
		has_pipe = 1;
	else
		has_pipe = 0;
	if (!has_pipe == 0 && strcmp(av[0], "cd") == 0)
		return (ft_cd(av, i));
	if (has_pipe && pipe(pipes_fd) == -1)
	{
		ft_putstr_err("error: fatal\n");
		exit(1);
	}
	pid = fork();
	if (pid == -1)
	{
		ft_putstr_err("error: fatal\n");
		exit(1);
	}
	if (pid == 0)
	{
		av[i] = 0;
		set_pipe(has_pipe, pipes_fd, 1);
		if (strcmp(av[0], "cd") == 0)
			exit(ft_cd(av, i));
		execve(av[0], av, envp);
		ft_putstr_err("error: cannot execute ");
		ft_putstr_err(av[0]);
		ft_putstr_err("\n");
		exit(1);
	}
	waitpid(pid, &status, 0);
	set_pipe(has_pipe, pipes_fd, 0);
	return (WIFEXITED(status) && WEXITSTATUS(status));
}

int main(int ac, char **av, char **envp)
{
	int	i;
	int	status;

	(void) ac;
	i = 0;
	status = 0;
	while (av[i])
	{
		av += i + 1;
		i = 0;
		while (av[i] && strcmp(av[i], "|") && strcmp(av[i], ";"))
			i++;
		if (i)
			status = exec(av, i, envp);
	}
	return status;
}
