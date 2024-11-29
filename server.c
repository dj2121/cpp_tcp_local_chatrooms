#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h> 
#include <sys/ipc.h> 
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>


union semun {
	int val;
	struct semid_ds *buf;
	ushort * array;
};

void readfile(int index, char* rBuf){
    char* fileName = "server_records.txt";
    FILE* file = fopen(fileName, "ab+");
    char line[256];

    int i = 1;

    while (fgets(line, sizeof(line), file)) {
        if(index == i){
            fclose(file);
            strcpy(rBuf, line);
            return;
        }
        i++;
    }

    fclose(file);
    strcpy(rBuf, "READX: Index does not exist in file.");
    return;
}

int writefile(char *msg){

    char* fileName = "server_records.txt"; 
    FILE* file = fopen(fileName, "ab+");
    char line[256];

    fputs(msg, file);
    fclose(file);
    return 1;
}


struct message{
    int cid;
    char data[255];
    int flag;
};


struct logDB{
    int cid;
    char data[255];
    int flag;
};

struct clientArr{
    int memid;
    int verified;
    int admin;
};

struct group{
    int gid;
    int size;
    struct clientArr members[100]; 
    int flag;
};

struct database{
    int messageNum;
    struct message messages[100];

    int logNum;
    struct logDB log[100];

    int groupNum;
    struct group groups[100];

    int clientNum;
    int clients[100];
};


void initDB(struct database *db){
    db->clientNum = 0;
    db->groupNum = 0;
    db->messageNum = 0;
    db->logNum = 0;
}

void addMessage(struct database *db, char *msg, int clientId){
    db->messages[db->messageNum].cid = clientId;
    sprintf(db->messages[db->messageNum].data, "%s", msg);
    db->messages[db->messageNum].flag = 0;
    db->messageNum++;
}

void addLog(struct database *db, char *msg, int clientId){
    db->log[db->logNum].cid = clientId;
    sprintf(db->log[db->logNum].data, "%s", msg);
    printf("Client %d: %s\n", clientId, db->log[db->logNum].data);
    db->log[db->logNum].flag = 0;
    db->logNum++;
}


void deleteGroup(struct database *db, int gIndex){

    int lIndex = db->groupNum - 1;

    if(gIndex == lIndex){
        db->groupNum--;
        return;
    }

    else{

        db->groups[gIndex].flag = db->groups[lIndex].flag;
        db->groups[gIndex].gid = db->groups[lIndex].gid;
        db->groups[gIndex].size = db->groups[lIndex].size;

        for(int i = 0; i < db->groups[lIndex].size; i++){
            db->groups[gIndex].members[i].admin = db->groups[lIndex].members[i].admin;
            db->groups[gIndex].members[i].memid = db->groups[lIndex].members[i].memid;
            db->groups[gIndex].members[i].verified = db->groups[lIndex].members[i].verified;
        }

        db->groupNum--;

        return;

    }

}


void processExit(struct database *db){

    int cDis = -1;

    for(int i = 0; i < db->logNum; i++){
        if(strstr(db->log[i].data, "/quit")){

            db->log[i].flag = 1;  

            //Deleting client from client List
            for(int j = 0; j < db->clientNum; j++){
                if(db->log[i].cid == db->clients[j]){

                    cDis = db->log[i].cid;

                    if(i == db->clientNum-1){
                        db->clientNum--;
                        break;
                    }
                    else{
                        db->clients[j] = db->clients[db->clientNum-1];
                        db->clientNum--;
                        break;
                    }
                }
            }

            //Deleting groups where client was admin
            for(int i = 0; i < db->groupNum; i++){

                for(int j = 0; j < db->groups[i].size; j++){
                    if(db->groups[i].members[j].memid == cDis && db->groups[i].members[j].admin == 1){
                        deleteGroup(db, i);
                        break;
                    } 
                    if(db->groups[i].members[j].memid == cDis && db->groups[i].members[j].admin == 0){
                        db->groups[i].members[j].verified = 0;
                        break;
                    } 
                }

            }

            char target[255];
            sprintf(target, "Server: Client %d has disconnected.", cDis);
            printf("Client with id %d bas disconnected.", cDis);

            for(int i = 0; i < db->clientNum; i++){
                addMessage(db, target, db->clients[i]);
            }        

        }

    }
}

