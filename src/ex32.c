// Nadav Sharabi 213056153
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>

#define NUM_OF_ARGS 1
#define OUTPUT_FILE_PATH "output_file_for_checking.txt"
#define RESULT_FILE_PATH "results.csv"
#define ERROR_FILE_PATH "errors.txt"
#define CONFIG_FILE_MAX_DATA 453 //150 chars for 3 rows + \n for each
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
    char* correctOutputFilePath;
    char* inFilePath;
}ConfigData;

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

    char* mainDirPath = strtok(buffer, "\n");
    configData->inFilePath = strtok(NULL, "\n");
    configData->correctOutputFilePath = strtok(NULL, "\n");

    if((configData->mainDirFd = opendir(mainDirPath)) == NULL){
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

int checkStudents(){
    
    return 0;
}

int main(int argc, char const *argv[])
{
    --argc;
    ConfigData configData;
    if((argc != NUM_OF_ARGS) || (!setConfigData(argv[1], &configData))){
        return -1;
    }

    int status = checkStudents(&configData);

    closedir(configData.mainDirFd);

    return status;
}
