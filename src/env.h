#ifndef ENV_H
#define ENV_H

typedef int bool;     // define bool type
#define true 1        // define true
#define false 0       // define false

#define IS_EQUAL(s1, s2) (strcmp(s1, s2) == 0)     // define two str is equal
#define IS_NOT_EQUAL(s1, s2) (strcmp(s1, s2) != 0) // define two str is not equal

enum SymbolHandle {S_L, S_P, S_H};

typedef struct find_option {
  int  pathes_num; 
  char ** find_pathes;

  int min_depth;
  int max_depth;

  bool is_exec;
  char ** argv;
  int argc;

  bool is_ok;
  char ** ok_argv;
  int ok_argc;

  bool is_print;
  bool is_print0;

  bool no_actions;

  enum SymbolHandle symbol_handle; 
} FindOption;

extern FindOption options;
extern const char * USAGE;

void print_error(const char * message);
void require_arg(const char * exp);

#endif
