
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <dirent.h>
#include <direct.h>
#include <sys/stat.h>
#include <unistd.h>
#include <windows.h>
#include <time.h>

#define MAX_CONFIG_LINE 1024
#define MAX_PATH_LENGTH 1024
#define MAX_LINE_LENGTH 1024
#define MAX_HISTORY 10
#define MAX_LOG_ENTRY_SIZE 2048
#define BRANCHES_DIR ".zengit/branches"
#define CURRENT_BRANCH_FILE ".zengit/CurrentBranch"
#define INDEX_FILE ".zengit/index"
#define COMMIT_DIR ".zengit/commits"
#define LOG_FILE_PATH ".zengit/logs"
#define TAGS_DIR ".zengit/tags"
#define MAX_TAG_INFO_SIZE 1024
#define _GNU_SOURCE
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_RESET   "\x1b[0m"

typedef struct LogEntry {
    char date[30];
    char user[50];
    char commitID[41];
    char branch[50];
    char message[256];
    int filesCommitted;
} LogEntry;


typedef struct {
    int lastN;
    char branchName[100];
    char authorName[100];
    char sinceDate[20];
    char beforeDate[20];
    char searchWord[100];
} LogOptions;

bool fileExists(const char *filename) {
    struct stat buffer;
    return (stat(filename, &buffer) == 0);
}

void ensureDirectoryExists(const char* path) {
    struct stat st;
    if (stat(path, &st) == -1) {
#ifdef _WIN32
        _mkdir(path);
#else
        mkdir(path, 0700);
#endif
    }
}

bool updateConfigFile(const char* filePath, const char* key, const char* value) {
    if (!fileExists(filePath)) {
        FILE *file = fopen(filePath, "w");
        if (file == NULL) {
            perror("Error creating config file");
            return false;
        }
        fclose(file);
    }

    char tempFilePath[1024];
    snprintf(tempFilePath, sizeof(tempFilePath), "%s.tmp", filePath);

    FILE* origFile = fopen(filePath, "r");
    if (origFile == NULL) {
        perror("Error opening original file");
        return false;
    }

    FILE* tempFile = fopen(tempFilePath, "w");
    if (tempFile == NULL) {
        perror("Error opening temp file");
        fclose(origFile);
        return false;
    }

    bool keyFound = false;
    char line[MAX_CONFIG_LINE];
    char currentKey[MAX_CONFIG_LINE];
    while (fgets(line, MAX_CONFIG_LINE, origFile) != NULL) {
        sscanf(line, "%1023[^=]", currentKey);
        if (strcmp(currentKey, key) == 0) {
            fprintf(tempFile, "%s=%s\n", key, value);
            keyFound = true;
        } else {
            fputs(line, tempFile);
        }
    }

    if (!keyFound) {
        fprintf(tempFile, "%s=%s\n", key, value);
    }

    fclose(origFile);
    fclose(tempFile);

    remove(filePath);
    rename(tempFilePath, filePath);

    return true;
}

bool getAliasCommand(const char* filePath, const char* aliasKey, char* command, size_t maxLen) {
    FILE* file = fopen(filePath, "r");
    if (file == NULL) {
        return false;
    }

    char line[MAX_CONFIG_LINE];
    char currentKey[MAX_CONFIG_LINE];
    while (fgets(line, MAX_CONFIG_LINE, file) != NULL) {
        sscanf(line, "%1023[^=]", currentKey);
        if (strcmp(currentKey, aliasKey) == 0) {
            char* foundCommand = strchr(line, '=');
            if (foundCommand != NULL && strlen(++foundCommand) < maxLen) {
                strcpy(command, foundCommand);
                command[strcspn(command, "\n")] = 0;
                fclose(file);
                return true;
            }
            break;
        }
    }
    fclose(file);
    return false;
}

bool processAlias(int *argc, char ***argv, const char *configPath) {

    if (strncmp((*argv)[1], "alias.", 6) != 0) {
        return true;
    }

    char aliasKey[MAX_CONFIG_LINE];
    snprintf(aliasKey, sizeof(aliasKey), "%s", (*argv)[1]);

    char command[MAX_CONFIG_LINE];
    if (getAliasCommand(configPath, aliasKey, command, sizeof(command))) {

        int newArgc = 1;
        char *newArgv[MAX_CONFIG_LINE];
        newArgv[0] = (*argv)[0];

        char *token = strtok(command, " ");
        while (token != NULL) {
            newArgv[newArgc++] = strdup(token);
            token = strtok(NULL, " ");
        }





        *argv = realloc(*argv, newArgc * sizeof(char*));
        memcpy(*argv, newArgv, newArgc * sizeof(char*));

        *argc = newArgc;
        return true;
    }


    return true;
}


bool isAliasCommandValid(const char* value) {

    if (value == NULL || strlen(value) == 0) {
        return false;
    }




    return true;
}

bool handleConfigCommand(int argc, char *argv[]) {
    if ((argc != 4 && !(argc == 5 && strcmp(argv[2], "-global") == 0)) ||
        (argc == 5 && strncmp(argv[2], "alias.", 6) != 0 && strcmp(argv[2], "-global") != 0)) {
        fprintf(stderr, "Usage: %s config [-global] key \"value\"\n", argv[0]);
        fprintf(stderr, "       %s config [-global] alias.<alias-name> \"command\"\n", argv[0]);
        return false;
    }

    const char* globalConfigPath = "C:/Users/parham/.zengitconfig";
    const char* localConfigPath = "./.zengitconfig";

    bool isGlobal = argc == 5 && strcmp(argv[2], "-global") == 0;
    const char* key = argv[2 + (isGlobal ? 1 : 0)];
    const char* value = argv[3 + (isGlobal ? 1 : 0)];


    if (strncmp(key, "alias.", 6) == 0 && !isAliasCommandValid(value)) {
        fprintf(stderr, "Invalid alias command.\n");
        return false;
    }

    const char* configPath = isGlobal ? globalConfigPath : localConfigPath;

    if (updateConfigFile(configPath, key, value)) {
        printf("Configuration updated successfully.\n");
        return true;
    } else {
        printf("Failed to update configuration.\n");
        return false;
    }
}

bool isRepositoryExistInCurrentOrParentDir(const char* dir) {
    char path[MAX_PATH_LENGTH];
    strcpy(path, dir);

    while (strcmp(path, "C:\\") != 0 && strcmp(path, "\\") != 0) {
        char zengitPath[MAX_PATH_LENGTH];
        snprintf(zengitPath, sizeof(zengitPath), "%s\\.zengit", path);

        if (fileExists(zengitPath)) {
            return true;
        }


        char* lastBackslash = strrchr(path, '\\');
        if (lastBackslash != NULL) {
            *lastBackslash = '\0';
        } else {

            break;
        }
    }

    return false;
}

bool initializeRepository(const char* repoPath) {
    int status;


    const char* directories[] = {
            "commits",
            "branches",
            "objects",
            "tags",

    };


    status = _mkdir(repoPath);
    if (status != 0) {
        perror("Failed to create .zengit directory");
        return false;
    }


    DWORD attributes = GetFileAttributesA(repoPath);
    if (attributes == INVALID_FILE_ATTRIBUTES || !SetFileAttributesA(repoPath, attributes | FILE_ATTRIBUTE_HIDDEN)) {
        perror("Failed to set .zengit directory as hidden");
        return false;
    }


    for (size_t i = 0; i < sizeof(directories) / sizeof(directories[0]); i++) {
        char dirPath[MAX_PATH_LENGTH];
        snprintf(dirPath, sizeof(dirPath), "%s/%s", repoPath, directories[i]);
        status = _mkdir(dirPath);
        if (status != 0 && errno != EEXIST) {
            perror("Failed to create directory within .zengit");
            return false;
        }
    }


    const char* files[] = {
            "shortcuts",
            "logs",
            "CurrentBranch",

    };

    for (size_t i = 0; i < sizeof(files) / sizeof(files[0]); i++) {
        char filePath[MAX_PATH_LENGTH];
        snprintf(filePath, sizeof(filePath), "%s/%s", repoPath, files[i]);
        FILE* file = fopen(filePath, "w");
        if (!file) {
            perror("Failed to create essential file within .zengit");
            return false;
        }
        fclose(file);
    }

    return true;
}

bool handleInitCommand() {
    const char* currentDirectory = ".";
    const char* zengitDirectory = "./.zengit";

    if (isRepositoryExistInCurrentOrParentDir(currentDirectory)) {
        fprintf(stderr, "zengit repository already exists.\n");
        return false;
    }

    return initializeRepository(zengitDirectory);
}

bool isFile(const char* path) {
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
}

bool isDirectory(const char* path) {
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISDIR(path_stat.st_mode);
}

bool isFileStaged(const char* path);
bool isDirectoryStaged(const char* dirPath);

bool isFileStaged(const char* path) {
    FILE *file = fopen(".zengit/index", "r");
    if (!file) {
        return false;
    }

    char line[MAX_PATH_LENGTH];
    bool found = false;
    while (fgets(line, sizeof(line), file) != NULL) {
        line[strcspn(line, "\n")] = 0;

        if (strcmp(line, path) == 0) {
            found = true;
            break;
        }
    }

    fclose(file);


    if (!found) {
        struct stat pathStat;
        if (stat(path, &pathStat) == 0 && S_ISDIR(pathStat.st_mode)) {
            found = isDirectoryStaged(path);
        }
    }

    return found;
}

