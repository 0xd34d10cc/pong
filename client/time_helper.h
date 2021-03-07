#ifndef TIME_HELPER_H
#define TIME_HELPER_H

#include <sys/time.h>

struct timeval timeval_subtract(struct timeval x, struct timeval y);

// returns -1 in case if lhs more than rhs, 1 if lhs less than rhs and 0 is they are equal
int timeval_compare(const struct timeval* lhs, const struct timeval* rhs);

#endif // TIME_HELPER_H
