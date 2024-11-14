#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>      // for close
#include <sys/types.h>   // for socket types
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>




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


typedef struct FwQuery
{
   char RawCmd[MAX_FW_CMD];
   uint8_t qiP[4];
   int qPort;
   struct FwQuery* pNext;
} FwQuery;


typedef struct FwRule
{
   char RawCmd[MAX_FW_CMD];
   uint8_t ip1[4];
   uint8_t ip2[4];
   int port1;
   int port2;


   struct FwRule* pNext;
   struct FwQuery* qHead;
} FwRule;


// Global variables
FwRule * fwRuleHead = NULL;
FwRequest* fwReqHead = NULL;
pthread_mutex_t lock;
int server_sockfd;


bool is_digit(char c)
{
   return c >= '0' && c <= '9';
}


bool is_integer(char* pch, int* num)
{
   char *endptr;
   *num = strtol(pch, &endptr, 10);
   return *endptr == '\0';
}


// Adds a request to the end of the linked list
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


// Adds a rule to the end of the linked list
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
}


bool isValidIP(FwRule* fwRule) {
   for (int i = 0; i < 4; i++) {
       if (fwRule->ip1[i] < 0 || fwRule->ip1[i] > 255 || fwRule->ip2[i] < 0 || fwRule->ip2[i] > 255) {
           return false;
       }
   }


   for (int i = 0; i < 4; i++) {
       if (fwRule->ip1[i] <= fwRule->ip2[i]) {
           return true;
       } else if (fwRule->ip1[i] > fwRule->ip2[i]) {
           return false;
       }
   }


   return true;
}


bool isValidPort(FwRule* fwRule) {
   // Check if both port1 and port2 are within valid range
   if (fwRule->port1 < 0 || fwRule->port1 > 65535 ||
       fwRule->port2 < 0 || fwRule->port2 > 65535) {
       return false; // Port values must be between 0 and 65535
   }


   // Ensure port1 is less than or equal to port2
   if (fwRule->port1 <= fwRule->port2) {
       return true; // Valid range (or single port)
   }


   // If port1 is greater than port2, it's an invalid range
   return false;
}


bool isValidRule(FwRule* fwRule)
{
   return isValidIP(fwRule) && isValidPort(fwRule);
}


bool process_args(int argc, char** argv, CmdArg* pcmd)
{
   pcmd->is_interactive = false;
   pcmd->port = 0;


   if (argc != 2) {
       return false;
   }


   if (strcmp(argv[1], "-i") == 0){
       pcmd->is_interactive = true;
       return true;
   } else {
       if (is_integer(argv[1], &pcmd->port)){
           pcmd->is_interactive = false;
           return true;
       }
   }
   return false;
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
   int ip1_parts[4], ip2_parts[4];
   if (sscanf(pch, "%d.%d.%d.%d", &ip1_parts[0], &ip1_parts[1], &ip1_parts[2], &ip1_parts[3]) != 4) {
       printf("Invalid IP address: %s\n", pch);
       free(fwRule);
       return NULL;
   }
   if (sscanf(dashPos + 1, "%d.%d.%d.%d", &ip2_parts[0], &ip2_parts[1], &ip2_parts[2], &ip2_parts[3]) != 4) {
       printf("Invalid IP address: %s\n", dashPos + 1);
       free(fwRule);
       return NULL;
   }
   // Validate and assign IP parts
   for (int i = 0; i < 4; i++) {
       if (ip1_parts[i] < 0 || ip1_parts[i] > 255 || ip2_parts[i] < 0 || ip2_parts[i] > 255) {
           printf("Invalid IP address: Out of range 0-255\n");
           free(fwRule);
           return NULL;
       }
       fwRule->ip1[i] = (uint8_t)ip1_parts[i];
       fwRule->ip2[i] = (uint8_t)ip2_parts[i];
   }
} else {
   int ip_parts[4];
   if (sscanf(pch, "%d.%d.%d.%d", &ip_parts[0], &ip_parts[1], &ip_parts[2], &ip_parts[3]) != 4) {
       printf("Invalid IP address: %s\n", pch);
       free(fwRule);
       return NULL;
   }
   // Validate and assign IP parts
   for (int i = 0; i < 4; i++) {
       if (ip_parts[i] < 0 || ip_parts[i] > 255) {
           printf("Invalid IP address: Out of range 0-255\n");
           free(fwRule);
           return NULL;
       }
       fwRule->ip1[i] = (uint8_t)ip_parts[i];
   }
   memcpy(fwRule->ip2, fwRule->ip1, sizeof(fwRule->ip1));
}
   // Process the port part
   pch = strtok(NULL, " ");
   if (dashPos != NULL) {
   *dashPos = '\0';
   if (!is_integer(pch, &fwRule->port1) || !is_integer(dashPos + 1, &fwRule->port2)) {
       printf("Invalid port numbers\n");
       free(fwRule);
       return NULL;
   }
} else {
   if (!is_integer(pch, &fwRule->port1)) {
       printf("Invalid port number\n");
       free(fwRule);
       return NULL;
   }
   fwRule->port2 = fwRule->port1;
}


   return fwRule;
}




