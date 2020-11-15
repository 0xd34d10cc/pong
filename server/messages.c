#include "messages.h"

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
