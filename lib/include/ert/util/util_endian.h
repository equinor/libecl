/*
   Copyright (C) 2012  Equinor ASA, Norway.

   The file 'util_endian.h' is part of ERT - Ensemble based Reservoir Tool.

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

#ifndef ERT_UTIL_ENDIAN_H
#define ERT_UTIL_ENDIAN_H

#ifdef __cplusplus
extern "C" {
#endif

void util_endian_flip_vector(void *data, int element_size, int elements);

#ifdef __cplusplus
}
#endif
#endif
