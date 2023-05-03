#!/bin/sh

if [ ! -f *.c ]
then
	echo "Change directory to example folder first (with .c file)"
else
	gcc \
		$(/home/codespace/.local/bin/php-config --includes) \
		-L$(/home/codespace/.local/bin/php-config --prefix)/lib \
		*.c \
		-lphp \
		-Wl,-rpath=$(/home/codespace/.local/bin/php-config --prefix)/lib
fi