bool isDirectoryStaged(const char* dirPath) {
    DIR* dir = opendir(dirPath);
    if (!dir) {
        return false;
    }

    struct dirent* entry;
    bool isStaged = true;

    while ((entry = readdir(dir)) != NULL) {

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        char filePath[MAX_PATH_LENGTH];
        snprintf(filePath, sizeof(filePath), "%s/%s", dirPath, entry->d_name);

        struct stat pathStat;
        if (stat(filePath, &pathStat) != 0) {

            isStaged = false;
            break;
        }


        if (S_ISREG(pathStat.st_mode)) {
            if (!isFileStaged(filePath)) {
                isStaged = false;
                break;
            }
        } else if (S_ISDIR(pathStat.st_mode)) {

            isStaged &= isDirectoryStaged(filePath);
            if (!isStaged) break;
        }
    }

    closedir(dir);
    return isStaged;
}

void logStageAction(const char* action, const char* path) {
    FILE* file = fopen(".zengit/stage_history.log", "a");
    if (!file) {
        fprintf(stderr, "Failed to open stage history log.\n");
        return;
    }
    fprintf(file, "%s %s\n", action, path);
    fclose(file);
}


void addToStage(const char* path) {
    if (isFileStaged(path)) {
        printf("File '%s' is already staged.\n", path);
        return;
    }

    FILE* file = fopen(".zengit/index", "a");
    if (file != NULL) {
        fprintf(file, "%s\n", path);
        fclose(file);
        logStageAction("ADD", path);
    } else {
        perror("Error opening staging area file");
    }
}

void stageDirectory(const char* dirPath) {
    DIR* dir = opendir(dirPath);
    if (dir == NULL) {
        perror("Failed to open directory");
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        char fullPath[MAX_PATH_LENGTH];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", dirPath, entry->d_name);

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        if (isFile(fullPath)) {
            addToStage(fullPath);
        } else if (isDirectory(fullPath)) {
            stageDirectory(fullPath);
        }
    }
    closedir(dir);
}

void listDirectoryContents(const char* basePath, int depth, int currentLevel) {
    if (depth < currentLevel) return;

    DIR* dir;
    struct dirent* entry;

    if (!(dir = opendir(basePath))) return;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", basePath, entry->d_name);


        char* formattedPath = path;
        if (strncmp(formattedPath, "./", 2) == 0) {
            formattedPath += 2;
        }

        struct stat statbuf;
        if (stat(formattedPath, &statbuf) == -1) continue;


        if (S_ISREG(statbuf.st_mode) || (S_ISDIR(statbuf.st_mode) && currentLevel == depth)) {
            printf("%s - %s\n", formattedPath, isFileStaged(formattedPath) ? "Staged" : "Not Staged");
        }


        if (S_ISDIR(statbuf.st_mode) && currentLevel < depth) {
            listDirectoryContents(formattedPath, depth, currentLevel + 1);
        }
    }
    closedir(dir);
}

bool logUnstagedFile(const char* path) {
    FILE* logFile = fopen(".zengit/unstage_log", "a");
    if (!logFile) {
        fprintf(stderr, "Failed to open unstage log file.\n");
        return false;
    }
    fprintf(logFile, "%s\n", path);
    fclose(logFile);
    return true;
}

bool removeFromStage(const char* path) {
    FILE* stageFile = fopen(".zengit/index", "r");
    if (!stageFile) {
        fprintf(stderr, "Error opening staging area.\n");
        return false;
    }

    bool found = false;
    char line[1024];
    FILE* tempFile = fopen(".zengit/index_tmp", "w");
    while (fgets(line, sizeof(line), stageFile) != NULL) {
        line[strcspn(line, "\n")] = 0;
        if (strcmp(line, path) != 0) {
            fprintf(tempFile, "%s\n", line);
        } else {
            found = true;

            if (!logUnstagedFile(path)) {

                fclose(stageFile);
                fclose(tempFile);
                return false;
            }
        }
    }

    fclose(stageFile);
    fclose(tempFile);

    if (found) {
        remove(".zengit/index");
        rename(".zengit/index_tmp", ".zengit/index");
        logStageAction("REMOVE", path);
    } else {
        remove(".zengit/index_tmp");
        fprintf(stderr, "File %s not found in staging area.\n", path);
    }
    return found;
}

void removeDirectoryFromStage(const char* dirPath) {
    DIR* dir = opendir(dirPath);
    if (!dir) {
        fprintf(stderr, "Error opening directory: %s\n", dirPath);
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        char fullPath[1024];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", dirPath, entry->d_name);

        struct stat pathStat;
        stat(fullPath, &pathStat);
        if (S_ISDIR(pathStat.st_mode)) {

            removeDirectoryFromStage(fullPath);
        } else {

            removeFromStage(fullPath);
        }
    }

    closedir(dir);
}

bool stageRedo() {

    FILE* logFile = fopen(".zengit/unstage_log", "r");
    if (!logFile) {
        fprintf(stderr, "No unstaged files to redo or log file missing.\n");
        return false;
    }

    char path[1024];
    while (fgets(path, sizeof(path), logFile)) {
        path[strcspn(path, "\n")] = 0;
        addToStage(path);
    }

    fclose(logFile);


    logFile = fopen(".zengit/unstage_log", "w");
    if (logFile) {
        fclose(logFile);
    } else {
        fprintf(stderr, "Failed to clear the unstage log.\n");
        return false;
    }
    return true;
}

void processResetPath(const char* path) {
    if (isFile(path)) {
        removeFromStage(path);
    } else if (isDirectory(path)) {
        removeDirectoryFromStage(path);
    } else {
        fprintf(stderr, "Error: The specified file or directory does not exist: %s\n", path);
    }
}

void unstageFiles(char* files) {

    char* file = strtok(files, " ");
    while (file != NULL) {
        removeFromStage(file);
        file = strtok(NULL, " ");
    }
}


void updateStagingHistory(char history[][MAX_LINE_LENGTH], int count) {
    FILE* file = fopen(".zengit/stage_history.log", "w");
    if (!file) {
        fprintf(stderr, "Error opening staging history for update.\n");
        return;
    }
    for (int i = 0; i < count; i++) {
        fprintf(file, "%s\n", history[i]);
    }
    fclose(file);
}

void stageUndo() {
    char history[MAX_HISTORY][MAX_PATH_LENGTH];
    char actions[MAX_HISTORY][10];
    int count = 0;


    FILE* file = fopen(".zengit/stage_history.log", "r");
    if (!file) {
        fprintf(stderr, "Error opening staging history.\n");
        return;
    }


    while (fscanf(file, "%s %s\n", actions[count], history[count]) == 2) {
        if (++count >= MAX_HISTORY) break;
    }
    fclose(file);

    if (count == 0) {
        printf("No staging actions to undo.\n");
        return;
    }


    int lastAddIndex = -1;
    for (int i = count - 1; i >= 0; i--) {
        if (strcmp(actions[i], "ADD") == 0) {
            lastAddIndex = i;
            break;
        }
    }

    if (lastAddIndex == -1) {
        printf("No ADD actions to undo.\n");
        return;
    }


    for (int i = lastAddIndex; i < count; i++) {
        if (strcmp(actions[i], "ADD") == 0) {
            removeFromStage(history[i]);
        }
    }


    file = fopen(".zengit/stage_history.log", "w");
    if (!file) {
        fprintf(stderr, "Failed to open stage history log for writing.\n");
        return;
    }

    for (int i = 0; i < lastAddIndex; i++) {
        fprintf(file, "%s %s\n", actions[i], history[i]);
    }
    fclose(file);

    printf("Last ADD action undone.\n");
}

bool handleAddCommand(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s add [-f | -n <depth>] <file or directory path> [...]\n", argv[0]);
        return false;
    }

    if (!isRepositoryExistInCurrentOrParentDir(".")) {
        fprintf(stderr, "No zengit repository found in this directory or any parent directories.\n");
        return false;
    }

    bool success = true;

    if (strcmp(argv[2], "-f") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Usage: %s add -f <file1> <file2> <dir1> [...]\n", argv[0]);
            return false;
        }

        for (int i = 3; i < argc; i++) {
            const char* path = argv[i];
            if (isFile(path) || isDirectory(path)) {
                addToStage(path);
            } else {
                fprintf(stderr, "Warning: The specified path does not exist, but -f was used: %s\n", path);
                success = false;
            }
        }
    }  else if (strcmp(argv[2], "-redo") == 0) {
        success = stageRedo();
    } else if (strcmp(argv[2], "-n") == 0) {
        if (argc != 4) {
            fprintf(stderr, "Usage: %s add -n <depth>\n", argv[0]);
            return false;
        }
        int depth = atoi(argv[3]);
        if (depth < 1) {
            fprintf(stderr, "Error: Depth must be greater than 0.\n");
            return false;
        }

        listDirectoryContents(".", depth, 1);
    } else {

        for (int i = 2; i < argc; i++) {
            const char* path = argv[i];
            if (isFile(path)) {
                addToStage(path);
            } else if (isDirectory(path)) {
                stageDirectory(path);
            }
            else {
                fprintf(stderr, "Error: The specified file or directory does not exist: %s\n", path);
                success = false;
            }
        }
    }
    return success;
}

