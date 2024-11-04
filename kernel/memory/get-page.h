#ifndef GET_PAGE_H
#define GET_PAGE_H

#include <cstddef>

void *getPage(size_t count);
void freePage(void *ptr);

#endif //GET_PAGE_H
