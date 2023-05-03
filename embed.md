# The embed SAPI

A server application programming interface (SAPI) is the entry point into the Zend Engine. The embed SAPI is a lightweight SAPI for calling into the Zend Engine from C or other languages that have C bindings.

## Basic Example

Below is a basic example in C that uses the embed SAPI to boot up the Zend Engine, start a request, and print the number of functions loaded in the function table.

```c
/* examples/embed_sapi_basic/embed_sapi_basic.c */

#include <sapi/embed/php_embed.h>

int main(int argc, char **argv)
{
	/* Invokes the Zend Engine initialization phase: SAPI (SINIT), modules
	 * (MINIT), and request (RINIT). It also opens a 'zend_try' block to catch
	 * a zend_bailout().
	 */
	PHP_EMBED_START_BLOCK(argc, argv)

	php_printf(
		"Number of functions loaded: %d\n",
		zend_hash_num_elements(EG(function_table))
	);

	/* Close the 'zend_try' block and invoke the shutdown phase: request
	 * (RSHUTDOWN), modules (MSHUTDOWN), and SAPI (SSHUTDOWN).
	 */
	PHP_EMBED_END_BLOCK()
}
```

To compile this, we must point the compiler to the PHP header files. The paths to the header files are listed from `php-config --includes`.

We must also point the linker and the runtime loader to the `libphp.so` shared lib for linking PHP (`-lphp`) which is located at `$(php-config --prefix)/lib`. So the complete command to compile ends up being:

```bash
$  gcc \
	$(php-config --includes) \
	-L$(php-config --prefix)/lib \
	embed_sapi_basic.c \
	-lphp \
	-Wl,-rpath=$(php-config --prefix)/lib \
	-o main.out
```

> :memo: The embed SAPI is disabled by default. In order for the above example to compile, PHP must be built with the embed SAPI enabled. To see what SAPIs are installed, run `php-config --php-sapis`. If you don't see `embed` in the list, you'll need to rebuild PHP with `./configure --enable-embed`. The PHP shared library `libphp.so` is built when the embed SAPI is enabled.

If all goes to plan you should be able to run the program.

```bash
$ ./a.out
Number of functions loaded: 1046
```

> :memo: To properly use embed SAPI you need to know some basics of the Zend Engine. In this document we will focus on SAPI details, please refer to [PHP Internals Book](https://www.phpinternalsbook.com/) for information about the Zend Engine.

## Calling PHP functions

PHP provides quite a few internal functions which can be used from SAPI. In the first example we called `php_printf` function, another useful one is `php_var_dump`, which can be used to dump the contents of a `zval` to stdout just like `var_dump`.

```c
/* examples/function_call_internal/function_call_internal.c */

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
```

Example output:

```bash
$ ./a.out
int(8)
```


This program can be compared to `var_dump(random_int(5, 10))`, with only difference that we didn't parse any PHP code yet. Also as you can see, some internal functions are not operating on the `zval`s, but only on their internal values, in this case `php_random_int` is expecting `zend_long`. `Z_LVAL` macro is used to get the `zend_long` value of the `zval`, we can use it to store function result in `zval`.

> :link: Read more about `zval`s: https://www.phpinternalsbook.com/php7/zvals.html

Let's try now to call a PHP function instead.

```c
/* examples/function_call_basic/function_call_basic.c */

// ...
	zval retval;

	// structures to store function info
	zend_fcall_info fci;
	zend_fcall_info_cache fcc;

	// define function name
	zval func_name;
	ZVAL_STRING(&func_name, "php_sapi_name");

	// initialize fci and fcc with supplied function name
	if (zend_fcall_info_init(&func_name, 0, &fci, &fcc, NULL, NULL) == FAILURE)
	{
		php_printf("Error initializing function call info\n");
	}
	else
	{
		// set return zval
		fci.retval = &retval;

		if (zend_call_function(&fci, &fcc) == SUCCESS)
		{
			php_var_dump(&retval, 1);
		}
	}

	zval_ptr_dtor(&retval);
	zval_ptr_dtor(&func_name);
// ...
```

Example output:

```bash
$ ./a.out
string(5) "embed"
```

Again we could compare above code to `var_dump(php_sapi_name())`. This time we're using string to call function, name can be built-in function or previously defined user-land function.

> :link: Read more about `zend_fcall_info` and `zend_fcall_info_cache`: https://www.phpinternalsbook.com/php7/internal_types/functions/callables.html

Some functions require arguments, we can pass them in `zend_fcall_info` structure.

```c
/* examples/function_call_params/function_call_params.c */

// ...
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
// ...
```

Example output:

```bash
$ ./a.out
string(11) "-=-=PHP-=-="
```

To get the same result we could write `var_dump(str_pad("PHP", 11, "-=", STR_PAD_BOTH))`. To provide parameters we allocated `params` property with size of 4 `zval`s and populated them with values, for the last parameter we're using `zend_get_constant_str` to get `zval` of `STR_PAD_BOTH` constant. Built-in constants are often defined in header files, in this example we could write `ZVAL_LONG(&fci.params[3], PHP_STR_PAD_BOTH)` (constant defined in `ext/standard/php_string.h`).


# WIP
## Parsing PHP code
## Parsing PHP files
## Error handling
## Multiple requests
## Input / output (+request_info)
## INI settings
## SAPI functions and constants (using zend_module_entry)
## ZTS
## Opcache
