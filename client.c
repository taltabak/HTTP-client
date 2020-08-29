
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

//---------------------------Forward Declarations-------------------------//

//-------------Variables and data types---------------//

// specifies a boolean variable.
typedef enum { FALSE = 0, TRUE } bool;

// flag used in the copy function, specifies whether to copy the string before or after the 'n' char.
typedef enum {COPY_FIRST = 0, COPY_LAST } copyflag;

//this struct represents the request the will be parsed from the url. 
struct request {
    char* post; //the data incase there is a post request (-p *post*)
    unsigned int postLength; //the length of the data incase there is a post request
    char* host; //the host of the request (http://*host*:port/path)
    char* path; //the path of the request (http://host:port/*path*)
    unsigned int port;  //the port of the request (http://host:*port*/path)
    unsigned int argumentsNum;  //the number of arguments in the request (-r *number* arguments)
    char** arguments;   //the arguments in the request (-r number *arguments*)
};
typedef struct request request_r;

//-----------------Functions------------------------//

//this function get a struct (request) and initializes it.
void init_request(request_r* request);

//this function gets an array of the arguments entered to the program and it's size, 
//it parses the URL, creates a struct (request), fills the fields of the request based on the arguments and returns it.
request_r* parse_url(int argc , char* argv[]);

//this function gets a string and check if it's in the form "xxx=xxx" (used in parse_url).
bool properArgumentCheck (char* str);

//this function gets a string and checks if it's a number (used in parse_url).
bool isnumber (char* str);

//this function gets a string a flag and an integer, returns the substring before or after the (n) char depends on the flag (used in parse_url).
char* copy (char* str , int n , copyflag flag);

//this function get a struct (request) and deallocate all it's fields. 
void destroy_request(request_r* request);

//this function get a struct (request), it concatenate it's data in the form needed. 
char* makeRequest (request_r* request) ;

//this function gets a number and return it's length.
int lengthofnum (int num);

//--------------------------End Of Declarations-------------------------//



//------------------Main---------------------//

int main (int argc , char* argv[])
{
    int sockfd , n , rc;
    struct sockaddr_in serv_addr;
    struct hostent* server;
    request_r* request = parse_url(argc,argv);
    if(request == NULL)
        exit(0);
    sockfd = socket(AF_INET , SOCK_STREAM , 0); //opening the socket
    if (sockfd <0)  //case fd faield
    {
        perror("ERROR opening socket");
        exit(0);
    }
    if(request->host == NULL)   //gethostbyname function cannot get a NULL pointer.
        request->host = "";
    server = gethostbyname(request->host);  //parse the host (get ip address).
    if (server == NULL) //case the host does not exist.
    {
            fprintf(stderr,  "ERROR , no such host\n");
            exit(0);
    }
    //the next 3 lines initialize the internet socket address
    serv_addr.sin_family = AF_INET; 
    bcopy((char*)server->h_addr , (char*)&serv_addr.sin_addr.s_addr,server->h_length);
    serv_addr.sin_port = htons(request->port);
    //initiate connection on a socket
    if (connect(sockfd,(const struct sockaddr*)&serv_addr,sizeof(serv_addr)) < 0)
    {
        perror("ERROR connecting"); 
        exit(0);
    }
    char* request_s = makeRequest(request); //making the request to the server
    printf("%s",request_s);
    rc = write(sockfd,request_s,strlen(request_s)+1);   //send the request to the server
    char buffer[256];
    
    while(1)    //the loop reads the respose from the server, prints it and runs untill there is nothing more to read.
    {
        rc = read(sockfd,buffer,255); 
        if (rc < 0) //case read failed
        {
            perror("ERROR reading"); 
            exit(0);    
        }
        if (rc == 0) // case there is no more chars to read
            break;
        buffer[rc] = '\0';    // explicit null termination
        printf("%s",buffer);   
        fflush(stdout);     // make sure everything makes it to the output.
        buffer[0] = '\0';     // clear the buffer 
    }
    //deallocate all data.
    if(request!= NULL)  
        destroy_request(request);
    free(request_s);
    
    return 0;
}

