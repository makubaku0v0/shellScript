#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#define safeFree(p) { if(p){ free(p);  (p)=NULL;}}

//返回指令执行结果
char *executeShell(char *command, int resultLength);

int isNormalFile(char *filePath);

int isDownloadCorrect(char *downloadCMD, char *filePath);

int main(int argc, char **argv) {

    //判断输入参数格式是否正确
    if (argc != 3) {
        printf("The startup parameters were not entered in the correct format\n");
        //提示输出
        printf("Please enter the test program startup parameters in the following format:\n");
        printf("./linux-terminal-detection IP Port\n");
        printf("Example of start command:./linux-terminal-detection 127.0.0.1 8086\n");
        return -1;
    }

    //获取参数ip
    char hostIP[16] = "";
    if (strlen(argv[1]) > 15 || strlen(argv[1]) < 7) {
        printf("Input IP format error\n");
        return -1;
    }
    strncpy(hostIP, argv[1], strlen(argv[1]));

    //获取参数port
    char port[6] = "";
    if (strlen(argv[2]) > 5 || strlen(argv[2]) < 1) {
        printf("Input port format error\n");
        return -1;
    }
    strncpy(port, argv[2], strlen(argv[2]));
    printf("\nThe host IP and listing port：http://%s:%s\n", hostIP, port);

    //获取当前路径
    char homePath[1024] = "";
    getcwd(homePath, 1024);

    //判断判定文件是否存在
    char *releaseFiles[3];
    char *osCMDs[3];
    char *versionCMDs[3];
    int iter = 0;

    //判定文件路径
    releaseFiles[0] = calloc(1024, sizeof(char));
    strcpy(releaseFiles[0], "/etc/redhat-release");

    releaseFiles[1] = calloc(1024, sizeof(char));
    strcpy(releaseFiles[1], "/etc/os-release");

    releaseFiles[2] = calloc(1024, sizeof(char));
    strcpy(releaseFiles[2], "/etc/system-release");

    //获取os指令
    osCMDs[0] = calloc(1024, sizeof(char));
    sprintf(osCMDs[0], "cat %s | head -n 1 | awk -F \" \" '{print $1}' | tr 'A-Z' 'a-z'", releaseFiles[0]);

    osCMDs[1] = calloc(1024, sizeof(char));
    sprintf(osCMDs[1],
            "cat %s |grep \"^NAME\"|awk -F \"=\" '{print $2}'|awk -F \" \" '{print $1}'|sed 's/\"//g'|tr 'A-Z' 'a-z'",
            releaseFiles[1]);

    osCMDs[2] = calloc(1024, sizeof(char));
    sprintf(osCMDs[2], "cat %s | head -n 1 |grep -oP '\\d+(\\.\\d*)*'|awk -F \".\" '{print $1}'", releaseFiles[2]);

    //获取version指令
    versionCMDs[0] = calloc(1024, sizeof(char));
    sprintf(versionCMDs[0], "cat %s | head -n 1 |grep -oP '\\d+(\\.\\d*)*'|awk -F \".\" '{print $1}'", releaseFiles[0]);

    versionCMDs[1] = calloc(1024, sizeof(char));
    sprintf(versionCMDs[1], "cat %s |grep \"^PRETTY_NAME=\"|grep -oP '\\d+(\\.\\d*)*'|awk -F \".\" '{print $1}'",
            releaseFiles[1]);

    versionCMDs[2] = calloc(1024, sizeof(char));
    sprintf(versionCMDs[2], "cat %s | head -n 1 |grep -oP '\\d+(\\.\\d*)*'|awk -F \".\" '{print $1}'", releaseFiles[2]);


    while (iter != 3) {
        //若该文件存在
        if ((access(releaseFiles[iter], F_OK)) != -1) {
            printf("\nJudgment document %s exist\n", releaseFiles[iter]);
            break;
        }
        iter++;
    }
    if (iter == 3) {
        printf("The judgment file could not be found\n");
        return -1;
    }

    //获取平台和版本
    printf("\n=====\t       \t  Obtaining Linux Version\t\t\t=====\n");

    char *os = NULL;
    char *version = NULL;
    char binaryFile[256] = "";
    os = executeShell(osCMDs[iter], 128);
    version = executeShell(versionCMDs[iter], 128);

    //获取平台和版本失败
    if (!os || !version) {
        printf("Obtaining Linux Version Failed\n");
        return -1;
    }

    //对redhat进行特殊处理
    if (strcmp(os, "red") == 0) {
        safeFree(os)
        if ((os = calloc(128, sizeof(char))) == NULL) {
            printf("Calloc Memory Failed\n");
            return -1;
        }
        strcpy(os, "redhat");
    }

    //处理尾部换行符
    if (os[strlen(os) - 1] == '\n')
        os[strlen(os) - 1] = '\0';
    if (version[strlen(version) - 1] == '\n')
        version[strlen(version) - 1] = '\0';

    //构造二进制检测文件名
    strcat(binaryFile, os);
    strcat(binaryFile, version);

    printf("=====\t       \t  Linux Version: %s\t\t\t=====\n", binaryFile);

    //创建工作目录了,为了方便测试工作目录命名为workspace，正式上线时改为.workspace
    printf("=====\t       \t  Creating and Entering Workspace\t\t=====\n");
    //若.workspace目录存在则删除
    if ((access(".workspace", F_OK)) != -1) {
        if (system("rm -rf .workspace") == -1) {
            printf("===== \t   !!! \tRemove exist .workspace Failed\t\t!!!   =====\n");
            return -1;
        }
    }
    //创建.workspace目录
    if (mkdir(".workspace", 0600) == -1) {
        printf("===== \t   !!! \tCreate .workspace Failed\t\t!!!   =====\n");
        return -1;
    }
    //切换.workspace目录
    if (chdir(".workspace") == -1) {
        printf("===== \t   !!! \tcd .workspace Failed\t\t!!!   =====\n");
        return -1;
    }

    //下载二进制文件
    printf("===== \t       \t  Downloading Binary File\t\t\t=====\n");
    char downloadCMD[1024] = "";
    snprintf(downloadCMD, 1024, "curl -o %s http://%s:%s/terminal/terminal-detect/%s", binaryFile, hostIP,
             port, binaryFile);
    //下载失败
    if (isDownloadCorrect(downloadCMD, binaryFile) == -1)
        return -1;
    else {
        char chmodCMD[128] = "";
        snprintf(chmodCMD, 128, "chmod +x %s", binaryFile);
        if (system(chmodCMD) == -1) {
            printf("===== \t   !!! \tChmod +x Binary File Failed\t\t!!!   =====\n");
            return -1;
        }
    }


    //下载配置文件
    printf("=====              Downloading Config File           \t\t=====\n");
    memset(downloadCMD, 0, 1024);
    snprintf(downloadCMD, 1024, "curl -o detect.conf http://%s:%s/terminal/conf-files/linux", hostIP, port);
    //下载失败
    if (isDownloadCorrect(downloadCMD, "detect.conf") == -1)
        return -1;

    //创建.sqlite目录
    if (mkdir(".sqlite", 0600) == -1) {
        printf("===== \t   !!! \tCreate .sqlite Failed\t\t!!!   =====\n");
        return -1;
    }

    //创建.tarfiles目录
    if (mkdir(".tarfiles", 0600) == -1) {
        printf("===== \t   !!! \tCreate .tarfiles Failed\t\t!!!   =====\n");
        return -1;
    }

    //下载工具软件和库文件,系统配置文件
    printf("===== \t     \tDownloading Util Files\t\t\t\t=====\n");

    memset(downloadCMD, 0, 1024);
    snprintf(downloadCMD, 1024, "curl -o lib64.tar.gz http://%s:%s/terminal/terminal-detect/utils/64/lib64", hostIP,
             port);
    //下载失败
    if (isDownloadCorrect(downloadCMD, "lib64.tar.gz") == -1)
        return -1;

    memset(downloadCMD, 0, 1024);
    snprintf(downloadCMD, 1024, "curl -o binaryutils.tar.gz http://%s:%s/terminal/terminal-detect/utils/64/binaryutils",
             hostIP, port);
    //下载失败
    if (isDownloadCorrect(downloadCMD, "binaryutils.tar.gz") == -1)
        return -1;

    memset(downloadCMD, 0, 1024);
    snprintf(downloadCMD, 1024, "curl -o utilconfs.tar.gz http://%s:%s/terminal/terminal-detect/utils/64/utilconfs",
             hostIP,
             port);
    //下载失败
    if (isDownloadCorrect(downloadCMD, "utilconfs.tar.gz") == -1)
        return -1;

    //解压工具软件和库文件
    if (system("tar -zxf lib64.tar.gz") == -1) {
        printf("===== \t   !!! \ttar -zxf lib64.tar.gz Failed\t\t!!!   =====\n");
        return -1;
    }
    if (system("tar -zxf binaryutils.tar.gz") == -1) {
        printf("===== \t   !!! \ttar -zxf binaryutils.tar.gz Failed\t\t!!!   =====\n");
        return -1;
    }
    if (system("tar -zxf utilconfs.tar.gz") == -1) {
        printf("===== \t   !!! \ttar -zxf utilconfs.tar.gz Failed\t\t!!!   =====\n");
        return -1;
    }

    //增加权限
    if (system("chmod +x lib64/*") == -1) {
        printf("===== \t   !!! \tchmod +x lib64/* Failed\t\t!!!   =====\n");
        return -1;
    }

    if (system("chmod +x binaryutils/*") == -1) {
        printf("===== \t   !!! \tchmod +x binaryutils/* Failed\t\t!!!   =====\n");
        return -1;
    }

    if (system("chmod +x binaryutils/p7zip/*") == -1) {
        printf("===== \t   !!! \tchmod +x p7zip/* Failed\t\t!!!   =====\n");
        return -1;
    }

    //添加LD_LIBRARY_PATH环境变量
    char *LLP = getenv("LD_LIBRARY_PATH");
    int LLPLength = 0;
    char *envCMD = NULL;

    if (LLP) {
        LLPLength = strlen(LLP);
        envCMD = calloc(sizeof(char), LLPLength + 1024);
        snprintf(envCMD, strlen(LLP) + 1024, "LD_LIBRARY_PATH=%s/.workspace/lib64:%s", homePath, LLP);
    } else {
        envCMD = calloc(sizeof(char), 1024);
        snprintf(envCMD, 1024, "LD_LIBRARY_PATH=%s/.workspace/lib64", homePath);
    }
    putenv(envCMD);
    safeFree(LLP)
    safeFree(envCMD)

    //添加PATH变量
    char *PATHEnv = getenv("PATH");
    int PATHLength = 0;
    if (PATHEnv) {
        PATHLength = strlen(PATHEnv);
        envCMD = calloc(sizeof(char), PATHLength + 1024);
        snprintf(envCMD, PATHLength + 1024, "PATH=%s:/.workspace/binaryutils/p7zip:%s", homePath, PATHEnv);
    } else {
        envCMD = calloc(sizeof(char), 1024);
        snprintf(envCMD, 1024, "PATH=%s:/.workspace/binaryutils/p7zip", homePath);
    }
    putenv(envCMD);
//    safeFree(PATHEnv)
    safeFree(envCMD)


    //查看系统配置文件
    int confFlag = -1;
    //-1：初始值,未进行拷贝； 0：有/etc/fonts目录且没有文件； 1：没有/etc/fonts目录
    if (access("/etc/fonts", F_OK) != -1) { //若/etc/fonts存在
        if (access("/etc/fonts/fonts.conf", F_OK) == -1) { //若/etc/fonts/fonts.conf不存在
            confFlag = 0;
            if (system("cp utilconfs/fonts.conf /etc/fonts") == -1) {
                printf("\"===== \\t   !!! \\tcp utilconfs/fonts.conf Failed\\t\\t!!!   =====\\n\"");
                return -1;
            }
        }
    } else {
        confFlag = 1;
        //创建/etc/fonts目录
        if (mkdir("/etc/fonts", 0644) == -1) {
            printf("===== \t   !!! \tCreate /etc/fonts Failed\t\t!!!   =====\n");
            return -1;
        }
        if (system("cp utilconfs/fonts.conf /etc/fonts") == -1) {
            printf("===== \\t   !!! \\tcp utilconfs/fonts.conf Failed\\t\\t!!!   =====\\n");
            return -1;
        }
    }

    //执行检测
    printf("=====        Executing Windows Terminal Detection Program\t=====\n");
    char detectCMD[1024] = "";
    snprintf(detectCMD, 1024, "./%s detect.conf", binaryFile);
    if (system(detectCMD) == -1) {
        printf("===== \\t   !!! \\tRun %s Failed\\t\\t!!!   =====\\n", binaryFile);
        return -1;
    }

    //回传检测结果
    printf("=====\t           Uploading Detection Raw Data\t\t\t\t=====\n");

    //获取随机数
    char *randomString = executeShell("echo $RANDOM$RANDOM$RANDOM", 1024);
    if (randomString == NULL) {
        printf("===== \\t   !!! \\tCreat Random Number Failed\\t\\t!!!   =====\\n");
        return -1;
    }

    //回传.sqlite文件
    //获取.sqlite文件个数
    char *fileNumInfo = executeShell("ls -A .sqlite/|wc -w", 1024);
//    char *fileNumInfo = executeShell("ls -A lib64 | wc -w", 1024);
    if (fileNumInfo == NULL) {
        printf("===== \\t   !!! \\tGet Files Number Failed\\t\\t!!!   =====\\n");
        return -1;
    }
    int fileNum = atoi(fileNumInfo);
    if (fileNum >= 1) {
        //读取目录下所有文件
        DIR *dir = NULL;
        if ((dir = (DIR *) opendir(".sqlite")) == NULL) {
//        if ((dir = (DIR *) opendir("lib64")) == NULL) {
            printf("===== \\t   !!! \\tOpen .sqlite Failed\\t\\t!!!   =====\\n");
            return -1;
        }

        //进入.sqlite文件夹
        if (chdir(".sqlite") == -1) {
//        if (chdir("lib64") == -1) {
            printf("===== \t   !!! \tcd .sqlite Dir Failed\t\t!!!   =====\n");
            return -1;
        }

        struct dirent *entry = NULL;
        for (entry = readdir(dir); entry != NULL; entry = readdir(dir)) {
            if (entry->d_type != DT_DIR) {
                //改名
                char mvCMD[1024] = "";
                snprintf(mvCMD, 1024, "mv %s %s.%s", entry->d_name, randomString, entry->d_name);
                if (system(mvCMD) == -1) {
                    printf("===== \\t   !!! \\t%s Failed\\t\\t!!!   =====\\n", mvCMD);
                    return -1;
                }

                //上传
                char uploadCMD[1024] = "";
                snprintf(uploadCMD, 1024, "curl -F sqliteFile=@%s.%s http://%s:%s/terminal/sqlite", randomString,
                         entry->d_name, hostIP, port);
                if (system(uploadCMD) == -1) {
                    printf("===== \\t   !!! \\tUpload %s Failed\\t\\t!!!   =====\\n", entry->d_name);
                    return -1;
                } else {
                    putchar('\n');
                }
            }
        }

        //切出.sqlite文件夹
        if (chdir("..") == -1) {
            printf("===== \t   !!! \tcd .. Dir Failed\t\t!!!   =====\n");
            return -1;
        }
    }

    //上传json检测文件
    //获取json文件名
    char *jsonFile = NULL;
    if ((jsonFile = executeShell("ls .| grep .json", 1024)) == NULL) {
        printf("===== \\t   !!! \\tGet Json File Failed\\t\\t!!!   =====\\n");
        return -1;
    }
    //将json改名
    char mvCMD[1024] = "";
    snprintf(mvCMD, 1024, "mv %s %s.%s", jsonFile, randomString, jsonFile);
    if (system(mvCMD) == -1) {
        printf("===== \\t   !!! \\t%s Failed\\t\\t!!!   =====\\n", mvCMD);
        return -1;
    }

    //重新获取json文件名
    safeFree(jsonFile)
    if ((jsonFile = executeShell("ls .| grep .json", 1024)) == NULL) {
        printf("===== \\t   !!! \\tGet Json File Failed\\t\\t!!!   =====\\n");
        printf("=====\t  ERROR!!! Linux Terminal Detection Failed\t\t\t=====\n");
        return -1;
    }
    //上传json文件
    char uploadCMD[1024] = "";
    snprintf(uploadCMD, 1024, "curl -F jsonfile=@%s http://%s:%s/terminal/terminal-detect/linux", jsonFile, hostIP,
             port);
    if (system(uploadCMD) == -1) {
        printf("===== \\t   !!! \\tUpload%s Failed\\t\\t!!!   =====\\n", jsonFile);
        return -1;
    } else {
        putchar('\n');
    }

    printf("=====\t         Linux Terminal Detection Success\t\t\t=====\n");
    safeFree(randomString)
    safeFree(fileNumInfo)

    //删除文件拷贝
    switch (confFlag) {
        case -1:
            break;
        case 0:
            if (system("rm -f /etc/fonts/fonts.conf") == -1) {
                printf("===== \\t   !!! \\tRemove fonts.conf File Failed\\t\\t!!!   =====\\n");
            }
            break;
        case 1:
            if (system("rm -rf /etc/fonts") == -1) {
                printf("===== \\t   !!! \\tRemove fonts Dir Failed\\t\\t!!!   =====\\n");
            }
            break;
        default:
            break;
    }

    //切出.workspace目录
    if (chdir("..") == -1) {
        printf("===== \t   !!! \tcd .. Dir Failed\t\t!!!   =====\n");
        return -1;
    }

    //删除.workspace目录
    if (system("rm -rf .workspace") == -1) {
        printf("===== \\t   !!! \\tRemove .workspace Dir Failed\\t\\t!!!   =====\\n");
        return -1;
    }

    return 0;
}

