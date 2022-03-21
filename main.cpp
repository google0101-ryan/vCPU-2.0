#include <CPU.hpp>

int main()
{
    CPU* cpus[2];
    cpus[0] = new CPU();
    cpus[1] = new CPU();

    printf("[MAIN]: Created two-core CPU\n");

    while (1)
    {
        cpus[0]->Clock();
        cpus[1]->Clock();
    }
}