bool handleResetCommand(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s reset [-f] <file or directory path> [...]\n", argv[0]);
        return false;
    }

    bool forceReset = false;
    int startIndex = 2;

    if (strcmp(argv[2], "-undo") == 0) {
        stageUndo();
        return true;
    } else if (strcmp(argv[2], "-f") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Usage: %s reset -f <file1> <file2> <dir1> [...]\n", argv[0]);
            return false;
        }
        forceReset = true;
        startIndex = 3;
    }

    for (int i = startIndex; i < argc; i++) {
        const char* path = argv[i];
        if (forceReset || isFile(path) || isDirectory(path)) {
            processResetPath(path);
        } else {
            fprintf(stderr, "Warning: The specified path does not exist, but -f was used: %s\n", path);
        }
    }

    return true;
}

bool getUserNameFromConfig(char* userName, size_t bufferSize, const char* configPath) {
    FILE* file = fopen(configPath, "r");
    if (!file) {
        return false;
    }

    char line[1024];
    bool found = false;
    while (fgets(line, sizeof(line), file) != NULL) {
        if (strncmp(line, "user.name=", 10) == 0) {
            strncpy(userName, line + 10, bufferSize - 1);
            userName[bufferSize - 1] = '\0';
            char* newline = strchr(userName, '\n');
            if (newline) *newline = '\0';
            found = true;
            break;
        }
    }

    fclose(file);
    return found;
}

char* getCurrentBranch() {
    static char currentBranch[256] = "master";
    FILE *file = fopen(CURRENT_BRANCH_FILE, "r");
    if (file) {
        if (fgets(currentBranch, sizeof(currentBranch), file) == NULL) {

            strcpy(currentBranch, "master");
        }
        fclose(file);
        currentBranch[strcspn(currentBranch, "\n")] = '\0';
    }
    return currentBranch;
}

void generateCommitID(char *commitID, size_t size) {
    srand((unsigned int)time(NULL));
    const char *chars = "abcdefghijklmnopqrstuvwxyz0123456789";
    for (size_t i = 0; i < size - 1; ++i) {
        commitID[i] = chars[rand() % (sizeof(chars) - 1)];
    }
    commitID[size - 1] = '\0';
}

void copyFile(const char* srcPath, const char* destPath);
void copyDirectoryRecursively(const char* srcDirPath, const char* destDirPath);

void copyStagedFilesToCommitDir(const char* commitDir, const char* indexPath) {
    FILE* index = fopen(indexPath, "r");
    if (!index) {
        perror("Failed to open index file");
        return;
    }

    char stagedFilePath[MAX_PATH_LENGTH];
    while (fgets(stagedFilePath, sizeof(stagedFilePath), index)) {
        stagedFilePath[strcspn(stagedFilePath, "\n")] = 0;

        char destPath[MAX_PATH_LENGTH];
        snprintf(destPath, sizeof(destPath), "%s/%s", commitDir, stagedFilePath);

        if (isDirectory(stagedFilePath)) {

            copyDirectoryRecursively(stagedFilePath, destPath);
        } else {

            copyFile(stagedFilePath, destPath);
        }
    }

    fclose(index);
}

void ensureDirectoryStructureExists(const char* path) {
    char tempPath[MAX_PATH_LENGTH];
    strcpy(tempPath, path);

    for (char* p = tempPath + 1; *p; p++) {
        if (*p == '/' || *p == '\\') {
            *p = '\0';
            mkdir(tempPath);
            *p = '/';
        }
    }

    mkdir(tempPath);
}

void copyFile(const char* srcPath, const char* destPath) {
    FILE* src = fopen(srcPath, "rb");
    if (!src) {
        perror("Failed to open source file for copying");
        return;
    }


    char* lastSlash = strrchr(destPath, '/');
    if (lastSlash) {
        *lastSlash = '\0';
        ensureDirectoryStructureExists(destPath);
        *lastSlash = '/';
    }

    FILE* dest = fopen(destPath, "wb");
    if (!dest) {
        fprintf(stderr, "Failed to open destination file for copying: %s\n", destPath);
        fclose(src);
        return;
    }

    char buffer[4096];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        fwrite(buffer, 1, bytesRead, dest);
    }

    fclose(src);
    fclose(dest);
}


void copyDirectoryRecursively(const char* srcDirPath, const char* destDirPath) {
    DIR* dir = opendir(srcDirPath);
    if (!dir) {
        perror("Failed to open source directory for copying");
        return;
    }

    ensureDirectoryStructureExists(destDirPath);

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        char srcPath[MAX_PATH_LENGTH];
        snprintf(srcPath, sizeof(srcPath), "%s/%s", srcDirPath, entry->d_name);

        char destPath[MAX_PATH_LENGTH];
        snprintf(destPath, sizeof(destPath), "%s/%s", destDirPath, entry->d_name);

        if (isDirectory(srcPath)) {
            copyDirectoryRecursively(srcPath, destPath);
        } else {
            copyFile(srcPath, destPath);
        }
    }

    closedir(dir);
}

void clearIndexFile(const char* indexPath) {

    FILE* index = fopen(indexPath, "w");
    if (!index) {
        perror("Failed to clear index file");
        return;
    }

    fclose(index);
}

int countFilesInCommitDir(const char* dirPath) {
    int count = 0;
    DIR* dir = opendir(dirPath);
    if (dir == NULL) {
        return 0;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char path[MAX_PATH_LENGTH];
            snprintf(path, MAX_PATH_LENGTH, "%s/%s", dirPath, entry->d_name);

            if (isDirectory(path)) {
                count += countFilesInCommitDir(path);
            } else {
                ++count;
            }
        }
    }

    closedir(dir);
    return count;
}

bool commitChanges(const char* message) {
    if (!fileExists(INDEX_FILE)) {
        printf("No files staged for commit.\n");
        return false;
    }

    char commitID[41];
    generateCommitID(commitID, sizeof(commitID));


    char commitDirPath[MAX_PATH_LENGTH];
    snprintf(commitDirPath, sizeof(commitDirPath), "%s/%s", COMMIT_DIR, commitID);
    ensureDirectoryExists(commitDirPath);
    copyStagedFilesToCommitDir(commitDirPath, INDEX_FILE);

    int filesCommitted = countFilesInCommitDir(commitDirPath);

    char userName[256];
    const char* globalConfigPath = "C:/Users/parham/.zengitconfig";
    const char* localConfigPath = "./.zengitconfig";
    if (!getUserNameFromConfig(userName, sizeof(userName), localConfigPath) &&
        !getUserNameFromConfig(userName, sizeof(userName), globalConfigPath)) {
        strcpy(userName, "Unknown");
    }


    char currentBranch[256] = {0};
    strncpy(currentBranch, getCurrentBranch(), sizeof(currentBranch) - 1);


    char logFilePath[MAX_PATH_LENGTH];
    snprintf(logFilePath, sizeof(logFilePath), ".zengit/logs");
    FILE* logFile = fopen(logFilePath, "a");
    if (logFile) {
        time_t now = time(NULL);
        fprintf(logFile, "Date: %sUser: %s\nCommit ID: %s\nBranch: %s\nMessage: %s\nFiles Committed: %d\n\n",
                ctime(&now), userName, commitID, currentBranch, message, filesCommitted);
        fclose(logFile);
    } else {
        perror("Failed to write to log file");
    }

    char branchHeadFilePath[MAX_PATH_LENGTH];
    snprintf(branchHeadFilePath, sizeof(branchHeadFilePath), "%s/%s_HEAD", COMMIT_DIR, currentBranch);
    FILE* branchHeadFile = fopen(branchHeadFilePath, "a");
    if (branchHeadFile) {
        fprintf(branchHeadFile, "%s\n", commitID);
        fclose(branchHeadFile);
    } else {
        perror("Failed to update branch HEAD");
        return false;
    }

    clearIndexFile(INDEX_FILE);
    printf("Commit successful [ID: %s]. Index cleared.\n", commitID);
    return true;
}

void setShortcut(const char* shortcutName, const char* message) {
    printf("Setting shortcut: %s with message: %s\n", shortcutName, message);
    FILE* file = fopen(".zengit/shortcuts", "a");
    if (file == NULL) {
        perror("Error opening shortcuts file");
        return;
    }

    fprintf(file, "%s=%s\n", shortcutName, message);
    fclose(file);

    printf("Shortcut '%s' set for message '%s'\n", shortcutName, message);
}

void createCommitWithShortcut(const char* shortcutName) {
    char message[1024] = {0};


    FILE* file = fopen(".zengit/shortcuts", "r");
    if (file == NULL) {
        fprintf(stderr, "Error: Shortcut '%s' not found.\n", shortcutName);
        return;
    }

    char line[1024];
    char currentShortcut[1024];
    bool shortcutFound = false;
    while (fgets(line, sizeof(line), file) != NULL) {
        sscanf(line, "%[^=]=%[^\n]", currentShortcut, message);
        if (strcmp(currentShortcut, shortcutName) == 0) {
            shortcutFound = true;
            break;
        }
    }
    fclose(file);

    if (!shortcutFound) {
        fprintf(stderr, "Error: Shortcut '%s' not found.\n", shortcutName);
        return;
    }


    commitChanges(message);
}

