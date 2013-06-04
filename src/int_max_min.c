#include <stdio.h> 
#include <limits.h> 
#include <float.h> 

int main(void) 
{ 
if (-2147483648 > 0) printf("positive\n"); 
if (-2147483647-1 < 0) printf("negative\n"); 
if (INT_MIN == -INT_MIN) printf("equal\n"); 
if (FLT_MIN > 0) printf("floating\n"); 

return 0; 
}
