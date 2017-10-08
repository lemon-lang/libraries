#include "lemon.h"
#include "larray.h"
#include "lstring.h"
#include "lmodule.h"

#include "cursor.h"
#include "connection.h"

#include <stdio.h>
#include <string.h>
#include <mysql.h>

struct lobject *           
lmysql_connection_cursor(struct lemon *lemon, struct lobject *self, int argc, struct lobject *argv[])
{
	MYSQL *mysql;
	MYSQL_STMT *stmt;

	mysql = ((struct lmysql_connection *)self)->mysql;

	stmt = mysql_stmt_init(mysql);

	return lmysql_cursor_create(lemon, stmt);
}

struct lobject *
lmysql_connection_get_attr(struct lemon *lemon,
                           struct lobject *self,
                           struct lobject *name)
{
        const char *cstr;

        cstr = lstring_to_cstr(lemon, name);
        if (strcmp(cstr, "cursor") == 0) {
                return lfunction_create(lemon, name, self, lmysql_connection_cursor);
        }

	return NULL;
}

struct lobject *
lmysql_connection_method(struct lemon *lemon,
                         struct lobject *self,
                         int method, int argc, struct lobject *argv[])
{
#define cast(a) ((struct lmysql_connection *)(a))

	switch (method) {
        case LOBJECT_METHOD_GET_ATTR:
                return lmysql_connection_get_attr(lemon, self, argv[0]);

        case LOBJECT_METHOD_DESTROY:
		mysql_close(cast(self)->mysql);
		return NULL;

	default:
		return lobject_default(lemon, self, method, argc, argv);
	}
}

void *
lmysql_connection_create(struct lemon *lemon, MYSQL *mysql)
{
	struct lmysql_connection *self;

	self = lobject_create(lemon, sizeof(*self), lmysql_connection_method);
	if (self) {
		self->mysql = mysql;
	}

	return self;
}
