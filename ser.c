#include <stdio.h>
#include <unistd.h>
#include <sqlite3.h>
#include <strings.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/wait.h>
#include <pthread.h>
#include "ser.h"
typedef void (*sighandler_t)(int);

void handler(int sig)
{
    while (waitpid(-1, NULL, 0) > 0)
        ;
}

int main(int argc, char const *argv[])
{
    //创建数据库
    sqlite3 *db = load_sql("./sql.db");
    if (NULL == db)
        PRINT("load_sql error\n", -1);

    //创建socker连接
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd < 0)
        PRINT_ERR("socket", -1);

    int reuse = 1;
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    if (bindaddr(sfd, PORT, IP) < 0)
        PRINT("bindaddr error\n", -1);
    if (listen(sfd, 100) < 0)
        PRINT_ERR("listen", -1);

    signal(17, handler); // 回收子进程

    char buff[200] = "";
    int res;
    int newfd;
    while (1)
    {
        newfd = accept(sfd, NULL, NULL);
        if (fork())
        { // 主线程处理分支
            close(newfd);
        }
        else //  子线程通话
        {
            close(sfd);

            char client[20] = "";
            while (1)
            {
                memset(buff, 5, sizeof(buff));
                printf("////////\n");
                res = recv(newfd, buff, sizeof(buff), 0); // 接受客户端数据
                if (0 > res)
                    PRINT_ERR("recv 子线程数据", -1);
                else if (0 == res)
                {
                    underline(client, db);
                    printf("%s已下线\n", client);
                    return 0;
                }
                if (0 == buff[0]) // 注册
                {
                    do_register(newfd, buff + 1, db);
                }
                else if (1 == buff[0]) // 登录
                {
                    res = do_login(newfd, buff + 1, db, client);
                }
                else if (2 == buff[0]) // 单词翻译
                {
                    res = do_translate(newfd, buff + 1, db, client);
                }
                else if (3 == buff[0]) // 查看历史记录
                {
                    res = do_history(newfd, buff + 1, db, client);
                }
                else if (4 == buff[0])
                {
                    underline(client, db);
                    printf("%s已下线\n", client);
                    return 0;
                }
                else
                    printf("程序出错\n");

                if (res < 0)
                {
                    underline(client, db);
                    printf("%s已下线\n", client);
                    return 0;
                }
            }
        }
    }

    close(sfd);
    sqlite3_close(db);
    return 0;
}