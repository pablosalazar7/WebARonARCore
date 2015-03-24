// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/indexed_db/leveldb/leveldb_database.h"

#include <cerrno>

#include "base/basictypes.h"
#include "base/files/file.h"
#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/metrics/histogram.h"
#include "base/strings/string16.h"
#include "base/strings/string_piece.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/sys_info.h"
#include "content/browser/indexed_db/indexed_db_class_factory.h"
#include "content/browser/indexed_db/leveldb/leveldb_comparator.h"
#include "content/browser/indexed_db/leveldb/leveldb_iterator_impl.h"
#include "content/browser/indexed_db/leveldb/leveldb_write_batch.h"
#include "third_party/leveldatabase/env_chromium.h"
#include "third_party/leveldatabase/env_idb.h"
#include "third_party/leveldatabase/src/helpers/memenv/memenv.h"
#include "third_party/leveldatabase/src/include/leveldb/db.h"
#include "third_party/leveldatabase/src/include/leveldb/env.h"
#include "third_party/leveldatabase/src/include/leveldb/filter_policy.h"
#include "third_party/leveldatabase/src/include/leveldb/slice.h"

using base::StringPiece;

namespace content {

// Forcing flushes to disk at the end of a transaction guarantees that the
// data hit disk, but drastically impacts throughput when the filesystem is
// busy with background compactions. Not syncing trades off reliability for
// performance. Note that background compactions which move data from the
// log to SSTs are always done with reliable writes.
//
// Sync writes are necessary on Windows for quota calculations; POSIX
// calculates file sizes correctly even when not synced to disk.
#if defined(OS_WIN)
static const bool kSyncWrites = true;
#else
// TODO(dgrogan): Either remove the #if block or change this back to false.
// See http://crbug.com/338385.
static const bool kSyncWrites = true;
#endif

static leveldb::Slice MakeSlice(const StringPiece& s) {
  return leveldb::Slice(s.begin(), s.size());
}

static StringPiece MakeStringPiece(const leveldb::Slice& s) {
  return StringPiece(s.data(), s.size());
}

LevelDBDatabase::ComparatorAdapter::ComparatorAdapter(
    const LevelDBComparator* comparator)
    : comparator_(comparator) {}

int LevelDBDatabase::ComparatorAdapter::Compare(const leveldb::Slice& a,
                                                const leveldb::Slice& b) const {
  return comparator_->Compare(MakeStringPiece(a), MakeStringPiece(b));
}

const char* LevelDBDatabase::ComparatorAdapter::Name() const {
  return comparator_->Name();
}

// TODO(jsbell): Support the methods below in the future.
void LevelDBDatabase::ComparatorAdapter::FindShortestSeparator(
    std::string* start,
    const leveldb::Slice& limit) const {}

void LevelDBDatabase::ComparatorAdapter::FindShortSuccessor(
    std::string* key) const {}

LevelDBSnapshot::LevelDBSnapshot(LevelDBDatabase* db)
    : db_(db->db_.get()), snapshot_(db_->GetSnapshot()) {}

LevelDBSnapshot::~LevelDBSnapshot() { db_->ReleaseSnapshot(snapshot_); }

LevelDBDatabase::LevelDBDatabase() {}

LevelDBDatabase::~LevelDBDatabase() {
  // db_'s destructor uses comparator_adapter_; order of deletion is important.
  db_.reset();
  comparator_adapter_.reset();
  env_.reset();
}

static leveldb::Status OpenDB(
    leveldb::Comparator* comparator,
    leveldb::Env* env,
    const base::FilePath& path,
    leveldb::DB** db,
    scoped_ptr<const leveldb::FilterPolicy>* filter_policy) {
  filter_policy->reset(leveldb::NewBloomFilterPolicy(10));
  leveldb::Options options;
  options.comparator = comparator;
  options.create_if_missing = true;
  options.paranoid_checks = true;
  options.filter_policy = filter_policy->get();
  options.reuse_logs = true;
  options.compression = leveldb::kSnappyCompression;

  // For info about the troubles we've run into with this parameter, see:
  // https://code.google.com/p/chromium/issues/detail?id=227313#c11
  options.max_open_files = 80;
  options.env = env;

  // ChromiumEnv assumes UTF8, converts back to FilePath before using.
  leveldb::Status s = leveldb::DB::Open(options, path.AsUTF8Unsafe(), db);

  return s;
}

leveldb::Status LevelDBDatabase::Destroy(const base::FilePath& file_name) {
  leveldb::Options options;
  options.env = leveldb::IDBEnv();
  // ChromiumEnv assumes UTF8, converts back to FilePath before using.
  return leveldb::DestroyDB(file_name.AsUTF8Unsafe(), options);
}

namespace {
class LockImpl : public LevelDBLock {
 public:
  explicit LockImpl(leveldb::Env* env, leveldb::FileLock* lock)
      : env_(env), lock_(lock) {}
  ~LockImpl() override { env_->UnlockFile(lock_); }

