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
//150 chars for 3 rows + \n for each
#define CONFIG_FILE_MAX_DATA 453 
#define MAX_SIZE_CONFIG_DATA_LINE 150
//150 chars max name + 22 chars max status (",10,COMPILATION_ERROR\n")
#define MAX_SIZE_LINE_IN_RESULT_FILE 172
#define STDIN_FD 0
#define STDOUT_FD 1
#define STDERROR_FD 2
#define ERROR -1
#define SUCCESS 0 

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

/**
 * @brief prints the string.
 * 
 * @param str the string to print
 */
void myPrint(const char *str){
    if(write(STDOUT_FD, str, strlen(str)) < 0){};
}

/**
 * @brief checks if file is a directory.
 * 
 * @param path the file path.
 * @return int true if is dir, else false.
 */
int isDirectory(const char *path) {
   struct stat statbuf;
   if (stat(path, &statbuf) < 0){
       myPrint("Error in: stat\n");
       exit(ERROR);
   }

   return S_ISDIR(statbuf.st_mode);
}

/**
 * @brief grades the student by the status got.
 * 
 * @param status the check status.
 * @param name the name of the student (dir);
 * @return int 0 if no errors, else -1.
 */
int gardeStudent(CheckStatus status, char* name) {
    //getting the line to write
    char line[MAX_SIZE_LINE_IN_RESULT_FILE + 1];
    strcpy(line, name);
    switch (status) // checking witch status to specify in line.
    {
    case NO_C_FILE:
        strcat(line, ",0,NO_C_FILE\n");
        break;
    case COMPILATION_ERROR:
        strcat(line, ",10,COMPILATION_ERROR\n");
        break;
    case TIMEOUT:
        strcat(line, ",20,TIMEOUT\n");
        break;
    case WRONG:
        strcat(line, ",50,WRONG\n");
        break;
    case SIMILAR:
        strcat(line, ",75,SIMILAR\n");
        break;
    case EXCELLENT:
        strcat(line, ",100,EXCELLENT\n");
        break;
    default: // if status says error in this program occurred.
        return ERROR;
    }

    // opening the results file.
    int resultFd;
    if((resultFd = open(RESULT_FILE_PATH, O_WRONLY | O_APPEND)) < 0){
        myPrint("Error in: open\n");
        return ERROR;
    }

    // writing the line
    if(write(resultFd, line, strlen(line)) < 0) {};

    // closint the results file.
    if(close(resultFd) < 0){
        myPrint("Error in: close\n");
        return ERROR;
    }

    return SUCCESS;
}

/**
 * @brief checks a c file.
 * 
 * @param path the c file path.
 * @param configData the config data.
 * @return CheckStatus the status of checking, returning -1 if errors occurred.
 */
