#ifndef MESSAGES_H
#define MESSAGES_H

#define PWDEFAULTSIZE 100
                     
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
  int ipv4;    
  int status_code;    
};    

#endif // MESSAGES_H
