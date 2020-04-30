// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: memory mapping
//

#pragma once

struct File_Map_;
using File_Map = File_Map_*;

/** @brief Map a file into memory.
  * @param[in] pszPath Path to the file to be mapped in.
  * @param[out] ppView Pointer where the address of the view will be stored.
  * @param[out] punLen Pointer where the size of the file will be stored.
  * @pre ppView, punLen and pszPath are all valid addresses and
  * pszPath points to a zero-terminated string.
  * @post If the operation was successful a non-NULL value is returned
  * and the values at ppView and punLen will be valid.
  * @remark If the file is empty (indicated by the returned handle
  * being valid, but the value at punLen being zero) then the address
  * stored at ppView should not be considered to be meaningful.
  * @remark The view must be unmapped using UnmapFile.
  * @return A handle that can be used later to unmap the file or NULL.
  * */
File_Map MapFile(char const** ppView, size_t* punLen, char const* pszPath);

/** @brief Unmap a file view.
  * @param[in] hMap Handle to the file mapping. Can be NULL.
  * */
void UnmapFile(File_Map hMap);

