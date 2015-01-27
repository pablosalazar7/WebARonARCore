// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/password_manager/password_store_factory.h"

#include "base/command_line.h"
#include "base/environment.h"
#include "base/metrics/histogram_macros.h"
#include "base/prefs/pref_service.h"
#include "chrome/browser/password_manager/password_manager_util.h"
#include "chrome/browser/password_manager/sync_metrics.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sync/glue/sync_start_util.h"
#include "chrome/browser/webdata/web_data_service_factory.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_switches.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/os_crypt/os_crypt_switches.h"
#include "components/password_manager/core/browser/login_database.h"
#include "components/password_manager/core/browser/password_store.h"
#include "components/password_manager/core/browser/password_store_default.h"
#include "components/password_manager/core/common/password_manager_pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "content/public/browser/browser_thread.h"

#if defined(OS_WIN)
#include "chrome/browser/password_manager/password_store_win.h"
#include "components/password_manager/core/browser/webdata/password_web_data_service_win.h"
#elif defined(OS_MACOSX)
#include "chrome/browser/password_manager/password_store_mac.h"
#include "crypto/apple_keychain.h"
#include "crypto/mock_apple_keychain.h"
#elif defined(OS_CHROMEOS) || defined(OS_ANDROID)
// Don't do anything. We're going to use the default store.
#elif defined(USE_X11)
#include "base/nix/xdg_util.h"
#if defined(USE_GNOME_KEYRING)
#include "chrome/browser/password_manager/native_backend_gnome_x.h"
#endif
#if defined(USE_LIBSECRET)
#include "base/metrics/field_trial.h"
#include "chrome/browser/password_manager/native_backend_libsecret.h"
#endif
#include "chrome/browser/password_manager/native_backend_kwallet_x.h"
#include "chrome/browser/password_manager/password_store_x.h"
#endif

using password_manager::PasswordStore;

namespace {

#if !defined(OS_CHROMEOS) && defined(USE_X11)
const LocalProfileId kInvalidLocalProfileId =
    static_cast<LocalProfileId>(0);
#endif

#if defined(USE_LIBSECRET)
const char kLibsecretFieldTrialName[] = "Libsecret";
const char kLibsecretFieldTrialDisabledGroupName[] = "Disabled";
#endif

void ReportOsPassword(password_manager_util::OsPasswordStatus status) {
  UMA_HISTOGRAM_ENUMERATION("PasswordManager.OsPasswordStatus",
                            status,
                            password_manager_util::MAX_PASSWORD_STATUS);
}

void DelayReportOsPassword() {
  // Avoid checking OS password until later on in browser startup
  // since it calls a few Windows APIs.
  content::BrowserThread::PostDelayedTask(
      content::BrowserThread::UI,
      FROM_HERE,
      base::Bind(&password_manager_util::GetOsPasswordStatus,
                 base::Bind(&ReportOsPassword)),
      base::TimeDelta::FromSeconds(40));
}

}  // namespace


PasswordStoreService::PasswordStoreService(
    scoped_refptr<PasswordStore> password_store)
    : password_store_(password_store) {}

PasswordStoreService::~PasswordStoreService() {}

scoped_refptr<PasswordStore> PasswordStoreService::GetPasswordStore() {
  return password_store_;
}

void PasswordStoreService::Shutdown() {
  if (password_store_.get())
    password_store_->Shutdown();
}

// static
scoped_refptr<PasswordStore> PasswordStoreFactory::GetForProfile(
    Profile* profile,
    ServiceAccessType sat) {
  if (sat == ServiceAccessType::IMPLICIT_ACCESS && profile->IsOffTheRecord()) {
    NOTREACHED() << "This profile is OffTheRecord";
    return NULL;
  }

  PasswordStoreFactory* factory = GetInstance();
  PasswordStoreService* service = static_cast<PasswordStoreService*>(
      factory->GetServiceForBrowserContext(profile, true));
  if (!service)
    return NULL;
  return service->GetPasswordStore();
}

