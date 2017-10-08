#include "lemon.h"
#include "larray.h"
#include "lstring.h"
#include "lmodule.h"

#include <pcre.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct lpcre {
	struct lobject object;

	pcre *compiled;
	pcre_extra *extra;
};

struct lobject *
lpcre_exec(struct lemon *lemon, struct lobject *self, int argc, struct lobject *argv[])
{
	int i;
	int rc;
	int offsets[30];
	const char *cstr;
	const char *match;
	struct lpcre *lpcre;
	struct lobject *array;

	lpcre = (struct lpcre *)self;

	cstr = lstring_to_cstr(lemon, argv[0]);
	rc = pcre_exec(lpcre->compiled, lpcre->extra, cstr, strlen(cstr), 0, 0, offsets, 30);

	array = larray_create(lemon, 0, NULL);
	if (rc >= 0) {
		if (rc == 0) {
			rc = 30 / 3;
		}

		for (i = 0; i < rc; i++) {
			struct lobject *string;

			pcre_get_substring(cstr, offsets, rc, i, &match);
			string = lstring_create(lemon, match, strlen(match));
			larray_append(lemon, array, 1, &string);
		}

		pcre_free_substring(match);
	}

	return array;
}

struct lobject *
lpcre_get_attr(struct lemon *lemon, struct lobject *self, struct lobject *name)
{
        const char *cstr;

        cstr = lstring_to_cstr(lemon, name);
        if (strcmp(cstr, "exec") == 0) {
                return lfunction_create(lemon, name, self, lpcre_exec);
        }

        return NULL;
}

struct lobject *
lpcre_method(struct lemon *lemon,
             struct lobject *self,
             int method, int argc, struct lobject *argv[])
{
#define cast(a) ((struct lpcre *)(a))        

        switch (method) {   
        case LOBJECT_METHOD_GET_ATTR:                    
                return lpcre_get_attr(lemon, self, argv[0]); 

        case LOBJECT_METHOD_DESTROY:
		pcre_free(cast(self)->compiled);
		if (cast(self)->extra) {
			pcre_free(cast(self)->extra);
		}
                return NULL;

        default:
                return lobject_default(lemon, self, method, argc, argv);
        }
}

struct lobject *
lpcre_compile(struct lemon *lemon, struct lobject *self, int argc, struct lobject *argv[])
{
	int offset;
	pcre *compiled;
	pcre_extra *extra;
	const char *error;
	const char *cstr;

	struct lpcre *lpcre;

	cstr = lstring_to_cstr(lemon, argv[0]);

	compiled = pcre_compile(cstr, 0, &error, &offset, NULL);
	if (compiled == NULL) {
		printf("ERROR: Could not compile '%s': %s\n", cstr, error);

		return NULL;
	}

	extra = pcre_study(compiled, 0, &error);
	if (error != NULL) {
		printf("ERROR: Could not study '%s': %s\n", cstr, error);

		return NULL;
	}

	lpcre = lobject_create(lemon, sizeof(*lpcre), lpcre_method);
	lpcre->compiled = compiled;
	lpcre->extra = extra;

	return (struct lobject *)lpcre;
}

struct lobject *
pcre_module(struct lemon *lemon)
{
        struct lobject *name;
        struct lobject *module;

        module = lmodule_create(lemon, lstring_create(lemon, "pcre", 4));

        name = lstring_create(lemon, "compile", 7);
        lobject_set_attr(lemon,
                         module,
                         name,
                         lfunction_create(lemon, name, NULL, lpcre_compile));

        return module;
}
