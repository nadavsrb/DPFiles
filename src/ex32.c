// Nadav Sharabi 213056153
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>

#define SIMILLAR_FILES 3
#define IDENTICAL_FILES 1
#define DIFFRENT_FILES 2
#define TIMEOUT_EXIT_VAL 124
#define NUM_OF_ARGS 1
#define MAX_RUNNING_TIME "5s"
#define EXIT_WITH_ERROR -127
#define CHECKED_OUTPUT_FILE_PATH "output_file_for_checking.txt"
#define RESULT_FILE_PATH "results.csv"
#define ERROR_FILE_PATH "errors.txt"
#define NAME_COMPILED_TO_CHECK "compiled_file_to_check.out"
#define RUN_COMPILED_TO_CHECK "./compiled_file_to_check.out"
#define RUN_COMPILED_CHECK_BY "./comp.out"
#define CONFIG_FILE_MAX_DATA 453 //150 chars for 3 rows + \n for each
#define MAX_SIZE_CONFIG_DATA_LINE 150
#define MAX_SIZE_LINE_IN_RESULT_FILE 172 //150 chars max name + 22 chars max status (",COMPILATION_ERROR,10\n")
#define STDIN_FD 0
#define STDOUT_FD 1
#define STDERROR_FD 2

typedef enum{
    FALSE,
    TRUE
}Bool;

typedef struct
{
    DIR* mainDirFd;
    char mainDirPath[MAX_SIZE_CONFIG_DATA_LINE + 1];
    char correctOutputFilePath[MAX_SIZE_CONFIG_DATA_LINE + 1];
    char inFilePath[MAX_SIZE_CONFIG_DATA_LINE + 1];
}ConfigData;

typedef enum
{
    NO_C_FILE,
    COMPILATION_ERROR,
    TIMEOUT,
    WRONG,
    SIMILAR,
    EXCELLENT
}CheckStatus;


int isDirectory(const char *path) {
   struct stat statbuf;
   if (stat(path, &statbuf) != 0)
       return 0;
   return S_ISDIR(statbuf.st_mode);
}

void myPrint(const char *str){
    write(STDOUT_FD, str, strlen(str));
}

Bool setConfigData(char const *configFilePath, ConfigData* configData){
    int configFd;
    if((configFd = open(configFilePath, O_RDONLY)) < 0) {
        return FALSE;
    }

    char buffer[CONFIG_FILE_MAX_DATA + 1]; //+ 1 for \0
    int charRead;
    if((charRead = read(configFd, buffer ,CONFIG_FILE_MAX_DATA)) < 0){
        close(configFd);
        return FALSE;
    }

    buffer[charRead] = '\0';
    close(configFd);

    strcpy(configData->mainDirPath, strtok(buffer, "\n"));
    strcpy(configData->inFilePath, strtok(NULL, "\n"));
    strcpy(configData->correctOutputFilePath, strtok(NULL, "\n"));

    if((configData->mainDirFd = opendir(configData->mainDirPath)) == NULL){
        myPrint("Not a valid directory\n");
        return FALSE;
    }

    if(access(configData->inFilePath, F_OK) < 0) {
        closedir(configData->mainDirFd);
        myPrint("Input file not exist\n");
        return FALSE;
    }

    if(access(configData->correctOutputFilePath, F_OK) < 0) {
        closedir(configData->mainDirFd);
        myPrint("Output file not exist\n");
        return FALSE;
    }

    return TRUE;
}

void gardeStudent(CheckStatus status, char* name) {
    int resultFd;
    if((resultFd = open(RESULT_FILE_PATH, O_WRONLY | O_APPEND)) < 0){
        return;
    }

    char line[MAX_SIZE_LINE_IN_RESULT_FILE + 1];
    strcpy(line, name);

    switch (status)
    {
    case NO_C_FILE:
        strcat(line, ",NO_C_FILE,0\n");
        break;
    case COMPILATION_ERROR:
        strcat(line, ",COMPILATION_ERROR,10\n");
        break;
    case TIMEOUT:
        strcat(line, ",TIMEOUT,20\n");
        break;
    case WRONG:
        strcat(line, ",WRONG,50\n");
        break;
    case SIMILAR:
        strcat(line, ",SIMILAR,75\n");
        break;
    case EXCELLENT:
        strcat(line, ",EXCELLENT,100\n");
        break;
    default:
        close(resultFd);
        return;
    }

    write(resultFd, line, strlen(line));
    close(resultFd);
}

