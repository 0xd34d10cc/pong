#include "messages.h"
#include "string.h"

enum MessageType getMessageType(char* buffer) {    
  
  short msg_id = 0; 
  memcpy(&msg_id, buffer+4, sizeof(short));
  
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