CheckStatus checkCFile(char* path, ConfigData* configData){
    // help vars:
    pid_t sonPID;
    int sonStatus;
    int sonExitStatus;

//compiling the .c file

    if((sonPID = fork()) < 0) { //fork failed
        myPrint("Error in: fork\n");
        return ERROR;
    } else if (sonPID == 0){ //we are the son process
        //sets the errors fd.
        int errorsFD;
        if((errorsFD = open(ERROR_FILE_PATH, O_WRONLY | O_APPEND)) < 0){
            myPrint("Error in: open\n");
            exit(EXIT_WITH_ERROR);
        }
        if(dup2(errorsFD, STDERROR_FD) < 0) {
            myPrint("Error in: dup2\n");
            exit(EXIT_WITH_ERROR);
        }
        if(close(errorsFD) < 0){
            myPrint("Error in: close\n");
            exit(EXIT_WITH_ERROR);
        }

        //compiling the c file
        char* args[] = {"gcc", "-o", NAME_COMPILED_TO_CHECK, path, NULL};
        execvp(args[0], args); 

        //if we got here exec failed:
        myPrint("Error in: execvp\n");
        exit(EXIT_WITH_ERROR);
    }

    //wait for end of compile.
    if(waitpid(sonPID, &sonStatus, 0) < 0) {
        myPrint("Error in: waitpid\n");
        return ERROR;
    }
    if (WIFEXITED(sonStatus)) {//if there is a return value.
        sonExitStatus = WEXITSTATUS(sonStatus);

        // gcc returns 1 or 4 if got compile errors.
        if((sonExitStatus == 1) || (sonExitStatus == 4)){
            return COMPILATION_ERROR;
        } else if(sonExitStatus == EXIT_WITH_ERROR) {
            return ERROR;
        }
    }

//Running the .c file
    if((sonPID = fork()) < 0) { //fork failed
        myPrint("Error in: fork\n");
        return ERROR;
    } else if (sonPID == 0){ //we are the son process
        //sets the errors fd.
        int errorsFD;
        if((errorsFD = open(ERROR_FILE_PATH, O_WRONLY | O_APPEND)) < 0){
            myPrint("Error in: open\n");
            exit(EXIT_WITH_ERROR);
        }
        if(dup2(errorsFD, STDERROR_FD) < 0) {
            myPrint("Error in: dup2\n");
            exit(EXIT_WITH_ERROR);
        }
        if(close(errorsFD) < 0){
            myPrint("Error in: close\n");
            exit(EXIT_WITH_ERROR);
        }

        //sets the in fd.
        int inFD;
        if((inFD = open(configData->inFilePath, O_RDONLY)) < 0){
            myPrint("Error in: open\n");
            exit(EXIT_WITH_ERROR);
        }
        if(dup2(inFD, STDIN_FD) < 0) {
            myPrint("Error in: dup2\n");
            exit(EXIT_WITH_ERROR);
        }
        if(close(inFD) < 0){
            myPrint("Error in: close\n");
            exit(EXIT_WITH_ERROR);
        }

        //sets the out fd.
        int outFD;
        if((outFD = open(CHECKED_OUTPUT_FILE_PATH, O_WRONLY | O_TRUNC)) < 0){
            myPrint("Error in: open\n");
            exit(EXIT_WITH_ERROR);
        }
        if(dup2(outFD, STDOUT_FD) < 0) {
            myPrint("Error in: dup2\n");
            exit(EXIT_WITH_ERROR);
        }
        if(close(outFD) < 0){
            myPrint("Error in: close\n");
            exit(EXIT_WITH_ERROR);
        }

        // runing the program with timeout.
        char* args[] = {"timeout", MAX_RUNNING_TIME, RUN_COMPILED_TO_CHECK, NULL};
        execvp(args[0], args); //do the command.

        //if we got here exec failed:
        myPrint("Error in: execvp\n");
        exit(EXIT_WITH_ERROR);
    }

    //wait for end of runing.
    if(waitpid(sonPID, &sonStatus, 0) < 0) {
        myPrint("Error in: waitpid\n");
        return ERROR;
    }

    //checks if timeout occurred.
    if(WIFEXITED(sonStatus) && (WEXITSTATUS(sonStatus) == TIMEOUT_EXIT_VAL)){
        return TIMEOUT;
    }

//checking output
    if((sonPID = fork()) < 0) { //fork failed
        myPrint("Error in: fork\n");
        return ERROR;
    } else if (sonPID == 0){ //we are the son process
        //sets the errors fd.
        int errorsFD;
        if((errorsFD = open(ERROR_FILE_PATH, O_WRONLY | O_APPEND)) < 0){
            myPrint("Error in: open\n");
            exit(EXIT_WITH_ERROR);
        }
        if(dup2(errorsFD, STDERROR_FD) < 0) {
            myPrint("Error in: dup2\n");
            exit(EXIT_WITH_ERROR);
        }
        if(close(errorsFD) < 0){
            myPrint("Error in: close\n");
            exit(EXIT_WITH_ERROR);
        }

        //checks the output
        char* args[] = {RUN_COMPILED_CHECK_BY,
         CHECKED_OUTPUT_FILE_PATH, configData->correctOutputFilePath, NULL};
        execvp(args[0], args); //do the command.

        //if we got here exec failed:
        myPrint("Error in: execvp\n");
        exit(EXIT_WITH_ERROR);
    }

    //wait for end of checking.
    if(waitpid(sonPID, &sonStatus, 0) < 0) {
        myPrint("Error in: waitpid\n");
        return ERROR;
    }

    //check the status of checking
    if(WIFEXITED(sonStatus)){
        switch(WEXITSTATUS(sonStatus)) {
            case DIFFRENT_FILES:
                return WRONG;
            case SIMILLAR_FILES:
                return SIMILAR;
            case IDENTICAL_FILES:
                return EXCELLENT;
            default:
                return ERROR;
        }
    }

    return ERROR;
}

