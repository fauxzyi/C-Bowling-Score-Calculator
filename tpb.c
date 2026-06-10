#include "queue.h"
#include "tpb.h"
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

/* add other headers from the CSTDLIB needed */

/* define my boolean type */

#define true 1
#define false 0
typedef int bool;

#define FRAME_NUMBER 10 /* you may need this */

/* Accepted Tokens */
typedef enum {
  TOKEN_LITERAL,
  TOKEN_SPARE,
  TOKEN_STRIKE,
  TOKEN_GUTTER,
  TOKEN_FAULT
} Token;

/* Struct for the lexical tokenization */
typedef struct
{
  Token token;
  char character;
  int lookahead;
  int pos;
} Lex;

/* I implement a few functions to help you */

void free_scoreboard(LinkedList * scoreboard)
{
  Node * next = scoreboard->head;
  while (next!=NULL)
  {
    free(next->data);
    next = next->next;
  }
  free_linked_list(scoreboard);
}

/* 
This function is not in the tpb.h, because it doesn't need to be externalised
Given a token of a roll, it return its value. prev_roll is the token of the
previous roll necessary to computer the spare
 */
int get_token_value(Lex* token, Lex * prev_roll)
{
  if (token!= NULL)
  {
    switch (token->token)
    {
      case TOKEN_STRIKE:
        return 10;
      case TOKEN_SPARE:
        if (prev_roll!= NULL)  return 10-get_token_value(prev_roll,NULL); 
        else break;
      case TOKEN_LITERAL:
        return token->character - '0';
      default: /* GUTTER / FAULT*/
        return 0;
    }

  }

  return -1; /* in case on an error */
}

/*
This function performs the tokenization
Also this one is not in the tpb.h as it's not needed to be seen outside

The purpose of this function is that, given a character, it makes the 
corresponding token. The struct Lex l is where the values are stored
because the function returns true in case of success, false otherwise.

*/

bool tokenization(char c, Lex *l)
{

    switch (c)
    {
      case 'x':
      case 'X':
        l->token = TOKEN_STRIKE;
        l->character = c;
        l->lookahead=2;
        break;
      case 'f':
      case 'F':
        l->token = TOKEN_FAULT;
        l->character = c;
        l->lookahead=0;
        break;
      case '-':
        l->token = TOKEN_GUTTER;
        l->character = c;
        l->lookahead=0;
        break;
      case '/':
        l->token = TOKEN_SPARE;
        l->character = c;
        l->lookahead=1; 
        break;
      default:
        if (isdigit((unsigned char)c) && (c!='0'))
        {
          l->token = TOKEN_LITERAL;
          l->character = c;
          l->lookahead=0;
        }
        else return false;
    }

    return true;
}

static Lex *copy_lex(Lex *l)
{
  Lex *n;
  n = malloc(sizeof(Lex));
  if (n != NULL) *n = *l;
  return n;
}

static bool is_roll(Lex *l)
{
  return l->token == TOKEN_LITERAL ||
         l->token == TOKEN_GUTTER ||
         l->token == TOKEN_FAULT;
}

static bool validate_frame(LinkedList *frame, int *err_pos)
{
    int count;
    Node *n;
    Lex *a;
    Lex *b;

    count = 0;
    n = frame->head;
    a = NULL;
    b = NULL;

    while (n != NULL)
    {
        count++;
        if (count == 1) a = n->data;
        if (count == 2) b = n->data;
        n = n->next;
    }

    if (count == 0) return false;

    if (a && a->token == TOKEN_STRIKE && count <= 3)
        return true;

    if (a && a->token == TOKEN_SPARE)
    {
        if (err_pos) *err_pos = a->pos;
        return false;
    }

    if (count >= 2 && a && b && is_roll(a) && b->token == TOKEN_SPARE && count <= 3)
        return true;

    if (count == 2 && a && b && is_roll(a) && is_roll(b))
        return true;
    
    if (count == 1)
        return false;

    if (err_pos && a) *err_pos = a->pos;
    return false;
}


