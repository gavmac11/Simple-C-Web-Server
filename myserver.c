//Gavin Mackenzie
//Programming Assignment 1

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUFFER_SIZE 512
#define MAX_FILE_SIZE 5*1024
#define MAXPENDING 5
#define TRUE 1
#define FALSE 0
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

int port; //Port to listen to
char *wwwroot; //The root directory
char *configFile; //config.txt - where you can specify the port and root directory instead of using parameters
char *mimeTypesFile;//mimeTypes.txt

FILE *filePointer = NULL; //will point to local file

struct sockaddr_in address; //Local address
struct sockaddr_storage connector; //Client address
int current_socket; //TCP Socket
int connecting_socket; //client socket
socklen_t addr_size;

//Sends *message to socket
int sendString(char *message, int socket)
{
	int length, bytes_sent;
	length = strlen(message);

	bytes_sent = send(socket, message, length, 0);

	return bytes_sent;
}

//Sends the bytes to the connected socket
int sendBinary(int *byte, int length)
{
	int bytes_sent;

	bytes_sent = send(connecting_socket, byte, length, 0);

	return bytes_sent;


	return 0;
}
//Responds to the HTTP request forming the header
void sendHeader(char *Status_code, char *Content_Type, int TotalSize, int socket)
{
	char *head = "\r\nHTTP/1.1 ";
	char *content_head = "\r\nContent-Type: ";
	char *server_head = "\r\nServer: PT06";
	char *length_head = "\r\nContent-Length: ";
	char *date_head = "\r\nDate: ";
	char *newline = "\r\n";
	char contentLength[100];

	time_t rawtime;
	time ( &rawtime );

	sprintf(contentLength, "%i", TotalSize);

	char *message = malloc((
		strlen(head) +
		strlen(content_head) +
		strlen(server_head) +
		strlen(length_head) +
		strlen(date_head) +
		strlen(newline) +
		strlen(Status_code) +
		strlen(Content_Type) +
		strlen(contentLength) +
		28 +
		sizeof(char)) * 2);

	if ( message != NULL )
	{
		strcpy(message, head);
		strcat(message, Status_code);

		strcat(message, content_head);
		strcat(message, Content_Type);
		strcat(message, server_head);
		strcat(message, length_head);
		strcat(message, contentLength);
		strcat(message, date_head);
		strcat(message, (char*)ctime(&rawtime));
		strcat(message, newline);

		sendString(message, socket);

		free(message);
	}    
}

void sendHTML(char *statusCode, char *contentType, char *content, int size, int socket)
{
	sendHeader(statusCode, contentType, size, socket);
	sendString(content, socket);
}

void sendFile(FILE *fp, int file_size)
{
	int current_char = 0;

	do{
		current_char = fgetc(fp);
		sendBinary(&current_char, sizeof(char));
	}
	while(current_char != EOF);
}

//used to parse the input
int scan(char *input, char *output, int start)
{
	if ( start >= strlen(input) )
		return -1;

	int appending_char_count = 0;
	int i = start;

	for ( ; i < strlen(input); i ++ )
	{
		if ( *(input + i) != '\t' && *(input + i) != ' ' && *(input + i) != '\n' && *(input + i) != '\r')
		{
			*(output + appending_char_count) = *(input + i ) ;

			appending_char_count += 1;
		}	
		else
			break;
	}
	*(output + appending_char_count) = '\0';	

	//Find next word start
	i += 1;

	for (; i < strlen(input); i ++ )
	{
		if ( *(input + i ) != '\t' && *(input + i) != ' ' && *(input + i) != '\n' && *(input + i) != '\r')
			break;
	}

	return i;
}
//Checks to see if the content type is listed in the supported mime types
int checkMime(char *extension, char *mime_type)
{
	char *current_word = malloc(600);
	char *word_holder = malloc(600);
	char *line = malloc(200);
	int startline = 0;

	FILE *mimeFile = fopen(mimeTypesFile, "r");

	free(mime_type);

	mime_type = (char*)malloc(200);

	memset (mime_type,'\0',200);

	while(fgets(line, 200, mimeFile) != NULL) { 

		if ( line[0] != '#' )
		{
			startline = scan(line, current_word, 0);
			while ( 1 )
			{
				startline = scan(line, word_holder, startline);
				if ( startline != -1 )
				{
					if ( strcmp ( word_holder, extension ) == 0 )
					{
						memcpy(mime_type, current_word, strlen(current_word));
						free(current_word);
						free(word_holder);
						free(line);
						return 1;	
					}
				}
				else
				{
					break;
				}
			}
		}

		memset (line,'\0',200);
	}

	free(current_word);
	free(word_holder);
	free(line);

	return 0;
}

