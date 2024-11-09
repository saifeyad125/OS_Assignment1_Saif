#include<stdbool.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>


#define MAX_FW_CMD 255


typedef struct CmdArg
{
    bool is_interactive;
    int port;
} CmdArg;

typedef struct FwRequest
{
    char RawCmd[MAX_FW_CMD];
    char Cmd;
    struct FwRequest* pNext;
} FwRequest;

typedef struct FwRule
{
    //ipAdress 1, 2
    //port 1, 2
    char RawCmd[MAX_FW_CMD];
    unsigned char ip1[15];
    unsigned char ip2[15];
    int port1;
    int port2;

    struct FwRule* pNext;
    struct FwQuery* qHead; 
} FwRule;

typedef struct FwQuery
{
    unsigned char qiP[15];
    int qPort;
    struct FwQuery* pNext;
} FwQuery;


bool is_digit(char c)
{
    if (c >= '0' && c <= '9'){
        return true;
    }
    else{
        return false;
    }
}

//I will make a power function, so 2^3 = 8
int powi(int b, int e)
{
    int result = 1;
    for (int i = 0; i < e; i++)
    {
        result *= b;
    }
    return result;
}

bool is_integer(char* pch, int* num)
{
   *num = 0;
   for (int i = 0; i < strlen(pch); i++)
   {
       if (is_digit(pch[i]))
       {
           *num += powi(10, i)*(pch[i] - 48);
       }
       else{
        return false;
       }
   }
   return true;
}

//adds a request to the end of the linked list
void add_to_Req_list(FwRequest* fwReq, FwRequest** fwReqHead)
{
    if (*fwReqHead == NULL)
    {
        *fwReqHead = fwReq;
        return;
    }
    FwRequest* cur = *fwReqHead;
    while (cur->pNext != NULL)
    {
        cur = cur->pNext;
    }
    cur->pNext = fwReq;
}

//adds a rule to the end of the linked list
void add_to_rule_list(FwRule* fwRule, FwRule** fwRuleHead)
{
    
    if (*fwRuleHead == NULL)
    {
        *fwRuleHead = fwRule;
        return;
    }
    FwRule* cur = *fwRuleHead;
    while (cur->pNext != NULL)
    {
        cur = cur->pNext;
    }
    cur->pNext = fwRule;
   printf("add_to_rule_list\n");
   return;
}


bool isValidIP(FwRule* fwRule){
    bool smaller = false;
    bool correctValues = true;

    for (int i = 0; i < 4; i++) {
        if (fwRule->ip1[i] < 0 || fwRule->ip1[i] > 255 || fwRule->ip2[i] < 0 || fwRule->ip2[i] > 255) {
            correctValues = false;
            break;
        }
        if (fwRule->ip1[i] < fwRule->ip2[i]) {
            smaller = true;
            break;
        } else if (fwRule->ip1[i] > fwRule->ip2[i]) {
            smaller = false;
            break;
        }
    }

    return smaller && correctValues;
}



bool isValidPort(FwRule* fwRule){
    bool correctValues = false;
    bool smaller = false;

    if (fwRule->port1 >= 0 && fwRule->port1 <= 65535 && fwRule->port2 >= 0 && fwRule->port2 <= 65535)
    {
        correctValues = true;
    }

    if (correctValues)
    {
        if (fwRule->port1 < fwRule->port2)
        {
            smaller = true;
        }
    }
    return smaller;
}


//if the rule is valid then return true
bool isValidRule(FwRule* fwRule)
{
    if (isValidIP(fwRule) && isValidPort(fwRule))
    {
        return true;
    }
    else{
        return false;
    }
}


bool process_args(int argc, char** argv, CmdArg* pcmd)
{
    pcmd->is_interactive = false; //default value
    pcmd->port = 0; //default value
    bool bSucess = false;

    if (argc != 2) {
        return false;}
    bSucess = false;
    

    if (strcmp(argv[1], "-i") == 0){
        pcmd->is_interactive = true;
        bSucess = true;
        }
    else{
        if (is_integer(argv[1], &pcmd->port)){
            pcmd->is_interactive = false;
            bSucess = true;
        }
    }
    return bSucess;
}