/**
 * @brief checks students directory.
 * 
 * @param path the student dir path
 * @param configData the config data.
 * @return CheckStatus the status of the check,
 * if errors (in this program) -1; 
 */
CheckStatus checkStudentDirectory(char* path, ConfigData* configData){
    // opens the students dir.
    DIR* studenDir;
    if((studenDir = opendir(path)) == NULL){
        myPrint("Error in: opendir\n");
        return ERROR;
    }

    char copyPath[MAX_SIZE_CONFIG_DATA_LINE + 1];
    strcpy(copyPath, path);

    // iterating the files in the students dir.
    struct dirent *pDirent;
    Bool isCFileFound = FALSE;
    while(((pDirent = readdir(studenDir)) != NULL) && !isCFileFound){
        if(pDirent < 0) {
            myPrint("Error in: readdir\n");
            return ERROR;
        }

        // setting the path of the c file
        strcat(path, "/");
        strcat(path, pDirent->d_name);

        // checking if is c file
        if( !isDirectory(path) && ((strlen(pDirent->d_name) > 2)
         && (strcmp(pDirent->d_name + strlen(pDirent->d_name) - 2, ".c") == 0))) {
            isCFileFound = TRUE;
        } else{
            strcpy(path, copyPath);
        }
    }
    if(closedir(studenDir) < 0){
        myPrint("Error in: closedir\n");
        return ERROR;
    }

    if(!isCFileFound) {
        return NO_C_FILE;
    }

    return checkCFile(path, configData);
}

/**
 * @brief checks all students directories.
 * 
 * @param configData the config data.
 * @return int 0 if no errors, else -1.
 */
int checkStudents(ConfigData* configData){
    struct dirent *pDirent; 

    // goes over the main dir0
    while ((pDirent = readdir(configData->mainDirFd)) != NULL){
        if(pDirent < 0){
            myPrint("Error in: readdir\n");
            return ERROR;
        }

        // we should ignore special names.
        if((strcmp(pDirent->d_name, "..") == 0)
         || (strcmp(pDirent->d_name, ".") == 0)) {
            continue;
        }

        // gets the path of the file iterate.
        char path[MAX_SIZE_CONFIG_DATA_LINE + 1];
        strcpy(path, configData->mainDirPath);
        if(path[strlen(path) - 1] != '/'){
            strcat(path, "/");
        }
        strcat(path, pDirent->d_name);

        //checks if the path is directory.
        if(isDirectory(path)){
            // grade the student directory.
            if(gardeStudent(checkStudentDirectory(path, configData), pDirent->d_name) == ERROR){
                return ERROR;
            }
        } 
    }

    return SUCCESS;
}

/**
 * @brief Sets the Config Data, and checks it.
 * 
 * @param configFilePath the config file path.
 * @param configData the config data to set.
 * @return int 0 if no errors, else -1.
 */
