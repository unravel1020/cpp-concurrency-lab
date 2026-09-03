#include <fcntl.h>
#include <unistd.h>

#include <iostream>

int main()
{
    int fd = open(
        "test.txt",
        O_WRONLY | O_CREAT | O_TRUNC,
        0644);

    if (fd == -1)
    {
        std::cerr << "open failed\n";
        return 1;
    }

    std::cout << "PID = " << getpid() << '\n';
    std::cout << "FD  = " << fd << '\n';

    const char* message =
        "hello from file descriptor\n";

    write(fd, message, 27);

    sleep(20);

    close(fd);

    return 0;
}