FwRule* process_rule_cmd(char* buffer)
{
    FwRule* fwRule = (FwRule*)malloc(sizeof(FwRule));
    if (fwRule == NULL) {
        printf("Memory allocation failed\n");
        exit(1);
    }
    fwRule->pNext = NULL;
    fwRule -> qHead = NULL;
    strcpy(fwRule->RawCmd, buffer);


    // Initialize the fields
    memset(fwRule->ip1, 0, sizeof(fwRule->ip1));
    memset(fwRule->ip2, 0, sizeof(fwRule->ip2));
    fwRule->port1 = 0;
    fwRule->port2 = 0;

    // Split the input string by spaces
    char* pch = strtok(buffer, " ");
    if (pch == NULL) {
        printf("Invalid Rule\n");
        free(fwRule);
        return NULL;
    }

    // Process the IP address part
    char* dashPos = strchr(pch, '-');
    if (dashPos != NULL) {
        *dashPos = '\0';
        if (sscanf(pch, "%hhu.%hhu.%hhu.%hhu", &fwRule->ip1[0], &fwRule->ip1[1], &fwRule->ip1[2], &fwRule->ip1[3]) != 4) {
            printf("Invalid IP address: %s\n", pch);
            free(fwRule);
            return NULL;
        }
        if (sscanf(dashPos + 1, "%hhu.%hhu.%hhu.%hhu", &fwRule->ip2[0], &fwRule->ip2[1], &fwRule->ip2[2], &fwRule->ip2[3]) != 4) {
            printf("Invalid IP address: %s\n", dashPos + 1);
            free(fwRule);
            return NULL;
        }
    } else {
        if (sscanf(pch, "%hhu.%hhu.%hhu.%hhu", &fwRule->ip1[0], &fwRule->ip1[1], &fwRule->ip1[2], &fwRule->ip1[3]) != 4) {
            printf("Invalid IP address: %s\n", pch);
            free(fwRule);
            return NULL;
        }
        memcpy(fwRule->ip2, fwRule->ip1, sizeof(fwRule->ip1));
    }

    // Process the port part
    pch = strtok(NULL, " ");
    if (pch != NULL) {
        dashPos = strchr(pch, '-');
        if (dashPos != NULL) {
            *dashPos = '\0';
            fwRule->port1 = atoi(pch);
            fwRule->port2 = atoi(dashPos + 1);
        } else {
            fwRule->port1 = atoi(pch);
            fwRule->port2 = atoi(pch);
        }
    }

    return fwRule;
}





void run_add_cmd(FwRequest* fwReq, FwRule** fwRuleHead)
{
    char buffer [255];
    printf("running add\n");
    
    printf("enter command\n");
    fgets(buffer, 255, stdin);
    FwRule* fwRule = process_rule_cmd(buffer);

    if (isValidRule(fwRule)){
        add_to_rule_list(fwRule, fwRuleHead);
        printf("Rule added\n");
    }
    else{
        printf("Illegal IP address or port Specified\n");
        free(fwRule);
    }
    
}

void run_del_cmd(FwRequest* fwReq, FwRule* fwRuleHead)
{
    printf("running del\n");
}

void run_list_cmd(FwRequest* fwReq, FwRule* fwRuleHead)
{
    printf("running list\n");
    FwRule* cur = fwRuleHead;
    if (cur == NULL)
    {
        printf("No rules\n");
        return;
    }
    while (cur != NULL)
    {
        printf("%s\n", cur->RawCmd);
        cur = cur->pNext;
    }
}

void run_list_requests_cmd(FwRequest* fwReq, FwRequest* fwReqHead)
{
    printf("running list requests\n");
    FwRequest* cur = fwReqHead;

    if (cur == NULL)
    {
        printf("No requests\n");
        return;
    }
    while (cur != NULL)
    {
        printf("%s\n", cur->RawCmd);
        cur = cur->pNext;
    }
}

