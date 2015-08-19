// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_UTILITY_SAFE_BROWSING_MAC_HFS_H_
#define CHROME_UTILITY_SAFE_BROWSING_MAC_HFS_H_

#include <hfs/hfs_format.h>
#include <stdint.h>

#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "base/strings/string16.h"

namespace safe_browsing {
namespace dmg {

class ReadStream;
class HFSBTreeIterator;
class HFSForkReadStream;

// HFSIterator is a read-only iterator over an HFS+ file system. It provides
// access to the data fork of all files on the system, as well as the path. This
// implementation has several deliberate limitations:
//   - Only HFS+ and HFSX are supported.
//   - The journal file is ignored. As this is intended to be used for HFS+ in
//     a DMG, replaying the journal should not typically be required.
//   - The extents overflow file is not consulted. In a DMG, the file system
//     should not be fragmented, and so consulting this should not typically be
//     required.
//   - No access is provided to resource forks.
//   - Getting the ReadStream for hard linked files is not supported.
//   - Files in hard linked directories are ignored.
//   - No content will be returned for files that are decmpfs compressed.
// For information on the HFS format, see
// <https://developer.apple.com/legacy/library/technotes/tn/tn1150.html>.
class HFSIterator {
 public:
  // Constructs an iterator from a stream.
  explicit HFSIterator(ReadStream* stream);
  ~HFSIterator();

  // Opens the filesystem and initializes the iterator. The iterator is
  // initialized to an invalid item before the first entry. Use Next() to
  // advance the iterator. This method must be called before any other
  // method. If this returns false, it is not legal to call any other methods.
  bool Open();

  // Advances the iterator to the next item. If this returns false, then it
  // is not legal to call any other methods.
  bool Next();

  // Returns true if the current iterator item is a directory and false if it
  // is a file.
  bool IsDirectory();

  // Returns true if the current iterator item is a symbolic link.
  bool IsSymbolicLink();

  // Returns true if the current iterator item is a hard link.
  bool IsHardLink();

  // Returns true if the current iterator item is decmpfs-compressed.
  bool IsDecmpfsCompressed();

  // Returns the full filesystem path of the current iterator item.
  base::string16 GetPath();

  // Returns a stream for the data fork of the current iterator item. This may
  // only be called if IsDirectory() and IsHardLink() returns false.
  scoped_ptr<ReadStream> GetReadStream();

 private:
  friend class HFSForkReadStream;

  // Moves the |stream_| position to a specific HFS+ |block|.
  bool SeekToBlock(uint64_t block);

  // Reads the catalog file to initialize the iterator.
  bool ReadCatalogFile();

  uint32_t block_size() const { return volume_header_.blockSize; }
  ReadStream* stream() const { return stream_; }

  ReadStream* const stream_;  // The stream backing the filesystem.
  HFSPlusVolumeHeader volume_header_;
  scoped_ptr<HFSForkReadStream> catalog_file_;  // Data of the catalog file.
  scoped_ptr<HFSBTreeIterator> catalog_;  // Iterator over the catalog file.

  DISALLOW_COPY_AND_ASSIGN(HFSIterator);
};

}  // namespace dmg
}  // namespace safe_browsing

#endif  // CHROME_UTILITY_SAFE_BROWSING_MAC_HFS_H_
