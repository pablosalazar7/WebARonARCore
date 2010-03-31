/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "StorageSyncManager.h"

#if ENABLE(DOM_STORAGE)

#include "EventNames.h"
#include "FileSystem.h"
#include "Frame.h"
#include "FrameTree.h"
#include "LocalStorageTask.h"
#include "LocalStorageThread.h"
#include "Page.h"
#include "PageGroup.h"
#include "StorageAreaSync.h"
#include <wtf/text/CString.h>
#include <wtf/StdLibExtras.h>

namespace WebCore {

PassRefPtr<StorageSyncManager> StorageSyncManager::create(const String& path)
{
    return adoptRef(new StorageSyncManager(path));
}

StorageSyncManager::StorageSyncManager(const String& path)
    : m_thread(LocalStorageThread::create())
    , m_path(path.crossThreadString())
{
    ASSERT(isMainThread());
    ASSERT(!m_path.isEmpty());
    m_thread->start();
}

StorageSyncManager::~StorageSyncManager()
{
    ASSERT(isMainThread());
    ASSERT(!m_thread);
}

// Called on a background thread.
String StorageSyncManager::fullDatabaseFilename(const String& databaseIdentifier)
{
    if (!makeAllDirectories(m_path)) {
        LOG_ERROR("Unabled to create LocalStorage database path %s", m_path.utf8().data());
        return String();
    }

    return pathByAppendingComponent(m_path, databaseIdentifier + ".localstorage");
}

void StorageSyncManager::close()
{
    ASSERT(isMainThread());

    if (m_thread) {
        m_thread->terminate();
        m_thread = 0;
    }
}

bool StorageSyncManager::scheduleImport(PassRefPtr<StorageAreaSync> area)
{
    ASSERT(isMainThread());
    ASSERT(m_thread);
    if (m_thread)
        m_thread->scheduleTask(LocalStorageTask::createImport(area.get()));
    return m_thread;
}

void StorageSyncManager::scheduleSync(PassRefPtr<StorageAreaSync> area)
{
    ASSERT(isMainThread());
    ASSERT(m_thread);
    if (m_thread)
        m_thread->scheduleTask(LocalStorageTask::createSync(area.get()));
}

} // namespace WebCore

#endif // ENABLE(DOM_STORAGE)
