Gavin MacKenzie
CS485g Programming Assignment 1

Included Files/Folders:
myserver.c - source code for the simple web server
config.txt - configuration file which allows you to set port number and directory
mimeTypes.txt - list of accepted mime types (html, txt, jpg, gif)
HTML - Folder with sample content for testing purposes
README.txt - this file)

How to Compile:
Navigate to the directory containing the program. Make sure the config.txt and mimeTypes.txt are in the same directory.
To compile type "gcc myserver.c" --- This will create the program a.out

To Run:
Navigate to the directory with the program and associated config.txt and mimeTypes.txt.
To specify the port and root directory as program parameters type "./a.out -p 7777 -d /path/" where 7777 can be any available port and /path/ is the path to the files being hosted.
If you do not want to use parameters, edit the config.txt by replacing the default port of 7777 and the default path of /Users/gavinmackenzie/Desktop/cs485g/HTML/
If you have modified the config.txt, you can run the program by typing "./a.out"

Program Description:
This is a simple web server coded in C. It will process HTTP GET requests. It supports only the content types found in mimeTypes.txt.
The program works by listening to incoming connections on the port specified and processes all connections through TCP sockets. 

Possible Issues:
The program should create children processes to handle connections so it can handle more than one at a time. As my implementation stands, I am not sure this is working correctly.
When using the browser, it tends to timeout when trying to access a filepath that is not on the server. Although when TELNET is used to analyze this, the correct responses are sent back.

References:
The implementation of this project was guided by content from "TCP/IP Sockets in C: Practical Guide for Programmers" and its sample code.
Also, "Beej's Guide to Network Programming Using Internet Sockets" helped in the understanding of implementing this serverside program. 
