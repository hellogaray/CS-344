#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static char s[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

/* Based on Professor's post on EdDiscussion. */
int main(int argc, char *argv[])
{
  FILE *randfp = fopen("/dev/urandom", "r");  // Per Prof.'s suggestion using /dev/urandom. 
  if (!randfp) {
    fprintf(stderr, "keygen: did not open urandom. \n");
    exit(1);
  }

  int keylength = atoi(argv[1]);

  for (int i = 0; i < keylength; ++i) {
    unsigned long long r;
    fread(&r, sizeof r, 1, randfp);
    r %= sizeof s - 1;
    printf("%c", s[r]);
  }
  putchar('\n');  // The last character keygen outputs should be a newline. 
  fclose(randfp); 
  return 0;
}