void replaceShortcutMessage(const char* shortcutName, const char* newMessage) {
    FILE* file = fopen(".zengit/shortcuts", "r");
    if (file == NULL) {
        perror("Error opening shortcuts file");
        return;
    }


    char lines[100][1024];
    int count = 0;
    bool found = false;
    while (fgets(lines[count], sizeof(lines[count]), file) != NULL) {
        char currentShortcut[1024];
        sscanf(lines[count], "%[^=]", currentShortcut);
        if (strcmp(currentShortcut, shortcutName) == 0) {
            snprintf(lines[count], sizeof(lines[count]), "%s=%s\n", shortcutName, newMessage);
            found = true;
        }
        count++;
    }
    fclose(file);

    if (!found) {
        fprintf(stderr, "Error: Shortcut '%s' not found.\n", shortcutName);
        return;
    }


    file = fopen(".zengit/shortcuts", "w");
    if (file == NULL) {
        perror("Error writing to shortcuts file");
        return;
    }

    for (int i = 0; i < count; i++) {
        fputs(lines[i], file);
    }

    fclose(file);
    printf("Shortcut '%s' message replaced successfully.\n", shortcutName);
}

void removeShortcut(const char* shortcutName) {
    FILE* file = fopen(".zengit/shortcuts", "r");
    if (file == NULL) {
        perror("Error opening shortcuts file");
        return;
    }

    char lines[100][1024];
    int count = 0;
    bool found = false;
    while (fgets(lines[count], sizeof(lines[count]), file) != NULL) {
        char currentShortcut[1024];
        sscanf(lines[count], "%[^=]", currentShortcut);
        if (strcmp(currentShortcut, shortcutName) == 0) {
            found = true;
            continue;
        }
        count++;
    }
    fclose(file);

    if (!found) {
        fprintf(stderr, "Error: Shortcut '%s' not found.\n", shortcutName);
        return;
    }


    file = fopen(".zengit/shortcuts", "w");
    if (file == NULL) {
        perror("Error writing to shortcuts file");
        return;
    }

    for (int i = 0; i < count; i++) {
        fputs(lines[i], file);
    }

    fclose(file);
    printf("Shortcut '%s' removed successfully.\n", shortcutName);
}


char* getLastCommitId(const char* branchName) {
    static char lastCommitId[MAX_PATH_LENGTH];
    char headFilePath[MAX_PATH_LENGTH];
    snprintf(headFilePath, sizeof(headFilePath), "%s/%s_HEAD", COMMIT_DIR, branchName);

    FILE *file = fopen(headFilePath, "r");
    if (file) {
        if (fgets(lastCommitId, sizeof(lastCommitId), file) != NULL) {
            fclose(file);
            lastCommitId[strcspn(lastCommitId, "\n")] = 0;
            return lastCommitId;
        }
        fclose(file);
    }
    return NULL;
}

void updateCurrentBranch(const char* branchName) {
    FILE *file = fopen(CURRENT_BRANCH_FILE, "w");
    if (file) {
        fprintf(file, "%s", branchName);
        fclose(file);
    } else {
        perror("Failed to update current branch");
    }
}

void createBranch(const char* branchName) {
    char branchHeadFilePath[MAX_PATH_LENGTH];
    snprintf(branchHeadFilePath, sizeof(branchHeadFilePath), "%s/%s_HEAD", COMMIT_DIR, branchName);


    if (fileExists(branchHeadFilePath)) {
        printf("Error: Branch '%s' already exists.\n", branchName);
        return;
    }

    char* currentCommitId = getLastCommitId(getCurrentBranch());
    if (!currentCommitId) {
        printf("Error: Could not retrieve current commit ID.\n");
        return;
    }

    FILE* branchHeadFile = fopen(branchHeadFilePath, "w");
    if (branchHeadFile) {
        fprintf(branchHeadFile, "%s", currentCommitId);
        fclose(branchHeadFile);
        printf("Branch '%s' created from the latest commit.\n", branchName);


        updateCurrentBranch(branchName);
    } else {
        perror("Failed to create branch HEAD file");
    }
}

void listBranches() {
    DIR* dir = opendir(COMMIT_DIR);
    if (!dir) {
        perror("Failed to open commits directory");
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, "_HEAD") != NULL) {
            char branchName[MAX_PATH_LENGTH];
            strncpy(branchName, entry->d_name, strlen(entry->d_name) - 5);
            branchName[strlen(entry->d_name) - 5] = '\0';
            printf("%s\n", branchName);
        }
    }

    closedir(dir);
}

void switchBranch(const char* branchName) {
    char branchHeadFilePath[MAX_PATH_LENGTH];
    snprintf(branchHeadFilePath, sizeof(branchHeadFilePath), "%s/%s_HEAD", COMMIT_DIR, branchName);


    if (fileExists(branchHeadFilePath)) {

        FILE* currentBranchFile = fopen(CURRENT_BRANCH_FILE, "w");
        if (currentBranchFile) {
            fprintf(currentBranchFile, "%s", branchName);
            fclose(currentBranchFile);




            printf("Switched to branch '%s'.\n", branchName);
        } else {
            perror("Failed to update CURRENT_BRANCH file");
        }
    } else {
        printf("Branch '%s' does not exist.\n", branchName);
    }
}

void normalizePath(char* path) {


    char* p = path;
    while (*p) {
        if (*p == '\\') *p = '/';
        ++p;
    }

    if (strncmp(path, "./", 2) == 0) {
        memmove(path, path + 2, strlen(path) - 1);
    }
}

bool FileStaged(const char* path) {
    char normalizedPath[MAX_PATH_LENGTH];
    strcpy(normalizedPath, path);
    normalizePath(normalizedPath);

    return isFileStaged(normalizedPath);
}

bool filesAreDifferent(const char *file1Path, const char *file2Path) {
    FILE *fp1 = fopen(file1Path, "rb");
    FILE *fp2 = fopen(file2Path, "rb");
    if (!fp1 || !fp2) {
        if (fp1) fclose(fp1);
        if (fp2) fclose(fp2);
        return true;
    }

    bool areDifferent = false;
    int ch1, ch2;
    do {
        ch1 = fgetc(fp1);
        ch2 = fgetc(fp2);
        if (ch1 != ch2) {
            areDifferent = true;
            break;
        }
    } while (ch1 != EOF && ch2 != EOF);


    if (!areDifferent && (ch1 != EOF || ch2 != EOF)) {
        areDifferent = true;
    }

    fclose(fp1);
    fclose(fp2);
    return areDifferent;
}

bool areFileAttributesDifferent(const char *file1, const char *file2) {
    DWORD attributesFile1 = GetFileAttributesA(file1);
    DWORD attributesFile2 = GetFileAttributesA(file2);


    if (attributesFile1 == INVALID_FILE_ATTRIBUTES || attributesFile2 == INVALID_FILE_ATTRIBUTES) {
        fprintf(stderr, "Error retrieving file attributes.\n");
        return false;
    }


    bool readOnlyFile1 = attributesFile1 & FILE_ATTRIBUTE_READONLY;
    bool readOnlyFile2 = attributesFile2 & FILE_ATTRIBUTE_READONLY;

    return readOnlyFile1 != readOnlyFile2;
}

void processFileStatus(const char* filePath, const char* commitDir) {
    char normalizedPath[MAX_PATH_LENGTH];
    strcpy(normalizedPath, filePath);
    normalizePath(normalizedPath);

    char commitFilePath[MAX_PATH_LENGTH];
    snprintf(commitFilePath, sizeof(commitFilePath), "%s/%s", commitDir, normalizedPath);

    if (!fileExists(filePath) && fileExists(commitFilePath)) {

        printf("%s %cD\n", normalizedPath, FileStaged(normalizedPath) ? '+' : '-');
    } else if (fileExists(filePath) && !fileExists(commitFilePath)) {

        printf("%s %cA\n", normalizedPath, FileStaged(normalizedPath) ? '+' : '-');
    } else if (fileExists(filePath) && fileExists(commitFilePath)) {
        if (filesAreDifferent(filePath, commitFilePath)) {

            printf("%s %cM\n", normalizedPath, FileStaged(normalizedPath) ? '+' : '-');
        } else if (areFileAttributesDifferent(filePath, commitFilePath)) {

            printf("%s %cT\n", normalizedPath, FileStaged(normalizedPath) ? '+' : '-');
        }
    }
}


void processDeletedFiles(const char* commitDir, const char* dirPath) {
    DIR* commitDirD = opendir(commitDir);
    if (!commitDirD) {
        fprintf(stderr, "Error opening commit directory '%s'\n", commitDir);
        return;
    }

    struct dirent* commitEntry;
    while ((commitEntry = readdir(commitDirD)) != NULL) {
        if (strcmp(commitEntry->d_name, ".") == 0 || strcmp(commitEntry->d_name, "..") == 0) continue;

        char commitFilePath[MAX_PATH_LENGTH];
        snprintf(commitFilePath, sizeof(commitFilePath), "%s/%s", commitDir, commitEntry->d_name);
        normalizePath(commitFilePath);


        char relativePath[MAX_PATH_LENGTH];
        strcpy(relativePath, commitEntry->d_name);
        normalizePath(relativePath);

        char currentFilePath[MAX_PATH_LENGTH];
        snprintf(currentFilePath, sizeof(currentFilePath), "%s/%s", dirPath, relativePath);

        if (!fileExists(currentFilePath) && fileExists(commitFilePath)) {

            printf("%s %cD\n", relativePath, FileStaged(relativePath) ? '+' : '-');
        }
    }

    closedir(commitDirD);
}

