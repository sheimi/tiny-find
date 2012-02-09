#ifndef PARSER_H
#define PARSER_H
#include "env.h"
void init_parser(int argc, char * argv[]);  
void free_parser(); 
char * get_exp();
#endif
