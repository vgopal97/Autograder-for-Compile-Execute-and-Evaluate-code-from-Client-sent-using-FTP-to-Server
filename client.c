// VENU GOPAL BANDHAKAVI 20CS60R45
// gcc client.c -o client
// ./client <HOSTNAME> <PORT>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <dirent.h>
#include <sys/stat.h>


//CONSTANTS
#define MAX_LIMIT 2048
#define BUF_SIZE 2048
#define FILE_NAME 512

//REMOVING UNWANTED CHARECTERS FROM THE READ BUFFER
void __prep__(char* buf , int SIZE) {
    int i;
    for(i=0 ; i<SIZE ; i++) {
        if(buf[i]=='\0') continue;

        if(buf[i]<33) {
            buf[i]='\0';
        }
    }
}

//GET THE FILE SIZE
int get_file_size(char*filename) {

    struct stat fileinfo;
    if(stat(filename , &fileinfo)==0) {
        return fileinfo.st_size;
    }

    return -1;
}


//SEND A FILE TO THE OTHERSIDE OF THE SOCKET
int RETR(int socket , char* filename){
   //printf("INSIDE RETR FUNCTION");
    int n,m;
    int bytes , bytes_read=0;
    
    unsigned char buffer[BUF_SIZE] = {'\0'} , byt;
    
    int file_size = get_file_size(filename);
    /*
    if(file_size==-1) {
        printf("INCORRECT FILE NAME\n");
        char* msg = "NO FILE FOUND";
        send(socket , msg , strlen(msg) , 0);
        return -1;
    }
    */

    send(socket , &file_size , sizeof(int) , 0);

    FILE *fp = fopen(filename , "rb");

    //printf("SENDING %s OF SIZE %d\n" , filename , file_size);

    while(1) {

        bytes=fread(&byt,1,1,fp);
        bytes_read += bytes;
        

        if ((m = send(socket, &byt, 1 , 0)) == -1) {
            perror("[-]Error in sending file.");
            return -1;
            } 

        //printf("SENT %d BYTES TILL NOW..\n" , bytes_read);
         
         if(bytes_read >= file_size) {
            fclose(fp);
            printf("FILE %s SENT TO SERVER\n" , filename);
            return 1;
        }
    }
    printf("ERROR WHILE SENDING THE FILE %s TO SERVER\n" , filename);
  return 1;
}

//RECIEVED A FILE FROM OTHER SIDE OF SOCKET
int STOR(int socket , char* filename){
    //printf("STOR FUNCTION STARTED..\n");
  FILE *fp;
  unsigned char buffer[BUF_SIZE] , byt;

  int file_size=0;

  int n = read(socket , &file_size , sizeof(int));
 // printf("RECIEVING %s OF SIZE %d\n" , filename , file_size);
  // printf("n : %d\n" , n);
  fp = fopen(filename, "wb");

  int bytes , bytes_written=0;

  while (1) {
    bytes = recv(socket, &byt, 1, 0);
    
    fwrite(&byt , 1 , 1, fp);
    bytes_written += bytes;

    //printf("RECIEVED %d BYTES TILL NOW..\n" , bytes_written);
    //printf("%s" , buffer);
    if(bytes_written>= file_size) {
        fclose(fp);
        printf("STORED THE FILE %s AT SERVER\n" , filename);
        return 1;
    }

    bzero(buffer, BUF_SIZE);
  }
  printf("STORE FUCNTION FAILED..\n");
  return -1;
}


int main(int argc , char* argv[]) {

    //FILE DESCRIPTOR FOR CLIENT
    int client_socket;
    int PORTNO = atoi(argv[2]);
    struct sockaddr_in server_addr;

    struct hostent *server;
	server = gethostbyname(argv[1]);
	bzero((char *) &server_addr, sizeof(server_addr));
	bcopy((char *)server->h_addr, (char *)&server_addr.sin_addr.s_addr, server->h_length);

    //CREATING THE SOCKET
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    
    if(client_socket < 0) {
        perror("ERROR WHILE CREATING SOCKET");
        exit(1);
    }


	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORTNO);
