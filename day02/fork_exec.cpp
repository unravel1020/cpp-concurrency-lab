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
    std::cout
        << "child: before exec, pid = "
        << getpid()
        << '\n';

    int fd = open(
        "exec_output.txt",
        O_WRONLY | O_CREAT | O_TRUNC,
        0644);

    if (fd == -1)
    {
        perror("open");
        return 1;
    }

    if (dup2(fd, STDOUT_FILENO) == -1)
    {
        perror("dup2");
        return 1;
    }

    close(fd);

    char* argv[] = {
        const_cast<char*>("ls"),
        const_cast<char*>("-l"),
        nullptr
    };

    char* envp[] = {
        nullptr
    };

    execve(
        "/usr/bin/ls",
        argv,
        envp);

    perror("execve");
    return 1;
}

    waitpid(pid, nullptr, 0);

    std::cout
        << "parent: child finished\n";

    return 0;
}