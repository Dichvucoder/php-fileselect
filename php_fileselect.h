#ifndef PHP_FILESELECT_H
#define PHP_FILESELECT_H

#define PHP_FILESELECT_EXT_NAME  "FILESELECT"
#define PHP_FILESELECT_EXT_VERSION  "1.0"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "php.h"

#ifdef ZTS
#include "TSRM.h"
#endif

#if defined(ZTS) && defined(COMPILE_DL_FILESELECT)
ZEND_TSRMLS_CACHE_EXTERN()
#endif

#endif /* PHP_FILESELECT_H */
