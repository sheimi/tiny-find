#ifndef PARSER_H
#define PARSER_H
#include "env.h"

void init_parser(int argc, char * argv[]);  //init the parser for filter 
void free_parser();                         //free the parser
char * get_exp();                           //get the expressions
#endif
