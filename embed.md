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

		zval_ptr_dtor(&retval);
	}

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
		fci.params[3] = *zend_get_constant_str(ZEND_STRL("STR_PAD_BOTH"));

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

		zval_ptr_dtor(&retval);
// ...
```

Example output:

```bash
$ ./a.out
string(11) "-=-=PHP-=-="
```

To get the same result we could write `var_dump(str_pad("PHP", 11, "-=", STR_PAD_BOTH))`. To provide parameters we allocated `params` property with size of 4 `zval`s and populated them with values, for the last parameter we're using `zend_get_constant_str` to get `zval` of `STR_PAD_BOTH` constant. Built-in constants are often defined in header files, in this example we could write `ZVAL_LONG(&fci.params[3], PHP_STR_PAD_BOTH)` (constant defined in `ext/standard/php_string.h`). `ZEND_STRL` is a simple macro to pass string and size as 2 parameters.

## Parsing PHP code

Time to start parsing some PHP code.

```c
/* examples/eval_code_basic/eval_code_basic.c */

// ...
	if (zend_eval_stringl(ZEND_STRL("echo 'This message is produced by ' . php_sapi_name() . ' SAPI' . PHP_EOL;"), NULL, "embed eval code") == FAILURE)
	{
		php_printf("Failed to eval PHP.\n");
	}
// ...
```

Example output:

```bash
$ ./a.out
This message is produced by embed SAPI
```

We're using `zend_eval_stringl` to execute echo with example message. The last argument is intended for filename, and it's used for error reporting. We can also get return value of evaluated code.

```c
/* examples/eval_code_return/eval_code_return.c */
// ...
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
// ...
```

When `retval` is passed to `zend_eval_stringl`, [`"return "` is added to the beggining](https://github.com/php/php-src/blob/f6c0c60ef63c1e528dd3bd945c8f22270bbe3837/Zend/zend_execute_API.c#L1278-L1280) of the code, because of that `return $a * 2;` is never executed.

Internally `zend_eval_string` calls `zend_compile_string` and `zend_execute` functions, we will go back to them later in [:bookmark_tabs: Error handling](#) chapter. For now we can explore another feature of PHP - global variables.

```c
/* examples/eval_code_global_var/eval_code_global_var.c */

// ...
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
// ...
```

As we can see any global variables created in evaluated code are available after execution, and not just variables, but also defined functions and classes. For now we're using single request, in [:bookmark_tabs: Multiple requests](#) chapter we'll change that. Before moving on we can also try calling method defined in PHP code.

```c
/* examples/eval_code_call_method/eval_code_call_method.c */

// ...
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
// ...
```

As we can see, calling userland and built-in functions is similar, this time for a change, we've used an array to call an anonymous class method. Also note that even if function returns `void`, we have to assign zval to `fci.retval`, because PHP will try to assign `NULL` there.

## Parsing PHP files

With some basics covered we can start interacting with PHP code.

```c
/* examples/execute_file_basic/execute_file_basic.c */
// ...
	zend_file_handle file_handle;
	zend_stream_init_filename(&file_handle, "example.php");

	if (php_execute_script(&file_handle) == FAILURE)
	{
		php_printf("Failed to execute PHP script.\n");
	}
// ...
```

```php
<?php

/* examples/execute_file_basic/example.php */

echo 'Hello from userland!' . PHP_EOL;
```

```bash
$ ./a.out
Hello from userland!
```

# WIP
## Error handling
## Multiple requests
## Input / output (+request_info)
## INI settings
## SAPI functions and constants (using zend_module_entry)
## ZTS
## Opcache