// static
PasswordStoreFactory* PasswordStoreFactory::GetInstance() {
  return Singleton<PasswordStoreFactory>::get();
}

PasswordStoreFactory::PasswordStoreFactory()
    : BrowserContextKeyedServiceFactory(
        "PasswordStore",
        BrowserContextDependencyManager::GetInstance()) {
  DependsOn(WebDataServiceFactory::GetInstance());
}

PasswordStoreFactory::~PasswordStoreFactory() {}

#if !defined(OS_CHROMEOS) && defined(USE_X11)
LocalProfileId PasswordStoreFactory::GetLocalProfileId(
    PrefService* prefs) const {
  LocalProfileId id =
      prefs->GetInteger(password_manager::prefs::kLocalProfileId);
  if (id == kInvalidLocalProfileId) {
    // Note that there are many more users than this. Thus, by design, this is
    // not a unique id. However, it is large enough that it is very unlikely
    // that it would be repeated twice on a single machine. It is still possible
    // for that to occur though, so the potential results of it actually
    // happening should be considered when using this value.
    static const LocalProfileId kLocalProfileIdMask =
        static_cast<LocalProfileId>((1 << 24) - 1);
    do {
      id = rand() & kLocalProfileIdMask;
      // TODO(mdm): scan other profiles to make sure they are not using this id?
    } while (id == kInvalidLocalProfileId);
    prefs->SetInteger(password_manager::prefs::kLocalProfileId, id);
  }
  return id;
}
#endif

KeyedService* PasswordStoreFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  DelayReportOsPassword();
  Profile* profile = static_cast<Profile*>(context);

  // Given that LoginDatabase::Init() takes ~100ms on average; it will be called
  // by PasswordStore::Init() on the background thread to avoid UI jank.
  base::FilePath login_db_file_path = profile->GetPath();
  login_db_file_path = login_db_file_path.Append(chrome::kLoginDataFileName);
  scoped_ptr<password_manager::LoginDatabase> login_db(
      new password_manager::LoginDatabase(login_db_file_path));

  scoped_refptr<base::SingleThreadTaskRunner> main_thread_runner(
      base::MessageLoopProxy::current());
  scoped_refptr<base::SingleThreadTaskRunner> db_thread_runner(
      content::BrowserThread::GetMessageLoopProxyForThread(
          content::BrowserThread::DB));

  scoped_refptr<PasswordStore> ps;
#if defined(OS_WIN)
  ps = new PasswordStoreWin(main_thread_runner, db_thread_runner,
                            login_db.Pass(),
                            WebDataServiceFactory::GetPasswordWebDataForProfile(
                                profile, ServiceAccessType::EXPLICIT_ACCESS));
#elif defined(OS_MACOSX)
  scoped_ptr<crypto::AppleKeychain> keychain(
      base::CommandLine::ForCurrentProcess()->HasSwitch(
          os_crypt::switches::kUseMockKeychain)
          ? new crypto::MockAppleKeychain()
          : new crypto::AppleKeychain());
  ps = new PasswordStoreMac(main_thread_runner, db_thread_runner,
                            keychain.Pass(), login_db.Pass());
#elif defined(OS_CHROMEOS) || defined(OS_ANDROID)
  // For now, we use PasswordStoreDefault. We might want to make a native
  // backend for PasswordStoreX (see below) in the future though.
  ps = new password_manager::PasswordStoreDefault(
      main_thread_runner, db_thread_runner, login_db.Pass());
