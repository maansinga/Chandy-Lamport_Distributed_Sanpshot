#ifndef __setup_helper_c__
#define __setup_helper_c__

#include "setup_helper.h"

int trim_carriage_return(char *str){
  int string_length = strlen(str);
  while((string_length > 0) && (str[string_length - 1] == '\n' || str[string_length - 1] == '\r')){
    str[string_length - 1] = '\0';
    string_length--;
  }

  return(string_length);
}

int trim_leading_whitespaces(char *str){
  int string_length = strlen(str);
  int leading_whitespace_index = 0;
  while((leading_whitespace_index < string_length) &&
   (str[leading_whitespace_index] == ' '))
    leading_whitespace_index++;

  if(leading_whitespace_index == string_length){
    str[0]='\0';
    return(0);
  }else if(leading_whitespace_index != 0)
    strcpy(str, str + sizeof(char) * leading_whitespace_index);

  return(strlen(str));
}

int trim_trailing_whitespaces(char *str){
  int string_length = strlen(str);
  while((string_length > 0) && (str[string_length - 1] == ' ')){
    str[string_length - 1] = '\0';
    string_length--;
  }

  return(string_length);
}

int trim_comments(char *str){
  int string_length = strlen(str);
  int comment_index = 0;

  while((comment_index < string_length) &&
    (str[comment_index] != '#'))
      comment_index++;

  if(comment_index == string_length){
    return(1);
  }else if(comment_index == 0){
    str[0]='\0';
    return(0);
  }else if(comment_index != 0)
    str[comment_index]='\0';

  return(strlen(str));
}

int preprocess_string(char *str){
  trim_carriage_return(str);
  trim_comments(str);
  trim_trailing_whitespaces(str);
  trim_leading_whitespaces(str);

  return(1);
}
#endif