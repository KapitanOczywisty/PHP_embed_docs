#include <sapi/embed/php_embed.h>

int main(int argc, char **argv)
{
	PHP_EMBED_START_BLOCK(argc, argv)

	if (zend_eval_stringl(ZEND_STRL("echo 'This message is produced by ' . php_sapi_name() . ' SAPI' . PHP_EOL;"), NULL, "embed eval code") == FAILURE)
	{
		php_printf("Failed to eval PHP.\n");
	}

	PHP_EMBED_END_BLOCK()
}
