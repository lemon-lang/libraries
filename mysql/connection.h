#ifndef LEMON_MYSQL_CONNECTION_H
#define LEMON_MYSQL_CONNECTION_H

#include "lobject.h"

#include <mysql.h>

struct lmysql_connection {
        struct lobject object;

        MYSQL *mysql;
};

void *
lmysql_connection_create(struct lemon *lemon, MYSQL *mysql);

#endif /* LEMON_MYSQL_CONNECTION_H */
