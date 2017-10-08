#include "lemon.h"
#include "larray.h"
#include "lstring.h"
#include "lmodule.h"
#include "linteger.h"
#include "cursor.h"

#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <mysql.h>

struct lobject *
lmysql_cursor_execute(struct lemon *lemon, struct lobject *self, int argc, struct lobject *argv[])
{
	int i;
	int count;
	MYSQL_RES *meta;
	MYSQL_STMT *stmt;
	MYSQL_BIND *bind;
	const char *query;
	my_bool max_length;
	struct lmysql_cursor *cursor;

	if (argc < 1 || !lobject_is_string(lemon, argv[0])) {
		return lobject_error_argument(lemon, "execute() require 1 query string");
	}
	query = lstring_to_cstr(lemon, argv[0]);

	cursor = (struct lmysql_cursor *)self;
	stmt = cursor->stmt;
	if (mysql_stmt_prepare(stmt, query, strlen(query))) {
		fprintf(stderr, "mysql_stmt_prepare() %s\n", mysql_stmt_error(stmt));

		return NULL;
	}

	count = mysql_stmt_param_count(stmt);
	if (count > argc - 1) {
		return lobject_error_argument(lemon, "execute() arguments not match parameter");
	}

	max_length = 1;
	mysql_stmt_attr_set(stmt, STMT_ATTR_UPDATE_MAX_LENGTH, &max_length);

	meta = mysql_stmt_result_metadata(stmt);
	cursor->description = lemon->l_nil;
	if (meta) {
		int nfields;
		MYSQL_FIELD *fields;
		struct lobject *field;
		struct lobject *description;

		fields = mysql_fetch_fields(meta);
		nfields = mysql_num_fields(meta);
		description = larray_create(lemon, 0, NULL);
		for (i = 0; i < nfields; i++) {
			struct lobject *items[2];

			items[0] = lstring_create(lemon, fields[i].name, strlen(fields[i].name));
			items[1] = linteger_create_from_long(lemon, fields[i].max_length);
			field = larray_create(lemon, 2, items);
			larray_append(lemon, description, 1, &field);

		}
		cursor->meta = meta;
		cursor->description = description;
	}

	if (count) {
		bind = lemon_allocator_alloc(lemon, sizeof(MYSQL_BIND) * count);
		memset(bind, 0, sizeof(MYSQL_BIND) * count);

		for (i = 0; i < count; i++) {
			struct lobject *object;

			object = argv[i + 1];

			if (lobject_is_integer(lemon, object)) {
				void *buffer;
				long value;

				buffer = lemon_allocator_alloc(lemon, sizeof(long));
				value = linteger_to_long(lemon, object);
				memcpy(buffer, &value, sizeof(value));
				
				bind[i].buffer_type = MYSQL_TYPE_LONG;
				bind[i].buffer = buffer;
				bind[i].is_unsigned = 0;
				bind[i].is_null = 0;
			} else if (lobject_is_string(lemon, object)) {
				bind[i].buffer_type = MYSQL_TYPE_STRING;
				bind[i].buffer = lstring_buffer(lemon, object);
				bind[i].buffer_length = lstring_length(lemon, object);
				bind[i].is_unsigned = 0;
				bind[i].is_null = 0;
			}
		}

		if (mysql_stmt_bind_param(stmt, bind) != 0) {
			fprintf(stderr, "mysql_stmt_bind_param() %s\n", mysql_stmt_error(stmt));

			return NULL;
		}
	}

	if (mysql_stmt_execute(stmt) != 0) {
		fprintf(stderr, "mysql_stmt_execute() %s\n", mysql_stmt_error(stmt));

		return NULL;
	}

	if (count) {
		for (i = 0; i < count; i++) {
			if (bind[i].buffer_type == MYSQL_TYPE_LONG) {
				lemon_allocator_free(lemon, bind[i].buffer);
			}
		}
		lemon_allocator_free(lemon, bind);
	}

	if (mysql_stmt_store_result(stmt)) {
		fprintf(stderr, "mysql_stmt_bind_result() %s\n", mysql_stmt_error(stmt));

		return NULL;
	}

	return lemon->l_nil;
}

