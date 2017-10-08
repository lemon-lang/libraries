#ifndef LEMON_MYSQL_CURSOR_H
#define LEMON_MYSQL_CURSOR_H

#include "lobject.h"

#include <mysql.h>

struct lmysql_cursor {
	struct lobject object;

	struct lobject *description;
	MYSQL_RES *meta;
	MYSQL_STMT *stmt;
};

void *
lmysql_cursor_create(struct lemon *lemon, MYSQL_STMT *stmt);

#endif /* LEMON_MYSQL_CURSOR_H */