void processDirectoryForStatus(const char* dirPath, const char* commitDir) {
    DIR* dir = opendir(dirPath);
    if (!dir) {
        fprintf(stderr, "Error opening directory '%s'\n", dirPath);
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 || strcmp(entry->d_name, ".zengit") == 0) continue;

        char fullPath[MAX_PATH_LENGTH];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", dirPath, entry->d_name);
        normalizePath(fullPath);


        processFileStatus(fullPath, commitDir);
    }

    closedir(dir);


    processDeletedFiles(commitDir, dirPath);
}

void handleStatusCommand() {
    char* currentBranch = getCurrentBranch();
    char* lastCommitId = getLastCommitId(currentBranch);
    if (lastCommitId) {
        char commitFolderPath[MAX_PATH_LENGTH];
        snprintf(commitFolderPath, sizeof(commitFolderPath), "%s/%s", COMMIT_DIR, lastCommitId);
        printf("Checking status against last commit ID: %s\n", lastCommitId);
        processDirectoryForStatus(".", commitFolderPath);
    } else {
        fprintf(stderr, "Error: Could not find last commit ID for branch '%s'.\n", currentBranch);
    }
}

int readLogEntries(LogEntry** entries) {
    FILE* file = fopen(".zengit/logs", "r");
    if (!file) {
        perror("Failed to open log file");
        return 0;
    }


    int capacity = 10, count = 0;
    *entries = malloc(capacity * sizeof(LogEntry));
    if (!*entries) {
        fclose(file);
        return 0;
    }


    LogEntry temp;
    while (fscanf(file, "Date: %[^\n]\nUser: %[^\n]\nCommit ID: %[^\n]\nBranch: %[^\n]\nMessage: %[^\n]\nFiles Committed: %d\n\n",
                  temp.date, temp.user, temp.commitID, temp.branch, temp.message, &temp.filesCommitted) == 6) {
        if (count == capacity) {
            capacity *= 2;
            *entries = realloc(*entries, capacity * sizeof(LogEntry));
            if (!*entries) {
                fclose(file);
                return 0;
            }
        }
        (*entries)[count++] = temp;
    }

    fclose(file);
    return count;
}

void displayLogEntry(const LogEntry* entry) {
    printf("Date: %s\nUser: %s\nCommit ID: %s\nBranch: %s\nMessage: %s\nFiles Committed: %d\n\n",
           entry->date, entry->user, entry->commitID, entry->branch, entry->message, entry->filesCommitted);
}

void displayLastNCommits(const LogEntry* entries, int count, int n) {
    printf("Last %d commits:\n", n > count ? count : n);
    int start = count - n < 0 ? 0 : count - n;
    for (int i = count - 1; i >= start; --i) {
        displayLogEntry(&entries[i]);
    }
}


void displayCommitsFromBranch(const LogEntry* entries, int count, const char* branchName) {
    printf("Commits from branch '%s':\n", branchName);
    int found = 0;
    for (int i = count - 1; i >= 0; --i) {
        if (strcmp(entries[i].branch, branchName) == 0) {
            displayLogEntry(&entries[i]);
            found = 1;
        }
    }
    if (!found) {
        printf("No commits found for branch '%s'.\n", branchName);
    }
}

void displayCommitsByAuthor(const LogEntry* entries, int count, const char* author) {
    printf("Commits by author: %s\n", author);
    int found = 0;
    for (int i = count - 1; i >= 0; --i) {
        if (strcmp(entries[i].user, author) == 0) {
            displayLogEntry(&entries[i]);
            found = 1;
        }
    }
    if (!found) {
        printf("No commits found by author %s.\n", author);
    }
}

time_t dateStringToTimeT(const char* dateString) {
    struct tm tm = {0};
    int year, month, day;


    sscanf(dateString, "%d-%d-%d", &year, &month, &day);


    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;


    tm.tm_hour = 0;
    tm.tm_min = 0;
    tm.tm_sec = 0;
    tm.tm_isdst = -1;


    return mktime(&tm);
}

void displayCommitsBefore(const LogEntry* entries, int count, const char* date) {
    time_t cutoffDate = dateStringToTimeT(date);
    printf("Commits before: %s\n", date);
    int found = 0;
    for (int i = count - 1; i >= 0; --i) {
        time_t commitDate = dateStringToTimeT(entries[i].date);
        if (commitDate < cutoffDate) {
            displayLogEntry(&entries[i]);
            found = 1;
        }
    }
    if (!found) {
        printf("No commits found before %s.\n", date);
    }
}

void displayCommitsSince(const LogEntry* entries, int count, const char* date) {
    time_t cutoffDate = dateStringToTimeT(date);
    printf("Commits since: %s\n", date);
    int found = 0;
    for (int i = count - 1; i >= 0; --i) {
        time_t commitDate = dateStringToTimeT(entries[i].date);
        if (commitDate >= cutoffDate) {
            displayLogEntry(&entries[i]);
            found = 1;
        }
    }
    if (!found) {
        printf("No commits found since %s.\n", date);
    }
}

int match(const char *pattern, const char *str) {
    const char *p = pattern, *s = str;
    const char *star = NULL, *ss = s;

    while (*s) {
        if (*p == '?' || *p == *s) { ++s; ++p; continue; }
        if (*p == '*') { star = p++; ss = s; continue; }
        if (star) { p = star + 1; s = ++ss; continue; }
        return 0;
    }
    while (*p == '*') { ++p; }
    return !*p;
}

void printLogEntryStartingAtCurrentLine(FILE* logFile) {
    char line[1024];
    int entryFound = 0;


    while (fgets(line, sizeof(line), logFile) != NULL) {

        if (strcmp(line, "\n") == 0) {
            if (entryFound) {

                break;
            }
        } else {

            entryFound = 1;

            printf("%s", line);
        }
    }
}

void zengitLogSearch(const char *searchString) {
    FILE *logFile = fopen(".zengit/logs", "r");
    if (!logFile) {
        printf("Log file not found.\n");
        return;
    }

    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), logFile)) {

        if (strstr(buffer, "Message: ")) {
            char *message = buffer + strlen("Message: ");
            message[strcspn(message, "\n")] = '\0';


            char *token = strtok(searchString, " ");
            while (token) {
                if (match(token, message)) {

                    printLogEntryStartingAtCurrentLine(logFile);
                    break;
                }
                token = strtok(NULL, " ");
            }
        }
    }

    fclose(logFile);
}

bool branchExists(const char* branchName) {
    char branchHeadFilePath[MAX_PATH_LENGTH];
    snprintf(branchHeadFilePath, sizeof(branchHeadFilePath), "%s/%s_HEAD", COMMIT_DIR, branchName);

    struct stat buffer;
    return (stat(branchHeadFilePath, &buffer) == 0);
}

bool checkDirectoryForUncommittedChanges(const char* dirPath, const char* commitDirPath) {
    DIR* dir = opendir(dirPath);
    if (!dir) {
        fprintf(stderr, "Error opening directory: %s\n", dirPath);
        return false;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        char filePath[MAX_PATH_LENGTH];
        snprintf(filePath, sizeof(filePath), "%s/%s", dirPath, entry->d_name);

        char commitFilePath[MAX_PATH_LENGTH];
        snprintf(commitFilePath, sizeof(commitFilePath), "%s/%s", commitDirPath, entry->d_name);


        if (fileExists(filePath) && fileExists(commitFilePath) && filesAreDifferent(filePath, commitFilePath)) {
            closedir(dir);
            return true;
        }
    }

    closedir(dir);
    return false;
}

void deletedirectorycontents(char *dir_path) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        fprintf(stderr, "Failed to open directory for deletion: %s\n", dir_path);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 || strcmp(entry->d_name, ".neogit") == 0) {
            continue;
        }

        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", dir_path, entry->d_name);

        struct stat path_stat;
        stat(path, &path_stat);
        if (S_ISDIR(path_stat.st_mode)) {

            deletedirectorycontents(path);
            rmdir(path);
        } else {
            remove(path);
        }
    }

    closedir(dir);
}

void copydirectoryrecursively(char *src_dir, char *dest_dir) {
    DIR *dir = opendir(src_dir);
    if (!dir) {
        fprintf(stderr, "Failed to open directory: %s\n", src_dir);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 || strcmp(entry->d_name, ".neogit") == 0) {

            continue;
        }

        char src_path[1024], dest_path[1024];
        snprintf(src_path, sizeof(src_path), "%s/%s", src_dir, entry->d_name);
        snprintf(dest_path, sizeof(dest_path), "%s/%s", dest_dir, entry->d_name);

        struct stat path_stat;
        stat(src_path, &path_stat);

        if (S_ISDIR(path_stat.st_mode)) {
            mkdir(dest_path);
            copydirectoryrecursively(src_path, dest_path);
        } else {

            FILE *src_file = fopen(src_path, "rb"), *dest_file = fopen(dest_path, "wb");
            if (src_file && dest_file) {
                char buffer[1024];
                size_t bytes;
                while ((bytes = fread(buffer, 1, sizeof(buffer), src_file)) > 0) {
                    fwrite(buffer, 1, bytes, dest_file);
                }
                fclose(src_file);
                fclose(dest_file);
            } else {
                if (src_file) fclose(src_file);
                if (dest_file) fclose(dest_file);
            }
        }
    }

    closedir(dir);
}



