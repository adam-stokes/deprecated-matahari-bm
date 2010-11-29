typedef struct cpuinfo_
{
  int initialized;
  char *model;
  unsigned int cpus;
  unsigned int cores;
  unsigned int wordsize;
} cpuinfo_t;

extern cpuinfo_t cpuinfo;