#elif defined(USE_X11)
  // On POSIX systems, we try to use the "native" password management system of
  // the desktop environment currently running, allowing GNOME Keyring in XFCE.
  // (In all cases we fall back on the basic store in case of failure.)
  base::nix::DesktopEnvironment desktop_env;
  std::string store_type =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          switches::kPasswordStore);
  if (store_type == "kwallet") {
    desktop_env = base::nix::DESKTOP_ENVIRONMENT_KDE4;
  } else if (store_type == "gnome") {
    desktop_env = base::nix::DESKTOP_ENVIRONMENT_GNOME;
  } else if (store_type == "basic") {
    desktop_env = base::nix::DESKTOP_ENVIRONMENT_OTHER;
  } else {
    // Detect the store to use automatically.
    scoped_ptr<base::Environment> env(base::Environment::Create());
    desktop_env = base::nix::GetDesktopEnvironment(env.get());
    const char* name = base::nix::GetDesktopEnvironmentName(desktop_env);
    VLOG(1) << "Password storage detected desktop environment: "
            << (name ? name : "(unknown)");
  }

  PrefService* prefs = profile->GetPrefs();
  LocalProfileId id = GetLocalProfileId(prefs);

  scoped_ptr<PasswordStoreX::NativeBackend> backend;
  if (desktop_env == base::nix::DESKTOP_ENVIRONMENT_KDE4) {
    // KDE3 didn't use DBus, which our KWallet store uses.
    VLOG(1) << "Trying KWallet for password storage.";
    backend.reset(new NativeBackendKWallet(id));
    if (backend->Init())
      VLOG(1) << "Using KWallet for password storage.";
    else
      backend.reset();
  } else if (desktop_env == base::nix::DESKTOP_ENVIRONMENT_GNOME ||
             desktop_env == base::nix::DESKTOP_ENVIRONMENT_UNITY ||
             desktop_env == base::nix::DESKTOP_ENVIRONMENT_XFCE) {
#if defined(USE_LIBSECRET)
    if (base::FieldTrialList::FindFullName(kLibsecretFieldTrialName) !=
        kLibsecretFieldTrialDisabledGroupName) {
      VLOG(1) << "Trying libsecret for password storage.";
      backend.reset(new NativeBackendLibsecret(id));
      if (backend->Init())
        VLOG(1) << "Using libsecret keyring for password storage.";
      else
        backend.reset();
    }
#endif  // defined(USE_LIBSECRET)
    if (!backend.get()) {
#if defined(USE_GNOME_KEYRING)
      VLOG(1) << "Trying GNOME keyring for password storage.";
      backend.reset(new NativeBackendGnome(id));
      if (backend->Init())
        VLOG(1) << "Using GNOME keyring for password storage.";
      else
        backend.reset();
#endif  // defined(USE_GNOME_KEYRING)
    }
  }

  if (!backend.get()) {
    LOG(WARNING) << "Using basic (unencrypted) store for password storage. "
        "See http://code.google.com/p/chromium/wiki/LinuxPasswordStorage for "
        "more information about password storage options.";
  }

  ps = new PasswordStoreX(main_thread_runner, db_thread_runner, login_db.Pass(),
                          backend.release());
#elif defined(USE_OZONE)
  ps = new password_manager::PasswordStoreDefault(
      main_thread_runner, db_thread_runner, login_db.Pass());
#else
  NOTIMPLEMENTED();
#endif
  if (!ps.get() ||
      !ps->Init(
          sync_start_util::GetFlareForSyncableService(profile->GetPath()))) {
    NOTREACHED() << "Could not initialize password manager.";
    return NULL;
  }

  return new PasswordStoreService(ps);
}

void PasswordStoreFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
#if !defined(OS_CHROMEOS) && defined(USE_X11)
  // Notice that the preprocessor conditions above are exactly those that will
  // result in using PasswordStoreX in BuildServiceInstanceFor().
  registry->RegisterIntegerPref(
      password_manager::prefs::kLocalProfileId,
      kInvalidLocalProfileId,
      user_prefs::PrefRegistrySyncable::UNSYNCABLE_PREF);
#endif
}

content::BrowserContext* PasswordStoreFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

bool PasswordStoreFactory::ServiceIsNULLWhileTesting() const {
  return true;
}
