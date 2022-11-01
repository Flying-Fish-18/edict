#ifndef __FUNSER_H__
#define __FUNSER_H__
#define IP "192.168.126.140"
#define PORT 8000
#define PRINT(msg, res) \
    do                  \
    {                   \
        printf(msg);    \
        return res;     \
    } while (0)
#define PRINT_ERR(msg, res)             \
    do                                  \
    {                                   \
        printf("line = %d,", __LINE__); \
        perror(msg);                    \
        return res;                     \
    } while (0)
#define PRINT_SQL(msg, res)                                                \
    do                                                                     \
    {                                                                      \
        printf("line = %d,sql errmsg = %s", __LINE__, sqlite3_errmsg(db)); \
        return res;                                                        \
    } while (0)


sqlite3 *load_sql(char *path);
int bindaddr(int sfd,int port,char *ip);
int do_register(int newfd, char *username, sqlite3 *db);
int do_login(int newfd,char *user, sqlite3 *db,char *client);
int do_translate(int newfd,char *data, sqlite3 *db,char *client);
int do_history(int newfd,char *history,sqlite3 *db,char *client);
int insert_history(char *word,sqlite3 *db,char *client);
int underline(char *username,sqlite3 *db);
#endif