static bool validate_last_frame(LinkedList *frame, int *err_pos)
{
  int count;
  Lex *a;
  Lex *b;
  Node *n;

  count = 0;
  a = NULL;
  b = NULL;
  n = frame->head;

  while (n)
  {
    count++;
    if (count == 1) a = n->data;
    if (count == 2) b = n->data;
    n = n->next;
  }

  if (count == 2)
  {
    if (is_roll(a) && is_roll(b)) return true;
    if (is_roll(a) && b->token == TOKEN_SPARE) return true;
  }

  if (count == 3)
  {
    if (a->token == TOKEN_STRIKE) return true;
    if (is_roll(a) && b->token == TOKEN_SPARE) return true;
  }

  if (err_pos) *err_pos = a ? a->pos : 0;
  return false;
}

static int calculate_frame_score(LinkedList *frame)
{
  int score;
  Node *n;
  Lex *prev;
  Lex *l;

  score = 0;
  n = frame->head;
  prev = NULL;

  while (n != NULL)
  {
    l = n->data;
    score += get_token_value(l, prev);
    prev = l;
    n = n->next;
  }

  return score;
}

LinkedList * bowling_score_parser(const char *game_characters, int *err_position)
{
    Queue *q;
    LinkedList *frame_list[FRAME_NUMBER];
    LinkedList *scoreboard;
    int i;
    int frame;
    int cumulative;
    Lex lex;
    Lex *a;
    Lex *b;
    Frame *f;
    int score;
    Node *n;
    int roll_count;
    Lex *first;
    Lex *second;
    Lex *l;
    int is_valid;
    int expecting_first_roll;
    Lex *temp_lex;

    q = initialise_queue();
    scoreboard = initialise_linked_list();
    frame = 0;
    cumulative = 0;
    expecting_first_roll = 1;

    for (i = 0; i < FRAME_NUMBER; i++)
        frame_list[i] = initialise_linked_list();

    for (i = 0; game_characters[i] != '\0'; i++)
    {
        if (!tokenization(game_characters[i], &lex))
        {
            if (err_position) *err_position = i;
            goto error;
        }
        lex.pos = i;
        
        if (expecting_first_roll && lex.token == TOKEN_SPARE)
        {
            if (err_position) *err_position = i;
            goto error;
        }
        
        if (lex.token == TOKEN_STRIKE)
        {
            expecting_first_roll = 1;
        }
        else if (lex.token == TOKEN_SPARE)
        {
            expecting_first_roll = 1;
        }
        else if (expecting_first_roll)
        {
            expecting_first_roll = 0;
        }
        else
        {
            expecting_first_roll = 1;
        }
        
        temp_lex = copy_lex(&lex);
        push_queue(q, temp_lex, sizeof(Lex));
        free(temp_lex);
    }

    while (q->head != NULL && frame < FRAME_NUMBER)
    {
        a = pop_queue(q);
        if (!a) break;
        
        if (frame < 9 && a->token == TOKEN_SPARE)
        {
            if (err_position) *err_position = a->pos;
            free(a);
            goto error;
        }
        
        append_linked_list(frame_list[frame], a);

        if (frame == 9)
        {
            while (q->head)
                append_linked_list(frame_list[frame], pop_queue(q));
            frame++;
            break;
        }

        if (a->token == TOKEN_STRIKE)
        {
            if (q->head)
            {
                Lex *la1 = copy_lex((Lex*)q->head->data);
                if (la1) append_linked_list(frame_list[frame], la1);
            }
            if (q->head && q->head->next)
            {
                Lex *la2 = copy_lex((Lex*)q->head->next->data);
                if (la2) append_linked_list(frame_list[frame], la2);
            }

            frame++;
        }
        else
        {
            if (!q->head)
                break;

            b = pop_queue(q);
            if (!b) break;
            
            if (b->token == TOKEN_STRIKE)
            {
                if (err_position) *err_position = b->pos;
                free(b);
                goto error;
            }
            
            append_linked_list(frame_list[frame], b);

            if (b->token == TOKEN_SPARE && q->head)
            {
                Lex *la = copy_lex((Lex*)q->head->data);
                if (la) append_linked_list(frame_list[frame], la);
            }

            frame++;
        }
    }

    for (i = 0; i <= frame && i < FRAME_NUMBER; i++)
    {
        if (!frame_list[i]->head) break;
        
        if (i < 9)
        {
            is_valid = validate_frame(frame_list[i], err_position);
        }
        else
        {
            is_valid = validate_last_frame(frame_list[i], err_position);
        }
        
        if (!is_valid)
        {
            if (frame_list[i]->head && ((Lex*)frame_list[i]->head->data)->token == TOKEN_SPARE)
            {
                goto error;
            }
            
            score = calculate_frame_score(frame_list[i]);
            cumulative += score;
            
            f = malloc(sizeof(Frame));
            f->score = cumulative;
            f->n_rolls = 0;
            
            n = frame_list[i]->head;
            roll_count = 0;
            
            while (n != NULL && roll_count < 3)
            {
                l = n->data;
                f->rolls[roll_count++] = l->character;
                n = n->next;
            }
            
            f->n_rolls = roll_count;
            append_linked_list(scoreboard, f);
            break;
        }

        score = calculate_frame_score(frame_list[i]);
        cumulative += score;

        f = malloc(sizeof(Frame));
        f->score = cumulative;
        f->n_rolls = 0;

        n = frame_list[i]->head;
        roll_count = 0;
        
        if (i < 9)
        {
            first = n ? (Lex*)n->data : NULL;
            if (first)
            {
                f->rolls[roll_count++] = first->character;
                if (first->token != TOKEN_STRIKE && n->next)
                {
                    second = (Lex*)n->next->data;
                    if (second)
                        f->rolls[roll_count++] = second->character;
                }
            }
        }
        else
        {
            while (n != NULL && roll_count < 3)
            {
                l = n->data;
                f->rolls[roll_count++] = l->character;
                n = n->next;
            }
        }
        
        f->n_rolls = roll_count;

        append_linked_list(scoreboard, f);
    }

    for (i = 0; i < FRAME_NUMBER; i++)
    {
        n = frame_list[i]->head;
        while (n)
        {
            free(n->data);
            n = n->next;
        }
        free_linked_list(frame_list[i]);
    }
    free_queue(q);

    if (scoreboard->head == NULL)
    {
        free_scoreboard(scoreboard);
        return NULL;
    }

    return scoreboard;

error:
    for (i = 0; i < FRAME_NUMBER; i++)
    {
        n = frame_list[i]->head;
        while (n)
        {
            free(n->data);
            n = n->next;
        }
        free_linked_list(frame_list[i]);
    }
    free_queue(q);
    free_scoreboard(scoreboard);
    return NULL;
}

