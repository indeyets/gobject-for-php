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
#include <zend_exceptions.h>

#include <ext/spl/spl_exceptions.h>

#include "php_gobject.h"



void static gobject_girepository_load_class(GIObjectInfo *info TSRMLS_DC)
{
	char *phpclass = namespaced_name(g_base_info_get_namespace(info), g_base_info_get_name(info));

	php_printf("-> class %s => %s\n", phpclass, g_object_info_get_type_name(info));

	// Is it loaded?
	zend_class_entry *class_ce = zend_fetch_class(phpclass, strlen(phpclass), ZEND_FETCH_CLASS_NO_AUTOLOAD TSRMLS_CC);

	if (class_ce) {
		// already registered
		efree(phpclass);
		return;
	}

	// Load parent
	GIObjectInfo *parent_info = g_object_info_get_parent(info);
	gobject_girepository_load_class(parent_info TSRMLS_CC);

	char *php_parent_name = namespaced_name(g_base_info_get_namespace(parent_info), g_base_info_get_name(parent_info));
	g_base_info_unref(parent_info);

	{
		zend_class_entry *parent_ce = zend_fetch_class(php_parent_name, strlen(php_parent_name), ZEND_FETCH_CLASS_NO_AUTOLOAD TSRMLS_CC);

		if (NULL == parent_ce) {
			zend_throw_exception_ex(spl_ce_OutOfBoundsException, 0 TSRMLS_CC, "Parent PHP class (%s) is not registered", php_parent_name);
			efree(php_parent_name);
			return;
		}
		efree(php_parent_name);

		zend_class_entry ce;

		INIT_CLASS_ENTRY_EX(ce, phpclass, strlen(phpclass), NULL);
		efree(phpclass);

		zend_class_entry *target = zend_register_internal_class_ex(&ce, parent_ce, NULL TSRMLS_CC);
		target->create_object  = gobject_gobject_object_new;
	}
}


PHP_FUNCTION(GIRepository_load_ns)
{
	char *ns_name = NULL, *version = NULL;
	int ns_name_len = 0, version_len = 0;
	long flags = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|sl", &ns_name, &ns_name_len, &version, &version_len, &flags) == FAILURE) {
		return;
	}

	GIRepository *gir = GOBJECT_G(gir);

	// 1. Check if loaded
	if (g_irepository_is_registered(gir, ns_name, version))
		RETURN_TRUE;

	// 2. Load
	GError *err = NULL;
	GITypelib *type_lib = g_irepository_require(gir, ns_name, version, 0, &err);

	if (err) {
		zend_throw_exception_ex(spl_ce_OutOfBoundsException, 0 TSRMLS_CC, "Failed to load \"%s\" namespace: %s", ns_name, err->message);
		g_error_free(err);
		return;
	}

	// 3. store type_lib in array of namespaces, which should be unloaded
	// TODO

	// 4. Init functions, classes, …
	gint n = g_irepository_get_n_infos(gir, ns_name);
	for (gint i = 0; i < n; i++) {
		GIBaseInfo *b_info = g_irepository_get_info(gir, ns_name, i);
		GIInfoType info_type = g_base_info_get_type(b_info);

		switch (info_type) {
			case GI_INFO_TYPE_OBJECT:
				gobject_girepository_load_class(b_info TSRMLS_CC);
			break;

			case GI_INFO_TYPE_TYPE:
				php_printf("-> type %s\n", g_base_info_get_name(b_info));
			break;

			case GI_INFO_TYPE_STRUCT:
				php_printf("-> struct %s\n", g_base_info_get_name(b_info));
			break;

			case GI_INFO_TYPE_ENUM:
				php_printf("-> enumeration %s\n", g_base_info_get_name(b_info));
			break;

			default:
				php_printf("-> ??? %s\n", g_base_info_get_name(b_info));
			break;
		}
		g_base_info_unref(b_info);
	}
	php_printf("\n");
	// TODO

	php_printf("Hello %s\n", ns_name);
	RETURN_TRUE;
}


PHP_RINIT_FUNCTION(gobject_girepository)
{
	return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(gobject_girepository)
{
	return SUCCESS;
}
