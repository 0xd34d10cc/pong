#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

#define SERVERPORT 1337
#define BUFFSIZE 512

#define PWDEFAULTSIZE 100

#define CREATEGAMESESSIONID 0x00
#define SENDSESSIONID 0x01
#define CONNECTTOSESSIONID 0x02
#define SENDSTATUSID 0x03
#define NOTIFYUSERID 0X04

enum StatusCode {
    OK = 0,
    WrongSessionID,
    WrongPassword
};

struct CreateGameSessionMsg {
    short id;
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


// len(255.255.255.255:50000) - 21 + 1 (zero terminated code)
struct ConnectionStorage {
    char player1_addr[22];
    char player2_ip[22];
    int pw_size; 
    char pw[PWDEFAULTSIZE];
};

struct Map {
    int id;
    struct ConnectionStorage connection_storage;
    struct Map* left;
    struct Map* right;
};

void insert(struct Map* map, int id, struct ConnectionStorage* connect) {
    if (map->id == id) {
        return;
    } 
    if (map->id < id) {
        if (map->right) insert(map->right, id, connect);
        else {
            Map * new_map = malloc(sizeof(Map));
            new_map->connection_storage = *connect;
            new_map->id = id;
            map->right = new_map;
            return;
        }
    }
    if (map->id > id) {
        if(map->left) insert(map->left, id, connect);
        else {
            Map * new_map = malloc(sizeof(Map));
            new_map->connection_storage = *connect;
            new_map->id = id;
            map->left = new_map;
            return;
        }
    }
}

int main(int argc, char argv[][]) {
    // ./server 127.0.0.1 1337
    if (argc != 3)
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2])); 

    // TODO
    addr.sin_addr.s_addr = inet_addr(argv[1]);
    return 0;
}