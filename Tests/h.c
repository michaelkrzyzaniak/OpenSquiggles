#include <stdio.h>
#include <stdint.h>

uint32_t hash(char* message)
{
  uint32_t hash = 5381;
  while (*message != '\0')
    hash = hash * 33 ^ (int)*message++;
  return hash;
}

int main(int argc, char* argv[])
{
  while(--argc)
    {   
        ++argv;
        uint32_t h = hash(*argv);
        fprintf(stderr, "%u\r\n", h);
    }
}
