// Venu Gopal Bandhakavi 20CS60R45
// gcc server.c -o server
// ./server <PORT-NUMBER>

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include<fcntl.h>
#include <sys/stat.h>
#include <netdb.h>
#include <dirent.h>
#include<sys/time.h>
#include<time.h>

//CONSTANTS
#define BUF_SIZE 2048
#define MSG_SIZE 2048
#define FILE_NAME 512
#define CMD_SIZE 2700

//STRUCTURES

typedef struct select_utils{
    int fdmax;
    fd_set main;
    fd_set read_fd;
} select_utils;

//INITIALIZE THE SELECT UTILITIES
void __init__(select_utils *s) {
    FD_ZERO(&(s->main)); 
    FD_ZERO(&(s->read_fd));
}

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

//CLEARING THE BUFFER
void __clear__(char* buf , int SIZE ){
    int i;
    for(i=0 ; i<SIZE ;i++) buf[i] = '\0';
}

//GET THE FILE SIZE
int get_file_size(char*filename) {

    struct stat fileinfo;
    if(stat(filename , &fileinfo)==0) {
        return fileinfo.st_size;
    }

    return -1;
}

//RETURN NUMBER OF LINES IN A FILE
int num_lines(char* filename) {
    FILE *fp = fopen(filename, "r"); 
    int count=0;
    char c;
    if (fp == NULL) return 0; 
  
    for (c = getc(fp); c != EOF; c = getc(fp)) 
        if (c == '\n')
            count = count + 1; 
  
    fclose(fp); 

    return count;
}

//GIVES THE MILLISECONDS TAKEN
double exec_time(clock_t start , clock_t stop) {
    return (double)(stop - start) * 1000.0 / CLOCKS_PER_SEC;
}

//SEND A FILE TO THE OTHERSIDE OF THE SOCKET
int RETR(int socket , char* filename){
   //printf("INSIDE RETR FUNCTION\n");
    int n,m;
    int bytes , bytes_read=0;
    
    unsigned char buffer[BUF_SIZE] = {'\0'} , byt;
   // printf("GETTING FILESIZE..\n");
    int file_size = get_file_size(filename);
    /*
    if(file_size==-1) {
        printf("INCORRECT FILE NAME\n");
        char* msg = "NO FILE FOUND";
        send(socket , msg , strlen(msg) , 0);
        return -1;
    }
    */

    write(socket , &file_size , sizeof(int));

    FILE *fp = fopen(filename , "rb");

    //printf("SENDING %s OF SIZE %d B\n" , filename , file_size);

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
            return 1;
        }
    }
    //printf("RETR FUNCTIN FINISHED\n");
  return 1;
}

//RECIEVED A FILE FROM OTHER SIDE OF SOCKET
int STOR(int socket , char* filename){
    //printf("INSIDE STOR FUNCTION\n");
  FILE *fp;
  unsigned char buffer[BUF_SIZE] , byt;

  int file_size;

  recv(socket , &file_size , sizeof(int) , 0);
  printf("RECIEVING %s OF SIZE %d B\n" , filename , file_size);

  fp = fopen(filename, "wb");
  // printf("Working!\n");
  int bytes , bytes_written=0;

  while (1) {
    bytes = recv(socket, &byt, 1, 0);
    fwrite(&byt , 1 , 1, fp);
    //printf("Working\n");
    bytes_written += bytes;

   // printf("RECIEVED %d BYTES TILL NOW..\n" , bytes_written);
    //printf("%s" , buffer);
    if(bytes_written>= file_size) {
        fclose(fp);
       // printf("STOR FUNCTION SUCCESFUL\n");
        return 1;
    }

    bzero(buffer, BUF_SIZE);
  }
    //printf("STOR FUNCTION NOT SUCCESFUL\n");
  return -1;
}

// TO LIST ALL THE FILES IN THE SERVER
void LIST(int socket) {
      //printf("NO PROBLEM BEFORE LIST START\n");
      system("ls > zzzzzzzz.txt");
      FILE *fp = fopen("zzzzzzzz.txt" , "r");
      char filename[FILE_NAME]={'\0'};
      int i=0 , lines = num_lines("zzzzzzzz.txt") -1;
      write(socket , &lines , sizeof(int));
     // printf("NUMBER OF LINES %d" , lines);

        for(i=0 ; i<lines ; i++) {
            fgets(filename , FILE_NAME , fp);
            write(socket , filename , FILE_NAME);
            memset(&filename , '\0' , FILE_NAME);
        }

      fclose(fp);
      //printf("NO PROBLEM AFTER LIST\n");
      remove("zzzzzzzz.txt");
     // printf("NO PROBLEM AT END\n");
      
  }


