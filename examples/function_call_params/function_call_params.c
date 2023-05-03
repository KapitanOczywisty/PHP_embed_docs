#include <main/php.h>
#include <ext/standard/php_var.h>
#include <sapi/embed/php_embed.h>

int main(int argc, char **argv)
{
	PHP_EMBED_START_BLOCK(argc, argv)

	zval retval;

	// structures to store function info
	zend_fcall_info fci;
	zend_fcall_info_cache fcc;

	// define function name
	zval func_name;
	ZVAL_STRING(&func_name, "str_pad");

	// initialize fci and fcc with supplied function name
	if (zend_fcall_info_init(&func_name, 0, &fci, &fcc, NULL, NULL) == FAILURE)
	{
		php_printf("Error initializing function call info\n");
	}
	else
	{
		// set return zval
		fci.retval = &retval;

		// add params
		fci.param_count = 4;
		fci.params = ecalloc(fci.param_count, sizeof(zval));
		ZVAL_STRING(&fci.params[0], "PHP");
		ZVAL_LONG(&fci.params[1], 11);
		ZVAL_STRING(&fci.params[2], "-=");
		fci.params[3] = *zend_get_constant_str("STR_PAD_BOTH", sizeof("STR_PAD_BOTH") - 1);

		if (zend_call_function(&fci, &fcc) == SUCCESS)
		{
			php_var_dump(&retval, 1);
		}

		// release params
		zval_ptr_dtor(&fci.params[0]);
		zval_ptr_dtor(&fci.params[1]);
		zval_ptr_dtor(&fci.params[2]);
		// no dtor for &fci.params[3]
		efree(fci.params);
	}

	zval_ptr_dtor(&retval);
	zval_ptr_dtor(&func_name);

	PHP_EMBED_END_BLOCK()
}