//This method returns 1 if using HTTP 1.1 or 0 if using HTTP 1.0
//-1 is returned if there is an error
int getHttpVersion(char *input, char *output)
{
	char *filename = malloc(100);
	int start = scan(input, filename, 4);
	if ( start > 0 )
	{
		if ( scan(input, output, start) )
		{

			output[strlen(output)+1] = '\0';

			if ( strcmp("HTTP/1.1" , output) == 0 )
				return 1;

			else if ( strcmp("HTTP/1.0", output) == 0 )

				return 0;
			else
				return -1;
		}
		else
			return -1;
	}

	return -1;
}

//Gets the extension of the input
int GetExtension(char *input, char *output)
{
	int in_position = 0;
	int appended_position = 0;
	int i = 0;

	for ( ; i < strlen(input); i ++ )
	{		
		if ( in_position == 1 )
		{
			output[appended_position] = input[i];
			appended_position +=1;
		}

		if ( input[i] == '.' )
			in_position = 1;

	}

	output[appended_position+1] = '\0';

	if ( strlen(output) > 0 )
    {
        return 1;
    }

	return -1;
}

//Returns filesize
int Content_Lenght(FILE *fp)
{
	int filesize = 0;

	fseek(fp, 0, SEEK_END);
	filesize = ftell(fp);
	rewind(fp);

	return filesize;
}

//This handles the GET request
//Returns -1 if the file does not exist, 1 if it does
int handleHttpGET(char *input)
{
	char *filename = (char*)malloc(200 * sizeof(char));
	char *path = (char*)malloc(1000 * sizeof(char));
	char *extension = (char*)malloc(10 * sizeof(char));
	char *mime = (char*)malloc(200 * sizeof(char));
	char httpVersion[20];

	int contentLength = 0;
	int mimeSupported = 0;
	int fileNameLength = 0;


	memset(path, '\0', 1000);
	memset(filename, '\0', 200);
	memset(extension, '\0', 10);
	memset(mime, '\0', 200);
	memset(httpVersion, '\0', 20);

	fileNameLength = scan(input, filename, 5);


	if ( fileNameLength > 0 )
	{

		if ( getHttpVersion(input, httpVersion) != -1 ) //If using HTTP 1.1 or 1.0
		{
			FILE *fp;

			if ( GetExtension(filename, extension) == -1 ) //If the filename does not have an extension - return
			{
				printf("File extension not existing");

				sendString("400 Bad Request - did you add the extension\n", connecting_socket);

				free(filename);
				free(mime);
				free(path);
				free(extension);

				return -1;
			}

			mimeSupported =  checkMime(extension, mime);


			if ( mimeSupported != 1) //If media type is not supported - return
			{
				printf("Mime type not supported");

				sendString("400 Bad Request - content type not supported\n", connecting_socket);

				free(filename);
				free(mime);
				free(path);
				free(extension);

				return -1;
			}

			//Open the requesting file as binary
			strcpy(path, wwwroot);
			strcat(path, filename);
			fp = fopen(path, "rb");

			if ( fp == NULL ) //If the file is not found - return
			{
				printf("file not found");

				sendString("404 Not Found\n", connecting_socket);

				free(filename);
				free(mime);
				free(extension);
				free(path);

				return -1;
			}


			//Calculate Content Length
			contentLength = Content_Lenght(fp);
			if (contentLength  < 0 )
			{
				printf("File size is zero");

				free(filename);
				free(mime);
				free(extension);
				free(path);

				fclose(fp);

				return -1;
			}

			//Send File Content
			sendHeader("200 OK", mime,contentLength, connecting_socket);

			sendFile(fp, contentLength);

			free(filename);
			free(mime);
			free(extension);
			free(path);

			fclose(fp);

			return 1;
		}
		else
		{
			sendString("501 Not Implemented\n", connecting_socket);
		}
	}

	return -1;
}

/* This gets the http request type 
   Returns -1 for invalid type
   Returns 0 for POST request
   Returns 1 for GET request - we only have implemtation to handle this
   Returns 2 for HEAD request
*/
int getRequestType(char *input)
{
	int type = -1;

	if ( strlen ( input ) > 0 )
	{
		type = 1;
	}

	char *requestType = malloc(5);

	scan(input, requestType, 0);

	if ( type == 1 && strcmp("GET", requestType) == 0)
	{
		type = 1;
	}
	else if (type == 1 && strcmp("HEAD", requestType) == 0)
	{
		type = 2;
	}
	else if (strlen(input) > 4 && strcmp("POST", requestType) == 0 )
	{
		type = 0;
	}
	else
	{
		type = -1;
	}
	return type;
}