void processMessage(char *msg, struct database *db, int clientId){


    if(strstr(msg, "/activegroups")){

        char target[255];
        sprintf(target, "Server: Active groups (IDs) that you are part of (space seperated): ");

        for(int i = 0; i < db->groupNum; i++){
            if(db->groups[i].flag == 1){

                for(int j = 0; j < db->groups[i].size; j++){
                    if(clientId == db->groups[i].members[j].memid && db->groups[i].members[j].verified == 1){
                        sprintf(target + strlen(target), "%d ", db->groups[i].gid);
                    }
                }

            }
        }

        addMessage(db, target, clientId);
    }

    else if(strstr(msg, "/activeallgroups")){

        char target[255];
        sprintf(target, "Server: Active groups (IDs) (space seperated): ");

        for(int i = 0; i < db->groupNum; i++){
            if(db->groups[i].flag == 1){
                sprintf(target + strlen(target), "%d ", db->groups[i].gid);
            }
        }

        addMessage(db, target, clientId);

    }

    
    else if(strstr(msg, "/active")){
        char target[255];
        sprintf(target, "List of Active Clients (Space Seperated): ");

        for(int i = 0; i < db->clientNum; i++){
            if(db->clients[i] == clientId){
                sprintf(target + strlen(target), "%d(Your ID) ", db->clients[i]);
            }
            else{
                sprintf(target + strlen(target), "%d ", db->clients[i]);
            }
        }
        addMessage(db, target, clientId);
    }


    else if(strstr(msg, "/sendgroup")){

        char *token = strtok(msg, " ");
        token = strtok(NULL, " ");

        int gId = atoi(token);

        int found = 0;
        int active = 0;
        int member = 0;
        int gIndex = -1;

        for(int i = 0; i < db->groupNum; i++){
            if(gId == db->groups[i].gid){
                found = 1;
                if(db->groups[i].flag == 1){
                    active = 1;
                    for(int j = 0; j < db->groups[i].size; j++){
                        if(db->groups[i].members[j].memid == clientId && db->groups[i].members[j].verified == 1){
                            member = 1;
                            gIndex = i;
                            break;
                        }
                    }
                    break;
                }                

            }
        }

        if(found == 0){
            char target[255];
            sprintf(target, "Server: Group with Id %d not found", gId);
            addMessage(db, target, clientId);
        }

        else if(found == 1 && active == 0){
            char target[255];
            sprintf(target, "Server: Group with Id %d is Inactive", gId);
            addMessage(db, target, clientId);
        }

        else if(found == 1 && active == 1 && member == 0){
            char target[255];
            sprintf(target, "Server: You are not a member of the group with Id %d", gId);
            addMessage(db, target, clientId);
        }

        else if(found == 1 && member == 1){

            token = strtok(NULL, " ");

            char target[255];

            sprintf(target, "Group %d: Client %d:", gId, clientId);

            while( token != NULL && strlen(target) < 255 ) {
                sprintf(target + strlen(target), "%s ", token);
                token = strtok(NULL, " ");
            }

            for(int j = 0; j < db->groups[gIndex].size; j++){

                if(db->groups[gIndex].members[j].memid != clientId && db->groups[gIndex].members[j].verified == 1){
                    addMessage(db, target, db->groups[gIndex].members[j].memid);
                }

            }
        }

    }


    else if(strstr(msg, "/send")){

        char *token = strtok(msg, " ");
        token = strtok(NULL, " ");
        int dest = atoi(token);

        char target[255];
        sprintf(target, "Message from client %d: ", clientId);

        token = strtok(NULL, " ");

        while( token != NULL && strlen(target) < 255 ) {
            sprintf(target + strlen(target), "%s ", token);
            token = strtok(NULL, " ");
        }

        int flag = 1;

        for(int i = 0; i < db->clientNum; i++){
            if(db->clients[i]==dest){
                addMessage(db, target, dest);
                flag = 0;
            }
        }

        if(flag){
            sprintf(target, "Destination client with id %d unavailable.", dest);
            addMessage(db, target, clientId);
        }

        

    }

    else if(strstr(msg, "/broadcast")){

        for(int i = 0; i < db->clientNum; i++){
            if(clientId != db->clients[i]){
                addMessage(db, msg+11, db->clients[i]);
            }
        }      

    }

    else if(strstr(msg, "/makegroupreq")){

        char *token = strtok(msg, " ");
        token = strtok(NULL, " ");

        int lastGid;

        if(db->groupNum > 0){
            lastGid = db->groups[db->groupNum-1].gid;
        }
        else{
            lastGid = 0;
        }

        int dArr[100];
        int mCount = 0;
        int valid = 0;

        while(token != NULL) {
            int dest = atoi(token);
            valid = 0;

            for(int i = 0; i < db->clientNum; i++){
                if(dest == db->clients[i]){
                    valid = 1;
                }
            }

            if(valid == 1){
                dArr[mCount] = dest;
                mCount++;
            }            

            token = strtok(NULL, " ");
        }

        int cIn = 0;

        for(int i = 0; i < mCount; i++){
            if(dArr[i]==clientId){
                cIn = 1;
                break;
            }
        }

        int newGid = lastGid + 1;

        if(cIn == 1){    

            db->groups[db->groupNum].gid = newGid;
            db->groups[db->groupNum].flag = 0;
            db->groups[db->groupNum].size = mCount;           

            char target[255];
            sprintf(target, "Server: You have been asked to a group with ID %d by client %d. Send /joingroup %d to confirm", newGid, clientId, newGid);

            for(int i = 0; i < mCount; i++) {
                int dest = dArr[i];
                db->groups[db->groupNum].members[i].memid = dest;
                db->groups[db->groupNum].members[i].verified = 0;

                db->groups[db->groupNum].members[i].admin = 0;

                if(dest == clientId){
                    db->groups[db->groupNum].members[i].admin = 1;
                    db->groups[db->groupNum].members[i].verified = 1;
                }
                else{
                    addMessage(db, target, dest);
                }
            }  

            db->groupNum++; 
        }

        else if(cIn == 0){
            char target[255];
            sprintf(target, "Server: You must include yourself in the created group.");
            addMessage(db, target, clientId);
        }            

    }

    else if(strstr(msg, "/makegroup")){

        char *token = strtok(msg, " ");
        token = strtok(NULL, " ");

        int lastGid;

        if(db->groupNum > 0){
            lastGid = db->groups[db->groupNum-1].gid;
        }
        else{
            lastGid = 0;
        }

        int dArr[100];
        int mCount = 0;
        int valid = 0;

        while(token != NULL) {
            int dest = atoi(token);
            valid = 0;

            for(int i = 0; i < db->clientNum; i++){
                if(dest == db->clients[i]){
                    valid = 1;
                }
            }

            if(valid == 1){
                dArr[mCount] = dest;
                mCount++;
            }            

            token = strtok(NULL, " ");
        }

        int cIn = 0;
        int bad = 0;

        for(int i = 0; i < mCount; i++){
            if(dArr[i]==clientId){
                cIn = 1;
                break;
            }
        }

        for(int i = 0; i < mCount; i++){
            for(int j = i+1; j < mCount; j++){
                if(dArr[i] == dArr[j]){
                    bad = 1;
                }
            }
        }

        int newGid = lastGid + 1;

        if(cIn == 1 && bad == 0){    

            db->groups[db->groupNum].gid = newGid;
            db->groups[db->groupNum].flag = 1;
            db->groups[db->groupNum].size = mCount;           

            char target[255];
            sprintf(target, "Server: You have been added to a group with ID %d by client %d.", newGid, clientId);

            for(int i = 0; i < mCount; i++) {
                int dest = dArr[i];
                db->groups[db->groupNum].members[i].memid = dest;
                db->groups[db->groupNum].members[i].verified = 1;

                db->groups[db->groupNum].members[i].admin = 0;

                if(dest == clientId){
                    db->groups[db->groupNum].members[i].admin = 1;
                }

                addMessage(db, target, dest);
            }  

            db->groupNum++; 
        }

        else if(cIn == 0){
            char target[255];
            sprintf(target, "Server: You must include yourself in the created group.");
            addMessage(db, target, clientId);
        }   

        else if(bad == 1){
            char target[255];
            sprintf(target, "Server: Please supply each clientID only once.");
            addMessage(db, target, clientId);
        }           

    }
    
    else if(strstr(msg, "/joingroup")){

        char *token = strtok(msg, " ");
        token = strtok(NULL, " ");
        int gId = atoi(token);

        int gExists = 0;
        int gValid = 0;
        int gIndex;

        for(int i = 0; i < db->groupNum; i++){
            if(gId == db->groups[i].gid){

                gExists = 1;
                gIndex = i;

                if(db->groups[i].flag == 0){
                    gValid = 1;
                }

                break;

            }
        }

        if(gExists == 0){
            char target[255];
            sprintf(target, "Server: Target group not found.");
            addMessage(db, target, clientId);
        }

        else if(gExists == 1 && gValid == 0){

            db->groups[gIndex].members[db->groups[gIndex].size].memid = clientId;
            db->groups[gIndex].members[db->groups[gIndex].size].admin = 0;
            db->groups[gIndex].members[db->groups[gIndex].size].verified = 0;
            db->groups[gIndex].size++;

            char target[255];
            sprintf(target, "Server: Group already active. Admin will confirm your Join request.");
            addMessage(db, target, clientId);

            int adminId = -1;

            for(int i = 0; i < db->groups[gIndex].size; i++){
                if(db->groups[gIndex].members[i].admin == 1){
                    adminId = db->groups[gIndex].members[i].memid;
                    break;
                }
            }

            char target2[255];
            sprintf(target2, "Server: Client %d wants to join group %d. Approve by sending /approve %d %d", clientId, db->groups[gIndex].gid, gIndex, clientId);
            addMessage(db, target2, adminId);
        }

        else if(gExists == 1 && gValid == 1){

            int cFound = 0;
            int cValid = 0;
            int cIndex;

            for(int i = 0; i < db->groups[gIndex].size; i++){
                if(db->groups[gIndex].members[i].memid == clientId){
                    cFound = 1;

                    if(db->groups[gIndex].members[i].verified == 0){
                        cValid = 1;
                        cIndex = i;
                    }

                    break;
                }
            }

            if(cFound == 0){
                char target[255];
                sprintf(target, "Server: You were not invited to this group.");
                addMessage(db, target, clientId); 
            }
            else if(cFound == 1 && cValid == 0){
                char target[255];
                sprintf(target, "Server: You are already an active member of this group.");
                addMessage(db, target, clientId); 
            }
            else if(cFound == 1 && cValid == 1){
                db->groups[gIndex].members[cIndex].verified = 1;
                char target[255];
                sprintf(target, "Server: You have successfully joined the group.");
                addMessage(db, target, clientId); 


                int gActive = 1;

                for(int i = 0; i < db->groups[gIndex].size; i++){
                    if(db->groups[gIndex].members[i].verified == 0){
                        gActive = 0;
                        break;
                    }
                }

                if(gActive == 1){

                    db->groups[gIndex].flag = 1;

                   for(int i = 0; i < db->groups[gIndex].size; i++){
                        char target[255];
                        sprintf(target, "Server: Group %d is active now.", db->groups[gIndex].gid);
                        addMessage(db, target, db->groups[gIndex].members[i].memid); 
                   } 
                }


            }

        }
    }


    else if(strstr(msg, "/approve")){

        char *token = strtok(msg, " ");
        token = strtok(NULL, " ");
        int gIndex = atoi(token);
        token = strtok(NULL, " ");
        int cId = atoi(token);

        if(gIndex >= 0 && gIndex < db->groupNum){

            int admin = -1;

            for(int i = 0; i < db->groups[gIndex].size; i++){
                if(db->groups[gIndex].members[i].memid == clientId && db->groups[gIndex].members[i].admin == 1){
                    admin = 1;
                    break;
                }
            }

            if(admin == -1){
                char target[255];
                sprintf(target, "Server: Invalid approve command. You are not an admin of that group.");
                addMessage(db, target, clientId); 
            }
            else if(admin == 1){

                int memfound = 0;

                for(int i = 0; i < db->groups[gIndex].size; i++){
                    if(db->groups[gIndex].members[i].memid == cId && db->groups[gIndex].members[i].verified == 0){
                        memfound = 1;
                        db->groups[gIndex].members[i].verified = 1;
                        char target[255];
                        sprintf(target, "Server: Join Request Approved by admin for group %d.", db->groups[gIndex].gid);
                        addMessage(db, target, cId); 

                    }
                }

                if(memfound == 0){
                    char target[255];
                    sprintf(target, "Server: Client supplied has no pending join request.");
                    addMessage(db, target, clientId);
                }
                
                else{

                    char target[255];
                    sprintf(target, "Server: Request Approved. Client %d added to the group.", cId);
                    addMessage(db, target, clientId);

                } 
            }

        }

        else{
            char target[255];
            sprintf(target, "Server: Invalid approve command. Group not found.");
            addMessage(db, target, clientId); 
        }

    }


    else if(strstr(msg, "/declinegroup")){

        char *token = strtok(msg, " ");
        token = strtok(NULL, " ");
        int gId = atoi(token);

        int gExists = 0;
        int gValid = 0;
        int gIndex;

        for(int i = 0; i < db->groupNum; i++){
            if(gId == db->groups[i].gid){

                gExists = 1;

                if(db->groups[i].flag == 0){
                    gValid = 1;
                    gIndex = i;
                }

                break;

            }
        }

        if(gExists == 0){
            char target[255];
            sprintf(target, "Server: Target group not found.");
            addMessage(db, target, clientId);
        }

        else if(gExists == 1 && gValid == 0){
            char target[255];
            sprintf(target, "Server: Invalid group Id provided.");
            addMessage(db, target, clientId);
        }

        else if(gExists == 1 && gValid == 1){

            int cFound = 0;
            int cValid = 0;
            int cIndex;

            for(int i = 0; i < db->groups[gIndex].size; i++){
                if(db->groups[gIndex].members[i].memid == clientId){
                    cFound = 1;

                    if(db->groups[gIndex].members[i].verified == 0){
                        cValid = 1;
                        cIndex = i;
                    }

                    break;
                }
            }

            if(cFound == 0){
                char target[255];
                sprintf(target, "Server: You were not invited to this group.");
                addMessage(db, target, clientId); 
            }
            else if(cFound == 1 && cValid == 0){
                char target[255];
                sprintf(target, "Server: You are already an active member of this group.");
                addMessage(db, target, clientId); 
            }
            else if(cFound == 1 && cValid == 1){
                db->groups[gIndex].members[cIndex].verified = 0;
                char target[255];
                sprintf(target, "Server: You have declined group invitation.");
                addMessage(db, target, clientId); 
            }

        }


    }


    else{
        char target[255];
        sprintf(target, "Server: Unrecognized Command.");
        addMessage(db, target, clientId);
    }
    

}