FwQuery* process_query_cmd(char* buffer)
{
   FwQuery* fwQuery = (FwQuery*)malloc(sizeof(FwQuery));
   if (fwQuery == NULL) {
       printf("Memory allocation failed\n");
       exit(1);
   }
   fwQuery->pNext = NULL;
   strcpy(fwQuery->RawCmd, buffer);


   // Split the query by space
   char* pch = strtok(buffer, " ");
   if (pch == NULL) {
       printf("Invalid Query\n");
       free(fwQuery);
       return NULL;
   }


  int ip_parts[4];
if (sscanf(pch, "%d.%d.%d.%d", &ip_parts[0], &ip_parts[1], &ip_parts[2], &ip_parts[3]) != 4) {
   printf("Invalid IP address: %s\n", pch);
   free(fwQuery);
   return NULL;
}
// Validate and assign IP parts
for (int i = 0; i < 4; i++) {
   if (ip_parts[i] < 0 || ip_parts[i] > 255) {
       printf("Invalid IP address: Out of range 0-255\n");
       free(fwQuery);
       return NULL;
   }
   fwQuery->qiP[i] = (uint8_t)ip_parts[i];
}


   // Process the port part
   pch = strtok(NULL, " ");
   if (pch != NULL) {
   if (!is_integer(pch, &fwQuery->qPort)) {
       printf("Invalid port number\n");
       free(fwQuery);
       return NULL;
   }    }
   else {
       printf("Invalid Query: Missing port\n");
       free(fwQuery);
       return NULL;
   }


   return fwQuery;
}


bool isValidQueryIP(FwQuery* fwQuery)
{
   // All IP bytes should be between 0 and 255
   for (int i = 0; i < 4; i++) {
       if (fwQuery->qiP[i] > 255) {
           return false;
       }
   }
   return true;
}


bool isValidQueryPort(FwQuery* fwQuery)
{
   return fwQuery->qPort >= 0 && fwQuery->qPort <= 65535;
}


bool add_query_to_rule(FwRule* fwRule, FwQuery* fwQuery, FwQuery** fwqHead)
{
   if (*fwqHead == NULL)
   {
       *fwqHead = fwQuery;
       fwRule->qHead = *fwqHead;
       return true;
   }
   FwQuery* cur = *fwqHead;


   while (cur != NULL)
   {
       // Check if the query is already in the rule
       if (strcmp(cur->RawCmd, fwQuery->RawCmd) == 0)
       {
           return false;
       }
       if (cur->pNext == NULL)
       {
           break;
       }
       cur = cur->pNext;
   }
   cur->pNext = fwQuery;
   return true;
}


FwRequest* process_cmd(char* buffer)
{
   FwRequest* fwReq = (FwRequest*)malloc(sizeof(FwRequest));
   strcpy(fwReq->RawCmd, buffer);
   fwReq->Cmd = buffer[0];
   fwReq->pNext = NULL;
   return fwReq;
}


