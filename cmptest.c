#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <ctype.h>

int mystrcmp(char*,char*);

int main (int argc, char *argv[]){

char* str1 = "potato";
char* str2 = "flower";
char* str3 = "potato";
char* strs[] = {
	"-sdfs", "potato"
};
printf("%s\n",str1);
printf("%s\n",strs[1]);
int a =mystrcmp(str1,str2);
printf("testing %s with %s -- result= %d\n",str1,str2,a);
int b =mystrcmp(str1,strs[1]);
printf("testing %s with %s -- result= %d\n",str1,strs[1],b);
int c =mystrcmp(str1,strs[0]);
printf("testing %s with %s -- result= %d\n",str1,strs[0],c);
int d =mystrcmp(str1,str3);
printf("testing %s with %s -- result= %d\n",str1,str3,d);
}


int mystrcmp(char *str1,char *str2){
  while (*str1 && *str1 == *str2) {
    str1++;
    str2++;
  }
  return *str1 - *str2;
}