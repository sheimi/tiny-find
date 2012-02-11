#ifndef ENV_H
#define ENV_H

typedef int bool;     // define bool type
#define true 1        // define true
#define false 0       // define false

#define IS_EQUAL(s1, s2) (strcmp(s1, s2) == 0)     // define two str is equal
#define IS_NOT_EQUAL(s1, s2) (strcmp(s1, s2) != 0) // define two str is not equal

#endif
