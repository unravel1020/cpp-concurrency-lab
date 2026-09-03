#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

#include <iostream>

int main()
{
    pid_t pid = fork();

    if (pid == -1)
    {
        std::cerr << "fork failed\n";
        return 1;
    }

    if (pid == 0)
    {
        int fd = open(
            "child_output.txt",
            O_WRONLY | O_CREAT | O_TRUNC,
            0644);

        if (fd == -1)
        {
            std::cerr << "open failed\n";
            return 1;
        }

        dup2(fd, STDOUT_FILENO);
        close(fd);

        std::cout << "hello from child\n";

        return 0;
    }

    waitpid(pid, nullptr, 0);

    std::cout << "hello from parent\n";

    return 0;
}