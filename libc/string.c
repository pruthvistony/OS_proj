

// String function for user space operations

#include <sys/defs.h>

int strcmp(int8_t *string1, int8_t *string2){
  int i=0,j=0;
  for(;string1[i]!='\0' && string2[j] !='\0' && string1[i] == string2[j]; ++i,++j){
    ;
  }
  return string1[i]-string2[j];
}

int strlen(int8_t *str)
{ 
  uint64_t len = 0, i;
  
  for(i = 0; str[i] != '\0'; i++)
  { 
    len++;
  }
  
  return len;
}


void strcpy(char *s1, char *s2)
{
  int i = 0;

  for(i = 0; s2[i] != '\0'; i++)
    s1[i] = s2[i];
  s1[i] = '\0';

  return;
}

int strspt(char * input, char str[][256], char delim){
  int i=0,j=0,k=0;
  for(;input[i] != '\0';i++){
    if(input[i] == delim){
      if(k>0){
        str[j][k]= '\0';
        k=0;
        j++;
      }
    }else{
      str[j][k] = input[i];
      k++;
    }
  }
  if(k>0){
    str[j][k]='\0';
    j++;
  }
  str[j][0]='\0';
  return j;
}

int strStartsWith(char origStr[], char checkStr[]){
  int i=0, result=0;
  while(checkStr[i] != '\0'){
    if(origStr[i] == '\0' || origStr[i]!=checkStr[i]){
      result = 1;
      break;
    }else{
      i++;
    }
  }
  return result;
}

void strconcat(char *first, char *second, char *final){
  int i=0;
  for(;first[i] != '\0';i++){
    final[i] = first[i];
  }
  int j=0;
  for(;second[j] != '\0';j++){
    final[i+j] = second[j];
  }
  final[i+j] = '\0';
}
