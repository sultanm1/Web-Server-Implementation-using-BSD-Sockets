#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <unistd.h>


#define BACKLOG 10 /* pending connections queue size */
#define MYPORT 5000 //Port

#define MAXSIZE 2048
#define MAXWRITESIZE 2097152


int main(){


  // char hello[25] = "mi.n.e .htm %20.txt.html";
  // printf("%s", strstr(hello, ".html"));
  //
  int sockfd, new_fd; /* listen on sock_fd, new connection on new_fd */
  struct sockaddr_in my_addr; /* my address */
  struct sockaddr_in their_addr; /* connector addr */
  unsigned int sin_size;
  char bufferForRead[MAXSIZE];
  char bufferForWrite[MAXWRITESIZE];
  char bufferForHeader[MAXSIZE];
  char bufferFinalWrite[MAXWRITESIZE];
  int headerOFF;
  int finalWriteSize;


  /* create a socket */
  if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
    perror("Error in creating socket");
    exit(1);
  }
  // ...
  /* set the address info */
  my_addr.sin_family = AF_INET;
  my_addr.sin_port = htons(MYPORT); /* short, network byte order */
  my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  /* INADDR_ANY allows clients to connect to any one of the host’s IP
  address. Optionally, use this line if you know the IP to use:
  my_addr.sin_addr.s_addr = inet_addr(“127.0.0.1”);
  */
  memset(my_addr.sin_zero, '\0', sizeof(my_addr.sin_zero));
  /* bind the socket */
  if (bind(sockfd, (struct sockaddr *) &my_addr,
    sizeof(struct sockaddr)) == -1) {
    perror("bind");
    exit(1);
  }
  if (listen(sockfd, BACKLOG) == -1) {
    perror("listen");
    exit(1);
  }
  while (1) { /* main accept() loop */

    printf("Entered server loop\n");


    sin_size = sizeof(struct sockaddr_in);
    if ((new_fd = accept(sockfd, (struct sockaddr*) &their_addr, &sin_size)) == -1) {   //Cannot print
                                                                    //until after connection resolved here
      perror("Didn't accpet connection!");
      continue;
    }

    //Read
    memset(bufferFinalWrite, 0, MAXWRITESIZE);
    memset(bufferForWrite, 0, MAXWRITESIZE);
    memset(bufferForHeader, 0, MAXSIZE);
    memset(bufferForRead, 0, MAXSIZE);

    int myRead;
    if((myRead = read(new_fd, bufferForRead, MAXSIZE)) < 0){
      perror("Couldn't read, closing");
      close(new_fd);
      continue;
    }

    const char * httpEndMethod = strchr(bufferForRead, ' ');
    const char * httpUrl = strchr(bufferForRead, '/') + 1;
    const char * httpUrlEnd = strchr(httpEndMethod + 1, ' ');

    char httpMethod[httpEndMethod - bufferForRead + 1];
    char myUrl[httpUrlEnd - httpUrl + 1];
    strncpy(httpMethod, bufferForRead,  httpEndMethod - bufferForRead);
    strncpy(myUrl, httpUrl, httpUrlEnd - httpUrl);
    httpMethod[sizeof(httpMethod) - 1] = '\0';
    myUrl[sizeof(myUrl) - 1] = '\0';

    printf("Method is %s \n", httpMethod);
    printf("URL is %s \n", myUrl);

    //I got the Method and the URL successfully now

    //Open FILE
    FILE *myFile = fopen(myUrl, "r");
    //If it did not open return 404 file not found
    if(!myFile){
    //   //404 file not found error
        char flf[100] = "HTTP/1.0 404 Not Found\r\nServer: Sultan Madkhali\r\n\r\n";
        printf("Couldn't open file");
        write(new_fd, flf, strlen(flf));
        perror("closing connection, no file found");
        close(new_fd);
    }



    //Use fread to read file
    size_t bufferForBodyOfFile = fread(bufferForWrite, sizeof(char), MAXWRITESIZE, myFile);
    if(ferror(myFile)){
      perror("Wasn't able to read file with fread()");
      fclose(myFile);
      close(new_fd);
    }

    //
    // //Make header
    // //Header status as OK
    headerOFF = 0;
    char responseLines[100] = "HTTP/1.0 200 OK\r\nServer: Sultan Madkhali\r\n";
    strcpy(bufferForHeader, responseLines);
    headerOFF += strlen(responseLines);



    //Make Content type; add all ones that are supported, and in default case binary
    // strstr(myUrl, ".html")
    char * contentType = "Content-Type: application/octet-stream\r\n";
    if(strstr(myUrl, ".html")){
      contentType = "Content-Type: text/html; charset=utf-8\r\n";
      printf("HTML!");
    }
    else if(strstr(myUrl, ".txt")){
      contentType = "Content-Type: text/plain; charset=utf-8\r\n";
      printf("TXT!");
    }
    else if(strstr(myUrl, ".jpg")){
      contentType = "Content-Type: image/jpg\r\n";
    }
    else if(strstr(myUrl, ".png")){
      contentType = "Content-Type: image/png\r\n";
    }
    else if(strstr(myUrl, ".gif")){
      contentType = "Content-Type: image/gif\r\n\0";
    }
    else{
      printf("BINARY!");
    }
    strcpy(bufferForHeader + headerOFF, contentType);
    headerOFF += strlen(contentType);
    printf("Content type is %s \n", contentType);

    // printf("%s", bufferForHeader);  //--> Prints correctly!

    //Add content Length


    char contLength[20] = "Content-Length: ";
    char lengthOfFile[20];
    sprintf(lengthOfFile, "%d", (unsigned int)bufferForBodyOfFile);
    strcpy(bufferForHeader + headerOFF, contLength);
    headerOFF += strlen(contLength);
    strcpy(bufferForHeader + headerOFF, lengthOfFile);
    headerOFF += strlen(lengthOfFile);

    printf("Length of file is %s \n", lengthOfFile);
    //printf("%s", bufferForHeader);  //--> Prints correctly!


    //
    // //Add CRLF
    strcpy(bufferForHeader + headerOFF, "\r\n\r\n");
    // headerOFF += strlen("\r\n\r\n");
    //
    // //Merge all of these with the buffer that had the file itself

    printf("%s", bufferForHeader);  //--> Prints correctly!


    strcpy(bufferFinalWrite, bufferForHeader);
    memcpy(bufferFinalWrite + strlen(bufferForHeader), bufferForWrite, bufferForBodyOfFile);
    finalWriteSize = strlen(bufferForHeader) + bufferForBodyOfFile;



    if(write(new_fd, bufferFinalWrite, finalWriteSize) < 0){
      perror("Closed bc of error on writing");
      close(new_fd);
    }


    //Write to client!

    //TCP Request comprises of :
    //   Method
      // • URL
      // • HTTP Version
      // • Header lines
      // • CRLF (carriage return and line feed)




    //TCP Response comprises of:
    //  HTTP Version
    // • Status line
    // • Header lines
    // • CRLF
    // • Data requested

    printf("Connection closed. \n\n");

    close(new_fd);
  }

  close(sockfd);

  return 0;
}