request_r* parse_url(int argc , char* argv[])
{
    request_r* request = (request_r*)malloc(sizeof(request_r));
    if(request == NULL) //case malloc failed
    {
        perror("malloc failed");
        return NULL;
    }
    init_request(request);
    for(int i = 1 ; i < argc ; i++) //for each argument except the first (program name).
    {
        if(strncmp(argv[i],"http://" ,7) ==0)   //case it is the URL
        {
            if((request->host) == NULL) //ensuranse the host was not given twice.
            {
                int j =7;
                while( j < strlen(argv[i])) //for each char after the "http://".
                {
                    if(argv[i][j] == '/')   //case there is a path.
                    { 
                        if(request->host == NULL) // check if the host parsed before. 
                            request->host = copy(&(argv[i][7]),j-7,COPY_FIRST);
                        if(request->host == NULL)   //case copy failed
                        {
                            destroy_request(request);
                            return NULL;
                        }
                        request->path = copy(&(argv[i][7]),j-7,COPY_LAST);
                        if(request->path == NULL)   //case copy failed
                        {
                            destroy_request(request);
                            return NULL;
                        }
                        break;
                    }
                    if(argv[i][j] == ':')   //case there is a port.
                    {
                        if(request->host != NULL)   // check if the host parsed before.
                            continue;
                        request->host = copy(&(argv[i][7]),j-7,COPY_FIRST);
                        if(request->host == NULL)   //case copy failed
                        {
                            destroy_request(request);
                            return NULL;
                        }
                        request->port = atoi(&(argv[i][j+1]));
                        if(request->port < 0)   //invalid port.
                            request->port = 0;
                    }
                    j++;
                }
                if(request->host == NULL)   //case there are not port or path.
                {
                    request->host = copy(argv[i],7,COPY_LAST);
                    if(request->host == NULL)   //case copy failed
                        {
                            destroy_request(request);
                            return NULL;
                        }
                }
            }
            else    //case the URL is given twice.
            {
                fprintf(stderr,"Usage: client [-p <text>] [-r n <pr1=value1 pr2=value2 ...>] <URL>\n");
                destroy_request(request);
                return NULL;
            }
            
        }
        else if(strcmp(argv[i],"-p") ==0)   //case it is the post note.
        {
            if(request->post == NULL)   //ensuranse the post was not given twice.
            {
                if(i+1 == argc) //case "-p" is the last argument in argv
                {
                    fprintf(stderr,"Usage: client [-p <text>] [-r n <pr1=value1 pr2=value2 ...>] <URL>\n");
                    destroy_request(request);
                    return NULL;
                }
                else  
                {
                    request->postLength = strlen(argv[i+1]);
                    request->post = (char*)malloc(sizeof(char)*request->postLength+1);
                    if(request->post == NULL) //case malloc failed
                    {
                        perror("malloc failed");
                        destroy_request(request);
                        return NULL;
                    }
                    strcpy(request->post,argv[i+1]);
                    i++;
                }
            }
            else    //case the post given twice.
            {
                fprintf(stderr,"Usage: client [-p <text>] [-r n <pr1=value1 pr2=value2 ...>] <URL>\n");
                destroy_request(request);
                return NULL;
            }
            
        }
        else if(strcmp(argv[i],"-r") == 0)  //case it is the arguments note.
        {
            if(request->arguments == NULL)  //ensuranse the arguments were not given twice.
            {
                if(i+1 == argc) //case "-r" is the last argument in argv
                {
                    fprintf(stderr,"Usage: client [-p <text>] [-r n <pr1=value1 pr2=value2 ...>] <URL>\n");
                    destroy_request(request);
                    return NULL;
                }
                else if(isnumber(argv[i+1])==FALSE) //case the argument after "-r" is not a number.
                {
                    fprintf(stderr,"Usage: client [-p <text>] [-r n <pr1=value1 pr2=value2 ...>] <URL>\n");
                    destroy_request(request);
                    return NULL;
                }
                else    //parse arguments.
                {
                    request->argumentsNum = atoi(argv[i+1]);
                    request->arguments = (char**)malloc(sizeof(char*)*(request->argumentsNum+1));
                    if(request->arguments == NULL) //case malloc failed
                    {
                        perror("malloc failed");
                        destroy_request(request);
                        return NULL;
                    }
                    for(int k =0 ; k < request->argumentsNum ; k++)
                        (request->arguments)[k] = NULL; //init arguments array.
                    int j = i+2;
                    while(j < request->argumentsNum+i+2)    //for each argument in argv
                    {
                        if(argc == j)   //case there are less arguments than the number specified.
                        {
                            fprintf(stderr,"Usage: client [-p <text>] [-r n <pr1=value1 pr2=value2 ...>] <URL>\n");
                            destroy_request(request);
                            return NULL;
                        }
                        if(properArgumentCheck(argv[j]) == TRUE)    //check if the arguments are in the proper form.
                        {
                            char* arg = (char*)malloc(sizeof(char)*(strlen(argv[j])+1));
                            if(arg == NULL) //case malloc failed
                            {
                                perror("malloc failed");
                                destroy_request(request);
                                return NULL;
                            }
                            strcpy(arg,argv[j]);
                            (request->arguments)[j-i-2] = arg;
                        }
                        else    //case the arguments are not in the proper form.
                        {
                            fprintf(stderr,"Usage: client [-p <text>] [-r n <pr1=value1 pr2=value2 ...>] <URL>\n");
                            destroy_request(request);
                            return NULL;
                        }
                        j++;
                    }
                    i=j-1;  //argv[i] is now the last argument.
                }
            }
            else    //case the argument were given twice.
            {
                fprintf(stderr,"Usage: client [-p <text>] [-r n <pr1=value1 pr2=value2 ...>] <URL>\n");
                destroy_request(request);
                return NULL;
            }

        }
        else    //invalid input. 
        {
            fprintf(stderr,"Usage: client [-p <text>] [-r n <pr1=value1 pr2=value2 ...>] <URL>\n");
            destroy_request(request);
            return NULL;
        }
    }
    return request;
}

