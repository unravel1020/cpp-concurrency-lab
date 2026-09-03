#include <iostream>
#include <unistd.h>

int main()
{
    // std::cout << "before fork\n";

    // pid_t pid = fork();

    // if (pid == 0)
    // {
    //     std::cout << "child:  pid = " << getpid()
    //               << ", ppid = " << getppid() << '\n';
    // }
    // else if (pid > 0)
    // {
    //     std::cout << "parent: pid = " << getpid()
    //               << ", child pid = " << pid << '\n';
    // }
    // else
    // {
    //     std::cout << "fork failed\n";
    // }

    // return 0;

    std::cout << "before fork\n";

    pid_t pid = fork();

    if (pid == 0)
    {
        sleep(2);

        std::cout << "child:  pid = " << getpid()
                  << ", ppid = " << getppid() << '\n';
    }
    else if (pid > 0)
    {
        std::cout << "parent: pid = " << getpid()
                  << ", child pid = " << pid << '\n';

        sleep(5);
    }
    else
    {
        std::cout << "fork failed\n";
    }

    return 0;
}