CheckStatus checkCFile(char* path, ConfigData* configData){
    pid_t sonPID;
    int sonStatus;
    int sonExitStatus;

//compiling the .c file

    if((sonPID = fork()) == -1) { //fork failed
        return -1;
    } else if (sonPID == 0){ //we are the son process
        int errorsFD;
        if((errorsFD = open(ERROR_FILE_PATH, O_WRONLY | O_APPEND)) < 0){
            exit(EXIT_WITH_ERROR);
        }
        dup2(errorsFD, STDERROR_FD);
        close(errorsFD);

        char* args[] = {"gcc", "-o", NAME_COMPILED_TO_CHECK, path, NULL};
        execvp(args[0], args); //do the command.

        //if we got here exec failed:
        exit(EXIT_WITH_ERROR);
    }

    waitpid(sonPID, &sonStatus, 0);
    if (WIFEXITED(sonStatus)) {
        sonExitStatus = WEXITSTATUS(sonStatus);

        // gcc returns 1 or 4 if got compile errors.
        if((sonExitStatus == 1) || (sonExitStatus == 4)){
            return COMPILATION_ERROR;
        } else if(sonExitStatus < 0) {
            return -1;
        }
    }

//Running the .c file
    if((sonPID = fork()) == -1) { //fork failed
        return -1;
    } else if (sonPID == 0){ //we are the son process
        int errorsFD;
        if((errorsFD = open(ERROR_FILE_PATH, O_WRONLY | O_APPEND)) < 0){
            exit(EXIT_WITH_ERROR);
        }
        dup2(errorsFD, STDERROR_FD);
        close(errorsFD);

        int inFD;
        if((inFD = open(configData->inFilePath, O_RDONLY)) < 0){
            exit(EXIT_WITH_ERROR);
        }
        dup2(inFD, STDIN_FD);
        close(inFD);

        int outFD;
        if((outFD = open(CHECKED_OUTPUT_FILE_PATH, O_WRONLY | O_TRUNC)) < 0){
            exit(EXIT_WITH_ERROR);
        }
        dup2(outFD, STDOUT_FD);
        close(outFD);

        char* args[] = {"timeout", MAX_RUNNING_TIME, RUN_COMPILED_TO_CHECK, NULL};
        execvp(args[0], args); //do the command.

        //if we got here exec failed:
        exit(EXIT_WITH_ERROR);
    }

    waitpid(sonPID, &sonStatus, 0);
    if(WIFEXITED(sonStatus) && (WEXITSTATUS(sonStatus) == TIMEOUT_EXIT_VAL)){
        return TIMEOUT;
    }
    
    if((sonPID = fork()) == -1) { //fork failed
        return -1;
    } else if (sonPID == 0){ //we are the son process
        int errorsFD;
        if((errorsFD = open(ERROR_FILE_PATH, O_WRONLY | O_APPEND)) < 0){
            exit(EXIT_WITH_ERROR);
        }
        dup2(errorsFD, STDERROR_FD);
        close(errorsFD);

        char* args[] = {RUN_COMPILED_CHECK_BY,
         CHECKED_OUTPUT_FILE_PATH, configData->correctOutputFilePath, NULL};
        execvp(args[0], args); //do the command.

        //if we got here exec failed:
        exit(EXIT_WITH_ERROR);
    }

    waitpid(sonPID, &sonStatus, 0);
    if(WIFEXITED(sonStatus)){
        switch(WEXITSTATUS(sonStatus)) {
            case DIFFRENT_FILES:
                return WRONG;
            case SIMILLAR_FILES:
                return SIMILAR;
            case IDENTICAL_FILES:
                return EXCELLENT;
            default:
                break;
        }
    }

    return -1;
}

CheckStatus checkStudentDirectory(char* path, ConfigData* configData){
    DIR* studenDir;
    if((studenDir = opendir(path)) == NULL){
        return -1;
    }

    struct dirent *pDirent;
    Bool isCFileFound = FALSE;
    while(((pDirent = readdir(studenDir)) != NULL) && !isCFileFound){
        if((strlen(pDirent->d_name) > 2)
         && (strcmp(pDirent->d_name + strlen(pDirent->d_name) - 2, ".c") == 0)) {
            strcat(path, "/");
            strcat(path, pDirent->d_name);
            isCFileFound = TRUE;
        }
    }
    closedir(studenDir);

    if(!isCFileFound) {
        return NO_C_FILE;
    }

    return checkCFile(path, configData);
}

int checkStudents(ConfigData* configData){
    struct dirent *pDirent; 

    while ((pDirent = readdir(configData->mainDirFd)) != NULL){
        if((strcmp(pDirent->d_name, "..") == 0)
         || (strcmp(pDirent->d_name, ".") == 0)) {
            continue;
        }

        char path[MAX_SIZE_CONFIG_DATA_LINE + 1];
        strcpy(path, configData->mainDirPath);
        if(path[strlen(path) - 1] != '/'){
            strcat(path, "/");
        }
        strcat(path, pDirent->d_name);

        if(isDirectory(path)){
            gardeStudent(checkStudentDirectory(path, configData), pDirent->d_name);
        } 
    }

    return 0;
}

int main(int argc, char const *argv[])
{
    --argc;
    ConfigData configData;
    if((argc != NUM_OF_ARGS) || (!setConfigData(argv[1], &configData))){
        return -1;
    }

    int resultFd;
    if((resultFd = open(RESULT_FILE_PATH, O_CREAT | O_TRUNC, 0644)) < 0){
        return -1;
    }
    close(resultFd);

    int errorsFD;
    if((errorsFD = open(ERROR_FILE_PATH, O_CREAT | O_TRUNC, 0644)) < 0){
        return -1;
    }
    close(errorsFD);

    int outFD;
    if((errorsFD = open(CHECKED_OUTPUT_FILE_PATH, O_CREAT | O_TRUNC, 0644)) < 0){
        return -1;
    }
    close(outFD);

    int status = checkStudents(&configData);

    closedir(configData.mainDirFd);
    remove(NAME_COMPILED_TO_CHECK);
    remove(CHECKED_OUTPUT_FILE_PATH);

    return status;
}
