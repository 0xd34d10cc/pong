#include "messages.h"

#define CREATEGAMESESSIONID 0x00    
#define SENDSESSIONID 0x01    
#define CONNECTTOSESSIONID 0x02    
#define SENDSTATUSID 0x03    
#define NOTIFYUSERID 0X04 

enum MessageType getMessageType(char* buffer) {    
  short msg_id = *((short*)buffer+4);    
  switch(msg_id) {    
    case(CREATEGAMESESSIONID):    
      return CreateGameSession;    
    case(SENDSESSIONID):    
      return SendSession;    
    case(CONNECTTOSESSIONID):    
      return ConnectToSession;    
    case(SENDSTATUSID):    
      return SendStatus;    
    case(NOTIFYUSERID):    
      return NotifyUser;    
    default:       
      return InvalidMsg;    
  }     
}
