
#include <stdint.h>
#include <stddef.h>

void * memset(void * s, int c, size_t n)
{
	for(size_t i = 0; i < n; i++)
		((char *)s)[i] = c;
	return s;
}

