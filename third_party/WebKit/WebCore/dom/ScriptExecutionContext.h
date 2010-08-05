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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 *
 */

#ifndef ScriptExecutionContext_h
#define ScriptExecutionContext_h

#include "Console.h"
#include "KURL.h"
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/Threading.h>

namespace WebCore {

    class ActiveDOMObject;
#if ENABLE(DATABASE)
    class Database;
    class DatabaseTaskSynchronizer;
    class DatabaseThread;
#endif
    class DOMTimer;
#if ENABLE(BLOB) || ENABLE(FILE_WRITER)
    class FileThread;
#endif
    class MessagePort;
    class SecurityOrigin;
    class ScriptString;
    class String;
#if ENABLE(INSPECTOR)
    class InspectorController;
#endif

    class ScriptExecutionContext {
    public:
        ScriptExecutionContext();
        virtual ~ScriptExecutionContext();

        virtual bool isDocument() const { return false; }
        virtual bool isWorkerContext() const { return false; }

#if ENABLE(DATABASE)
        virtual bool isDatabaseReadOnly() const = 0;
        virtual void databaseExceededQuota(const String& name) = 0;
        DatabaseThread* databaseThread();
        void setHasOpenDatabases() { m_hasOpenDatabases = true; }
        bool hasOpenDatabases() const { return m_hasOpenDatabases; }
        // When the database cleanup is done, cleanupSync will be signalled.
        void stopDatabases(DatabaseTaskSynchronizer*);
#endif
        virtual bool isContextThread() const = 0;
        virtual bool isJSExecutionTerminated() const = 0;

        const KURL& url() const { return virtualURL(); }
        KURL completeURL(const String& url) const { return virtualCompleteURL(url); }

        virtual String userAgent(const KURL&) const = 0;

        SecurityOrigin* securityOrigin() const { return m_securityOrigin.get(); }
#if ENABLE(INSPECTOR)
        virtual InspectorController* inspectorController() const { return 0; }
#endif

        virtual void reportException(const String& errorMessage, int lineNumber, const String& sourceURL) = 0;
        virtual void addMessage(MessageSource, MessageType, MessageLevel, const String& message, unsigned lineNumber, const String& sourceURL) = 0;
        
        // Active objects are not garbage collected even if inaccessible, e.g. because their activity may result in callbacks being invoked.
        bool canSuspendActiveDOMObjects();
        // Active objects can be asked to suspend even if canSuspendActiveDOMObjects() returns 'false' -
        // step-by-step JS debugging is one example.
        void suspendActiveDOMObjects();
        void resumeActiveDOMObjects();
        void stopActiveDOMObjects();
        void createdActiveDOMObject(ActiveDOMObject*, void* upcastPointer);
        void destroyedActiveDOMObject(ActiveDOMObject*);
        typedef const HashMap<ActiveDOMObject*, void*> ActiveDOMObjectsMap;
        ActiveDOMObjectsMap& activeDOMObjects() const { return m_activeDOMObjects; }

        // MessagePort is conceptually a kind of ActiveDOMObject, but it needs to be tracked separately for message dispatch.
        void processMessagePortMessagesSoon();
        void dispatchMessagePortEvents();
        void createdMessagePort(MessagePort*);
        void destroyedMessagePort(MessagePort*);
        const HashSet<MessagePort*>& messagePorts() const { return m_messagePorts; }

        void ref() { refScriptExecutionContext(); }
        void deref() { derefScriptExecutionContext(); }

        class Task : public Noncopyable {
        public:
            virtual ~Task();
            virtual void performTask(ScriptExecutionContext*) = 0;
            // Certain tasks get marked specially so that they aren't discarded, and are executed, when the context is shutting down its message queue.
            virtual bool isCleanupTask() const { return false; }
        };

        virtual void postTask(PassOwnPtr<Task>) = 0; // Executes the task on context's thread asynchronously.

        void addTimeout(int timeoutId, DOMTimer*);
        void removeTimeout(int timeoutId);
        DOMTimer* findTimeout(int timeoutId);

#if USE(JSC)
        JSC::JSGlobalData* globalData();
#endif

#if ENABLE(BLOB) || ENABLE(FILE_WRITER)
        FileThread* fileThread();
        void stopFileThread();
#endif

    protected:
        // Explicitly override the security origin for this script context.
        // Note: It is dangerous to change the security origin of a script context
        //       that already contains content.
        void setSecurityOrigin(PassRefPtr<SecurityOrigin>);

    private:
        virtual const KURL& virtualURL() const = 0;
        virtual KURL virtualCompleteURL(const String&) const = 0;

        RefPtr<SecurityOrigin> m_securityOrigin;

        HashSet<MessagePort*> m_messagePorts;

        HashMap<ActiveDOMObject*, void*> m_activeDOMObjects;

        HashMap<int, DOMTimer*> m_timeouts;

        virtual void refScriptExecutionContext() = 0;
        virtual void derefScriptExecutionContext() = 0;

#if ENABLE(DATABASE)
        RefPtr<DatabaseThread> m_databaseThread;
        bool m_hasOpenDatabases; // This never changes back to false, even after the database thread is closed.
#endif

#if ENABLE(BLOB) || ENABLE(FILE_WRITER)
        RefPtr<FileThread> m_fileThread;
#endif
    };

} // namespace WebCore


#endif // ScriptExecutionContext_h
