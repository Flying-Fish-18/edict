#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sqlite3.h>
#include <strings.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include "cli.h"
int connectaddr(int sfd)
{
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8000);
    addr.sin_addr.s_addr = inet_addr("192.168.126.140");
    if (connect(sfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        PRINT_ERR("connect", -1);

    return 0;
}

int menu()
{
    printf("-------在线电子词典------\n");
    printf("1.注册账号\n");
    printf("2.登录账号\n");
    printf("3.退出\n");

    printf("\n请选择功能: ");
    int a = 0;
    while (1)
    {
        scanf("%d", &a);
        getchar();
        if (a >= 1 && a <= 3)
            return a;
        printf("输入错误！请重新输入\n");
    }
    return 0;
}

int do_register(int sfd)
{
    int res;
    char buff[100] = "";
    char usr[20] = "";
    char passwd[20] = "";

    // 用户名设置
    printf("请设置用户名:");
    while (1)
    {
        scanf("%s", usr);
        getchar();
        sprintf(buff, "%c%s", 0, usr);
        if (send(sfd, buff, sizeof(buff), 0) < 0) // 发送数据
            PRINT_ERR("register send", -1);

        res = recv(sfd, buff, sizeof(buff), 0);
        if (0 > res) // 发送失败
            PRINT_ERR("register recv", -1);
        else if (0 == res) // 连接断开
            PRINT("连接已断开\n", -2);
        else
        {
            if (1 == buff[0])
                break;
            else
            {
                printf("用户名已存在，请重新设置用户名:");
                continue;
            }
        }
    }

    // 密码设置
    while (1)
    {
        printf("\n请设置用户密码(最多10位字符):");
        scanf("%s", passwd);
        getchar();
        if (strlen(passwd) > 10)
        {
            printf("密码最多为10位!!!\n");
            continue;
        }
        sprintf(buff, "%s", passwd);
        if (send(sfd, passwd, sizeof(passwd), 0) < 0) // 发送数据
            PRINT_ERR("register send", -1);

        res = recv(sfd, buff, sizeof(buff), 0);
        if (0 > res) // 发送失败
            PRINT_ERR("register recv", -1);
        else if (0 == res) // 连接断开
            PRINT("连接已断开\n", -2);
        else
        {
            if (1 == buff[0])
                break;
            else
            {
                printf("发生未知错误...\n");
                continue;
            }
        }
    }

    // 注册成功
    printf("\n注册成功！！\n");
    getchar();
    system("clear");
    return 0;
}

int do_login(int sfd)
{
    char username[20] = "";
    char passwd[20] = "";
    char buff[100] = "";
    int res;
 
        while (1) // 用户名输入
        {
            printf("请输入账号:");
            scanf("%s", username);
            getchar();
            if (strlen(username) > 20)
            {
                printf("\n输入用户名不合法!!\n");
                continue;
            }
            break;
        }

        while (1) // 密码输入
        {
            printf("请输入密码:");
            scanf("%s", passwd);
            getchar();
            if (strlen(passwd) > 20)
            {
                printf("\n输入密码不合法!!\n");
                continue;
            }
            break;
        }
    
        // 组数据
        sprintf(buff, "%c%s%c%s", 1, username, 0, passwd);
        if (send(sfd, buff, sizeof(buff), 0) < 0)
            PRINT_ERR("send 账号密码", -1);

        res = recv(sfd, buff, sizeof(buff), 0);
        printf("%s\n", buff);
        if (0 > res)
            PRINT_ERR("recv passwd", -1);
        else if (0 == res)
            PRINT("已下线\n", -2);
        else
            printf("%s\n", buff);

        if (1 == buff[0])
        {
            printf("登录成功！！");
            getchar();
            return 1;
        }
        else if(2 == buff[0])
        {
            printf("该账号已被登录！！\n");
        }
        else
        {
            printf("输入密码或账号有误,请重新输入!!\n");
        }
    
    getchar();

    return 0;
}

int option(int sfd)
{
    char buff[10] = {4};
    int flag;
    while (1)
    {
        system("clear");
        printf("------您已登陆----------\n");
        printf("    1.单词翻译\n");
        printf("    2.查看历史记录\n");
        printf("    3.退出程序\n");
        printf("\n请输入功能号:");
        scanf("%d",&flag);
        getchar();
        switch (flag)
        {
        case 1:
            system("clear");
            do_translate(sfd);
            break;
        case 2:
            do_history(sfd);
            break;
        case 3:
            if (send(sfd, &buff, sizeof(buff), 0) < 0)
                PRINT_ERR("send quit", -1);
            sleep(1);
            return -1;
            break;
        default:
            printf("程序出错\n");
            getchar();
        }
    }

    return 0;
}

int do_translate(int sfd)
{
    char buff[200] = "";
    char word[50] = "";
    char mean[150] = "";
    int res;
    printf("(按#号键退出)\n");
    while(1)
    {
        printf("请输入要查询的单词:");
        // scanf("%s",word);  // 输入要翻译的单词
        fgets(word,sizeof(buff),stdin);
        word[strlen(word)-1] = 0;
        if('#' == word[0])
            break;
                // 组数据
        sprintf(buff,"%c%s",2,word);  // 数据头为 2
        if(send(sfd,buff,sizeof(buff),0) < 0)
            PRINT_ERR("send word",-1);

        res = recv(sfd,buff,sizeof(buff),0);
        if(res < 0)
            PRINT_ERR("recv word and mean",-1);
        else if(0 == res)
            PRINT("对方已下线\n",-2);
        
        if(0 == buff[0])  // 单词不存在
        {
            printf("\n单词查找失败,请重新输入!!\n\n");
            continue;
        }
        strcpy(word,buff);  // 复制单词
        strcpy(mean,buff+strlen(word)+1);  // 复制翻译
        
        printf("\n\t%s\t\t%s\n\n",word,mean);
    }

    return 0;
}

int do_history(int sfd)
{
    char buff[1000] = "";
    int res;
    buff[0] = 3;
    if(send(sfd,buff,strlen(buff)+1,0) < 0)
        PRINT_ERR("send history",-1);

    res = recv(sfd,buff,sizeof(buff),0);
    if(res < 0)
        PRINT_ERR("recv history",-1);
    else if(0 == res)
        PRINT("对方已下线\n",-2);
    
    if(0 == buff[0])  // 历史记录不存在
        printf("\n无历史记录!!\n\n");
    else
        printf("%s\n",buff);
    
    getchar();
}

