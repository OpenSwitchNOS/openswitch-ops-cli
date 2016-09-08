#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "tunnel_vtysh_utils.h"

char *get_mode_from_line(const char line[])
{
  unsigned int count = 0, i = 0, j = 0, indx = 0;
  char *temp = malloc(sizeof(strlen(line)));

  for(i=0; i<strlen(line); i++)
  {
    if(line[i]==0x20)
      count += 1;

    if(count >= 3)
    {
      for(j=i+1; j<strlen(line); j++)
      {
        indx = j-i-1;
        temp[indx] = line[j];
      }
      temp[j-i] = '\0';
      break;
    }
  }
  return temp;
}