//返回指令执行结果
char *executeShell(char *command, int resultLength) {

    char *cmdResult = calloc(resultLength, sizeof(char));
    FILE *fp = NULL;

    fp = popen(command, "r");

    if (fp == NULL) {
        printf("popen error!\n");
        return NULL;
    }

    int i = 0;
    while (!feof(fp) && i < resultLength) {
        int ch;
        if ((ch = fgetc(fp)) != EOF) {
            *(cmdResult + i) = (char) ch;
            i++;
        }
    }
    pclose(fp);

    //去除换行符
    if (cmdResult[i - 1] == '\n')
        cmdResult[i - 1] = '\0';

    return cmdResult;
}

//@return
//-1：不是正常文件； 0：无法判断； 1：是正常文件
int isNormalFile(char *filePath) {
    //若路径为空
    if (!filePath)
        return 0;
    if (access(filePath, F_OK) == -1) {//若文件不存在
        return -1;
    } else {//若文件存在
        struct stat fileStat;
        if (stat(filePath, &fileStat) < 0) { //若获取stat失败
            return 0;
        } else { //若获取stat成功
            if (fileStat.st_size == 0) { //文件大小为0
                return -1;
            } else { //文件大小不为0
                return 1;
            }
        }
    }
}

//@return
//-1：下载文件失败； 0：下载文件成功
int isDownloadCorrect(char *downloadCMD, char *filePath) {
    if (system(downloadCMD) == -1) {
        printf("===== \t   !!! \tDownloading %s Failed\t\t!!!   =====\n", filePath);
        return -1;
    } else {
        //文件大小为0
        if (isNormalFile(filePath) != 1) {
            printf("===== \t   !!! \tDownloading %s Failed\t\t!!!   =====\n", filePath);
            return -1;
        }
    }
    return 0;
}