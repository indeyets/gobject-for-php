/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2010 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Alexey Zakhlestin <indeyets@php.net>                         |
  +----------------------------------------------------------------------+
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <php.h>
#include <zend_interfaces.h>

#include "php_gobject_private.h"

static zend_bool zhashtable_to_gvalue(HashTable *zhash, GValue *gvalue TSRMLS_DC)
{
	GValueArray *array = g_value_array_new(zend_hash_num_elements(zhash));

	HashPosition iterator;
	zval **tmp_zvalue;

	zend_hash_internal_pointer_reset_ex(zhash, &iterator);
	while (zend_hash_get_current_data_ex(zhash, (void **)&tmp_zvalue, &iterator) == SUCCESS) {
		GValue tmp_gvalue = {0,};

		if (FALSE == zval_to_gvalue(*tmp_zvalue, &tmp_gvalue, 1 TSRMLS_CC)) {
			g_value_array_free(array);
			return FALSE;
		}

		g_value_array_append(array, &tmp_gvalue);
		g_value_unset(&tmp_gvalue);

		zend_hash_move_forward_ex(zhash, &iterator);
	}

	g_value_take_boxed(gvalue, array);

	return TRUE;
}

gunichar g_utf8_to_unichar(const char *src, int len)
{
	gunichar *buffer = g_utf8_to_ucs4(src, len, NULL, NULL, NULL);

	gunichar retval = buffer[0];
	g_free(buffer);

	return retval;
}

zend_bool zval_to_gvalue(const zval *zvalue, GValue *gvalue, zend_bool init TSRMLS_DC)
{
	if (zvalue == NULL) {
		return FALSE;
	}

	GType type = 0;

	switch (Z_TYPE_P(zvalue)) {
		case IS_NULL:
			type = G_TYPE_NONE;
		break;

		case IS_BOOL:
			type = G_TYPE_BOOLEAN;
		break;

		case IS_LONG:
			type = G_TYPE_LONG;
		break;

		case IS_DOUBLE:
			type = G_TYPE_DOUBLE;
		break;

		case IS_CONSTANT:
		case IS_STRING:
			type = G_TYPE_STRING;
		break;

		case IS_CONSTANT_ARRAY:
		case IS_ARRAY:
			type = G_TYPE_VALUE_ARRAY;
		break;

		case IS_OBJECT:
			type = G_TYPE_OBJECT;
		break;

		default:
			type = 0;
		break;
	}

	return zval_with_gtype_to_gvalue(type, zvalue, gvalue, init TSRMLS_CC);
}

zend_bool zval_with_gtype_to_gvalue(GType type, const zval *zvalue, GValue *gvalue, zend_bool init TSRMLS_DC)
{
	switch (Z_TYPE_P(zvalue)) {
		case IS_NULL:
			if (init) {
				g_value_init(gvalue, G_TYPE_NONE);
			}
			break;

		case IS_BOOL:
			if (init) {
				g_value_init(gvalue, G_TYPE_BOOLEAN);
			}
			g_value_set_boolean(gvalue, Z_BVAL_P(zvalue));
			break;

		case IS_LONG:
			if (init) {
				g_value_init(gvalue, G_TYPE_LONG);
			}
			g_value_set_long(gvalue, Z_LVAL_P(zvalue));
			break;

		case IS_DOUBLE:
			if (init) {
				g_value_init(gvalue, G_TYPE_DOUBLE);
			}
			g_value_set_double(gvalue, (gfloat)Z_DVAL_P(zvalue));
			break;

		case IS_CONSTANT:
		case IS_STRING:
			if (type == G_TYPE_PARAM_UNICHAR) {
				if (init) {
					g_value_init(gvalue, G_TYPE_UINT);
				}
				gunichar char_val = g_utf8_to_unichar(Z_STRVAL_P(zvalue), Z_STRLEN_P(zvalue));
				g_value_set_uint(gvalue, char_val);
			} else {
				if (init) {
					g_value_init(gvalue, G_TYPE_STRING);
				}
				g_value_set_string(gvalue, Z_STRVAL_P(zvalue));
			}
			break;

		case IS_ARRAY:
		case IS_CONSTANT_ARRAY:
			if (init) {
				g_value_init(gvalue, G_TYPE_VALUE_ARRAY);
			}
			return zhashtable_to_gvalue(Z_ARRVAL_P(zvalue), gvalue TSRMLS_CC);
			break;

		case IS_OBJECT:
			if (!instanceof_function(Z_OBJCE_P(zvalue), gobject_ce_gobject TSRMLS_CC)) {
				// NOT GOBJECT. Don't know what to do with it
				php_error(E_WARNING, "This object is not GObject-derived. Can't convert");
				return FALSE;
			}

			gobject_gobject_object *php_gobject = __php_objstore_object(zvalue);

			if (!G_IS_OBJECT(php_gobject->gobject)) {
				php_error(E_WARNING, "Underlying object is not GObject. Shouldn't happen");
				return FALSE;
			}

			if (init) {
				g_value_init(gvalue, G_TYPE_OBJECT);
			}
			g_value_set_object(gvalue, G_OBJECT(php_gobject->gobject));

			break;

		case IS_RESOURCE:
			/* There's no way to handle resource gracefully */
			php_error(E_WARNING, "Can not convert resource to GObject");
			return FALSE;
			break;

		default:
			php_error(E_WARNING, "Unknown data type");
			return FALSE;
	}

	return TRUE;
}

