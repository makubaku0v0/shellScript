#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

//返回指令执行结果
char *executeShell(char *command, int resultLength);

int isNormalFile(char *filePath);

int main(int argc, char **argv) {
    //提示输出
    printf("请按如下格式输入检测程序启动参数\n");
    printf("IP Port\n");
    printf("启动指令示例:./linux-terminal-detection 127.0.0.1 8086\n");

    //判断输入参数格式是否正确
    if (argc != 3) {
        printf("未按正确格式输入启动参数\n");
        return -1;
    }

    //获取参数ip
    char hostIP[16] = "";
    if (strlen(argv[1]) > 16) {
        printf("输入IP格式错误\n");
        return -1;
    }
    strncpy(hostIP, argv[1], strlen(argv[1]));

    //获取参数port
    char port[6] = "";
    if (strlen(argv[2]) > 6) {
        printf("输入端口格式错误\n");
        return -1;
    }
    strncpy(port, argv[2], strlen(argv[2]));
    printf("\n主机IP和端口号为：http://%s:%s\n", hostIP, port);

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
            printf("\n判定文件 %s 存在\n", releaseFiles[iter]);
            break;
        }
        iter++;
    }
    if (iter == 3) {
        printf("未能找到判定文件");
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
        printf("Obtaining Linux Version Failed");
        return -1;
    }

    //对redhat进行特殊处理
    if (strcmp(os, "red") == 0) {
        free(os);
        os = calloc(128, sizeof(char));
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
    printf("=====\t       \t  Creating and Entering Workspace\t\t\t=====\n");
    //若.workspace目录存在则删除
    if ((access(".workspace", F_OK)) != -1) {
        if (system("rm -rf .workspace") == -1) {
            printf("===== \t   !!! \tRemove exist .workspace Failed\t\t!!!   =====\n");
            return -1;
        }
    }
    //创建.workspace目录
    if (mkdir(".workspace", S_IRUSR | S_IWUSR) == -1) {
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
    char dowmloadBinaryCMD[1024] = "";
    snprintf(dowmloadBinaryCMD, 1024, "curl -o %s http://%s:%s/terminal/terminal-detect/%s", binaryFile, hostIP,
             port, binaryFile);

    //下载失败
    if (system(dowmloadBinaryCMD) == -1) {
        printf("===== \t   !!! \tDownloading Binary File Failed\t\t!!!   =====\n");
        return -1;
    } else {
        //下载失败
        if (isNormalFile(binaryFile) != 1) {
            printf("===== \t   !!! \tDownloading Binary File Failed\t\t!!!   =====\n");
            return -1;
        } else { //下载成功，则加权限
            char chmodCMD[128] = "";
            snprintf(chmodCMD, 128, "chmod +x %s", binaryFile);
            if (system(chmodCMD) == -1) {
                printf("===== \t   !!! \tChmod +x Binary File Failed\t\t!!!   =====\n");
                return -1;
            }
        }
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