void copyFromLastCommit(const char* commitID) {
    char srcDir[MAX_PATH_LENGTH];
    snprintf(srcDir, sizeof(srcDir), ".zengit/commits/%s", commitID);
    char destDir[] = ".";


    deletedirectorycontents(destDir);
    copydirectoryrecursively(srcDir, destDir);
}

void clearWorkingDirectoryExceptZengit() {
    char workingDir[] = ".";
    deletedirectorycontents(workingDir);
}

void updateCurrentBranchName(const char* branchName) {
    char currentBranchPath[1024];
    snprintf(currentBranchPath, sizeof(currentBranchPath), ".zengit/CurrentBranch");
    FILE* file = fopen(currentBranchPath, "w");
    if (!file) {
        fprintf(stderr, "Error updating current branch name.\n");
        return;
    }
    fprintf(file, "%s", branchName);
    fclose(file);
}

void updateBranchHeadWithCommit(const char* branchName, const char* commitID) {
    char branchHeadFilePath[1024];
    snprintf(branchHeadFilePath, sizeof(branchHeadFilePath), ".zengit/commits/%s_HEAD", branchName);

    FILE* file = fopen(branchHeadFilePath, "a");
    if (!file) {
        fprintf(stderr, "Error opening %s_HEAD for update.\n", branchName);
        return;
    }
    fprintf(file, "%s\n", commitID);


    fclose(file);
}

bool getLastCommitIDForBranch(const char* branchName, char* lastCommitID, size_t size) {
    char headFilePath[1024];
    snprintf(headFilePath, sizeof(headFilePath), ".zengit/commits/%s_HEAD", branchName);
    FILE* file = fopen(headFilePath, "r");
    if (!file) {
        fprintf(stderr, "Failed to open HEAD file for branch %s.\n", branchName);
        return false;
    }

    char line[256];
    while (fgets(line, sizeof(line), file) != NULL) {

    }

    size_t len = strlen(line);
    if (len > 0 && line[len - 1] == '\n') {
        line[len - 1] = '\0';
    }
    strncpy(lastCommitID, line, size);
    fclose(file);

    return true;
}

bool getCurrentBranchName(char* branchName, size_t size) {
    FILE* file = fopen(CURRENT_BRANCH_FILE, "r");
    if (!file) {
        fprintf(stderr, "Error: Unable to open current branch file.\n");
        return false;
    }

    if (!fgets(branchName, size, file)) {
        fprintf(stderr, "Error: Unable to read current branch name.\n");
        fclose(file);
        return false;
    }


    branchName[strcspn(branchName, "\n")] = '\0';
    fclose(file);
    return true;
}

bool getCurrentCommit(char* commitID, size_t size) {
    char branchName[MAX_PATH_LENGTH];
    if (!getCurrentBranchName(branchName, sizeof(branchName))) {
        return false;
    }

    char headFilePath[MAX_PATH_LENGTH];
    snprintf(headFilePath, sizeof(headFilePath), "%s/%s_HEAD", COMMIT_DIR, branchName);

    FILE* file = fopen(headFilePath, "r");
    if (!file) {
        fprintf(stderr, "Error: Unable to open HEAD file for branch %s.\n", branchName);
        return false;
    }

    if (!fgets(commitID, size, file)) {
        fprintf(stderr, "Error: Unable to read latest commit ID from HEAD file for branch %s.\n", branchName);
        fclose(file);
        return false;
    }


    commitID[strcspn(commitID, "\n")] = '\0';
    fclose(file);
    return true;
}

void checkoutToBranch(const char* branchName) {
    char lastCommitID[256];
    if (!getLastCommitIDForBranch(branchName, lastCommitID, sizeof(lastCommitID))) {
        fprintf(stderr, "No commits found for branch '%s'.\n", branchName);
        return;
    }

    deletedirectorycontents(".");

    char commitFolderPath[1024];
    snprintf(commitFolderPath, sizeof(commitFolderPath), ".zengit/commits/%s", lastCommitID);
    copydirectoryrecursively(commitFolderPath, ".");


    updateCurrentBranchName(branchName);

    printf("Checked out to the latest commit %s in branch %s.\n", lastCommitID, branchName);
}

bool commitExists(const char* commitID) {
    char commitDirPath[MAX_PATH_LENGTH];
    snprintf(commitDirPath, sizeof(commitDirPath), ".zengit/commits/%s", commitID);
    struct stat buffer;
    return (stat(commitDirPath, &buffer) == 0 && S_ISDIR(buffer.st_mode));
}

void checkoutToCommit(const char* commitID) {
    char currentCommitDir[MAX_PATH_LENGTH];


    if (getCurrentCommit(commitID, sizeof(commitID))) {

        snprintf(currentCommitDir, sizeof(currentCommitDir), ".zengit/commits/%s", commitID);
        printf("Current commit directory: %s\n", currentCommitDir);
    } else {
        fprintf(stderr, "Failed to get current commit ID.\n");
    }



    if (checkDirectoryForUncommittedChanges(".", currentCommitDir)) {
        fprintf(stderr, "Error: Uncommitted changes present. Please commit or stash your changes before checking out.\n");
        return;
    }

    if (!commitExists(commitID)) {
        fprintf(stderr, "Commit %s does not exist.\n", commitID);
        return;
    }

    deletedirectorycontents(".");

    char commitFolderPath[MAX_PATH_LENGTH];
    snprintf(commitFolderPath, sizeof(commitFolderPath), ".zengit/commits/%s", commitID);
    copydirectoryrecursively(commitFolderPath, ".");


    printf("Checked out to commit %s.\n", commitID);
}

bool getLatestCommitID(const char* branchName, char* commitID, size_t size) {
    char filePath[1024];
    snprintf(filePath, sizeof(filePath), ".zengit/commits/%s_HEAD", branchName);

    FILE* file = fopen(filePath, "r");
    if (!file) {
        fprintf(stderr, "Error: Unable to open %s_HEAD.\n", branchName);
        return false;
    }

    if (fgets(commitID, size, file) == NULL) {
        fprintf(stderr, "Error: Unable to read latest commit ID from %s_HEAD.\n", branchName);
        fclose(file);
        return false;
    }


    commitID[strcspn(commitID, "\n")] = '\0';
    fclose(file);
    return true;
}


void checkout_head() {
    char currentBranch[256];
    if (!getCurrentBranchName(currentBranch, sizeof(currentBranch))) {
        fprintf(stderr, "Error: Unable to get current branch name.\n");
        return;
    }

    char latestCommitID[260];
    if (!getLatestCommitID(currentBranch, latestCommitID, sizeof(latestCommitID))) {

        return;
    }


    char commitDirPath[1024];
    snprintf(commitDirPath, sizeof(commitDirPath), ".zengit/commits/%s", latestCommitID);
    if (checkDirectoryForUncommittedChanges(".", commitDirPath)) {
        fprintf(stderr, "Error: There are uncommitted changes.\n");
        return;
    }

    checkoutToCommit(latestCommitID);
    printf("Checked out to the latest commit (HEAD): %s\n", latestCommitID);
}

bool isValidCommitID(const char* commitID);
bool isMergeCommit(const char* commitID);
char* getCommitMessage(const char* commitID);
void applyInverseChanges(const char* commitID);
void saveChangesWithoutCommit(void);
bool createNewCommit(const char* message, const char* commitID); // Adjusted prototype
void updateLog(const char* commitID, const char* message);
void updateHEAD(const char* commitID);
void generateCommitID(char* commitID, size_t size);
void copyDirectoryRecursively(const char* srcPath, const char* destPath);
bool fileExists(const char* path);
bool getLatestCommitID(const char* branchName, char* commitID, size_t size);
bool getCurrentBranchName(char* branchName, size_t size);
void checkoutToCommit(const char* commitID);
bool checkDirectoryForUncommittedChanges(const char* dirPath, const char* commitDirPath);
char* getTagFilePath(const char* tagName);
void writeTagInfo(const char* tagFilePath, const char* commitID, const char* taggerName, const char* message);
char* getCurrentCommitID(void) {
    // This function should return the commit ID of the current HEAD.
    // The implementation will depend on how you're tracking the HEAD in your system.
    static char currentCommitID[41] = "current_head_commit_id"; // Example ID
    return currentCommitID;
}