char* process_request(char* buffer)
{
   FwRequest* fwReq = process_cmd(buffer);


   // Allocate response buffer
   char *response = malloc(1024 * sizeof(char)); // make sure to free it after use
   if (!response) {
       perror("Failed to allocate response buffer");
       exit(1);
   }
   bzero(response, 1024);


   // Lock mutex before modifying shared data
   pthread_mutex_lock(&lock);
   add_to_Req_list(fwReq, &fwReqHead);
   pthread_mutex_unlock(&lock);


   switch (fwReq->Cmd)
   {
   case 'A':
       pthread_mutex_lock(&lock);
       {
           char tempBuffer[256];
           strcpy(tempBuffer, buffer + 2); // Skip 'A '
           FwRule* fwRule = process_rule_cmd(tempBuffer);
           if (fwRule != NULL && isValidRule(fwRule)){
               add_to_rule_list(fwRule, &fwRuleHead);
               strcpy(response, "Rule added");
           }
           else{
               strcpy(response, "Invalid rule");
               if (fwRule != NULL)
                   free(fwRule);
           }
       }
       pthread_mutex_unlock(&lock);
       break;
   case 'D':
       pthread_mutex_lock(&lock);
       {
           char tempBuffer[256];
           strcpy(tempBuffer, buffer + 2); // Skip 'D '
           FwRule* fwRuleToDelete = process_rule_cmd(tempBuffer);
           if (fwRuleToDelete == NULL || !isValidRule(fwRuleToDelete)) {
               strcpy(response, "Rule invalid");
               if (fwRuleToDelete != NULL)
                   free(fwRuleToDelete);
           } else {
               // Search for the rule in fwRuleHead
               FwRule *prev = NULL, *curr = fwRuleHead;
               bool found = false;
               while (curr != NULL) {
                   if (strcmp(curr->RawCmd, fwRuleToDelete->RawCmd) == 0) {
                       // Found the rule
                       if (prev == NULL) {
                           fwRuleHead = curr->pNext;
                       } else {
                           prev->pNext = curr->pNext;
                       }
                       // Free the rule and its queries
                       FwQuery* qcurr = curr->qHead;
                       while (qcurr != NULL) {
                           FwQuery* qnext = qcurr->pNext;
                           free(qcurr);
                           qcurr = qnext;
                       }
                       free(curr);
                       strcpy(response, "Rule deleted");
                       found = true;
                       break;
                   }
                   prev = curr;
                   curr = curr->pNext;
               }
               if (!found) {
                   strcpy(response, "Rule not found");
               }
               free(fwRuleToDelete);
           }
       }
       pthread_mutex_unlock(&lock);
       break;
   case 'L':
       pthread_mutex_lock(&lock);
       {
           FwRule* currRule = fwRuleHead;
           response[0] = '\0'; // reset response
           while (currRule != NULL) {
               char ruleLine[512];
               snprintf(ruleLine, 512, "Rule: %s\n", currRule->RawCmd);
               strcat(response, ruleLine);
               // For each query
               FwQuery* currQuery = currRule->qHead;
               while (currQuery != NULL) {
                   char queryLine[512];
                   snprintf(queryLine, 512, "Query: %hhu.%hhu.%hhu.%hhu %d\n",
                            currQuery->qiP[0], currQuery->qiP[1], currQuery->qiP[2], currQuery->qiP[3], currQuery->qPort);
                   strcat(response, queryLine);
                   currQuery = currQuery->pNext;
               }
               currRule = currRule->pNext;
           }
           if (strlen(response) == 0) {
               strcpy(response, "No rules");
           }
       }
       pthread_mutex_unlock(&lock);
       break;
   case 'R':
       pthread_mutex_lock(&lock);
       {
           FwRequest* currReq = fwReqHead;
           response[0] = '\0'; // reset response
           while (currReq != NULL) {
               strcat(response, currReq->RawCmd);
               strcat(response, "\n");
               currReq = currReq->pNext;
           }
           if (strlen(response) == 0) {
               strcpy(response, "No requests");
           }
       }
       pthread_mutex_unlock(&lock);
       break; // Add this break


   case 'C':
       pthread_mutex_lock(&lock);
       {
           char tempBuffer[256];
           strcpy(tempBuffer, buffer + 2); // Skip 'C '
           FwQuery* fwQuery = process_query_cmd(tempBuffer);
           if (fwQuery == NULL || !isValidQueryIP(fwQuery) || !isValidQueryPort(fwQuery)) {
               strcpy(response, "Illegal IP address or port specified");
               if (fwQuery != NULL)
                   free(fwQuery);
           } else {
               // Check if the IP and port match any rule
               FwRule* currRule = fwRuleHead;
               bool matched = false;
               while (currRule != NULL) {
                   // Check if the IP and port are within the rule's ranges
                   bool ipMatches = true;
                   for (int i = 0; i < 4; i++) {
                       if (fwQuery->qiP[i] < currRule->ip1[i] || fwQuery->qiP[i] > currRule->ip2[i]) {
                           ipMatches = false;
                           break;
                       }
                   }
                   if (ipMatches && fwQuery->qPort >= currRule->port1 && fwQuery->qPort <= currRule->port2) {
                       // Check if the query is already in the rule
                       FwQuery* qcur = currRule->qHead;
                       bool alreadyInRule = false;
                       while (qcur != NULL) {
                           if (strcmp(qcur->RawCmd, fwQuery->RawCmd) == 0) {
                               alreadyInRule = true;
                               break;
                           }
                           qcur = qcur->pNext;
                       }
                       if (!alreadyInRule) {
                           // Query not in this rule, add it
                           add_query_to_rule(currRule, fwQuery, &(currRule->qHead));
                           matched = true;
                           break; // Only add to one rule
                       }
                   }
                   currRule = currRule->pNext;
               }
               if (matched) {
                   strcpy(response, "Connection accepted");
               } else {
                   strcpy(response, "Connection rejected");
                   free(fwQuery);
               }
           }
       }
       pthread_mutex_unlock(&lock);
       break;


   default:
       strcpy(response, "Illegal request");
       break;
   }


   return response;
}
void handle_sigint(int sig) {
   printf("Caught signal %d, shutting down server...\n", sig);
   close(server_sockfd);
   pthread_mutex_destroy(&lock);
   exit(0);
}




