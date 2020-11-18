#include "messages.h"
#include "string.h"

int getMessageType(char* buffer) {

  short msg_id = 0;
  memcpy(&msg_id, buffer+4, sizeof(short));

  return msg_id;
}
