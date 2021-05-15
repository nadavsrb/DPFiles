// Nadav Sharabi 213056153
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#define NUN_ARG 2
#define ERROR -1
#define BUFFER_SIZE 1024

typedef enum{
    ERROR_OCCURRED = -1,
    FILES_EQUAL = 1,
    FILES_DIFFRENT = 2,
    FILES_SIMILLAR = 3
}Status;

typedef enum{
    FALSE,
    TRUE
}Bool;

Bool isBufferEquals(char bufFile1[], int charRead1, char bufFile2[], int charRead2) {
    if (charRead1 != charRead2) {
        return FALSE;
    }

    for(int i = 0; i < charRead1; ++i){
        if(bufFile1[i] != bufFile2[i]) {
            return FALSE;
        }
    }

    return TRUE;
}

Bool isValidChar(char c) {
    if (c == ' ' || c == '\n' || c == '\r') {
        return FALSE;
    }

    return TRUE;
}

Bool isSimillarChars(char c1, char c2){
    if('A' <= c1 && c1 <= 'Z'){
        c1 = c1 - 'A' + 'a';
    }

    if('A' <= c2 && c2 <= 'Z'){
        c2 = c2 - 'A' + 'a';
    }

    return (c1 == c2);
}

Bool isFilesSimilar(int fdFile1, int fdFile2, 
    char bufFile1[], int* charRead1, char bufFile2[], int* charRead2) {
    
    do {
        int index1 = 0, index2 = 0;
        while(*charRead1 != 0 || *charRead2 != 0) {
            if(*charRead1 == 0){
                index1 = 0;
                if((*charRead1 = read(fdFile1, bufFile1, BUFFER_SIZE)) < 0) {
                    return FALSE;
                }

                if(*charRead1 == 0){ //end of first file
                    do{
                        while(*charRead2 != 0) {
                            if(isValidChar(bufFile2[index2])){
                                return FALSE;
                            }

                            ++index2;
                            --(*charRead2);
                        }

                        index2 = 0;
                    }while((*charRead2 = read(fdFile2, bufFile2, BUFFER_SIZE)) > 0);

                    return TRUE;
                }
            }

            if(*charRead2 == 0){
                index2 = 0;
                if((*charRead2 = read(fdFile2, bufFile2, BUFFER_SIZE)) < 0) {
                    return FALSE;
                }

                if(*charRead2 == 0){ //end of second file
                    do{
                        while(*charRead1 != 0) {
                            if(isValidChar(bufFile1[index1])){
                                return FALSE;
                            }

                            ++index1;
                            --(*charRead1);
                        }

                        index1 = 0;
                    }while((*charRead1 = read(fdFile1, bufFile1, BUFFER_SIZE)) > 0);

                    return TRUE;
                }
            }

            while(*charRead1 != 0 && *charRead2 != 0) {
                char char1 = bufFile1[index1];
                char char2 = bufFile2[index2];

                if(!isValidChar(char1)){
                    ++index1;
                    --(*charRead1);
                    continue;
                }

                if(!isValidChar(char2)){
                    ++index2;
                    --(*charRead2);
                    continue;
                }
                
                //printf("%c,%c ", char1, char2);
                if(!isSimillarChars(char1, char2)){
                    //printf("not similar\n");
                    return FALSE;
                }
                //printf("similar\n");

                ++index1;
                --(*charRead1);

                ++index2;
                --(*charRead2);
            }
        }

        *charRead1 = read(fdFile1, bufFile1, BUFFER_SIZE);
        *charRead2 = read(fdFile2, bufFile2, BUFFER_SIZE);
    }while(*charRead1 > 0 || *charRead2 > 0);

    return TRUE;
}

Status compareFiles(int fdFile1, int fdFile2) {
    Status status = FILES_EQUAL;
    char bufFile1[BUFFER_SIZE + 1], bufFile2[BUFFER_SIZE + 1]; 
    int charRead1, charRead2;
    
    charRead1 = read(fdFile1, bufFile1, BUFFER_SIZE);
    charRead2 = read(fdFile2, bufFile2, BUFFER_SIZE);
    while(charRead1 > 0 || charRead2 > 0){

        if(!isBufferEquals(bufFile1, charRead1, bufFile2, charRead2)) {
            status = FILES_SIMILLAR;
            break;
        }

        charRead1 = read(fdFile1, bufFile1, BUFFER_SIZE);
        charRead2 = read(fdFile2, bufFile2, BUFFER_SIZE);
    }

    if(status == FILES_SIMILLAR){
        if(!isFilesSimilar(fdFile1, fdFile2,
            bufFile1, &charRead1, bufFile2, &charRead2)){
            status = FILES_DIFFRENT;
        }
    }

    if(charRead1 < 0 || charRead2 < 0) {
        printf("Error in: read\n");
        status = ERROR_OCCURRED;
    }

    return status;
}

int main(int argc, char const *argv[])
{
    //first args is the name of the program
    //should be ignored
    --argc;
    int index = 1;

    if(argc != NUN_ARG) {
        return ERROR;
    }

    int fdFile1 = open(argv[index], O_RDONLY);
    if(fdFile1 < 0){
        printf("Error in: open\n");
        return ERROR;
    }

    ++index;

    int fdFile2 = open(argv[index], O_RDONLY);
    if(fdFile2 < 0){
        close(fdFile1);

        printf("Error in: open\n");
        return ERROR;
    }

    int result = compareFiles(fdFile1, fdFile2);

    close(fdFile1);
    close(fdFile2);

    return result;
}
