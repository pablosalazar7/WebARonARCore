// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_SYNCABLE_SERVICE_H__
#define COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_SYNCABLE_SERVICE_H__

#include <string>
#include <vector>

#include "base/memory/scoped_ptr.h"
#include "base/threading/non_thread_safe.h"
#include "components/password_manager/core/browser/password_store_change.h"
#include "sync/api/sync_change.h"
#include "sync/api/sync_data.h"
#include "sync/api/sync_error.h"
#include "sync/api/syncable_service.h"
#include "sync/protocol/password_specifics.pb.h"
#include "sync/protocol/sync.pb.h"

template <typename T>
class ScopedVector;

namespace autofill {
struct PasswordForm;
}

namespace syncer {
class SyncErrorFactory;
}

namespace password_manager {

class PasswordStoreSync;

// The implementation of the SyncableService API for passwords.
class PasswordSyncableService : public syncer::SyncableService,
                                public base::NonThreadSafe {
 public:
  // Since the constructed |PasswordSyncableService| is typically owned by the
  // |password_store|, the constructor doesn't take ownership of the
  // |password_store|.
  explicit PasswordSyncableService(PasswordStoreSync* password_store);
  ~PasswordSyncableService() override;

  // syncer::SyncableService:
  syncer::SyncMergeResult MergeDataAndStartSyncing(
      syncer::ModelType type,
      const syncer::SyncDataList& initial_sync_data,
      scoped_ptr<syncer::SyncChangeProcessor> sync_processor,
      scoped_ptr<syncer::SyncErrorFactory> error_handler) override;
  void StopSyncing(syncer::ModelType type) override;
  syncer::SyncDataList GetAllSyncData(syncer::ModelType type) const override;
  syncer::SyncError ProcessSyncChanges(
      const tracked_objects::Location& from_here,
      const syncer::SyncChangeList& change_list) override;

  // Notifies the Sync engine of changes to the password database.
  void ActOnPasswordStoreChanges(const PasswordStoreChangeList& changes);

  // Provides a StartSyncFlare to the SyncableService. See
  // chrome/browser/sync/glue/sync_start_util.h for more.
  void InjectStartSyncFlare(
      const syncer::SyncableService::StartSyncFlare& flare);

 private:
  typedef std::vector<autofill::PasswordForm*> PasswordForms;
  // Map from password sync tag to password form.
  typedef std::map<std::string, autofill::PasswordForm*> PasswordEntryMap;

  // The type of PasswordStoreSync::AddLoginImpl,
  // PasswordStoreSync::UpdateLoginImpl and PasswordStoreSync::RemoveLoginImpl.
  typedef PasswordStoreChangeList (PasswordStoreSync::*DatabaseOperation)(
      const autofill::PasswordForm& form);

  struct SyncEntries;

  // Retrieves the entries from password db and fills both |password_entries|
  // and |passwords_entry_map|. |passwords_entry_map| can be NULL.
  bool ReadFromPasswordStore(
      ScopedVector<autofill::PasswordForm>* password_entries,
      PasswordEntryMap* passwords_entry_map) const;

  // Uses the |PasswordStore| APIs to change entries.
  void WriteToPasswordStore(const SyncEntries& entries);

  // Examines |data|, an entry in sync db, and updates |sync_entries| or
  // |updated_db_entries| accordingly. An element is removed from
  // |unmatched_data_from_password_db| if its tag is identical to |data|'s.
  static void CreateOrUpdateEntry(
      const syncer::SyncData& data,
      PasswordEntryMap* unmatched_data_from_password_db,
      SyncEntries* sync_entries,
      syncer::SyncChangeList* updated_db_entries);

  // Calls |operation| for each element in |entries| and appends the changes to
  // |all_changes|.
  void WriteEntriesToDatabase(DatabaseOperation operation,
                              const PasswordForms& entries,
                              PasswordStoreChangeList* all_changes);

  // The factory that creates sync errors. |SyncError| has rich data
  // suitable for debugging.
  scoped_ptr<syncer::SyncErrorFactory> sync_error_factory_;

  // |sync_processor_| will mirror the |PasswordStore| changes in the sync db.
  scoped_ptr<syncer::SyncChangeProcessor> sync_processor_;

  // The password store that adds/updates/deletes password entries. Not owned.
  PasswordStoreSync* const password_store_;

  // A signal activated by this class to start sync as soon as possible.
  syncer::SyncableService::StartSyncFlare flare_;

  // True if processing sync changes is in progress.
  bool is_processing_sync_changes_;

  DISALLOW_COPY_AND_ASSIGN(PasswordSyncableService);
};

}  // namespace password_manager

#endif  // COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_SYNCABLE_SERVICE_H__
