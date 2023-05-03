#include <main/php.h>
#include <ext/standard/php_var.h>
#include <sapi/embed/php_embed.h>

int main(int argc, char **argv)
{
	PHP_EMBED_START_BLOCK(argc, argv)

	// initialize int
	zval retval;
	ZVAL_LONG(&retval, 0);

	// generate random int between 5 and 10, and store result in zval
	if (php_random_int(5, 10, &Z_LVAL(retval), false) == FAILURE)
	{
		php_printf("Failed to generate random int.\n");
	}
	else
	{
		// dump zval
		php_var_dump(&retval, 0);
	}

	PHP_EMBED_END_BLOCK()
}