 private:
  leveldb::Env* env_;
  leveldb::FileLock* lock_;

  DISALLOW_COPY_AND_ASSIGN(LockImpl);
};
}  // namespace

scoped_ptr<LevelDBLock> LevelDBDatabase::LockForTesting(
    const base::FilePath& file_name) {
  leveldb::Env* env = leveldb::IDBEnv();
  base::FilePath lock_path = file_name.AppendASCII("LOCK");
  leveldb::FileLock* lock = NULL;
  leveldb::Status status = env->LockFile(lock_path.AsUTF8Unsafe(), &lock);
  if (!status.ok())
    return scoped_ptr<LevelDBLock>();
  DCHECK(lock);
  return scoped_ptr<LevelDBLock>(new LockImpl(env, lock));
}

static int CheckFreeSpace(const char* const type,
                          const base::FilePath& file_name) {
  std::string name =
      std::string("WebCore.IndexedDB.LevelDB.Open") + type + "FreeDiskSpace";
  int64 free_disk_space_in_k_bytes =
      base::SysInfo::AmountOfFreeDiskSpace(file_name) / 1024;
  if (free_disk_space_in_k_bytes < 0) {
    base::Histogram::FactoryGet(
        "WebCore.IndexedDB.LevelDB.FreeDiskSpaceFailure",
        1,
        2 /*boundary*/,
        2 /*boundary*/ + 1,
        base::HistogramBase::kUmaTargetedHistogramFlag)->Add(1 /*sample*/);
    return -1;
  }
  int clamped_disk_space_k_bytes = free_disk_space_in_k_bytes > INT_MAX
                                       ? INT_MAX
                                       : free_disk_space_in_k_bytes;
  const uint64 histogram_max = static_cast<uint64>(1e9);
  static_assert(histogram_max <= INT_MAX, "histogram_max too big");
  base::Histogram::FactoryGet(name,
                              1,
                              histogram_max,
                              11 /*buckets*/,
                              base::HistogramBase::kUmaTargetedHistogramFlag)
      ->Add(clamped_disk_space_k_bytes);
  return clamped_disk_space_k_bytes;
}

static void ParseAndHistogramIOErrorDetails(const std::string& histogram_name,
                                            const leveldb::Status& s) {
  leveldb_env::MethodID method;
  base::File::Error error = base::File::FILE_OK;
  leveldb_env::ErrorParsingResult result =
      leveldb_env::ParseMethodAndError(s, &method, &error);
  if (result == leveldb_env::NONE)
    return;
  std::string method_histogram_name(histogram_name);
  method_histogram_name.append(".EnvMethod");
  base::LinearHistogram::FactoryGet(
      method_histogram_name,
      1,
      leveldb_env::kNumEntries,
      leveldb_env::kNumEntries + 1,
      base::HistogramBase::kUmaTargetedHistogramFlag)->Add(method);

  std::string error_histogram_name(histogram_name);

  if (result == leveldb_env::METHOD_AND_BFE) {
    DCHECK_LT(error, 0);
    error_histogram_name.append(std::string(".BFE.") +
                                leveldb_env::MethodIDToString(method));
    base::LinearHistogram::FactoryGet(
        error_histogram_name,
        1,
        -base::File::FILE_ERROR_MAX,
        -base::File::FILE_ERROR_MAX + 1,
        base::HistogramBase::kUmaTargetedHistogramFlag)->Add(-error);
  }
}

static void ParseAndHistogramCorruptionDetails(
    const std::string& histogram_name,
    const leveldb::Status& status) {
  int error = leveldb_env::GetCorruptionCode(status);
  DCHECK_GE(error, 0);
  std::string corruption_histogram_name(histogram_name);
  corruption_histogram_name.append(".Corruption");
  const int kNumPatterns = leveldb_env::GetNumCorruptionCodes();
  base::LinearHistogram::FactoryGet(
      corruption_histogram_name,
      1,
      kNumPatterns,
      kNumPatterns + 1,
      base::HistogramBase::kUmaTargetedHistogramFlag)->Add(error);
}

static void HistogramLevelDBError(const std::string& histogram_name,
                                  const leveldb::Status& s) {
  if (s.ok()) {
    NOTREACHED();
    return;
  }
  enum {
    LEVEL_DB_NOT_FOUND,
    LEVEL_DB_CORRUPTION,
    LEVEL_DB_IO_ERROR,
    LEVEL_DB_OTHER,
    LEVEL_DB_MAX_ERROR
  };
  int leveldb_error = LEVEL_DB_OTHER;
  if (s.IsNotFound())
    leveldb_error = LEVEL_DB_NOT_FOUND;
  else if (s.IsCorruption())
    leveldb_error = LEVEL_DB_CORRUPTION;
  else if (s.IsIOError())
    leveldb_error = LEVEL_DB_IO_ERROR;
  base::Histogram::FactoryGet(histogram_name,
                              1,
                              LEVEL_DB_MAX_ERROR,
                              LEVEL_DB_MAX_ERROR + 1,
                              base::HistogramBase::kUmaTargetedHistogramFlag)
      ->Add(leveldb_error);
  if (s.IsIOError())
    ParseAndHistogramIOErrorDetails(histogram_name, s);
  else
    ParseAndHistogramCorruptionDetails(histogram_name, s);
}

leveldb::Status LevelDBDatabase::Open(const base::FilePath& file_name,
                                      const LevelDBComparator* comparator,
                                      scoped_ptr<LevelDBDatabase>* result,
                                      bool* is_disk_full) {
  base::TimeTicks begin_time = base::TimeTicks::Now();

  scoped_ptr<ComparatorAdapter> comparator_adapter(
      new ComparatorAdapter(comparator));

  leveldb::DB* db;
  scoped_ptr<const leveldb::FilterPolicy> filter_policy;
  const leveldb::Status s = OpenDB(comparator_adapter.get(),
                                   leveldb::IDBEnv(),
                                   file_name,
                                   &db,
                                   &filter_policy);

  if (!s.ok()) {
    HistogramLevelDBError("WebCore.IndexedDB.LevelDBOpenErrors", s);
    int free_space_k_bytes = CheckFreeSpace("Failure", file_name);
    // Disks with <100k of free space almost never succeed in opening a
    // leveldb database.
    if (is_disk_full)
      *is_disk_full = free_space_k_bytes >= 0 && free_space_k_bytes < 100;

    LOG(ERROR) << "Failed to open LevelDB database from "
               << file_name.AsUTF8Unsafe() << "," << s.ToString();
    return s;
  }

  UMA_HISTOGRAM_MEDIUM_TIMES("WebCore.IndexedDB.LevelDB.OpenTime",
                             base::TimeTicks::Now() - begin_time);

  CheckFreeSpace("Success", file_name);

  (*result).reset(new LevelDBDatabase);
  (*result)->db_ = make_scoped_ptr(db);
  (*result)->comparator_adapter_ = comparator_adapter.Pass();
  (*result)->comparator_ = comparator;
  (*result)->filter_policy_ = filter_policy.Pass();

  return s;
}

scoped_ptr<LevelDBDatabase> LevelDBDatabase::OpenInMemory(
    const LevelDBComparator* comparator) {
  scoped_ptr<ComparatorAdapter> comparator_adapter(
      new ComparatorAdapter(comparator));
  scoped_ptr<leveldb::Env> in_memory_env(leveldb::NewMemEnv(leveldb::IDBEnv()));

  leveldb::DB* db;
  scoped_ptr<const leveldb::FilterPolicy> filter_policy;
  const leveldb::Status s = OpenDB(comparator_adapter.get(),
                                   in_memory_env.get(),
                                   base::FilePath(),
                                   &db,
                                   &filter_policy);

  if (!s.ok()) {
    LOG(ERROR) << "Failed to open in-memory LevelDB database: " << s.ToString();
    return scoped_ptr<LevelDBDatabase>();
  }

  scoped_ptr<LevelDBDatabase> result(new LevelDBDatabase);
  result->env_ = in_memory_env.Pass();
  result->db_ = make_scoped_ptr(db);
  result->comparator_adapter_ = comparator_adapter.Pass();
  result->comparator_ = comparator;
  result->filter_policy_ = filter_policy.Pass();

  return result.Pass();
}

leveldb::Status LevelDBDatabase::Put(const StringPiece& key,
                                     std::string* value) {
  base::TimeTicks begin_time = base::TimeTicks::Now();

  leveldb::WriteOptions write_options;
  write_options.sync = kSyncWrites;

  const leveldb::Status s =
      db_->Put(write_options, MakeSlice(key), MakeSlice(*value));
  if (!s.ok())
    LOG(ERROR) << "LevelDB put failed: " << s.ToString();
  else
    UMA_HISTOGRAM_TIMES("WebCore.IndexedDB.LevelDB.PutTime",
                        base::TimeTicks::Now() - begin_time);
  return s;
}

leveldb::Status LevelDBDatabase::Remove(const StringPiece& key) {
  leveldb::WriteOptions write_options;
  write_options.sync = kSyncWrites;

  const leveldb::Status s = db_->Delete(write_options, MakeSlice(key));
  if (!s.IsNotFound())
    LOG(ERROR) << "LevelDB remove failed: " << s.ToString();
  return s;
}

leveldb::Status LevelDBDatabase::Get(const StringPiece& key,
                                     std::string* value,
                                     bool* found,
                                     const LevelDBSnapshot* snapshot) {
  *found = false;
  leveldb::ReadOptions read_options;
  read_options.verify_checksums = true;  // TODO(jsbell): Disable this if the
                                         // performance impact is too great.
  read_options.snapshot = snapshot ? snapshot->snapshot_ : 0;

  const leveldb::Status s = db_->Get(read_options, MakeSlice(key), value);
  if (s.ok()) {
    *found = true;
    return s;
  }
  if (s.IsNotFound())
    return leveldb::Status::OK();
  HistogramLevelDBError("WebCore.IndexedDB.LevelDBReadErrors", s);
  LOG(ERROR) << "LevelDB get failed: " << s.ToString();
  return s;
}

leveldb::Status LevelDBDatabase::Write(const LevelDBWriteBatch& write_batch) {
  base::TimeTicks begin_time = base::TimeTicks::Now();
  leveldb::WriteOptions write_options;
  write_options.sync = kSyncWrites;

  const leveldb::Status s =
      db_->Write(write_options, write_batch.write_batch_.get());
  if (!s.ok()) {
    HistogramLevelDBError("WebCore.IndexedDB.LevelDBWriteErrors", s);
    LOG(ERROR) << "LevelDB write failed: " << s.ToString();
  } else {
    UMA_HISTOGRAM_TIMES("WebCore.IndexedDB.LevelDB.WriteTime",
                        base::TimeTicks::Now() - begin_time);
  }
  return s;
}

scoped_ptr<LevelDBIterator> LevelDBDatabase::CreateIterator(
    const LevelDBSnapshot* snapshot) {
  leveldb::ReadOptions read_options;
  read_options.verify_checksums = true;  // TODO(jsbell): Disable this if the
                                         // performance impact is too great.
  read_options.snapshot = snapshot ? snapshot->snapshot_ : 0;

  scoped_ptr<leveldb::Iterator> i(db_->NewIterator(read_options));
  return scoped_ptr<LevelDBIterator>(
      IndexedDBClassFactory::Get()->CreateIteratorImpl(i.Pass()));
}

const LevelDBComparator* LevelDBDatabase::Comparator() const {
  return comparator_;
}

void LevelDBDatabase::Compact(const base::StringPiece& start,
                              const base::StringPiece& stop) {
  const leveldb::Slice start_slice = MakeSlice(start);
  const leveldb::Slice stop_slice = MakeSlice(stop);
  // NULL batch means just wait for earlier writes to be done
  db_->Write(leveldb::WriteOptions(), NULL);
  db_->CompactRange(&start_slice, &stop_slice);
}

void LevelDBDatabase::CompactAll() { db_->CompactRange(NULL, NULL); }

}  // namespace content
