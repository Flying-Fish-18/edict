#include <stdio.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <time.h>
#include <sqlite3.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include "ser.h"
sqlite3 *load_sql(char *path) // 本地数据库加载，成功返回数据库指针
{
    sqlite3 *db = NULL;
    // char *errmsg = NULL;
    if (sqlite3_open(path, &db) != SQLITE_OK)
        PRINT_SQL("sql open", NULL);
    char opr[100] = "create table if not exists wordbox (word char,mean char);";
    if (sqlite3_exec(db, opr, NULL, NULL, NULL) != SQLITE_OK)
        PRINT_SQL("sql exec", NULL);
    strcpy(opr, "create table if not exists user (username char,passwd char,status int);");
    if (sqlite3_exec(db, opr, NULL, NULL, NULL) != SQLITE_OK)
        PRINT_SQL("sql exec", NULL);
    strcpy(opr, "create table if not exists history (word char,time char,username char);");
    if (sqlite3_exec(db, opr, NULL, NULL, NULL) != SQLITE_OK)
        PRINT_SQL("sql exec", NULL);

    return db;
}

// socket绑定ip、port
int bindaddr(int sfd, int port, char *ip)
{
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);
    if (bind(sfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        PRINT_ERR("bind", -1);

    return 0;
}

int do_register(int newfd, char *username, sqlite3 *db)
{
    char buff[100] = "";
    char opr[100] = "";
    char **p_res;
    int prow;
    int pcol;
    // 查重用户名
    sprintf(opr, "select * from user where username = \"%s\";", username);
    // printf("%s,%s\n",username,opr);
    if (sqlite3_get_table(db, opr, &p_res, &prow, &pcol, NULL) != SQLITE_OK)
        PRINT_SQL("select username from user\n", -1);
    if (0 == prow) // 用户名不存在
    {
        buff[0] = 1;
        send(newfd, buff, sizeof(buff), 0);
    }
    else
    {
        buff[0] = 0;
        send(newfd, buff, sizeof(buff), 0);
        return 0; // 退出函数，让用户重新输入用户名
    }

    char passwd[20] = "";
    int res;

    res = recv(newfd, passwd, sizeof(passwd), 0); // 接受客户端密码
    // printf("%s\n",passwd);
    if (0 > res)
        PRINT_ERR("recv passwd", -1);
    else if (0 == res)
        PRINT("已下线\n", -2);
    else
        printf("%s\n", buff);

    // 插入数据
    sprintf(opr, "insert into user values (\"%s\",\"%s\",0)", username, passwd);
    if (sqlite3_exec(db, opr, NULL, NULL, NULL) != SQLITE_OK)
        PRINT_SQL("sql insert user", -1);
    buff[0] = 1;
    if (send(newfd, buff, sizeof(buff), 0) < 0)
        PRINT_ERR("send passwd", -1);
    // printf("%s\n",opr);

    return 0;
}

int do_login(int newfd, char *user, sqlite3 *db, char *client)
{
    // printf("%s\n",user);
    char username[20] = "";
    char passwd[20] = "";
    char buff[100] = "";
    char opr[100] = "";

    strcpy(username, user); // 复制用户名
    // printf("%s\n",username);
    strcpy(passwd, user + strlen(username) + 1); // 复制密码
    // printf("%s\n",passwd);

    char **p_res;
    int prow;
    int pcol;
    sprintf(opr, "select * from user where username = \"%s\" and passwd = \"%s\";", username, passwd);
    if (sqlite3_get_table(db, opr, &p_res, &prow, &pcol, NULL) != SQLITE_OK)
        PRINT_SQL("select username from user\n", -1);
    if (0 == prow) // 用户不存在
    {
        buff[0] = 0;
        send(newfd, buff, sizeof(buff), 0);
    }
    else  // 账号密码正确
    {
        sprintf(opr, "select * from user where username = \"%s\" and passwd = \"%s\" and status = 1;", username, passwd);
        if (sqlite3_get_table(db, opr, &p_res, &prow, &pcol, NULL) != SQLITE_OK)
            PRINT_SQL("select username from user\n", -1);
        if (0 < prow)
        {
            buff[0] = 2;
            send(newfd, buff, sizeof(buff), 0);
        }
        else
        {
            buff[0] = 1;
            send(newfd, buff, sizeof(buff), 0);
            strcpy(client, username);
            sprintf(opr, "update user set status = 1 where username = \"%s\" and passwd = \"%s\";", username, passwd);
            if (sqlite3_get_table(db, opr, &p_res, &prow, &pcol, NULL) != SQLITE_OK)
                PRINT_SQL("select username from user\n", -1);
            // printf("%s\n",username);
        }
    }
    return 0;
}

int do_translate(int newfd, char *data, sqlite3 *db, char *client)
{
    char buff[150] = "";
    char word[50] = "";
    char mean[100] = "";
    char opr[200] = "";

    strcpy(word, data); // 用户上传的单词

    char **p_res;
    int prow;
    int pcol;
    sprintf(opr, "select mean from wordbox where word = \"%s\";", word);
    if (sqlite3_get_table(db, opr, &p_res, &prow, &pcol, NULL) != SQLITE_OK)
        PRINT_SQL("select word and mean from wordbox\n", -1);
    if (0 == prow) // 单词不存在
    {
        buff[0] = 0;
        send(newfd, buff, sizeof(buff), 0);
    }
    else // 发送单词和翻译
    {
        sprintf(buff, "%s%c%s", word, 0, p_res[1]);
        for (int i = 2; i <= prow; i++)
        {
            sprintf(buff + strlen(word) + strlen(p_res[1]) + 1, "%c%s", ' ', p_res[i]);
        }

        if (send(newfd, buff, sizeof(buff), 0) < 0)
            PRINT_ERR("send translate", -1);
    }

    insert_history(word, db, client);

    return 0;
}

int insert_history(char *word, sqlite3 *db, char *client)
{
    // 获取时间
    time_t now = time(NULL);
    struct tm *tim = localtime(&now);
    char time[50] = ""; // 时间表
    sprintf(time, "%d.%d.%d  %d:%d:%d", tim->tm_year + 1900, tim->tm_mon + 1, tim->tm_mday, tim->tm_hour, tim->tm_min, tim->tm_sec);

    char opr[200] = "";
    sprintf(opr, "insert into history values (\"%s\",'%s',\"%s\");", word, time, client);
    if (sqlite3_exec(db, opr, NULL, NULL, NULL) != SQLITE_OK)
        PRINT_SQL("sql insert history", -1);
}

int do_history(int newfd, char *history, sqlite3 *db, char *client)
{
    char buff[800] = "";
    char temp[100] = "";
    char opr[200] = "";
    char **p_res;
    int prow;
    int pcol;
    sprintf(opr, "select word,time from history where username = \"%s\";", client);
    if (sqlite3_get_table(db, opr, &p_res, &prow, &pcol, NULL) != SQLITE_OK)
        PRINT_SQL("select history from user\n", -1);

    if (1 == prow || 0 == prow)
    {
        buff[0] = 0;
     
        if (send(newfd, buff, 2, 0) < 0)
            PRINT_ERR("send history", -1);
    }
    else
    {
        sprintf(temp, "\t\t%s\t%s\n", p_res[0], p_res[1]);
        sprintf(buff, "%s\t%s\n", temp, "-------------------------------------");
        for (int i = 2; i <= pcol * (1 + prow) - 1; i = i + 2)
        {
            sprintf(temp, "\t\t%s\t%s\n", p_res[i], p_res[i + 1]);
            strcat(buff, temp);
        }
        if (send(newfd, buff, strlen(buff) + 1, 0) < 0)
            PRINT_ERR("send history", -1);
    }
    return 0;
}

int underline(char *username, sqlite3 *db)
{
    char opr[200] = "";
    sprintf(opr, "update user set status=0 where username=\"%s\";", username);
    // printf("%s\n", opr);
    if (sqlite3_exec(db, opr, NULL, NULL, NULL) != SQLITE_OK)
        PRINT_SQL("select username from user\n", -1);

    return 0;
}
