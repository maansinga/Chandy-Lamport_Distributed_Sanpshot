#ifndef __setup_helper_h__
#define __setup_helper_h__

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>

int trim_carriage_return(char *);
int trim_leading_whitespaces(char *);
int trim_trailing_whitespaces(char *);
int trim_comments(char *);
int preprocess_string(char *);

#endif