int setConfigData(char const *configFilePath, ConfigData* configData){
    // opens the config file
    int configFd;
    if((configFd = open(configFilePath, O_RDONLY)) < 0) {
        myPrint("Error in: open\n");
        return ERROR;
    }

    // reads the config file
    char buffer[CONFIG_FILE_MAX_DATA + 1]; //+ 1 for \0
    int charRead;
    if((charRead = read(configFd, buffer ,CONFIG_FILE_MAX_DATA)) < 0){
        myPrint("Error in: read\n");
        close(configFd);
        return ERROR;
    }

    // sets the end of the contant.
    buffer[charRead] = '\0';

    if(close(configFd) < 0) {
        myPrint("Error in: close\n");
        return ERROR;
    }

    // sets the data from the lines that were read.
    strcpy(configData->mainDirPath, strtok(buffer, "\n"));
    strcpy(configData->inFilePath, strtok(NULL, "\n"));
    strcpy(configData->correctOutputFilePath, strtok(NULL, "\n"));

    //checks if main directory exists and if it is directory.
    if((access(configData->mainDirPath, F_OK) < 0) || (!isDirectory(configData->mainDirPath))){
        myPrint("Not a valid directory\n");
        return ERROR;
    }

    // checks if input file exists
    if(access(configData->inFilePath, F_OK) < 0) {
        myPrint("Input file not exist\n");
        return ERROR;
    }

    // checks if output file exists
    if(access(configData->correctOutputFilePath, F_OK) < 0) {
        myPrint("Output file not exist\n");
        return ERROR;
    }

    //opens the main dir.
    if((configData->mainDirFd = opendir(configData->mainDirPath)) == NULL){
        myPrint("Error in: opendir\n");
        return ERROR;
    }

    return SUCCESS;
}

/**
 * @brief prepering for the start of the program.
 * 
 * @return int 0 if no errors, else -1.
 */
int preperSettings() {
    // creating and restarting the results.csv file.
    int resultFd;
    if((resultFd = open(RESULT_FILE_PATH, O_CREAT | O_TRUNC, 0644)) < 0){
        myPrint("Error in: open\n");
        return ERROR;
    }
    if(close(resultFd) < 0){
        myPrint("Error in: close\n");
        return ERROR;
    }

    // creating and restarting the errors.csv file.
    int errorsFD;
    if((errorsFD = open(ERROR_FILE_PATH, O_CREAT | O_TRUNC, 0644)) < 0){
        myPrint("Error in: open\n");
        return ERROR;
    }
    if(close(errorsFD) < 0){
        myPrint("Error in: close\n");
        return ERROR;
    }
    
    // creating and restarting the output file for checking file.
    int outFD;
    if((outFD = open(CHECKED_OUTPUT_FILE_PATH, O_CREAT | O_TRUNC, 0644)) < 0){
        myPrint("Error in: open\n");
        return ERROR;
    }
    if(close(outFD) < 0) {
        myPrint("Error in: close\n");
        return ERROR;
    }

    return SUCCESS;
}

/**
 * @brief prepering for the end of the program.
 * 
 * @param configData the config data.
 * @return int 0 if no errors, else -1.
 */
int preperToEnd(ConfigData* configData){

    // closing the main dir fd.
    if(closedir(configData->mainDirFd) < 0){
        myPrint("Error in: closedir\n");
        return ERROR;
    }

    // removing the output file for checking.
    if(remove(CHECKED_OUTPUT_FILE_PATH) < 0){
        myPrint("Error in: remove\n");
        return ERROR;
    }

    // removing the compiled file for checking.
    // (error can be if no c files in dir)
    if(remove(NAME_COMPILED_TO_CHECK) < 0 ){}


    return SUCCESS;
}

/**
 * @brief these program checks code of students.
 * 
 * @param argc num of args.
 * @param argv must include the config file.
 * @return int 0 if no errors, else -1.
 */
int main(int argc, char const *argv[])
{
    // the first arg shouldn't be noticed.
    --argc;
    
    // will store the config data
    ConfigData configData;

    // checks if enough args, and validate start point,
    // updating configData;
    if((argc != NUM_OF_ARGS) ||
    (setConfigData(argv[1], &configData) == ERROR) || 
    (preperSettings() == ERROR)){
        exit(ERROR);
    }

    // checks the students.
    int status = checkStudents(&configData);

    // prepering for ending the program.
    if((preperToEnd(&configData) == ERROR) || (status == ERROR)){
        exit(ERROR);
    }

    return SUCCESS;
}