//TO DELETE A FILE FROM THE SERVER
int DELE(int socket , char* filename) {
    return remove(filename);
}

int get_extension(char* extension) {
    
    if(strcmp(extension , "c")==0) return 1;
    else if(strcmp(extension , "cpp")==0) return 2;

    return 0;
}

void rm(char* filename) {
    char cmd[CMD_SIZE]={'\0'};
    sprintf(cmd , "rm %s" , filename);
    system(cmd);
}

int CODEJUD(char* CMD , int socket , char* MSG) {
    //FILE NEEDED FOR EVAUATION
    char filename[FILE_NAME]={'\0'} , extension[5] = {'\0'} , file_noex[FILE_NAME]={'\0'} , ex[5]={'\0'};
    //PARSING THE ARGUEMENTS
    sscanf(CMD , "%s %s" , filename , extension);
    __prep__(filename , FILE_NAME);
    __prep__(extension , 5);

    int h=0;
    for(h=0 ; h<strlen(filename) ; h++) {
        if(filename[h]=='.') break;
        file_noex[h] = filename[h];
    }
    //printf("FILE WITH NO EXTENSION: %s\n" , file_noex);

    char model_output[BUF_SIZE]={'\0'} , testcases[BUF_SIZE]={'\0'};
    sprintf(model_output , "testcase_%s.txt" , file_noex);
    sprintf(testcases , "input_%s.txt" , file_noex);
    //IF THE FILES NEEDED FOR THE EVALUATIONS OF THE PROGRAM ARE NOT AVAILABLE
    if( (access( model_output, F_OK ) != 0) || (access(testcases,F_OK)!=0) ) {
        sprintf(MSG , "REQUIRED FILES FOR THE %s PROGRAM EVALUATION NOT AVAILABLE" , filename);
        return 8;
    }

    //STORE THE FILE
    int f , n=6;
    char buffer[BUF_SIZE]={'\0'};
    time_t start,stop;
    write(socket , &n , sizeof(int));
    write(socket , filename , FILE_NAME);
    read(socket , &f , sizeof(int));
    if(f==1) {
        f = STOR(socket , filename);
        printf("RECIEVED THE FILE \n");
        }
    else {
        printf("NO FILE WITH %s NAME EXISTS!\n" ,filename);
        sprintf(MSG , "NO FILE %s FOUND" , filename);
        return 1;
    }

    //HANDLE THE EXTENSION
    int ch=get_extension(extension);

    if(!ch) {
        rm(filename);
        sprintf(MSG , "FILE TYPE NOT SUPPORTED");
        return 2;
    }

    //EVALUATE THE PROGRAM
    char cmd[CMD_SIZE]={'\0'} , temp_cmd[CMD_SIZE]={'\0'};
    char compile_file[FILE_NAME]={'\0'} , output_file[FILE_NAME]={'\0'};
    sprintf(compile_file , "compile_out_%d.txt" , socket);
    sprintf(output_file , "output_file_%d.txt" , socket);


    if(ch==1) {
        sprintf(cmd , "gcc %s -o output_%d 2> %s" , filename , socket , compile_file);
    }
    else if(ch==2) {
        sprintf(cmd , "g++ %s  -o output_%d 2> %s" , filename , socket , compile_file);
    }

    //COMPILATION
    system(cmd);
    printf("COMPILATION DONE!\n");

    FILE *fp = fopen(compile_file , "r");
    //WRITING TO THE CLIENT ABOUT COMPILATION
    fgets(buffer , BUF_SIZE , fp);
    fclose(fp);
    if(strlen(buffer)==0) {
        printf("COMPILATION SUCCESFULL! NO ERROR!\n");
    }
    else {
        printf("COMPILATION ERRORS!\n");
        printf("%s" , buffer);
        rm(compile_file);
        rm(filename);
        sprintf(MSG , "COMPILATION ERROR \n %s" , buffer);
            return 3;
    }
    
    //REMOVAL OF THE COMPILE OUTPUT FILE
    rm(compile_file);


    //EXECUTION OF THE PROGRAM
    int t = num_lines(testcases)+1;
    long int time_taken;

    FILE *tfp=fopen(testcases , "r") , *outfp=fopen(output_file , "w");
    
    memset(&cmd , '\0' , CMD_SIZE);
    sprintf(cmd , "./output_%d < __input__.txt 1> __output__.txt" , socket);
    system("touch __input__.txt");
    system("touch __output__.txt");
    system("touch __error__.txt");

    
    sprintf(temp_cmd , "./output_%d < __input__.txt 2> __error__.txt" , socket);

    char* line=NULL;
    size_t len;
    
    while(t--) {
        //GETTING THE TESTCASE
         FILE *ifp = fopen("__input__.txt" , "w"); 
         getline(&line ,&len , tfp);
         fprintf(ifp , "%s" , line);
         fclose(ifp);
         
         //RUNNING THE PROGRAM FOR THE GIVEN TESTCASE
         time(&start);
         system(cmd);
         time(&stop);
        
        //CALCULATING THE TIME TAKEN
         time_taken = stop-start;
         
         //CHECKING FOR RUNTIME ERRORS
         system(temp_cmd);
         //printf("TESTCASE %d , TIME TAKEN: %lf\n" , t , time_taken);
         FILE *efp=fopen("__error__.txt" , "r");
         memset(&buffer , '\0' , BUF_SIZE );
         fgets(buffer  , BUF_SIZE , efp);
         fclose(efp);

         if(strlen(buffer)>0) {
             rm(filename);
             rm("__input__.txt");
             rm("__output__.txt");
             rm("__error__.txt");
             fclose(outfp);
             fclose(tfp);
             rm(output_file);
             sprintf(MSG , "RUNTIME ERROR \n %s" , buffer); 
             return 6;
         }
         //CHECKING FOR TLE
         if(time_taken > 1){
             rm(filename);
             rm("__input__.txt");
             rm("__output__.txt");
             rm("__error__.txt");
             fclose(outfp);
             fclose(tfp);
             rm(output_file);
             sprintf(MSG , "TIME LIMIT EXCEEDED");
             return 4;
         } 

         //RECORDING THE OUTPUT OF THE PROGRAM
         FILE *ofp=fopen("__output__.txt" , "r");
         line=NULL;
         getline(&line  , &len , ofp);
         fclose(ofp);
         fprintf(outfp , "%s" , line);
    }
    fclose(outfp);
    fclose(tfp);
    rm("__input__.txt");
    rm("__output__.txt");
    rm("__error__.txt");

    //CHECKING FOR DIFFERENCE
    memset(&cmd , '\0' , CMD_SIZE);
    sprintf(cmd , "diff --strip-trailing-cr -b %s %s > __output__.txt" , model_output , output_file);
    system(cmd);
    

    //EVALUATING WHETHER THE ANSWER IS CORRECT OR NOT
    FILE *hfp=fopen("__output__.txt" , "r");
    memset(&buffer , '\0' , BUF_SIZE);
    fgets(buffer , BUF_SIZE , hfp);
    fclose(fp);

    if(strlen(buffer)==0) {
        printf("SUCESS\n");
        sprintf(MSG , "ACCEPTED");
    }
    else {
        printf("%d\n" , (int)strlen(buffer) );
        printf("FAILURE\n");
        rm(output_file);
        rm(filename);
        rm("__output__.txt");
        sprintf(MSG , "WRONG ANSWER");
        return 5;
    }

    //SEND THE RESULT


    //DELETE THE FILE
    rm(output_file);
    rm(filename);
    rm("__output__.txt");

    return 0;
}

