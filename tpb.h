#ifndef _TPB_C_
#define _TPB_C_

#include "linked_list.h"

typedef struct
{
  int score; /* cumulative score up to this fame*/
  char rolls[3];  /*actual rolls symbols for this frame*/
  int n_rolls;   /* number of rolls for this frame*/
} Frame;

/*
Function: bowling_score_parser
Param 1: string to parse (array of characters)
Param 2: (Optional) integer indicating the position of a wrong token

It performs the parsing of the first parameters.
In case of error, the second parameter (if any) is set to the position 
on which an error is found on the string in the first parameter

Return: A linked list of exactly 10 Frame structs OR NULL (in case of an error)

*/
LinkedList * bowling_score_parser(const char *, int *);

/*
Function: free_scoreboard
Param 1: Linked list of frames

This function removes all the items in the linked list, including its content

Return: Nothing

*/
void free_scoreboard(LinkedList *);

/*
Function: print_scoreboard
Param1: Linked list of frames

Print scoreboard on screen. Depending on how the project is compiled, it can be either 

7-5/xxxxxx71xxx: 7 27 57 87 117 147 174 192 200 230

or

+-----------------------------------------+
|7 -|5 /|x  |x  |x  |x  |x  |x  |7 1|x x x|
|  7| 27| 57| 87|117|147|174|192|200|  230|
+-----------------------------------------+

Return: Nothing
*/

void print_scoreboard(LinkedList *);

#endif