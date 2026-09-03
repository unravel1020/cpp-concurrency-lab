#include <unistd.h>

#include <cstring>
#include <iostream>

int main()
{
    int pipefd[2];

    if (pipe(pipefd) == -1)
    {
        std::cerr << "pipe failed\n";
        return 1;
    }

    std::cout << "read fd  = " << pipefd[0] << '\n';
    std::cout << "write fd = " << pipefd[1] << '\n';

    pid_t pid = fork();

    if (pid == -1)
    {
        std::cerr << "fork failed\n";
        return 1;
    }

    if (pid == 0)
    {
        // Child: read
        close(pipefd[1]);

        char buffer[128] = {};

        size_t n = read(
            pipefd[0],
            buffer,
            sizeof(buffer) - 1);

        std::cout << "read returned: " << n << '\n';

        if (n > 0)
        {
            buffer[n] = '\0';

            std::cout
                << "child received: "
                << buffer
                << '\n';
        }

        close(pipefd[0]);
    }
    else
    {
        // Parent: write
        close(pipefd[0]);

        close(pipefd[1]);
    }

    return 0;
}