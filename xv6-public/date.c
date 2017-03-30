#include "types.h"
#include "user.h"
#include "date.h"

int
main(int argc, char *argv[])
{
  struct rtcdate r;
  memset(&r, 0, sizeof r);

  if (date(&r)) {
    printf(2, "date failed\n");
    exit();
  }

  printf(2, "time : %d, %d, %d, %d:%d:%d\n", r.hour, r.minute, r.second, r.month, r.day, r.year);

  exit();
}