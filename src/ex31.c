// Nadav Sharabi 213056153
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define NUN_ARG 2
#define ERROR -1
#define BUFFER_SIZE 1024
#define STDOUT_FD 1

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

/**
 * @brief prints the string.
 * 
 * @param str the string to print
 */
void myPrint(const char *str){
    if(write(STDOUT_FD, str, strlen(str)) < 0){};
}

/**
 * @brief Checks if two buffers are equals.
 * 
 * @param bufFile1 the first buffer.
 * @param charRead1 num of char in the first buffer.
 * @param bufFile2 the second buffer.
 * @param charRead2 num of char in the second buffer.
 * @return Bool TRUE if equals, else FALSE.
 */
Bool isBufferEquals(char bufFile1[], int charRead1, char bufFile2[], int charRead2) {
    // if the num of char is diffrent they aren't equal.
    if (charRead1 != charRead2) {
        return FALSE;
    }

    // iterating the buffers.
    int i;
    for(i = 0; i < charRead1; ++i){
        // if there is diffrent char they aren't equal.
        if(bufFile1[i] != bufFile2[i]) {
            return FALSE;
        }
    }

    // if we got here they are equal
    return TRUE;
}

/**
 * @brief Check if char is char that
 * should be noticed when checking if files are simillar.
 * 
 * @param c the char
 * @return Bool True if it is, else FALSE
 */
Bool isValidChar(char c) {
    // Those are the chars that should be ignored:
    return !(c == ' ' || c == '\n' || c == '\r');
}

/**
 * @brief Checks if two chars are simillar.
 * 
 * @param c1 the first char.
 * @param c2 the second char.
 * @return Bool 
 */
Bool isSimillarChars(char c1, char c2){
    // if c1 is in upper-case than change to lower case.
    if('A' <= c1 && c1 <= 'Z'){
        c1 = c1 - 'A' + 'a';
    }

    // if c2 is in upper-case than change to lower case.
    if('A' <= c2 && c2 <= 'Z'){
        c2 = c2 - 'A' + 'a';
    }

    // checks if are simillar:
    return (c1 == c2);
}

/**
 * @brief checks if two files are similar.
 * the buffer are those who first not equal
 * the checking will resume from them 
 * (assuming all before char are the equal)
 * 
 * @param fdFile1 the first file.
 * @param fdFile2 the second file.
 * @param bufFile1 the first diffrent buffer in file 1.
 * @param charRead1 num chars in the first buffer.
 * @param bufFile2 the second diffrent buffer in file 2.
 * @param charRead2 num chars in the second buffer.
 * @return Bool True if similar, else false.
 * also charRead1, charRead2, bufFile1, bufFile2 are updating.
 * if was an error while reading: charRead1 < 0 || charRead2 < 0.
 */
Bool isFilesSimilar(int fdFile1, int fdFile2, 
    char bufFile1[], int* charRead1, char bufFile2[], int* charRead2) {
    
    do {
        int index1 = 0, index2 = 0;
        while(*charRead1 != 0 || *charRead2 != 0) {
            // if the first buffer is empty.
            if(*charRead1 == 0){
                // getting the next chars to check.
                index1 = 0;
                if((*charRead1 = read(fdFile1, bufFile1, BUFFER_SIZE)) < 0) {
                    return FALSE;
                }

                //if end of first file
                if(*charRead1 == 0){
                    do{ // keep reading file 2.
                        while(*charRead2 != 0) {
                            // if file 2 contains a valid char the files are diffrent.
                            if(isValidChar(bufFile2[index2])){
                                return FALSE;
                            }
                            
                            // setting for next char.
                            ++index2;
                            --(*charRead2);
                        }

                        index2 = 0;
                    }while((*charRead2 = read(fdFile2, bufFile2, BUFFER_SIZE)) > 0);

                    // if we got here the files are identical.
                    return TRUE;
                }
            }

            // if the second buffer is empty.
            if(*charRead2 == 0){
                // getting the next chars to check.
                index2 = 0;
                if((*charRead2 = read(fdFile2, bufFile2, BUFFER_SIZE)) < 0) {
                    return FALSE;
                }

                //if end of second file
                if(*charRead2 == 0){
                    do{ // keep reading file 1.
                        while(*charRead1 != 0) {
                            // if file 1 contains a valid char the files are diffrent.
                            if(isValidChar(bufFile1[index1])){
                                return FALSE;
                            }

                            // setting for next char.
                            ++index1;
                            --(*charRead1);
                        }

                        index1 = 0;
                    }while((*charRead1 = read(fdFile1, bufFile1, BUFFER_SIZE)) > 0);
                    
                    // if we got here the files are identical.
                    return TRUE;
                }
            }

            // iterating the buffers.
            while(*charRead1 != 0 && *charRead2 != 0) {
                char char1 = bufFile1[index1];
                char char2 = bufFile2[index2];

                // if the char1 isn't valid it shouldn't be checked.
                if(!isValidChar(char1)){
                    ++index1;
                    --(*charRead1);
                    continue;
                }

                // if the char2 isn't valid it shouldn't be checked.
                if(!isValidChar(char2)){
                    ++index2;
                    --(*charRead2);
                    continue;
                }
                
                // if chars aren't similar files are diffrent.
                if(!isSimillarChars(char1, char2)){
                    return FALSE;
                }

                // updating next chars checking
                ++index1;
                --(*charRead1);
                ++index2;
                --(*charRead2);
            }
        }

        // reading next chars.
        *charRead1 = read(fdFile1, bufFile1, BUFFER_SIZE);
        *charRead2 = read(fdFile2, bufFile2, BUFFER_SIZE);
    }while(*charRead1 > 0 || *charRead2 > 0);

    //if we got her file are identical (assuming no error occurred).
    return TRUE;
}

