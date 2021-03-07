#include "time_helper.h"

struct timeval timeval_subtract(struct timeval x, struct timeval y) {
  struct timeval result;
  /* Perform the carry for the later subtraction by updating y. */
  if (x.tv_usec < y.tv_sec) {
    int nsec = (y.tv_usec - x.tv_usec) / 1000000 + 1;
    y.tv_usec -= 1000000 * nsec;
    y.tv_sec += nsec;
  }
  if (x.tv_usec - y.tv_usec > 1000000) {
    int nsec = (x.tv_usec - y.tv_usec) / 1000000;
    y.tv_usec += 1000000 * nsec;
    y.tv_sec -= nsec;
  }

  /* Compute the time remaining to wait.
     tv_usec is certainly positive. */
  result.tv_sec = x.tv_sec - y.tv_sec;
  result.tv_usec = x.tv_usec - y.tv_usec;

  return result;
}

static int compare_values(int lhs, int rhs) {
    return lhs > rhs ? -1 :
           lhs < rhs ?  1 :
                        0;

}

int timeval_compare(const struct timeval* lhs, const struct timeval* rhs) {
    int sec_cmp = compare_values(lhs->tv_sec, rhs->tv_sec);
    return sec_cmp != 0 ? sec_cmp : compare_values(lhs->tv_usec, rhs->tv_usec);
}