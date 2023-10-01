PHP_ARG_ENABLE(fileselect,
[Whether to enable fileselect support],
[--enable-fileselect           Enable fileselect support])

if test "$PHP_fileselect" != "no"; then
    PHP_NEW_EXTENSION(fileselect, fileselect.c, $ext_shared)
fi