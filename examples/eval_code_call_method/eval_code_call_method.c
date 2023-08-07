#include <main/php.h>
#include <ext/standard/php_var.h>
#include <sapi/embed/php_embed.h>

int main(int argc, char **argv)
{
	PHP_EMBED_START_BLOCK(argc, argv)

	zval retval;

	if (zend_eval_stringl(ZEND_STRL("new class { function SayHi(){ echo 'Hello!' . PHP_EOL; } }"), &retval, "embed eval code") == FAILURE)
	{
		php_printf("Failed to eval PHP.\n");
	}
	else
	{
		zend_fcall_info fci;
		zend_fcall_info_cache fcc;

		// define function name as [$class, "SayHi"]
		zval func_name;
		array_init(&func_name);
		add_next_index_zval(&func_name, &retval);
		add_next_index_stringl(&func_name, ZEND_STRL("SayHi"));

		if (zend_fcall_info_init(&func_name, 0, &fci, &fcc, NULL, NULL) == FAILURE)
		{
			php_printf("Error initializing function call info\n");
		}
		else
		{
			// fci.retval is required even if function doesn't return anything
			// we can reuse retval, its value was already copied to array
			fci.retval = &retval;

			if (zend_call_function(&fci, &fcc) == FAILURE)
			{
				php_printf("Error calling function\n");
			}
		}

		zval_ptr_dtor(&func_name);
	}

	zval_ptr_dtor(&retval);

	PHP_EMBED_END_BLOCK()
}