struct lobject *
lmysql_cursor_fetchmany(struct lemon *lemon, struct lobject *self, int argc, struct lobject *argv[])
{
	int i;
	int nfields;
	long many;
	MYSQL_RES *meta;
	MYSQL_STMT *stmt;
	MYSQL_BIND *bind;
	MYSQL_FIELD *fields;
	struct lobject *row;
	struct lobject *results;
	struct lobject **items;
	struct lmysql_cursor *cursor;

	many = LONG_MAX;
	if (argc) {
		many = linteger_to_long(lemon, argv[0]);
	}

	cursor = (struct lmysql_cursor *)self;
	stmt = cursor->stmt;
	if (!cursor->meta) {
		return lobject_error_type(lemon, "no result");
	}

	meta = cursor->meta;
	fields = mysql_fetch_fields(meta);
	nfields = mysql_num_fields(meta);
	if (!nfields) {
		return lobject_error_type(lemon, "no result");
	}
	bind = lemon_allocator_alloc(lemon, sizeof(MYSQL_BIND) * nfields);
	memset(bind, 0, sizeof(MYSQL_BIND) * nfields);

	for (i = 0; i < nfields; i++) {
		if (fields[i].type == MYSQL_TYPE_LONG) {
			bind[i].buffer_type = MYSQL_TYPE_LONG;
			bind[i].buffer = lemon_allocator_alloc(lemon, sizeof(long));
			bind[i].is_unsigned = 0;
			bind[i].is_null = 0;
		} else if (fields[i].type == MYSQL_TYPE_STRING ||
		           fields[i].type == MYSQL_TYPE_VAR_STRING)
		{
			bind[i].buffer_type = fields[i].type;
			bind[i].buffer = lemon_allocator_alloc(lemon, fields[i].max_length);
			bind[i].buffer_length = fields[i].max_length;
			bind[i].length = lemon_allocator_alloc(lemon, sizeof(unsigned long));
			bind[i].is_unsigned = 0;
			bind[i].is_null = 0;
		}
	}

	if (mysql_stmt_bind_result(stmt, bind) != 0) {
		fprintf(stderr, "mysql_stmt_bind_result() %s\n", mysql_stmt_error(stmt));

		return NULL;
	}

	results = larray_create(lemon, 0, NULL);
	items = lemon_allocator_alloc(lemon, sizeof(struct lobject *) * nfields);
	while (many--) {
		if (mysql_stmt_fetch(stmt) != 0) {
			mysql_stmt_free_result(stmt);
			mysql_free_result(meta);
			cursor->meta = NULL;

			break;
		}
		for (i = 0; i < nfields; i++) {
			struct lobject *object;
			if (bind[i].buffer_type == MYSQL_TYPE_LONG) {
				object = linteger_create_from_long(lemon, *(long *)bind[i].buffer);
			} else if (bind[i].buffer_type == MYSQL_TYPE_STRING ||
			           bind[i].buffer_type == MYSQL_TYPE_VAR_STRING)
			{
				object = lstring_create(lemon, bind[i].buffer, *bind[i].length);
			}
			items[i] = object;
		}
		row = larray_create(lemon, nfields, items);
		larray_append(lemon, results, 1, &row);
	}
	lemon_allocator_free(lemon, items);

	for (i = 0; i < nfields; i++) {
		lemon_allocator_free(lemon, bind[i].buffer);
		lemon_allocator_free(lemon, bind[i].length);
	}
	lemon_allocator_free(lemon, bind);

	return results;
}

struct lobject *
lmysql_cursor_fetchone(struct lemon *lemon, struct lobject *self, int argc, struct lobject *argv[])
{
	struct lobject *one;

	one = linteger_create_from_long(lemon, 1);

	return lmysql_cursor_fetchmany(lemon, self, 1, &one);
}

struct lobject *
lmysql_cursor_fetchall(struct lemon *lemon, struct lobject *self, int argc, struct lobject *argv[])
{
	return lmysql_cursor_fetchmany(lemon, self, 0, NULL);
}

struct lobject *
lmysql_cursor_get_attr(struct lemon *lemon,
                           struct lobject *self,
                           struct lobject *name)
{
        const char *cstr;

        cstr = lstring_to_cstr(lemon, name);
        if (strcmp(cstr, "execute") == 0) {
                return lfunction_create(lemon, name, self, lmysql_cursor_execute);
        }

        if (strcmp(cstr, "fetchone") == 0) {
                return lfunction_create(lemon, name, self, lmysql_cursor_fetchone);
        }

        if (strcmp(cstr, "fetchmany") == 0) {
                return lfunction_create(lemon, name, self, lmysql_cursor_fetchmany);
        }

        if (strcmp(cstr, "fetchall") == 0) {
                return lfunction_create(lemon, name, self, lmysql_cursor_fetchall);
        }

        if (strcmp(cstr, "description") == 0) {
		return ((struct lmysql_cursor *)self)->description;
        }

	return NULL;
}

struct lobject *
lmysql_cursor_method(struct lemon *lemon,
                     struct lobject *self,
                     int method, int argc, struct lobject *argv[])
{
#define cast(a) ((struct lmysql_cursor *)(a))

	switch (method) {
        case LOBJECT_METHOD_GET_ATTR:
                return lmysql_cursor_get_attr(lemon, self, argv[0]);

        case LOBJECT_METHOD_MARK:
		if (cast(self)->description) {
			lobject_mark(lemon, cast(self)->description);
		}
		return NULL;

        case LOBJECT_METHOD_DESTROY:
		mysql_stmt_free_result(cast(self)->stmt);
		mysql_free_result(cast(self)->meta);
		mysql_stmt_close(cast(self)->stmt);
		return NULL;

	default:
		return lobject_default(lemon, self, method, argc, argv);
	}
}

void *
lmysql_cursor_create(struct lemon *lemon, MYSQL_STMT *stmt)
{
	struct lmysql_cursor *self;

	self = lobject_create(lemon, sizeof(*self), lmysql_cursor_method);
	if (self) {
		self->stmt = stmt;
	}

	return self;
}