//	server_addr.sin_addr.s_addr = inet_addr("192.168.0.116");

    char input[MAX_LIMIT] , buffer[BUF_SIZE]={'\0'};
    int i,k,n;

    int Exit=0;

    if(connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("ERROR WHILE CONNECTING!\n");
        exit(1);
    }

    printf("CONNECTED TO THE SERVER..\n");

    while(!Exit) {
        printf("FTP_SERVER(%s:%s)$ ",argv[1] , argv[2]);
        memset(&input , '\0' , BUF_SIZE);
         fgets(input, BUF_SIZE , stdin); 

        if(write(client_socket , input , BUF_SIZE)==-1) {
            printf("SERVER DISCONNECTED!\n");
            break;
        }

        //RECIEVE ACTION FROM THE SERVER
        recv(client_socket , &n , sizeof(int) , 0);
        //printf("n : %d\n" , n);

        switch(n) {
            case 1: {
               // printf("RETRIEVE COMMAND\n");
                char filename[FILE_NAME];
                recv(client_socket , filename , FILE_NAME , 0);
               // printf("RECIEVED FILENAME %s\n" , filename);
                __prep__(filename , FILE_NAME);
                int r = STOR(client_socket , filename);
                //printf("RETRIEVING OF FILE FINISHED..\n");
                break;
            }

            case 2: {
                //printf("STOR COMMAND..\n");
                char filename[FILE_NAME];
                read(client_socket , filename , FILE_NAME );
                //printf("FILE NAME IS %s ACCORDING TO SERVER\n" , filename);
                __prep__(filename  , FILE_NAME);
                int f=1;
                if( access( filename, F_OK ) != 0 ) f=-1;

                write(client_socket , &f , sizeof(int));
                if(f==1)
                    RETR(client_socket , filename);
                else printf("NO FILE EXISTS WITH %s NAME\n" , filename);
                break;
            }

            case 3: {
                int i=0 , lines=0;
                char filename[FILE_NAME] = {'\0'};
                read(client_socket , &lines , sizeof(int));
                //printf("NUMBER OF LINES %d" , lines);
                printf("\nLIST OF FILES AT SERVER:\n");
                for(i=0 ; i<lines ; i++) {
                    read(client_socket , filename , FILE_NAME );
                    __prep__(filename , FILE_NAME);
                    printf("%s\t",filename);
                    memset(&filename , '\0' , FILE_NAME);
                }
                printf("\n");
                break;
            }

            case 4: {
                char msg[1000];
                read(client_socket , msg , 1000);
                printf("%s\n",msg);
                break;
            }

            case 5: {
                char msg[1000];
                recv(client_socket , msg , 1000 , 0);
                close(client_socket);
                Exit=1;
                break;
            }

            case 6: {
                    //printf("STOR COMMAND..\n");
                    char filename[FILE_NAME];
                    read(client_socket , filename , FILE_NAME );
                    //printf("FILE NAME IS %s ACCORDING TO SERVER\n" , filename);
                    __prep__(filename  , FILE_NAME);
                    int f=1;
                    if( access( filename, F_OK ) != 0 ) f=-1;

                    write(client_socket , &f , sizeof(int));
                    if(f==1)
                         RETR(client_socket , filename);
                    else printf("NO FILE EXISTS WITH %s NAME\n" , filename);
                    char buffer[BUF_SIZE]={'\0'};
                    read(client_socket , buffer , BUF_SIZE);
                    printf("CODEJUD : %s" ,buffer);
                    break;
            }

            case -1 : {
                printf("INVALID FILENAME\n");
                break;
            }

            default: {
                printf("INVALID COMMAND\n");
                break;
            }
        }
        memset(&input , '\0' , sizeof(input) );
        printf("\n");
    }
    return 0;
}