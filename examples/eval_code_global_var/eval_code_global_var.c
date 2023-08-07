#include <main/php.h>
#include <ext/standard/php_var.h>
#include <sapi/embed/php_embed.h>

int main(int argc, char **argv)
{
	PHP_EMBED_START_BLOCK(argc, argv)

	// variables in global scope can be accesed later
	if (zend_eval_stringl(ZEND_STRL("$a = 3;$a *= 2;"), NULL, "embed eval code") == FAILURE)
	{
		php_printf("Failed to eval PHP.\n");
	}
	else
	{
		zval *result;
		zend_string *var_name = zend_string_init(ZEND_STRL("a"), 0);

		// find global variable $a
		result = zend_hash_find(&EG(symbol_table), var_name);
		if (result == NULL)
		{
			php_printf("Failed to find variable.\n");
		}
		else
		{
			php_var_dump(result, 0);
		}

		zend_string_release(var_name);
	}

	PHP_EMBED_END_BLOCK()
}
