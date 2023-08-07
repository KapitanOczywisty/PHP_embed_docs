#include <main/php.h>
#include <ext/standard/php_var.h>
#include <sapi/embed/php_embed.h>

int main(int argc, char **argv)
{
	PHP_EMBED_START_BLOCK(argc, argv)

	zval retval;

	// this should store int(6) in retval, right?
	if (zend_eval_stringl(ZEND_STRL("$a = 3;return $a * 2;"), &retval, "embed eval code") == FAILURE)
	{
		php_printf("Failed to eval PHP.\n");
	}
	else
	{
		// retval has int(3), that's unexpected...
		php_var_dump(&retval, 0);
	}

	PHP_EMBED_END_BLOCK()
}