void error(char *msg)
{
    perror(msg);
    exit(1);
}

int main(){

    int sockIn, sockAcc, portNum;
    struct sockaddr_in serverAddr, clientAddr;
    char buffer[256];

    printf("Enter Port Number to Listen to: ");
    scanf("%d", &portNum);
    printf("\n");

    //Create a Socket
    sockIn = socket(AF_INET, SOCK_STREAM, 0);
    if (sockIn < 0){
        error("ERROR opening socket");
    }

    bzero((char *) &serverAddr, sizeof(serverAddr));

    //Set values in serverAddr
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(portNum);
    if (bind(sockIn, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0){
        error("ERROR on binding");
    }

    int handler = 1;

    listen(sockIn, 5);

    int pid = 0;
    int cid = -1;
    int fres = -2;

    union semun argument1, argument0;
	argument1.val = 1;
    argument0.val = 0;

    //Semaphore childToggle
    int semc = 1234;
   	int id1 = semget(semc, 1, 0666 | IPC_CREAT);
    semctl(id1, 0, SETVAL, argument1);

    //Generate a unique key 
    key_t key = ftok("sharedmem", 65); 
    int shmid = shmget(key, sizeof(struct database), 0666|IPC_CREAT); 
    struct database *db = (struct database *) shmat(shmid, NULL, 0); 
    initDB(db);


    if(handler > 0){

        printf("Waiting to client...\n"); 

        while(1){   

            //Wait for client to connect, and accept conection
            int cSize = sizeof(clientAddr);
            sockAcc = accept(sockIn, (struct sockaddr *) &clientAddr, &cSize);
            if(sockAcc < 0){
                error("Error on accept");
            }

            pid++;

            int badP = 0;

            fres = fork();

            if(fres == 0){

                struct database *db = (struct database *) shmat(shmid, (void*)0, 0); 

                //Waiting on Child Semaphore
                {
                    int id; 
                    struct sembuf operations[1];
                    int retval;

                    id = semget(semc, 1, 0666);

                    if(id < 0)
                    {
                        fprintf(stderr, "Cannot find semaphore, exiting.\n");
                        exit(0);
                    }

                    operations[0].sem_num = 0;
                    operations[0].sem_op = -1;  
                    operations[0].sem_flg = 0;

                    retval = semop(id, operations, 1);
                }

                if(db->clientNum > 4){
                    char target[255];
                    sprintf(target, "Server: Client capacity full. Maybe try at a later time");
                    int valread = write(sockAcc, target, strlen(target));
                    badP = 1;
                }

                else{
                    db->clients[db->clientNum] = sockAcc;
                    db->clientNum++; 

                    char target[255];
                    sprintf(target, "Server: Welcome to the chatroom. Your ClientID: %d", sockAcc);
                    int valread = write(sockAcc, target, strlen(target));

                    printf("New client with clientid %d added. \n", sockAcc);
                }

                //Signalling child Semaphore
                {
                    int id; 
                    struct sembuf operations[1];
                    int retval;

                    id = semget(semc, 1, 0666);

                    if(id < 0)
                    {
                        fprintf(stderr, "Cannot find semaphore, exiting.\n");
                        exit(0);
                    }

                    operations[0].sem_num = 0;
                    operations[0].sem_op = 1;  
                    operations[0].sem_flg = 0;

                    retval = semop(id, operations, 1);
                }

                int n;

                if(badP == 1){
                    break;
                }

                while(1){

                    //Read from socket and process message
                    bzero(buffer, 256);
                    n = read(sockAcc, buffer, 255);
                    if (n < 0) error("ERROR reading from socket");

                    //Waiting on Child Semaphore
                    {
                        int id; 
                        struct sembuf operations[1];
                        int retval;

                        id = semget(semc, 1, 0666);

                        if(id < 0)
                        {
                            fprintf(stderr, "Cannot find semaphore, exiting.\n");
                            exit(0);
                        }

                        operations[0].sem_num = 0;
                        operations[0].sem_op = -1;  
                        operations[0].sem_flg = 0;

                        retval = semop(id, operations, 1);
                    }

                    if(strstr(buffer, "recv") != buffer){
                        
                        addLog(db, buffer, sockAcc); 

                        processExit(db);

                        for(int i = 0; i < db->logNum; i++){
                            if(db->log[i].flag == 0){
                                processMessage(db->log[i].data, db, db->log[i].cid);
                                sprintf(db->log[i].data, "0");
                            }
                        }

                        //Clear log
                        db->logNum = 0;
                    }

                    //Write to open sockets
                    for (int i = 0; i < db->messageNum; i++) {

                        if(db->messages[i].cid == sockAcc){

                            int valread = write(db->messages[i].cid, db->messages[i].data, strlen(db->messages[i].data));
                            printf("Outgoing Message: Client %d: %s\n", db->messages[i].cid, db->messages[i].data);

                            if(i == db->messageNum - 1){
                                db->messageNum--;
                            }

                            else{
                                sprintf(db->messages[i].data, "%s", db->messages[db->messageNum - 1].data);
                                db->messages[i].cid = db->messages[db->messageNum - 1].cid;
                                db->messages[i].flag = db->messages[db->messageNum - 1].flag;
                                db->messageNum--;
                                i--;
                            }

                        }
                    }

                    //Signalling child Semaphore
                    {
                        int id; 
                        struct sembuf operations[1];
                        int retval;

                        id = semget(semc, 1, 0666);

                        if(id < 0)
                        {
                            fprintf(stderr, "Cannot find semaphore, exiting.\n");
                            exit(0);
                        }

                        operations[0].sem_num = 0;
                        operations[0].sem_op = 1;  
                        operations[0].sem_flg = 0;

                        retval = semop(id, operations, 1);
                    }

                    if(strstr(buffer, "/quit")){
                        break;
                    }

                }   

                close(sockAcc);
                break;

            }

            else if(fres == -1){
                printf("Process creation error.\n");
            }

        }

        if(fres != 0){
            close(sockIn);
        }


    }

    return 0;
}