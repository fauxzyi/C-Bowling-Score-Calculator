#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "queue.h"
#include "tpb.h"

#define ERR_FMT "\nInvalid token near %d (token found %c)\n"

int main(int argc, char **argv)
{
  LinkedList * scoreboard;
  const char *game;
  int err;
  int  i;
  
  if (argc>1)
  {
    for (i=1;i<argc;i++)
    {
      game = argv[i];
      scoreboard = bowling_score_parser(game,&err);

      if (scoreboard != NULL)
      {
        print_scoreboard(scoreboard);
        free_scoreboard(scoreboard);
      }
      else fprintf(stderr,ERR_FMT,err+1,game[err]);
    }
  }

  return 0;
}