void print_scoreboard(LinkedList *scoreboard)
{
    Node *n;
    int i;
    Frame *f;
    int frame_idx;

#ifdef SCOREBOARD
    printf("+-----------------------------------------+\n");
    printf("|");
    
    n = scoreboard->head;
    frame_idx = 0;
    
    while (n != NULL && frame_idx < 10)
    {
        f = n->data;
        
        if (frame_idx < 9)
        {
            for (i = 0; i < f->n_rolls; i++)
            {
                printf("%c", f->rolls[i]);
                if (i < f->n_rolls - 1) printf(" ");
            }
            
            for (i = f->n_rolls; i < 2; i++)
            {
                if (i > 0) printf(" ");
                printf(" ");
            }
        }
        else
        {
            for (i = 0; i < f->n_rolls; i++)
            {
                printf("%c", f->rolls[i]);
                if (i < f->n_rolls - 1) printf(" ");
            }
            
            for (i = f->n_rolls; i < 3; i++)
            {
                if (i > 0) printf(" ");
                printf(" ");
            }
        }
        
        printf("|");
        frame_idx++;
        n = n->next;
    }
    
    while (frame_idx < 10)
    {
        if (frame_idx < 9)
            printf("   |");
        else
            printf("     |");
        frame_idx++;
    }
    
    printf("\n|");
    
    n = scoreboard->head;
    frame_idx = 0;
    
    while (n != NULL && frame_idx < 10)
    {
        f = n->data;
        
        if (frame_idx < 9)
            printf("%3d|", f->score);
        else
            printf("%5d|", f->score);
        
        frame_idx++;
        n = n->next;
    }
    
    while (frame_idx < 10)
    {
        if (frame_idx < 9)
            printf("   |");
        else
            printf("     |");
        frame_idx++;
    }
    
    printf("\n+-----------------------------------------+\n");
#else
    n = scoreboard->head;

    while (n != NULL)
    {
        f = n->data;

        for (i = 0; i < f->n_rolls; i++)
            putchar(f->rolls[i]);

        n = n->next;
    }

    printf(": ");

    n = scoreboard->head;
    while (n != NULL)
    {
        f = n->data;
        printf("%d", f->score);
        if (n->next != NULL)
            putchar(' ');
        n = n->next;
    }

    printf("\n");
#endif
}