zend_bool gvalue_to_zval(const GValue *gvalue, zval *zvalue TSRMLS_DC)
{
	if (!G_IS_VALUE(gvalue)) {
		return FALSE;
	}

	GType g_gtype = G_TYPE_FUNDAMENTAL(G_VALUE_TYPE(gvalue));

	return gvalue_with_gtype_to_zval(g_gtype, gvalue, zvalue TSRMLS_CC);
}

zend_bool gvalue_with_gtype_to_zval(GType type, const GValue *gvalue, zval *zvalue TSRMLS_DC)
{
	if (type == G_TYPE_OBJECT || type == G_TYPE_INTERFACE) {
		GObject *gobject = g_value_get_object(gvalue);

		if (!gobject) {
			php_error(E_ERROR, "It's gobject, but we can't get it? can't happen!");
			ZVAL_NULL(zvalue);
			return FALSE;
		}

		GType gclass_type = G_OBJECT_TYPE(gobject);

		if (gclass_type == 0)
			return FALSE;

		char *php_class_name = phpname_from_gtype(gclass_type);
		zend_class_entry *ce = zend_fetch_class(php_class_name, strlen(php_class_name), ZEND_FETCH_CLASS_NO_AUTOLOAD TSRMLS_CC);

		if (!ce) {
			php_error(E_ERROR, "Didn't find %s class", php_class_name);
			efree(php_class_name);
			ZVAL_NULL(zvalue);
			return FALSE;
		}

		efree(php_class_name);

		object_init_ex(zvalue, ce);
		gobject_gobject_object *zobj = (gobject_gobject_object *) zend_object_store_get_object(zvalue TSRMLS_CC);

		g_object_ref(gobject);
		zobj->gobject = gobject;

		return TRUE;
	} else if (type == G_TYPE_STRING || type == G_TYPE_PARAM_STRING) {
		const gchar *gstr = g_value_get_string(gvalue);

		if (gstr == NULL) {
			ZVAL_STRING(zvalue, "", 1);
		} else {
			ZVAL_STRING(zvalue, gstr, 1);
		}

		return TRUE;
	} else if (type == G_TYPE_CHAR || type == G_TYPE_PARAM_CHAR) {
		gchar val = g_value_get_char(gvalue);
		ZVAL_LONG(zvalue, val);
		return TRUE;
	} else if (type == G_TYPE_UCHAR || type == G_TYPE_PARAM_UCHAR) {
		guchar val = g_value_get_uchar(gvalue);
		ZVAL_LONG(zvalue, val);
		return TRUE;
	} else if (type == G_TYPE_INT || type == G_TYPE_PARAM_INT) {
		gint val = g_value_get_int(gvalue);
		ZVAL_LONG(zvalue, val);
		return TRUE;
	} else if (type == G_TYPE_UINT || type == G_TYPE_PARAM_UINT) {
		guint val = g_value_get_uint(gvalue);
		ZVAL_LONG(zvalue, val);
		return TRUE;
	} else if (type == G_TYPE_LONG || type == G_TYPE_PARAM_LONG) {
		glong val = g_value_get_long(gvalue);
		ZVAL_LONG(zvalue, val);
		return TRUE;
	} else if (type == G_TYPE_DOUBLE || type == G_TYPE_PARAM_DOUBLE) {
		gdouble val = g_value_get_double(gvalue);
		ZVAL_DOUBLE(zvalue, val);
		return TRUE;
	} else if (type == G_TYPE_FLOAT || type == G_TYPE_PARAM_FLOAT) {
		float val = g_value_get_float(gvalue);
		ZVAL_DOUBLE(zvalue, val);
		return TRUE;
	} else if (type == G_TYPE_BOOLEAN || type == G_TYPE_PARAM_BOOLEAN) {
		gboolean val = g_value_get_boolean(gvalue);
		ZVAL_BOOL(zvalue, val);
		return TRUE;
	} else if (type == G_TYPE_PARAM_UNICHAR) {
		gunichar val = g_value_get_uint(gvalue);
		char *buffer = ecalloc(6, sizeof(char));
		int len = g_unichar_to_utf8(val, buffer);

		ZVAL_STRINGL(zvalue, buffer, len, 0);

		return TRUE;
	}

	php_error(E_WARNING, "Don't know how to handle '%s' type. returning NULL instead", g_type_name(type));
	ZVAL_NULL(zvalue);
	return FALSE;
}