/* void revertChanges(const char* commitID, bool createCommit, const char* customMessage) {
    if (!isValidCommitID(commitID)) {
        fprintf(stderr, "Error: Invalid commit ID.\n");
        return;
    }

    applyInverseChanges(commitID); // Assuming this function applies the inverse of the changes made by commitID

    if (createCommit) {
        char message[256];
        if (customMessage != NULL) {
            strncpy(message, customMessage, sizeof(message));
        } else {
            char* commitMessage = getCommitMessage(commitID);
            if (commitMessage != NULL) {
                snprintf(message, sizeof(message), "Revert: %s", commitMessage);
            } else {
                strncpy(message, "Revert commit", sizeof(message));
            }
        }
        char newCommitID[41];
        generateCommitID(newCommitID, sizeof(newCommitID)); // Assuming this generates a new commit ID
        if (!createNewCommit(message, newCommitID)) {
            fprintf(stderr, "Error: Failed to create new commit.\n");
        }
    } else {
        saveChangesWithoutCommit(); // Assuming this function stages changes without committing
    }
}

bool createCommitDirectory(const char* path) {
    char tempPath[MAX_PATH_LENGTH];
    strcpy(tempPath, path);
    bool success = true;

    for (char* p = tempPath + 1; *p; p++) {
        if (*p == '/' || *p == '\\') {
            *p = '\0';
#ifdef _WIN32
            if (_mkdir(tempPath) != 0 && errno != EEXIST) success = false;
#else
            if (mkdir(tempPath, 0755) != 0 && errno != EEXIST) success = false;
#endif
            *p = '/';
        }
    }

#ifdef _WIN32
    if (_mkdir(tempPath) != 0 && errno != EEXIST) success = false;
#else
    if (mkdir(tempPath, 0755) != 0 && errno != EEXIST) success = false;
#endif
    return success;
}

bool createNewCommit(const char* message, const char* commitID) {
    char commitDirPath[1024];
    snprintf(commitDirPath, sizeof(commitDirPath), ".zengit/commits/%s", commitID);
    if (!createCommitDirectory(commitDirPath)) {
        fprintf(stderr, "Failed to create commit directory for commit %s\n", commitID);
        return false;
    }

    copyDirectoryRecursively(".", commitDirPath);

    char messageFilePath[1024];
    snprintf(messageFilePath, sizeof(messageFilePath), "%s/message", commitDirPath);
    FILE* messageFile = fopen(messageFilePath, "w");
    if (!messageFile) {
        fprintf(stderr, "Failed to write commit message for commit %s\n", commitID);
        return false;
    }
    fprintf(messageFile, "%s", message);
    fclose(messageFile);

    updateLog(commitID, message);
    updateHEAD(commitID);

    return true;
}

bool revertCommand(int argc, char* argv[]) {
    char commitID[41] = {0};
    char* customMessage = NULL;
    bool createCommit = true;  // Default to true, change to false if -n flag is used

    if (argc < 3) {
        fprintf(stderr, "Usage: zengit revert [-m <message>] <commit-id>\n");
        return false;
    }

    int i = 1;
    if (strcmp(argv[i], "-m") == 0) {
        if (argc < 5) {
            fprintf(stderr, "Error: Missing commit ID or message.\n");
            return false;
        }
        customMessage = argv[i + 1];
        strncpy(commitID, argv[i + 2], sizeof(commitID) - 1);
        commitID[sizeof(commitID) - 1] = '\0';  // Ensure null-termination
        i += 2;  // Skip past the message and commit ID arguments
    } else {
        strncpy(commitID, argv[i], sizeof(commitID) - 1);
        commitID[sizeof(commitID) - 1] = '\0';  // Ensure null-termination
        i++;  // Skip past the commit ID argument
    }

    // Check for -n flag in remaining arguments
    for (; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0) {
            createCommit = false;
        }
    }

    if (!isValidCommitID(commitID)) {
        fprintf(stderr, "Error: Invalid commit ID.\n");
        return false;
    }

    if (isMergeCommit(commitID)) {
        fprintf(stderr, "Error: Reverting merge commits is not supported.\n");
        return false;
    }

    revertChanges(commitID, createCommit, customMessage);  // Call with all required arguments

    if (createCommit) {
        char* message = customMessage ? customMessage : getCommitMessage(commitID);
        if (message && !createNewCommit(message, commitID)) {
            fprintf(stderr, "Error: Failed to create new commit.\n");
            return false;
        }
    }

    printf("Changes from commit %s have been reverted.\n", commitID);
    return true;
}

bool isValidCommitID(const char* commitID) {
    char commitPath[1024];
    snprintf(commitPath, sizeof(commitPath), ".zengit/commits/%s", commitID);
    struct stat statbuf;
    return (stat(commitPath, &statbuf) == 0 && S_ISDIR(statbuf.st_mode));
}

bool isMergeCommit(const char* commitID) {
    return false;
}


char* getCommitMessage(const char* commitID) {
    static char message[256];
    char logPath[1024];
    snprintf(logPath, sizeof(logPath), ".zengit/commits/%s/message", commitID);
    FILE* file = fopen(logPath, "r");
    if (file) {
        if (fgets(message, sizeof(message), file) != NULL) {
            fclose(file);
            message[strcspn(message, "\n")] = '\0';
            return message;
        }
        fclose(file);
    }
    return "No commit message found";
}

void updateLog(const char* commitID, const char* message) {
    char logFilePath[1024];
    snprintf(logFilePath, sizeof(logFilePath), ".zengit/logs");

    FILE* file = fopen(logFilePath, "a");
    if (!file) {
        perror("Failed to open log file for updating");
        return;
    }

    time_t now = time(NULL);
    char* dateString = ctime(&now);
    dateString[strlen(dateString) - 1] = '\0';

    char authorName[256] = "Author Name";

    fprintf(file, "Commit ID: %s\nAuthor: %s\nDate: %s\nMessage: %s\n\n",
            commitID, authorName, dateString, message);
    fclose(file);
}

void updateHEAD(const char* commitID) {
    char currentBranchName[256];
    FILE* currentBranchFile = fopen(".zengit/CurrentBranch", "r");
    if (!currentBranchFile) {
        perror("Failed to read CurrentBranch file");
        return;
    }
    fscanf(currentBranchFile, "%s", currentBranchName);
    fclose(currentBranchFile);

    char branchHeadFilePath[1024];
    snprintf(branchHeadFilePath, sizeof(branchHeadFilePath), ".zengit/%s_HEAD", currentBranchName);

    FILE* branchHeadFile = fopen(branchHeadFilePath, "a");
    if (!branchHeadFile) {
        perror("Failed to update branch HEAD file");
        return;
    }
    fprintf(branchHeadFile, "%s\n", commitID);
    fclose(branchHeadFile);
}


void handleRevertCommand(int argc, char* argv[]) {
    char commitID[41] = {0};
    bool createCommit = true;
    char* customMessage = NULL;

    // Parse arguments
    if (argc < 2) {
        fprintf(stderr, "Usage: zengit revert [-n | -m <message>] <commit-id | HEAD-X>\n");
        return;
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0) {
            createCommit = false;
        } else if (strcmp(argv[i], "-m") == 0) {
            if (i + 1 < argc) {
                customMessage = argv[++i];
            } else {
                fprintf(stderr, "Error: Message not specified for -m flag.\n");
                return;
            }
        } else if (strncmp(argv[i], "HEAD-", 5) == 0) {
            // Here you need to define the logic for convertHeadToCommitID or handle it accordingly.
            // For now, let's assume it's not implemented and return an error.
            fprintf(stderr, "Error: Handling of 'HEAD-X' is not implemented.\n");
            return;
        } else {
            strncpy(commitID, argv[i], sizeof(commitID) - 1);
            commitID[sizeof(commitID) - 1] = '\0';
        }
    }

    revertChanges(commitID, createCommit, customMessage);
}

bool tagCommand(int argc, char* argv[]) {
    char tagName[256] = {0};
    char message[1024] = {0};
    char commitID[41] = {0};
    bool force = false;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-a") == 0 && i + 1 < argc) {
            strncpy(tagName, argv[++i], sizeof(tagName) - 1);
        } else if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
            strncpy(message, argv[++i], sizeof(message) - 1);
        } else if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
            strncpy(commitID, argv[++i], sizeof(commitID) - 1);
        } else if (strcmp(argv[i], "-f") == 0) {
            force = true;
        }
    }

    if (tagName[0] == '\0') {
        fprintf(stderr, "Tag name is required.\n");
        return false;
    }

    if (commitID[0] == '\0') {
        strcpy(commitID, getCurrentCommitID());
        if (commitID[0] == '\0') {
            fprintf(stderr, "Failed to determine current commit ID.\n");
            return false;
        }
    }

    char* tagFilePath = getTagFilePath(tagName);
    if (fileExists(tagFilePath) && !force) {
        fprintf(stderr, "Tag '%s' already exists. Use -f to force overwrite.\n", tagName);
        free(tagFilePath);
        return false;
    }

    char taggerName[256] = "Default Tagger";
    writeTagInfo(tagFilePath, commitID, taggerName, message);
    free(tagFilePath);
    printf("Tag '%s' created for commit %s.\n", tagName, commitID);
    return true;
}




void updateTagFile(const char* tagName, const char* commitID, const char* message, bool force) {
    //.....
}

char* getTagFilePath(const char* tagName) {
    char* path = malloc(MAX_TAG_INFO_SIZE);
    if (path != NULL) {
        snprintf(path, MAX_TAG_INFO_SIZE, "%s/%s", TAGS_DIR, tagName);
    }
    return path;
}

void writeTagInfo(const char* tagFilePath, const char* commitID, const char* taggerName, const char* message) {
    FILE* file = fopen(tagFilePath, "w");
    if (file == NULL) {
        fprintf(stderr, "Failed to open tag file for writing.\n");
        return;
    }

    fprintf(file, "Tag Name: %s\nCommit ID: %s\nTagger: %s\nDate: ", tagFilePath, commitID, taggerName);
    time_t now = time(NULL);
    fprintf(file, "%s", ctime(&now));
    fprintf(file, "Message: %s\n", message);
    fclose(file);
}

bool grepInFile(const char* filePath, const char* word, bool includeLineNumber);
bool grepInCommit(const char* commitID, const char* filePath, const char* word, bool includeLineNumber);
void printHighlightedWord(const char* line, const char* word, bool includeLineNumber, int lineNumber);

bool grepCommand(int argc, char* argv[]) {
    char filePath[256] = {0};
    char word[256] = {0};
    char commitID[41] = {0};
    bool includeLineNumber = false;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-f") == 0 && i + 1 < argc) {
            strncpy(filePath, argv[++i], sizeof(filePath) - 1);
        } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            strncpy(word, argv[++i], sizeof(word) - 1);
        } else if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
            strncpy(commitID, argv[++i], sizeof(commitID) - 1);
        } else if (strcmp(argv[i], "-n") == 0) {
            includeLineNumber = true;
        }
    }

    if (filePath[0] == '\0' || word[0] == '\0') {
        fprintf(stderr, "Usage: neogit grep -f <file> -p <word> [-c <commit-id>] [-n]\n");
        return false;
    }

    if (commitID[0] != '\0') {
        return grepInCommit(commitID, filePath, word, includeLineNumber);
    } else {
        return grepInFile(filePath, word, includeLineNumber);
    }
}

bool grepInFile(const char* filePath, const char* word, bool includeLineNumber) {
    FILE* file = fopen(filePath, "r");
    if (!file) {
        perror("Failed to open file");
        return false;
    }

    char line[1024];
    int lineNumber = 1;
    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, word)) {
            printHighlightedWord(line, word, includeLineNumber, lineNumber);
        }
        lineNumber++;
    }
    fclose(file);
    return true;
}

void printHighlightedWord(const char* line, const char* word, bool includeLineNumber, int lineNumber) {
    if (includeLineNumber) {
        printf("%d: ", lineNumber);
    }
    char* match = strstr(line, word);
    while (match) {
        fwrite(line, 1, match - line, stdout);
        printf(ANSI_COLOR_RED "%s" ANSI_COLOR_RESET, word);
        line = match + strlen(word);
        match = strstr(line, word);
    }
    printf("%s", line);
}

bool grepInCommit(const char* commitID, const char* filePath, const char* word, bool includeLineNumber) {
    printf("Searching in commit %s not implemented.\n", commitID);
    return false;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command> [<args>]\n", argv[0]);
        return 1;
    }

    const char* globalConfigPath = "C:/Users/parham/.zengitconfig";

    if (!processAlias(&argc, &argv, globalConfigPath)) {
        fprintf(stderr, "Failed to process alias.\n");
        return 1;
    }


    if (strcmp(argv[1], "init") == 0) {
        return handleInitCommand() ? 0 : 1;
    } else if (strcmp(argv[1], "config") == 0) {
        return handleConfigCommand(argc, argv) ? 0 : 1;
    } else if (strcmp(argv[1], "add") == 0) {
        return handleAddCommand(argc, argv) ? 0 : 1;
    } else if (strcmp(argv[1], "reset") == 0) {
        return handleResetCommand(argc, argv) ? 0 : 1;
    } else if (strcmp(argv[1], "status") == 0) {
        handleStatusCommand();
        return 0;
    }  if (strcmp(argv[1], "commit") == 0) {

        if (argc == 4 && strcmp(argv[2], "-m") == 0) {
            commitChanges(argv[3]);
            return 0;
        }

        else if (argc == 4 && strcmp(argv[2], "-s") == 0) {
            createCommitWithShortcut(argv[3]);
            return 0;
        } else {
            fprintf(stderr, "Usage: %s commit -m \"commit message\"\n", argv[0]);
            fprintf(stderr, "       %s commit -s shortcut-name\n", argv[0]);
            return 1;
        }
    } else if (strcmp(argv[1], "set") == 0) {

        if (argc != 6 || strcmp(argv[2], "-m") != 0 || strcmp(argv[4], "-s") != 0) {
            fprintf(stderr, "Usage: %s set -m \"message\" -s shortcut-name\n", argv[0]);
            return 1;
        }

        setShortcut(argv[5], argv[3]);
        return 0;
    } else if (strcmp(argv[1], "replace") == 0) {

        if (argc != 6 || strcmp(argv[2], "-m") != 0 || strcmp(argv[4], "-s") != 0) {
            fprintf(stderr, "Usage: %s replace -m \"new message\" -s shortcut-name\n", argv[0]);
            return 1;
        }

        replaceShortcutMessage(argv[5], argv[3]);
        return 0;
    } else if (strcmp(argv[1], "remove") == 0) {
        if (argc != 4 || strcmp(argv[2], "-s") != 0) {
            fprintf(stderr, "Usage: %s remove -s shortcut-name\n", argv[0]);
            return 1;
        }
        removeShortcut(argv[3]);
        return 0;
    } else if (strcmp(argv[1], "branch") == 0) {
        if (argc == 2) {

            listBranches();
        } else if (argc == 3) {

            createBranch(argv[2]);
        } else {
            fprintf(stderr, "Usage: %s branch [branch-name]\n", argv[0]);
            return 1;
        }
        return 0;
    } if (argc > 1 && strcmp(argv[1], "log") == 0) {

        LogEntry *entries;
        int count = readLogEntries(&entries);
        if (count <= 0) {
            printf("No commits found.\n");
            return 0;
        }

        if (argc == 2) {

            for (int i = count - 1; i >= 0; --i) {
                displayLogEntry(&entries[i]);
            }
        } else if (argc == 4 && strcmp(argv[2], "-n") == 0) {

            int n = atoi(argv[3]);
            displayLastNCommits(entries, count, n);
        } else if (argc == 4 && strcmp(argv[2], "-branch") == 0) {

            displayCommitsFromBranch(entries, count, argv[3]);
        } else if (argc == 4 && strcmp(argv[2], "-author") == 0) {

            displayCommitsByAuthor(entries, count, argv[3]);
        } else if (argc == 4 && strcmp(argv[2], "-since") == 0) {

            displayCommitsSince(entries, count, argv[3]);
        } else if (argc == 4 && strcmp(argv[2], "-before") == 0) {

            displayCommitsBefore(entries, count, argv[3]);
        } else if (argc == 4 && strcmp(argv[2], "-search") == 0) {

            zengitLogSearch(argv[3]);
        } else {
            fprintf(stderr, "Unknown log option: %s\n", argv[2]);
        }

        free(entries);
    } else if (strcmp(argv[1], "checkout") == 0 && argc == 3) {
        const char *identifier = argv[2];

        char currentCommitDir[MAX_PATH_LENGTH];
        char commitID[260];

        if (strcmp(identifier, "HEAD") == 0) {
            checkout_head();
        } else if (strncmp(identifier, "HEAD-", 5) == 0) {
            int n = atoi(identifier + 5);
            if (n > 0) {

            } else {
                fprintf(stderr, "Error: Invalid number for HEAD-n.\n");
            }
        } else if (commitExists(identifier)) {

            if (!getCurrentCommit(commitID, sizeof(commitID))) {
                fprintf(stderr, "Failed to get current commit ID.\n");
                return 0;
            }


            snprintf(currentCommitDir, sizeof(currentCommitDir), ".zengit/commits/%s", commitID);


            if (checkDirectoryForUncommittedChanges(".", currentCommitDir)) {
                fprintf(stderr,
                        "Error: Uncommitted changes present. Please commit or stash your changes before checking out.\n");
                return 0;
            }


            checkoutToCommit(identifier);
        } else {
            fprintf(stderr, "Error: The specified identifier does not match any commit or branch.\n");
        }
        return 0;
    } else if (strcmp(argv[1], "revert") == 0) {
        if (argc >= 3 && strcmp(argv[2], "-m") == 0) {
          //  int result = handleRevertCommand(argc, argv) ? 0 : 1;
        } else if (argc >= 3 && strcmp(argv[2], "-n") == 0) {
            // return handleRevertWithoutCommit(argc, argv) ? 0 : 1;
        } else if (argc == 3 && strstr(argv[2], "HEAD-") == argv[2]) {
          //  return handleRevertToPreviousCommit(argc, argv) ? 0 : 1;
        } else {
            fprintf(stderr, "Unknown revert option or missing arguments.\n");
            return 1;
        }
    } else if (strcmp(argv[1], "tag") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: zengit tag -a <tag-name> [-m <message>] [-c <commit-id>] [-f]\n");
            return 1; // Indicate error
        }

        if (strcmp(argv[2], "-a") == 0) {
            // Ensure there's at least a tag name after the "-a" option
            if (argc >= 4) {
                if (!tagCommand(argc, argv)) {
                    fprintf(stderr, "Failed to create tag.\n");
                    return 1; // Indicate error
                }
                return 0; // Indicate success
            } else {
                fprintf(stderr, "Tag name is required after '-a'.\n");
                return 1; // Indicate error
            }
        } else {
            fprintf(stderr, "Missing '-a' option for tag command.\n");
            return 1; // Indicate error
        }
    } else if (strcmp(argv[1], "grep") == 0) {
        if (!grepCommand(argc, argv)) {
            fprintf(stderr, "Failed to execute grep command.\n");
            return 1; // Indicate an error
        }
        return 0; // Success
    } else {
        fprintf(stderr, "Unknown command: %s\n", argv[1]);
        return 1;
    }
}