FwQuery* process_query_cmd(char* buffer)
{
    FwQuery* fwQuery = (FwQuery*)malloc(sizeof(FwQuery));
    if (fwQuery == NULL) {
        printf("Memory allocation failed\n");
        exit(1);
    }
    fwQuery->pNext = NULL;

    //split the query by space
    char* pch = strtok(buffer, " ");
    if (pch == NULL) {
        printf("Invalid Query\n");
        free(fwQuery);
        return NULL;
    }

    //process the IP address part
    if (sscanf(pch, "%hhu.%hhu.%hhu.%hhu", &fwQuery->qiP[0], &fwQuery->qiP[1], &fwQuery->qiP[2], &fwQuery->qiP[3]) != 4) {
            printf("Invalid IP address: %s\n", pch);
            free(fwQuery);
            return NULL;
        }
    
    //process the port part
    pch = strtok(NULL, " ");
    if (pch != NULL) {
        fwQuery->qPort = atoi(pch);
    }
    else{
        printf("Invalid Query\n");
        free(fwQuery);
        return NULL;
    }

    return fwQuery;
}

bool isValidQueryIP(FwQuery* fwQuery)
{
    bool correctValues = true;
    for (int i = 0; i < 4; i++) {
        if (fwQuery->qiP[i] < 0 || fwQuery->qiP[i] > 255) {
            correctValues = false;
            break;
        }
    }
    return correctValues;
}

bool isValidQueryPort(FwQuery* fwQuery)
{
    bool correctValues = false;
    if (fwQuery->qPort >= 0 && fwQuery->qPort <= 65535)
    {
        correctValues = true;
    }
    return correctValues;
}

void searchThroughRules(FwQuery* fwQuery, FwRule* fwRuleHead)
{
    bool found = false;
    if (fwRuleHead == NULL)
    {
        printf("No rules\n");
        return;
    }
    FwRule* cur = fwRuleHead;

   while (cur != NULL) {
    //here we would check if the query is within the rules
    if (fwQuery->qPort >= cur->port1 && fwQuery->qPort <= cur->port2 &&
        fwQuery->qiP[0] >= cur->ip1[0] && fwQuery->qiP[0] <= cur->ip2[0] &&
        fwQuery->qiP[1] >= cur->ip1[1] && fwQuery->qiP[1] <= cur->ip2[1] &&
        fwQuery->qiP[2] >= cur->ip1[2] && fwQuery->qiP[2] <= cur->ip2[2] &&
        fwQuery->qiP[3] >= cur->ip1[3] && fwQuery->qiP[3] <= cur->ip2[3]) 
    {
        printf("Rule Found\n");
        found = true;
        return;
    }
    cur = cur->pNext;
    }
    if (!found)
    {
        printf("No rule found\n");
    }
}



void run_check_cmd(FwRequest* fwReq, FwRequest* fwReqHead, FwRule* fwRuleHead)
{
    char buffer[255];
    printf("enter query: \n");
    fgets(buffer, 255, stdin);
    FwQuery* fwQuery = process_query_cmd(buffer);

    if (isValidQueryIP(fwQuery) && isValidQueryPort(fwQuery))
    {
       searchThroughRules(fwQuery, fwRuleHead);
       printf("searching thru queries\n");
    }
    




}


FwRequest* process_cmd(char* buffer)
{
    FwRequest* fwReq = (FwRequest*)malloc(sizeof(FwRequest));
    strcpy(fwReq->RawCmd, buffer);
    fwReq->Cmd = buffer[0];
    return fwReq;
}

void run_interactive(CmdArg* pcmd)
{
    printf("running interactive\n");
    char buffer[255];
    FwRule * fwRuleHead = NULL;
    FwRequest* fwReqHead = NULL;

    while (true)
    {

        printf("Enter a command: ");
        fgets(buffer, 255, stdin);
        FwRequest* fwReq = process_cmd(buffer);
        add_to_Req_list(fwReq, &fwReqHead);
        switch (fwReq->Cmd)
        {
        case 'A':
            run_add_cmd(fwReq, &fwRuleHead);
            break;
        case 'D':
            run_del_cmd(fwReq, fwRuleHead);
            break;
        case 'L':
            run_list_cmd(fwReq, fwRuleHead);
            break;
        case 'R':
            run_list_requests_cmd(fwReq, fwReqHead);
            break;
        case 'C':
            run_check_cmd(fwReq, fwReqHead, fwRuleHead);
            break;
        case 'Q':
            //DO NOT FORGET TO FREE THE MEMORY
            printf("Exiting\n");
            return;

        default:
            printf("Illegal Request\n");
            break;
        }
    }
}

void run_listen(CmdArg* pcmd)
{
    printf("running listen on port %d\n", pcmd->port);
}