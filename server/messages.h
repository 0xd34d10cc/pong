#ifndef MESSAGES_H
#define MESSAGES_H

#define PWDEFAULTSIZE 100

#define CREATEGAMESESSIONID 0x00        
#define SENDSESSIONID 0x01        
#define CONNECTTOSESSIONID 0x02        
#define SENDSTATUSID 0x03        
#define NOTIFYUSERID 0X04     

enum MessageType {    
  CreateGameSession = 0,    
  SendSession,    
  ConnectToSession,    
  SendStatus,    
  NotifyUser,    
  InvalidMsg    
};    

enum MessageType getMessageType(char* buffer);

struct CreateGameSessionMsg {    
  int pw_size;    
  char pw[PWDEFAULTSIZE];    
};    
     
struct SendSessionMsg {    
  short id;    
  int session_id;    
};    
     
struct ConnectToSessionMsg {    
  short id;    
  int session_id;    
  int pw_size;    
  char pw[PWDEFAULTSIZE];    
};    
     
struct SendStatusMsg {    
  short id;    
  int status_code;    
};    
     
struct NotifyUserMsg {    
  short id;    
  char ipv4[16];    
  int status_code;    
};    

#endif // MESSAGES_H