void *client_handler(void *arg) {
   int newsockfd = *((int *)arg);
   free(arg);
   char buffer[256];
   bzero(buffer,256);
   int n;


   // Read command from client
   n = read(newsockfd,buffer,255);
   if (n < 0) {
       perror("ERROR reading from socket");
       close(newsockfd);
       pthread_exit(NULL);
   }
   buffer[n] = '\0'; // Null-terminate the buffer


   // Process the command
   char* response = process_request(buffer);


   // Send response to client
   n = write(newsockfd, response, strlen(response));
   if (n < 0) {
       perror("ERROR writing to socket");
   }
   free(response);
   close(newsockfd);
   pthread_exit(NULL);
}


void run_interactive(CmdArg* pcmd)
{
   printf("running interactive\n");
   char buffer[255];


   while (true)
   {
       fgets(buffer, 255, stdin);
       // Remove the newline character from buffer
       buffer[strcspn(buffer, "\n")] = 0;


       char* response = process_request(buffer);
       printf("%s\n", response);
       free(response);
   }
}


void run_listen(CmdArg* pcmd)
{
   printf("running listen on port %d\n", pcmd->port);
   int sockfd, newsockfd, portno;
   socklen_t clilen;
   struct sockaddr_in serv_addr, cli_addr;
   pthread_t thread_id;


   // Create socket
   sockfd = socket(AF_INET, SOCK_STREAM, 0);
   if (sockfd < 0) {
      perror("ERROR opening socket");
      exit(1);
   }
   int opt = 1;
   if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
       perror("setsockopt(SO_REUSEADDR) failed");
       exit(1);
   }


   // Bind socket to port
   bzero((char *) &serv_addr, sizeof(serv_addr));
   portno = pcmd->port;
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = INADDR_ANY;
   serv_addr.sin_port = htons(portno);
   if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
            perror("ERROR on binding");
            exit(1);
   }


   // Listen
   listen(sockfd,5);
   clilen = sizeof(cli_addr);


   // Accept connections
   while (1) {
       newsockfd = accept(sockfd,
            (struct sockaddr *) &cli_addr,
            &clilen);
       if (newsockfd < 0) {
            perror("ERROR on accept");
            continue;
       }
       // Create a new thread to handle the client
       int *pclient = malloc(sizeof(int));
       *pclient = newsockfd;
       if (pthread_create(&thread_id, NULL, client_handler, pclient) != 0) {
           perror("Failed to create thread");
       }
       // Detach the thread so that resources are freed when it finishes
       pthread_detach(thread_id);
   }


   close(sockfd);
}


int main(int argc, char** argv)
{
   CmdArg cmdArg;
   if (!process_args(argc, argv, &cmdArg)){
       printf("Invalid arguments\n");
       return 1;
   }
   signal(SIGINT, handle_sigint);
   signal(SIGTERM, handle_sigint);


   pthread_mutex_init(&lock, NULL);


   if (cmdArg.is_interactive){
       run_interactive(&cmdArg);
   } else {
       run_listen(&cmdArg);
   }


   pthread_mutex_destroy(&lock);


   return 0;
}