int receive(int socket)
{
    int msgLen = 0;
    char buffer[BUFFER_SIZE];
    
    memset (buffer,'\0', BUFFER_SIZE);
    
    if ((msgLen = recv(socket, buffer, BUFFER_SIZE, 0)) == -1)
    {
        printf("Error handling incoming request");
        return -1;
    }
    
    int request = getRequestType(buffer);
    
    if ( request == 1 )				// GET
    {
        handleHttpGET(buffer);
    }
    else if ( request == 2 )		// HEAD
    {
        // SendHeader();
    }
    else if ( request == 0 )		// POST
    {
        sendString("501 Not Implemented\n", connecting_socket);
    }
    else							//anything else
    {
        sendString("400 Bad Request\n", connecting_socket);
    }
    
    return 1;
}

//Creates a new TCP socket and assign it to current_socket
void createSocket()
{
    if ((current_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        exit(-1); //failed to create
    }
}

//Bind socket to port
void bindSocket()
{
	address.sin_family = AF_INET; //Internet Address Family
	address.sin_addr.s_addr = INADDR_ANY; //Any incoming interface
	address.sin_port = htons(port); //local port

	if (bind(current_socket, (struct sockaddr *)&address, sizeof(address)) < 0 )
	{
		exit(-1); //failed to bind
	}
}

//Starts to listen and will wait for incoming connections on the socket from createSocket()
void startListener()
{
	if (listen(current_socket, MAXPENDING) < 0 )
	{
		exit(-1); //failed
	}
}

//Handle connection and process request
void handle(int socket)
{
    if (receive((int)socket) < 0)
    {
        exit(-1);
    }
}

//Accepts a new connection
void acceptConnection()
{
    addr_size = sizeof(connector);

	connecting_socket = accept(current_socket, (struct sockaddr *)&connector, &addr_size);

	if ( connecting_socket < 0 )
	{
		exit(-1); //Failed to accept connection
	}
    
    
    //Not sure if the following handles child processes correctly
    pid_t processID = fork();
    if (processID < 0)
    {
        printf("fork failed");
    }
    else if (processID == 0) //If this is the child process
    {
       // close(current_socket); //Close parent
        handle(connecting_socket);
        close(connecting_socket);//close child
    }
    else
    {
        handle(connecting_socket);
        close(connecting_socket);

    }
    
}

void start()
{
	createSocket(); //creates TCP socket

	bindSocket(); //Binds socket to port

	startListener(); //Set socket to listen

    //Forever
	while ( 1 )
	{
		acceptConnection();
	}
    
    
}

void init()
{
	char* currentLine = malloc(100);
	wwwroot = malloc(100);
	configFile = malloc(100);
	mimeTypesFile = malloc(600);

	// Setting default values
	configFile = "config.txt";
	strcpy(mimeTypesFile, "mimeTypes.txt");
		
	filePointer = fopen(configFile, "r");

	//Checks to see if config.txt is open
	if (filePointer == NULL)
	{
		fprintf(stderr, "Error opening config.txt");
		exit(1);
	}

	// Gets server root directory from config.txt if the user does not specify in the params
	if (fscanf(filePointer, "%s %s", currentLine, wwwroot) != 2)
	{
		fprintf(stderr, "Error in config.txt on line 1\n");
		exit(1);
	}

	// Gets the default port from config.txt if the user does not specify in the params
	if (fscanf(filePointer, "%s %i", currentLine, &port) != 2)
	{
		fprintf(stderr, "Error in config.txt on line 2\n");
		exit(1);
	}

	fclose(filePointer);
	free(currentLine);
}

int main(int argc, char* argv[])
{
	int parameterCount;
	char* fileExt = malloc(10);
	char* mime_type = malloc(800);

	init();

	for (parameterCount = 1; parameterCount < argc; parameterCount++)
	{
		// If flag -p is used, set port
		if (strcmp(argv[parameterCount], "-p") == 0)
		{
			// Indicate that we want to jump over the next parameter
			parameterCount++;
			printf("Setting port to %i\n", atoi(argv[parameterCount]));
			port = atoi(argv[parameterCount]);
		}

		else if (strcmp(argv[parameterCount], "-d") == 0)
		{
			// Indicate that we want to jump over the next parameter
			parameterCount++;
			printf("Setting root directory to %s\n", argv[parameterCount]);
			wwwroot = (char*)argv[parameterCount];
		}
		else
		{
			printf("Enter params or set port and directory in config.txt");
			return -1;
		}
	}

	printf("Port:\t\t\t%i\n", port);
	printf("Server root:\t\t%s\n", wwwroot);
	printf("Configuration file:\t%s\n", configFile);
    
    

	start();

	return 0;
}