GType g_type_from_phpname(const char *name TSRMLS_DC)
{
	gchar **tokens = g_strsplit(name, "\\", 0);
	size_t parts;
	for (parts = 0; tokens[parts]; parts++) {
		;
	}

	GType type = 0;
	if (parts == 2) {
		if (g_irepository_is_registered(GOBJECT_G(gir), tokens[0], NULL)) {
			GIBaseInfo *info = g_irepository_find_by_name(GOBJECT_G(gir), tokens[0], tokens[1]);

			if (info) {
				type = g_registered_type_info_get_g_type(info);
				// php_printf("--> FOUND (type = %lu, NONE = %lu)\n", type, G_TYPE_NONE);
				g_base_info_unref(info);
			}
		}
	}

	// fallback
	if (0 == type) {
		gchar *new_name = g_strjoinv("__", tokens);
		type = g_type_from_name(new_name);
		g_free(new_name);
	}

	g_strfreev(tokens);

	return type;
}

char* namespaced_name(const char *ns_name, const char *name)
{
	char *phpname = emalloc(strlen(ns_name) + strlen(name) + 2);
	zend_sprintf(phpname, "%s\\%s", ns_name, name);

	return phpname;
}


char* phpname_from_gtype(GType type)
{
	char *phpname;

	GIBaseInfo *info = g_irepository_find_by_gtype(NULL, type);
	if (info) {
		const char *ns_name = g_base_info_get_namespace(info);
		const char *name = g_base_info_get_name(info);

		phpname = namespaced_name(ns_name, name);

		g_base_info_unref(info);
	} else {
		const gchar *gclass = g_type_name(type);
		size_t gclass_len = strlen(gclass);

		phpname = emalloc(gclass_len + 1);

		size_t pos = 0, tpos = 0;
		while (pos < gclass_len) {
			if (gclass[pos] == '_' && gclass[pos+1] == '_') {
				phpname[tpos++] = '\\';
				pos += 2;
				continue;
			}
		
			phpname[tpos++] = gclass[pos++];
		}
		phpname[tpos] = '\0';
	}

	return phpname;
}



zend_bool callback_is_empty(zend_fcall_info *fci)
{
	return (memcmp(fci, &empty_fcall_info, sizeof(zend_fcall_info)) == 0);
}
