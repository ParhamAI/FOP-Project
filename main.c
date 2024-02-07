
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
#include <tchar.h>

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

typedef struct {
    char commitId[MAX_PATH];
    FILETIME creationTime;
} CommitEntry;

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
            "HEAD",

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


bool isFileEmpty(const char* filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        return true;
    }
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fclose(file);
    return fileSize == 0;
}

bool commitChanges(const char* message) {
    if (!fileExists(INDEX_FILE) || isFileEmpty(INDEX_FILE)) {
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
    char tempLine[MAX_PATH_LENGTH];
    snprintf(headFilePath, sizeof(headFilePath), "%s/%s_HEAD", COMMIT_DIR, branchName);

    FILE *file = fopen(headFilePath, "r");
    if (file) {
        while (fgets(tempLine, sizeof(tempLine), file) != NULL) {
            strncpy(lastCommitId, tempLine, sizeof(lastCommitId));
            lastCommitId[sizeof(lastCommitId) - 1] = '\0';
        }
        fclose(file);

        lastCommitId[strcspn(lastCommitId, "\n")] = 0;
        return lastCommitId;
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
        fprintf(branchHeadFile, "%s\n", currentCommitId);
        fclose(branchHeadFile);

        char headFilePath[MAX_PATH_LENGTH];
        snprintf(headFilePath, sizeof(headFilePath), ".zengit/HEAD");
        FILE* headFile = fopen(headFilePath, "a");
        if (headFile) {
            fprintf(headFile, "%s\n", branchName);
            fclose(headFile);
        } else {
            perror("Failed to record branch name in .zengit/HEAD file");
        }

        printf("Branch '%s' created from the latest commit and recorded in .zengit/HEAD.\n", branchName);
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

time_t logDateStringToTimeT(const char* logDateString) {
    struct tm tm = {0};
    char monthStr[4];
    int month, day, hour, minute, second, year;
    const char* months = "JanFebMarAprMayJunJulAugSepOctNovDec";

    sscanf(logDateString, "%*s %3s %d %d:%d:%d %d", monthStr, &day, &hour, &minute, &second, &year);

    month = (strstr(months, monthStr) - months) / 3;

    tm.tm_mon = month;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_sec = second;
    tm.tm_year = year - 1900;
    tm.tm_isdst = -1;

    return mktime(&tm);
}

time_t cutoffDateToTimeT(const char* dateString) {
    struct tm tm = {0};
    sscanf(dateString, "%d-%d-%d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday);

    tm.tm_year -= 1900; // Adjust for tm structure's year (years since 1900)
    tm.tm_mon -= 1; // Adjust month from 1-12 to 0-11
    tm.tm_hour = 0;
    tm.tm_min = 0;
    tm.tm_sec = 0;
    tm.tm_isdst = -1; // Not considering DST

    return mktime(&tm);
}

void displayCommitsBefore(const LogEntry* entries, int count, const char* date) {
    time_t cutoffDate = cutoffDateToTimeT(date);
    printf("Commits before: %s\n", date);
    int found = 0;
    for (int i = count - 1; i >= 0; --i) {
        time_t commitDate = logDateStringToTimeT(entries[i].date);
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
    time_t cutoffDate = cutoffDateToTimeT(date);
    printf("Commits since: %s\n", date);
    int found = 0;
    for (int i = count - 1; i >= 0; --i) {
        time_t commitDate = logDateStringToTimeT(entries[i].date);
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

int messageContainsTerms(const char* message, const char* terms[], int numTerms) {
    for (int i = 0; i < numTerms; i++) {
        if (strstr(message, terms[i]) != NULL) {
            return 1;
        }
    }
    return 0;
}

int readLogEntry(FILE* file, LogEntry* entry) {
    if (fscanf(file, "Date: %[^\n]\nUser: %[^\n]\nCommit ID: %[^\n]\nBranch: %[^\n]\nMessage: %[^\n]\nFiles Committed: %d\n\n",
               entry->date, entry->user, entry->commitID, entry->branch, entry->message, &entry->filesCommitted) == 6) {
        return 1;
    } else {
        return 0;
    }
}

void zengitLogSearch(const char *searchString) {
    char* terms[256];
    int numTerms = 0;
    char* searchStringCopy = strdup(searchString);
    char* token = strtok(searchStringCopy, " ");

    while (token != NULL && numTerms < 256) {
        terms[numTerms++] = token;
        token = strtok(NULL, " ");
    }

    FILE *logFile = fopen(".zengit/logs", "r");
    if (!logFile) {
        printf("Log file not found.\n");
        free(searchStringCopy);
        return;
    }

    LogEntry entry;
    while (readLogEntry(logFile, &entry)) {
        if (messageContainsTerms(entry.message, terms, numTerms)) {
            displayLogEntry(&entry);
        }
    }

    fclose(logFile);
    free(searchStringCopy);
}

void deleteDirectoryRecursively(const TCHAR* path) {
    WIN32_FIND_DATA findFileData;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    TCHAR dirPathWildcard[MAX_PATH];

    _stprintf_s(dirPathWildcard, MAX_PATH, TEXT("%s\\*"), path); // Append wildcard to search pattern
    hFind = FindFirstFile(dirPathWildcard, &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        return;
    }

    do {
        if (_tcscmp(findFileData.cFileName, TEXT(".")) == 0 || _tcscmp(findFileData.cFileName, TEXT("..")) == 0) {
            continue;
        }

        TCHAR fullPath[MAX_PATH];
        _stprintf_s(fullPath, MAX_PATH, TEXT("%s\\%s"), path, findFileData.cFileName);

        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            deleteDirectoryRecursively(fullPath);
            RemoveDirectory(fullPath);
        } else {
            DeleteFile(fullPath);
        }
    } while (FindNextFile(hFind, &findFileData) != 0);

    FindClose(hFind);
}

void clearWorkingDirectoryExceptZengit(const char* dirPath) {
    WIN32_FIND_DATA findFileData;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    TCHAR dirPathWildcard[MAX_PATH];
    _stprintf_s(dirPathWildcard, MAX_PATH, TEXT("%s\\*"), dirPath);

    hFind = FindFirstFile(dirPathWildcard, &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        printf("Unable to open directory for clearing: %s\n", dirPath);
        return;
    }

    do {

        if (_tcscmp(findFileData.cFileName, TEXT(".")) == 0 || _tcscmp(findFileData.cFileName, TEXT("..")) == 0) {
            continue;
        }

        TCHAR fullPath[MAX_PATH];
        _stprintf_s(fullPath, MAX_PATH, TEXT("%s\\%s"), dirPath, findFileData.cFileName);

        if (_tcscmp(findFileData.cFileName, TEXT(".zengit")) == 0) {
            continue;
        }

        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            deleteDirectoryRecursively(fullPath);
            RemoveDirectory(fullPath);
        } else {
            DeleteFile(fullPath);
        }
    } while (FindNextFile(hFind, &findFileData) != 0);

    FindClose(hFind);
}

void zengitCheckout(const char* branchName) {
    char* lastCommitId = getLastCommitId(branchName);
    if (lastCommitId == NULL) {
        printf("Error: Could not find the last commit for branch '%s'.\n", branchName);
        return;
    }

    clearWorkingDirectoryExceptZengit(".");

    char commitDirPath[MAX_PATH_LENGTH];
    snprintf(commitDirPath, sizeof(commitDirPath), ".zengit/commits/%s", lastCommitId);
    copyDirectoryRecursively(commitDirPath, ".");

    printf("Switched to branch '%s'.\n", branchName);
}

int commitIdExists(const char* commitId) {
    WIN32_FIND_DATA findFileData;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    char dirPath[MAX_PATH];
    snprintf(dirPath, sizeof(dirPath), ".zengit/commits/%s", commitId);

    hFind = FindFirstFile(dirPath, &findFileData);
    if (hFind != INVALID_HANDLE_VALUE) {
        FindClose(hFind);
        return (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }
    return 0;
}

void zengitCheckoutCommitId(const char* commitId) {
    if (!commitIdExists(commitId)) {
        printf("Error: Commit ID '%s' does not exist.\n", commitId);
        return;
    }

    clearWorkingDirectoryExceptZengit(".");

    char commitDirPath[MAX_PATH];
    snprintf(commitDirPath, sizeof(commitDirPath), ".zengit/commits/%s", commitId);

    copyDirectoryRecursively(commitDirPath, ".");

    printf("Checked out commit '%s'.\n", commitId);
}

void zengitCheckoutHead() {
    char* currentBranch = getCurrentBranch();
    zengitCheckout(currentBranch);
}

int compareCommitEntries(const void* a, const void* b) {
    FILETIME* timeA = &((CommitEntry*)a)->creationTime;
    FILETIME* timeB = &((CommitEntry*)b)->creationTime;

    return CompareFileTime(timeB, timeA);
}

CommitEntry* getSortedCommits(const char* commitDirPath, int* count) {
    WIN32_FIND_DATA findFileData;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    char searchPath[MAX_PATH];
    snprintf(searchPath, sizeof(searchPath), "%s/*", commitDirPath);

    int capacity = 10;
    *count = 0;
    CommitEntry* commits = malloc(capacity * sizeof(CommitEntry));
    if (!commits) return NULL;

    hFind = FindFirstFile(searchPath, &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        free(commits);
        return NULL;
    }

    do {
        if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
        if (strcmp(findFileData.cFileName, ".") == 0 || strcmp(findFileData.cFileName, "..") == 0) continue;

        if (*count >= capacity) {
            capacity *= 2;
            CommitEntry* resized = realloc(commits, capacity * sizeof(CommitEntry));
            if (!resized) {
                free(commits);
                FindClose(hFind);
                return NULL;
            }
            commits = resized;
        }

        snprintf(commits[*count].commitId, MAX_PATH, "%s", findFileData.cFileName);
        commits[*count].creationTime = findFileData.ftCreationTime;
        (*count)++;
    } while (FindNextFile(hFind, &findFileData) != 0);

    FindClose(hFind);

    qsort(commits, *count, sizeof(CommitEntry), compareCommitEntries);

    return commits;
}

void zengitCheckoutHeadN(const char* commitsDir, int n) {
    int count = 0;
    CommitEntry* commits = getSortedCommits(commitsDir, &count);

    int targetIndex = n;

    if (!commits || count <= targetIndex) {
        printf("Error: Unable to find the specified commit. Only %d commits available.\n", count);
        free(commits);
        return;
    }

    zengitCheckoutCommitId(commits[targetIndex].commitId);
    free(commits);
}




int isBranchName(const char* branchName) {
    char branchHeadFilePath[MAX_PATH_LENGTH];
    snprintf(branchHeadFilePath, sizeof(branchHeadFilePath), "%s/%s_HEAD", COMMIT_DIR, branchName);

    FILE* file = fopen(branchHeadFilePath, "r");
    if (file) {
        fclose(file);
        return 1;
    }

    return 0;
}

int isCommitId(const char* commitId) {
    char commitDirPath[MAX_PATH_LENGTH];
    struct stat statbuf;
    snprintf(commitDirPath, sizeof(commitDirPath), "%s/%s", COMMIT_DIR, commitId);

    if (stat(commitDirPath, &statbuf) == 0) {
        if (S_ISDIR(statbuf.st_mode)) {
            return 1;
        }
    }

    return 0;
}

void writePlaceholderToIndex() {
    const char* indexPath = ".zengit/index";

    FILE* file = fopen(indexPath, "w");
    if (file == NULL) {
        perror("Failed to open index file");
        return;
    }

    fprintf(file, "aaa\n");

    fclose(file);
}

void zengitRevert(const char* message, const char* commitId) {
    zengitCheckoutCommitId(commitId);

    writePlaceholderToIndex();

    if (!commitChanges(message)) {
        printf("Failed to create a new commit with the revert message.\n");
    } else {
        printf("Reverted to commit %s and created a new commit with message: %s\n", commitId, message);
    }
}
char* findCommitMessage(const char* commitId) {
    static char message[256];
    FILE* file = fopen(".zengit/logs", "r");
    if (!file) {
        perror("Failed to open log file");
        return NULL;
    }

    char line[1024];
    bool isTargetCommit = false;

    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, "Commit ID: ") && strstr(line, commitId)) {
            isTargetCommit = true;
        } else if (isTargetCommit && strstr(line, "Message: ")) {
            sscanf(line, "Message: %[^\n]", message);
            fclose(file);
            return message;
        } else if (line[0] == '\n') {
            isTargetCommit = false;
        }
    }

    fclose(file);
    return NULL;
}

void zengitRevertWithoutMessage(const char* commitId) {
    char* commitMessage = findCommitMessage(commitId);
    if (!commitMessage) {
        printf("Error: Could not find commit message for commit ID '%s'.\n", commitId);
        return;
    }

    zengitCheckoutCommitId(commitId);

    writePlaceholderToIndex();

    if (!commitChanges(commitMessage)) {
        printf("Failed to create a new commit with the revert message.\n");
    } else {
        printf("Reverted to commit %s and created a new commit with message: %s\n", commitId, commitMessage);
    }
}

void zengitRevertToCommit(const char* commitId) {
    zengitCheckoutCommitId(commitId);
}

void zengitRevertToHead() {
    zengitCheckoutHead();
}

void zengitRevertHeadXWithMessage(int X, const char* message) {
    zengitCheckoutHeadN(".zengit/commits", X-1);

    writePlaceholderToIndex();

    if (!commitChanges(message)) {
        printf("Failed to create a new commit after reverting.\n");
    } else {
        printf("Successfully reverted to HEAD-%d and created a new commit with message: %s\n", X, message);
    }
}

void createTag(const char* tagName, const char* message, const char* commitId, bool force) {
    char tagFilePath[256];
    snprintf(tagFilePath, sizeof(tagFilePath), "%s/%s", TAGS_DIR, tagName);

    struct stat buffer;
    if (stat(tagFilePath, &buffer) == 0) { // File exists
        if (!force) {
            printf("Error: Tag '%s' already exists. Use -f to overwrite.\n", tagName);
            return;
        }
    }

    char lastCommitId[256];
    if (!commitId) {
        commitId = getLastCommitId(getCurrentBranch());
        if (!commitId) {
            printf("Error: Unable to determine the last commit ID.\n");
            return;
        }
    }

    FILE* file = fopen(tagFilePath, "w");
    if (!file) {
        perror("Failed to create or overwrite tag file");
        return;
    }

    char userName[256];
    if (!getUserNameFromConfig(userName, sizeof(userName), "./.zengitconfig") &&
        !getUserNameFromConfig(userName, sizeof(userName), "C:/Users/parham/.zengitconfig")) {
        strcpy(userName, "Unknown");
    }

    time_t now = time(NULL);
    char formattedTime[64];
    strftime(formattedTime, sizeof(formattedTime), "%Y-%m-%d %H:%M:%S", localtime(&now));

    fprintf(file, "Tag Name: %s\nCommit ID: %s\nUser: %s\nDate: %s\nMessage: %s\n",
            tagName, commitId, userName, formattedTime, message ? message : "");

    fclose(file);
    printf("Tag '%s' created successfully.\n", tagName);
}

int compareStrings(const void* a, const void* b) {
    return strcmp(*(const char**)a, *(const char**)b);
}

void listTags() {
    WIN32_FIND_DATA findFileData;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    char path[MAX_PATH];
    snprintf(path, MAX_PATH, "%s/*", TAGS_DIR);

    char** tags = malloc(sizeof(char*) * 10);
    int capacity = 10, n = 0;

    hFind = FindFirstFile(path, &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        printf("No tags found.\n");
        return;
    }

    do {

        if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {

            if (n >= capacity) {
                capacity *= 2;
                tags = realloc(tags, sizeof(char*) * capacity);
                if (!tags) {
                    printf("Memory allocation error.\n");
                    FindClose(hFind);
                    return;
                }
            }

            tags[n] = malloc(strlen(findFileData.cFileName) + 1);
            strcpy(tags[n], findFileData.cFileName);
            n++;
        }
    } while (FindNextFile(hFind, &findFileData) != 0);

    FindClose(hFind);

    qsort(tags, n, sizeof(char*), compareStrings);

    for (int i = 0; i < n; i++) {
        printf("%s\n", tags[i]);
        free(tags[i]);
    }

    free(tags);
}

void showTagInfo(const char* tagName) {
    char tagFilePath[256];
    snprintf(tagFilePath, sizeof(tagFilePath), "%s/%s", TAGS_DIR, tagName);

    FILE* file = fopen(tagFilePath, "r");
    if (!file) {
        printf("Error: Could not find tag '%s'.\n", tagName);
        return;
    }

    printf("Information for tag '%s':\n", tagName);
    char line[1024];
    while (fgets(line, sizeof(line), file)) {
        printf("%s", line);
    }

    fclose(file);
}

void highlightPattern(const char* line, const char* pattern) {
    const char* start = line;
    const char* found = NULL;

    const char* RED = "\x1B[31m";
    const char* RESET = "\x1B[0m";

    while ((found = strstr(start, pattern)) != NULL) {
        printf("%.*s", (int)(found - start), start);

        printf("%s%s%s", RED, pattern, RESET);

        start = found + strlen(pattern);
    }

    printf("%s", start);
}

void grepInFile(const char* dir, const char* filename, const char* pattern, bool showLineNum) {
    char fullPath[1024];
    if (dir) {
        snprintf(fullPath, sizeof(fullPath), "%s/%s", dir, filename);
        filename = fullPath;
    }

    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        return;
    }

    char line[1024];
    int lineNum = 0;
    while (fgets(line, sizeof(line), file) != NULL) {
        lineNum++;
        if (strstr(line, pattern) != NULL) {
            if (showLineNum) {
                printf("%d: ", lineNum);
            }
            highlightPattern(line, pattern); // Assuming highlightPattern is defined as before
        }
    }

    fclose(file);
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
        const char *checkoutTarget = argv[2];

        if (strcmp(checkoutTarget, "HEAD") == 0) {
            zengitCheckoutHead();
        } else if (strncmp(checkoutTarget, "HEAD-", 5) == 0) {
            // Extract the number following "HEAD-"
            int n = atoi(checkoutTarget + 5);
            if (n > 0) {
                zengitCheckoutHeadN(".zengit/commits", n);
            } else {
                printf("Error: Invalid checkout target '%s'.\n", checkoutTarget);
            }
        } else if (isBranchName(checkoutTarget)) {
            zengitCheckout(checkoutTarget);
        } else if (isCommitId(checkoutTarget)) {
            zengitCheckoutCommitId(checkoutTarget);
        }

    } else if (strcmp(argv[1], "revert") == 0 && argc == 5 && strcmp(argv[2], "-m") == 0) {
        const char* message = argv[3];
        const char* commitId = argv[4];
        zengitRevert(message, commitId);
    }  else if (strcmp(argv[1], "revert") == 0 && argc == 3) {
        const char* commitId = argv[2];
        zengitRevertWithoutMessage(commitId);
    } else if (strcmp(argv[1], "revert") == 0 && argc == 4 && strcmp(argv[2], "-n") == 0) {
        const char* commitId = argv[3];
        zengitRevertToCommit(commitId);
    } else if (strcmp(argv[1], "revert") == 0 && argc == 3 && strcmp(argv[2], "-n") == 0) {
        zengitRevertToHead();
    } else if (argc == 2 && strcmp(argv[1], "tag") == 0) {
        listTags();
    } if (argc == 4 && strcmp(argv[1], "tag") == 0 && strcmp(argv[2], "show") == 0) {
        showTagInfo(argv[3]);
    } else if (strcmp(argv[1], "tag") == 0 && strcmp(argv[2], "-a") == 0 && argc >= 4) {
        const char* tagName = argv[3];
        const char* message = NULL;
        const char* commitId = NULL;
        bool force = false;

        for (int i = 4; i < argc; i++) {
            if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
                message = argv[++i];
            } else if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
                commitId = argv[++i];
            } else if (strcmp(argv[i], "-f") == 0) {
                force = true;
            }
        }
        createTag(tagName, message, commitId, force);
    }     else if (strcmp(argv[1], "grep") == 0) {
        char* filename = NULL;
        char* pattern = NULL;
        char* commitId = NULL;
        bool showLineNumbers = false; // Corrected to false as default
        char basePath[256] = "."; // Default to current directory

        // Parse grep-specific options
        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "-f") == 0 && i + 1 < argc) {
                filename = argv[++i];
            } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
                pattern = argv[++i];
            } else if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
                commitId = argv[++i];
                // Update basePath to include the commit ID directory
                snprintf(basePath, sizeof(basePath), ".zengit/commits/%s", commitId);
            } else if (strcmp(argv[i], "-n") == 0) {
                showLineNumbers = true; // Only set to true if -n is present
            }
        }

        if (filename && pattern) {
            grepInFile(basePath, filename, pattern, showLineNumbers);
        } else {
            printf("Usage: zengit grep -f <file> -p <word> [-c <commit-id>] [-n]\n");
        }
    } else {
        fprintf(stderr, "Unknown command: %s\n", argv[1]);
        return 1;
    }
}