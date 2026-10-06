#include <conio.h>
#include <stdio.h>

#ifndef DEMO_NAME
#define DEMO_NAME "DEMO"
#endif

int main(void)
{
  printf("%s is running.\n", DEMO_NAME);
  printf("Press any key to return to the menu.\n");
  getch();
  return 0;
}
