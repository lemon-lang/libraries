#include "lemon.h"
#include "larray.h"
#include "lstring.h"
#include "lmodule.h"
#include "connection.h"
#include <stdio.h>
#include <mysql.h>

struct lobject *
lmysql_connect(struct lemon *lemon, struct lobject *self, int argc, struct lobject *argv[])
{
	MYSQL *conn;
	char query[256];
	const char *host;
	const char *username;
	const char *password;
	const char *database;

	if (argc != 4) {
		return lobject_error_argument(lemon, "connect() require host, username and password");
	}

	conn = mysql_init(NULL);
	if (!conn) {
		fprintf(stderr, "%s\n", mysql_error(conn));

		return NULL;
	}

	host = lstring_to_cstr(lemon, argv[0]);
	username = lstring_to_cstr(lemon, argv[1]);
	password = lstring_to_cstr(lemon, argv[2]);
	database = lstring_to_cstr(lemon, argv[3]);

	if (!mysql_real_connect(conn, host, username, password, NULL, 0, NULL, 0)) {
		fprintf(stderr, "%s\n", mysql_error(conn));
		mysql_close(conn);

		return NULL;
	}
	snprintf(query, sizeof(query), "USE %s", database);
	if (mysql_query(conn, query) != 0) {
		fprintf(stderr, "%s\n", mysql_error(conn));
		mysql_close(conn);

		return NULL;
	}

	return lmysql_connection_create(lemon, conn);
}

struct lobject *
mysql_module(struct lemon *lemon)
{
        struct lobject *name;
        struct lobject *module;

        module = lmodule_create(lemon, lstring_create(lemon, "mysql", 5));

        name = lstring_create(lemon, "connect", 7);
        lobject_set_attr(lemon,
                         module,
                         name,
                         lfunction_create(lemon, name, NULL, lmysql_connect));

	printf("MySQL client version: %s\n", mysql_get_client_info());

	return module;
}
