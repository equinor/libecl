/*
   Copyright (C) 2018  Statoil ASA, Norway.

   The file 'util_portability.h' is part of ERT - Ensemble based Reservoir Tool.

   ERT is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   ERT is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.

   See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html>
   for more details.
*/

#ifndef ERT_UTIL_PORTABILITY_H
#define ERT_UTIL_PORTABILITY_H

#include <ert/util/ert_api_config.h>


#ifdef __cplusplus
extern"C" {
#endif


#ifdef ERT_WINDOWS
#define UTIL_PATH_SEP_STRING           "\\"   /* A \0 terminated separator used when we want a (char *) instance.                   */
#define UTIL_PATH_SEP_CHAR             '\\'   /* A simple character used when we want an actual char instance (i.e. not a pointer). */
#else
#define UTIL_PATH_SEP_STRING           "/"   /* A \0 terminated separator used when we want a (char *) instance.                   */
#define UTIL_PATH_SEP_CHAR             '/'   /* A simple character used when we want an actual char instance (i.e. not a pointer). */
#endif

#define UTIL_WINDOWS_PATH_SEP_CHAR '\\'
#define UTIL_POSIX_PATH_SEP_CHAR   '/'

#define UTIL_NEWLINE_STRING "          \n"
#define UTIL_DEFAULT_MKDIR_MODE 0777         /* Directories are by default created with mode a+rwx - and then comes the umask ... */


void    util_usleep( unsigned long micro_seconds );



#ifdef __cplusplus
}
#endif

#endif
