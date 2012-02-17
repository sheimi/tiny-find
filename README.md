TINY-FIND 
==============
    Homework -- to fake a command -- find >_<!~~~

## OVERVIEW
    This is my homework of "Linux Programming". 
    This is the of "Tiny Find", my version of "find".

### MAKE
    make or make DEBUG=1 for debug version

## SPECIFICATION
    usage: tinyfind [-H | -L | -P] [-EXdsx] [-f path] path ... [expression]
           tinyfind [-H | -L | -P] [-EXdsx] -f path [path ...] [expression]
### OPTIONS
    -H      Cause the file information and file type (see stat(2)) returned
            for each symbolic link specified on the command line to be those
            of the file referenced by the link, not the link itself.  If the
            referenced file does not exist, the file information and type
            will be for the link itself.  File information of all symbolic
            links not on the command line is that of the link itself.

    -L      Cause the file information and file type (see stat(2)) returned
            for each symbolic link to be those of the file referenced by the
            link, not the link itself.  If the referenced file does not
            exist, the file information and type will be for the link itself.
            This option is equivalent to the deprecated -follow primary.

    -P      Cause the file information and file type (see stat(2)) returned
            for each symbolic link to be those of the link itself.  This is
            the default.



### LOGIC EXPRESSIONS
    exps can linked with logic expression and '(', ')' 
    -not
    -and
    -or
    '(', ')' should be used after Escape character \. exp '\(' and '\(' 

### ACTIONS
    -exec [argv] \;
      if it is true, excute the cmd. 
    -ok [argv] \;
      if it is true, excute the cmd.(ask first) 
    -print
      if it is true, printf it
    -print0
      if it is true, printf it without '\n'


### EXPRESSIONS
    -name
      True if the last component of the pathname being examined matches pattern.
      Special shell pattern matching characters (``['', ``]'', ``*'', and ``?'')
      may be used as part of pattern.  These characters may be matched 
      explicitly by escaping them with a backslash (``\'').
    -iname
      Like -name, but the match is case insensitive
    -user [username | user_id]
      True if the file belongs to the user
    -group [groupname | group_id]
      True if the file belongs to the group
    -perm [perm(exp 777)]
      True if the file match such perm
    -regex [reg]
      True if the filename match the regex expression
    -amin [n]
      True if the file is accessed in n minutes 
    -atime [n]
      True if the file is accessed in n days 
    -anewer [file2]
      True if the file is accessed later than file2
    -cmin [n]
      True if the file is changed in n minutes 
    -ctime [n]
      True if the file is changed in n days 
    -cnewer [file2]
      True if the file is changed later than file2
    -mmin [n]
      True if the file is modified in n minutes 
    -mtime [n]
      True if the file is modified in n days 
    -mnewer [file2]
      True if the file is modified later than file2
    -type [b|c|d|f|l|p]
      b       block special
      c       character special
      d       directory
      f       regular file
      l       symbolic link
      p       FIFO
      s       socket
    -size
      True if the file's size, rounded up, in 512-byte blocks is n. 
      If n is followed by a c, then the primary is true if the file's 
      size is n bytes (characters).  Similarly if n is followed by a 
      scale indicator then the file's size is compared to n scaled as:

        k       kilobytes (1024 bytes)
        M       megabytes (1024 kilobytes)
        G       gigabytes (1024 megabytes)
        T       terabytes (1024 gigabytes)
        P       petabytes (1024 terabytes)

## DESIGN
### SOURCE FILES
    the src folder includes:
      Makefile    the makefile of the tool
      env.h       | global include, define, and struct

      parser.h    | the parser of expressions from the bash.
      parser.c    | it will translate the exp to postfix expression

      filter.h    | build the filter tree and  
      filter.c    | inplement all the filters

      tinyfind.c  | the main file of tool and it use api to 
                  | traverse a file hierarchy   

### FILTER DESIGN
    a. Filter Tree And Filter
       Filter tree is a binary tree with node has a two pointer, passed and failed.
       If the filter fails, it will exicute the filter that failed pointer pointed.
       Filter contains a pointer to a filter_cmd.
         filter_cmd  | a function pointer which contains the test function to test   
                     | whether a file can meets a expression
       Filter contains a FilterType.
         FilterType  | it is a enum and cantains the type information of a filter
       Filter contains a void pointer, info.
         info        | it contains some necessary information of a specific filter
       each filter should implement at least two function:
         init_xxx    | to init the a filter
         xxx_filter  | it is filter_cmd, the test function.

    b. Build the Filter Tree
       There are three functions to build the tree
         filter_and  | connect to filter tree with and operation
         filter_or   | connect to filter tree with or operation
         filter_not  | wrap a filter in a not filter adapter
       There are a stack to compute logic operations.

    c. RELEASE the Filter Tree
       There is a list to track all the filters.
       if a filter should release the information in filter, it should be add to
       the _free_filter fuction. 

## FLEXIBLE
### ADD A EXPRESSION
    a. add the name of the  expression to FilterType in 'filter.h' and ALL_EXP
       in 'parser.c'
    b. impelment function init_xxx and xxx_filter in 'filter.c'
    c. register the filter to the init_filter_tree function in 'filter.c' 
    that's all
### ADD A ACTION
    a. add the name of the action to ALL_OPTIONS in 'parser.c'
    b. add flags in options
    c. add some code of the action in set_post_exp in 'parser.c'
    d. add the operation in function check in 'tinyfind.c'
    that's all
### ADD A OPTION
    a. add the name of the option to ALL_OPTIONS in 'parser.c'
    b. add flags in options
    c. add some code of the option  in set_post_exp in 'parser.c'
    d. add the operation in function find in 'tinyfind.c'
    that's all

## COMPARISON
    It can have a tree of test functions 'predicate tree'
    It uses 'fts' api to traverse a file hierarchy
    pred.c    | it contains all the test functions
    tree.c    | to connect all the test functions into a tree

    In my version of 'find', filter tree plays a similar role as tree 
    and filter plays a similar role as predicate.

    The offical 'find' also has a parser to parse the bash parameters, but it is
    more powerful.



