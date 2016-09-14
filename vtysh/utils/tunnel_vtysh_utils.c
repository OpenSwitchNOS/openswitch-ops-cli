#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "tunnel_vtysh_utils.h"

void get_mode_from_line(const char line[], char *mode_str)
{
  unsigned int count = 0, i = 0, j = 0, indx = 0;

  for(i=0; i<strlen(line); i++)
  {
    if(line[i]==0x20)
      count += 1;

    if(count >= 3)
    {
      for(j=i+1; j<strlen(line); j++)
      {
        indx = j-i-1;
        mode_str[indx] = line[j];
      }
      mode_str[j-i] = '\0';
      break;
    }
  }
}