void init_request(request_r* request)
{
    request->post = NULL;
    request->postLength = 0;
    request->host = NULL;
    request->path = NULL;
    request->port = 80; //default port number.
    request->argumentsNum = 0;
    request->arguments = NULL;
}

void destroy_request(request_r* request)   
{   
     //for each field check if its not equal to NULL and deallocate it.
    if(request == NULL)
        return;
    if(request->post != NULL)
        free(request->post);
    if(request->host != NULL)
        free(request->host);
    if(request->path != NULL)
        free(request->path);
    if(request->arguments != NULL)
    {
        for(int i = 0 ; i < request->argumentsNum  ; i++)
        {
            if((request->arguments)[i] != NULL)
                free((request->arguments)[i]); 
        }
    free(request->arguments);
    }
    free(request);
}

bool properArgumentCheck (char* str)
{   
    //search '=' for each char in str not included the first and the last.
    for (int i = 1; i < strlen(str)-1 ; i++)    
        if(str[i] == '=')   
            return TRUE;
    return FALSE;
}

bool isnumber (char* str)
{
    //for each char in str check if it is a digit.
    for(int i = 0 ; i < strlen(str) ; i++)  
        if(!isdigit(str[i]))
            return FALSE;
    return TRUE;
}

char* copy (char* str , int n , copyflag flag)
{
    char* ret = NULL;
    if (flag == COPY_FIRST) //if str = "xxxxnyyyy" return "xxxx" 
    {
        ret = (char*)malloc(sizeof(char)*(n+1));
        if(ret == NULL) //case malloc failed
        {
            perror("malloc failed");
            return NULL;
        }
        for(int i = 0 ; i < n ; i++)
            ret[i] = str[i];
        ret[n] = '\0';
    }   
    if (flag == COPY_LAST)  //if str = "xxxxnyyyy" return "nyyyy"
    {
        char* endPtr = str+strlen(str)-1;
        int size = (int)(endPtr-&(str[n-1])+1);
        ret = (char*)malloc(sizeof(char)*size);
        if(ret == NULL) //case malloc failed
        {
            perror("malloc failed");
            return NULL;
        }
        strcpy(ret,&(str[n]));
    }
    return ret;
}

char* makeRequest (request_r* request) 
{
    char* type = NULL;  //initialize a string to concatenate - type of the request  
    char* path = NULL;  //initialize a string to concatenate - path(if it's null it has to be '/')  
    if(request->post == NULL)   
        type = "GET ";
    else
        type = "POST ";
    if(request->path == NULL)
        path = "/";
    else
        path = request->path;
    int arglen = request->argumentsNum; //length of arguments to allocate 
    for(int i = 0 ; i < request->argumentsNum ; i++)
        arglen += (strlen((request->arguments)[i])+1);
    char* request_s = NULL; //intialize the string to return.
    //calculate exact amount to allocate 
    int size = (int)(sizeof(type)+sizeof(path)+arglen+sizeof(request->host)+sizeof(" HTTP/1.0\r\nHost: ")+sizeof("\r\n\r\n"));
    if(request->post != NULL)
        size = (int)(size+sizeof("\r\nContent-length:\r\n")+lengthofnum(request->postLength)+request->postLength);    
    request_s = (char*)calloc(10,sizeof(char)*(size));   
    //concatenate the first part of the request (type: path-args)
    strcat(request_s, type);
    strcat(request_s, path);
    for(int i = 0 ; i < request->argumentsNum ; i++)
    {
        if(i == 0)
        {
            strcat(request_s, "?");
            strcat(request_s, (request->arguments)[i]);
        }
        else
        {
            strcat(request_s, "&");
            strcat(request_s, (request->arguments)[i]);
        }    
    }
    //concatenate the second part of the request (host:)
    strcat(request_s," HTTP/1.0\r\nHost: ");
    strcat(request_s, request->host);
    ////concatenate the third part of the request if exist (post and post length).
    if(request->post != NULL)
    {
        strcat(request_s, "\r\nContent-length:");
        char* num = NULL;
        num = (char*)malloc(sizeof(char)*(lengthofnum(request->postLength)+1));
        sprintf(num, "%d", request->postLength);
        strcat(request_s, num);
        free(num);
        strcat(request_s, "\r\n\r\n");
        strcat(request_s, request->post);    
        strcat(request_s, "\r\n"); 
    }
    else
        strcat(request_s, "\r\n\r\n");
    return request_s;
}

int lengthofnum (int num)
{
    int length = 0; 
    while(num!=0)
    {
        length++;
        num = num/10;
    }
    return length;
}