/**
 * @brief Compares between two files. 
 * 
 * @param fdFile1 the first file discriptor.
 * @param fdFile2 the second file discriptor.
 * @return Status (of the comparing)
 */
Status compareFiles(int fdFile1, int fdFile2) {
    // At first we assume the files are equal.
    Status status = FILES_EQUAL;

    char bufFile1[BUFFER_SIZE + 1], bufFile2[BUFFER_SIZE + 1]; 
    int charRead1, charRead2;
    
    // we read chars from the files
    charRead1 = read(fdFile1, bufFile1, BUFFER_SIZE);
    charRead2 = read(fdFile2, bufFile2, BUFFER_SIZE);
    while(charRead1 > 0 || charRead2 > 0){

        // if the chars are not equal we will break the 
        // loop and assume they are simillar.
        if(!isBufferEquals(bufFile1, charRead1, bufFile2, charRead2)) {
            status = FILES_SIMILLAR;
            break;
        }

        // reads the next chars
        charRead1 = read(fdFile1, bufFile1, BUFFER_SIZE);
        charRead2 = read(fdFile2, bufFile2, BUFFER_SIZE);
    }

    // if we assumed the files are simillar, we will check if they are.
    if(status == FILES_SIMILLAR){
        // if the files aren't simillar the are diffrent.
        if(!isFilesSimilar(fdFile1, fdFile2,
            bufFile1, &charRead1, bufFile2, &charRead2)){
            status = FILES_DIFFRENT;
        }
    }

    //Here we check if errors occurred while reading.
    if(charRead1 < 0 || charRead2 < 0) {
        myPrint("Error in: read\n");
        status = ERROR_OCCURRED;
    }

    return status;
}

/**
 * @brief This program check similarity between
 * two files.
 * 
 * @param argc num of args
 * @param argv the args (should contain path to two files)
 * @return int: -1 =Error, 1=identical, 2=diffrent ,3=similar
 */
int main(int argc, char const *argv[])
{
    // first args is the name of the program
    // should be ignored
    --argc;
    int index = 1;

    // checks if num of args are as needed.
    if(argc != NUN_ARG) {
        return ERROR;
    }

    // Opening the first file
    int fdFile1 = open(argv[index], O_RDONLY);
    if(fdFile1 < 0){
        myPrint("Error in: open\n");
        return ERROR;
    }

    // Opening the second file
    ++index;
    int fdFile2 = open(argv[index], O_RDONLY);
    if(fdFile2 < 0){
        close(fdFile1);

        myPrint("Error in: open\n");
        return ERROR;
    }

    // comparing the files
    int result = compareFiles(fdFile1, fdFile2);

    // closing the first file
    if(close(fdFile1) < 0){
        myPrint("Error in: close\n");
        return ERROR;
    }

    // closing the second file
    if(close(fdFile2) < 0){
        myPrint("Error in: close\n");
        return ERROR;
    }

    return result;
}
