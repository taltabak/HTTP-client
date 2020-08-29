
CREATED BY: Tal Tabak 

DESCRIPTION:
This program is an implementation of an HTTP client in 'c'. 
1. Parse a URL from the input. 
2. Generate an http request out of the URL. 
3. Connect to the server through a socket and sends the    	request.
4. Recieve the response from the server and print it to the user. 

PROGRAM FILES: 
     client.c -An implementaion of an HTTP client, 
    
REMARKS:

*   Workspace: Visual Studio Code
*   Input: URL in the command line.
*   Output: The parsed request sent to the server, The response from the server to the request. 
*   The implementaion of the program is done with a helpful structure (request) which holds fields of 
    the data parsed from the url. 
    