//RETURNS TH COMMAND NUMBER
int command(char* cmd) {
  
    if(strcmp(cmd , "RETR")==0) return 1;
    else if(strcmp(cmd , "STOR")==0) return 2;
    else if(strcmp(cmd , "LIST")==0) return 3;
    else if(strcmp(cmd , "DELE")==0) return 4;
    else if(strcmp(cmd , "QUIT")==0) return 5;
    else if(strcmp(cmd , "CODEJUD")==0) return 6;

    return 0;
}

// PARSES THE COMMAND TO GET COMMAND AND FILENAME
void parse(char* buffer , char* cmd , char* filename) {
    int i=0,j;

    for(i=0;i<strlen(buffer);i++) {
        if(buffer[i]==' ') {
            i++;
            break;
        }
        cmd[i] = buffer[i];
    }

    while(buffer[i]==' ') i++;

    for(j=0 ; i<strlen(buffer) ; i++ , j++) {
        filename[j] = buffer[i];
    }
    //printf("filename : %s\n" , filename);
}

//GET ADDRESS OF THE SERVER
void *getAddress(struct sockaddr *addr)
{
	if (addr->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)addr)->sin_addr);
	}

	return &(((struct sockaddr_in6*)addr)->sin6_addr);
}

int main(int argc , char* argv[]) {

    //PORT NUMBER FOR THE SERVER
    char* PORT = argv[1];

    select_utils s_utils;
    __init__(&s_utils);

    //SERVER FD USED FOR ACCEPTING CLIENT SOCKET
    int server_fd , client_fd;

    //CLIENT ADDRESS STRUCTURE
    //USING SOCKADD_STORAGE BEACAUSE IT 
    //ALLOWS BOTH IPV4 AND IPV6
    struct sockaddr_storage client_addr;
    socklen_t addrlen;

    char clientIP[INET6_ADDRSTRLEN];

    char buffer[BUF_SIZE];
    int i , j , k , fd;

    //FLAGS
    int Exit=0;

    //MISC
    int opt=1;

    //IT GIVES INFO WHETHER AN IP ADDRESS IS
    //AVALIABLE OR NOT
    struct addrinfo h , *a , *iter;

    memset(&h , 0 , sizeof(h));
    h.ai_family = AF_UNSPEC;
    h.ai_socktype = SOCK_STREAM; 
    h.ai_flags = AI_PASSIVE;
    
    //ITER WILL HAVE POSSIBLE IP ADDRESSES
    if( getaddrinfo(NULL , PORT , &h , &a) != 0) {
        perror("GET ADDRESS INFO ERROR!");
        exit(1);
    }

    for(iter=a ; iter!=NULL;iter=iter->ai_next) {
        server_fd = socket(iter->ai_family , iter->ai_socktype , iter->ai_protocol);

        if(server_fd<0) continue;

    setsockopt(server_fd , SOL_SOCKET , SO_REUSEADDR , &opt , sizeof(int));

    if(bind(server_fd , iter->ai_addr , iter->ai_addrlen) < 0) {
        close(server_fd);
        continue;
    }
    break;

    }

    if(!iter) {
        perror("BINDING ERROR!");
        exit(2);
    }

    freeaddrinfo(a);

    if(listen(server_fd , 15) == -1) {
        perror("LISTENING ERROR!");
        exit(3);
    }

    FD_SET(server_fd , &(s_utils.main));

    s_utils.fdmax = server_fd;

    printf("FTP SERVER RUNNING...\n");

    while(!Exit) {

        s_utils.read_fd = s_utils.main;

        if (select(s_utils.fdmax+1, &(s_utils.read_fd), NULL, NULL, NULL) == -1) {
            perror("SELECTION ERROR!");
            exit(4);
        }

        for(fd=0 ; fd<=s_utils.fdmax ; fd++) {
            if(FD_ISSET(fd , &(s_utils.read_fd))) {
                if(fd == server_fd) {
                    addrlen = sizeof(client_addr);
                    client_fd = accept(server_fd , (struct sockaddr *)&client_addr,&addrlen);

                    if(client_fd==-1) {
                        perror("ACCEPT ERROR!");
                    }
                    else {
                        FD_SET(client_fd , &(s_utils.main));

                        if(client_fd > s_utils.fdmax) {
                            s_utils.fdmax = client_fd;
                        }

                         printf("SERVER: NEW CONNECTION FROM %s ON " "socket %d\n",inet_ntop(client_addr.ss_family,getAddress((struct sockaddr*)&client_addr),clientIP, INET6_ADDRSTRLEN),client_fd);

                    }//NEW CLIENT ELSE
                }//NEW CLIENT

                else {
                    __clear__(buffer,BUF_SIZE);
                    read(fd , buffer , BUF_SIZE);
                    //printf("RECIEVED FROM CLIENT : %s OF LENGTH %d\n",buffer , (int)strlen(buffer));
                   // __prep__(buffer , BUF_SIZE);
                    

                //BUFFER PARSING
                char cmd[BUF_SIZE]={'\0'} , filename[FILE_NAME]={'\0'};
                parse(buffer , cmd , filename);
                int n=1;
                __prep__(cmd , BUF_SIZE);
                if(command(cmd)!=6) __prep__(filename , FILE_NAME);
                //printf("PARSED TO COMMAND : %s OF LENGTH %d\n AND FILENAME IS : %s OF LENGTH %d\n" , cmd , (int)strlen(cmd) , filename , (int)strlen(filename));
                //printf("n : %d\n" , command(cmd));
                switch((n=command(cmd))) {

                    case 1: {
                        printf("RETR COMMAND RECIEVED FROM CLIENT %d\n" , fd);
                       if( access( filename, F_OK ) != 0 ) n=-1;

                       send(fd , &n , sizeof(int) , 0);
                      // printf("n = %d\n" , n);

                       if(n==1) {
                          // printf("EVERYTHING IS VALID AND SENDING DATA TO CLIENT..\n");
                           send(fd , filename , FILE_NAME , 0);
                           int r = RETR(fd , filename);
                           if(r==1) printf("FILE %s SENT FROM CLIENT%d .." , filename ,fd);
                       } else {
                          // printf("n = %d\n" , n);
                           printf("NO FILE WITH NAME %s EXISTS!\n",filename);
                       }
                        break;
                    }

                    case 2: {
                        printf("STOR COMMAND RECIEVED FROM CLIENT %d\n" , fd);
                        int f;
                        //printf("SENDING n = 2\n");
                        write(fd , &n , sizeof(int));
                       // printf("SENDING THE FILENAME %s\n" , filename);
                        write(fd , filename , FILE_NAME);
                        read(fd , &f , sizeof(int));
                       // printf("f = %d\n" , f);
                        if(f==1) {
                            //printf("FILE EXISTS AT THE CLIENT!\n");
                            f = STOR(fd , filename);
                        }
                        else printf("NO FILE WITH %s NAME EXISTS!\n" ,filename);
                        break;
                    }

                    case 3: {
                        printf("LIST COMMAND RECIEVED FROM CLIENT %d\n" , fd);
                        send(fd , &n , sizeof(int) , 0);
                        LIST(fd);
                        printf("LIST SENT TO CLIENT %d!\n" , fd);
                        break;
                    }

                    case 4: {
                        printf("DELE COMMAND RECIEVED FROM CLIENT %d\n" , fd);
                        if( access( filename, F_OK ) != 0 ){
                            n=-1;
                            printf("DELE FAILED NO FILE FOUND WITH %s NAME\n" , filename);
                        } 

                       send(fd , &n , sizeof(int) , 0);
                       char msg[1000];
                       if(n==4) {
                           
                           DELE(fd , filename);
                           sprintf(msg , "FILE %s HAS BEEN DELETED!" , filename);
                           printf("FILE %s HAS BEEN DELETED!\n" , filename);
                           write(fd , msg , 1000);
                       }

                       break;
                    }

                    case 6: {
                        printf("CODEJUD COMMAND RECIEVED FROM CLIENT %d\n" , fd);
                        char MSG[BUF_SIZE]={'\0'};
                        printf("ARGS ARE %s\n" , filename);
                        int flag = CODEJUD(filename , fd , MSG);
                        printf("\nCODEJUD : %s\n" , MSG);
                        write(fd , MSG , BUF_SIZE);
                        break;
                    }

                    case 5: { 
                        printf("QUIT COMMAND RECIEVED FROM CLIENT %d\n" , fd);
                        send(fd , &n , sizeof(int) , 0);

                        printf("DISCONNECTING FROM THE CLIENT %d" , fd);

                        send(fd , "SERVER: DISCONNECTING" ,21,0);
                        close(fd);
                        FD_CLR(fd , &(s_utils.main));
                        break;
                    }

                    default : {
                        send(fd , &n , sizeof(int) , 0);
                        printf("INVALID COMAND. TRY AGAIN..!\n");
                        break;
                    }

                }

                }
            }

        }//FOR LOOP
        printf("\n");
       // printf("\n\n______________TRANSACTION FINISHED______________________\n\n");

    }// WHILE 1 LOOP


    return 0;
}
