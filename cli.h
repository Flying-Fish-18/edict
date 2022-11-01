#ifndef __FUNCLI_H__
#define __FUNCLI_H__
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


int connectaddr(int sfd);
int menu();
int do_register(int sfd);
int do_login(int sfd);
int option(int sfd);
int do_translate(int sfd);
int do_history(int sfd);
#endif