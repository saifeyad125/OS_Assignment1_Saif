#include<stdbool.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>


#define MAX_FW_CMD 255
#define MAX_FW_RULE 255
#define MAX_FW_QUERY 255

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
    unsigned char ip1[15];
    unsigned char ip2[15];
    int port1;
    int port2;

    struct FwRule* pNext;
} FwRule;

typedef struct Query
{
    char rawQuery[MAX_FW_QUERY];
    char query;
    struct Query* pNext;
} Query;


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
void add_to_Req_list(FwRequest* fwReq, FwRequest* fwReqHead)
{
    if (fwReqHead == NULL)
    {
        fwReqHead = fwReq;
        return;
    }
    FwRequest* cur = fwReqHead;
    while (cur->pNext != NULL)
    {
        cur = cur->pNext;
    }
    cur->pNext = fwReq;
}

//adds a rule to the end of the linked list
void add_to_rule_list(FwRule* fwRule, FwRule* fwRuleHead)
{
    
    if (fwRuleHead == NULL)
    {
        fwRuleHead = fwRule;
        return;
    }
    FwRule* cur = fwRuleHead;
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





void run_add_cmd(FwRequest* fwReq, FwRule* fwRuleHead)
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
        printf("Invalid Rule\n");
    }
    
}

void run_del_cmd(FwRequest* fwReq, FwRule* fwRuleHead)
{
    printf("running del\n");
}

void run_list_cmd(FwRequest* fwReq, FwRule* fwRuleHead)
{
    printf("running list\n");
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
    while (cur -> pNext != NULL)
    {
        printf("%s\n", cur->RawCmd);
        cur = cur->pNext;
    }
}

void run_check_cmd(FwRequest* fwReq, FwRequest* fwReqHead)
{
    printf("running check\n");
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
        add_to_Req_list(fwReq, fwReqHead);
        switch (fwReq->Cmd)
        {
        case 'A':
            run_add_cmd(fwReq, fwRuleHead);
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
            run_check_cmd(fwReq, fwReqHead);
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