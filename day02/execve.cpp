#include <unistd.h>

#include <iostream>

int main()
{
    std::cout << "before exec" << std::endl;

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

    std::cerr << "execve failed\n";

    return 1;
}