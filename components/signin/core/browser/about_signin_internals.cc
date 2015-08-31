// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/signin/core/browser/about_signin_internals.h"

#include "base/command_line.h"
#include "base/hash.h"
#include "base/i18n/time_formatting.h"
#include "base/logging.h"
#include "base/prefs/pref_service.h"
#include "base/profiler/scoped_tracker.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/trace_event/trace_event.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/signin/core/browser/account_tracker_service.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"
#include "components/signin/core/browser/signin_client.h"
#include "components/signin/core/browser/signin_internals_util.h"
#include "components/signin/core/browser/signin_manager.h"
#include "components/signin/core/common/profile_management_switches.h"
#include "components/signin/core/common/signin_switches.h"

using base::Time;
using namespace signin_internals_util;

namespace {

std::string GetTimeStr(base::Time time) {
  return base::UTF16ToUTF8(base::TimeFormatShortDateAndTime(time));
}

base::ListValue* AddSection(base::ListValue* parent_list,
                            const std::string& title) {
  scoped_ptr<base::DictionaryValue> section(new base::DictionaryValue());
  base::ListValue* section_contents = new base::ListValue();

  section->SetString("title", title);
  section->Set("data", section_contents);
  parent_list->Append(section.release());
  return section_contents;
}

void AddSectionEntry(base::ListValue* section_list,
                     const std::string& field_name,
                     const std::string& field_status,
                     const std::string& field_time = "") {
  scoped_ptr<base::DictionaryValue> entry(new base::DictionaryValue());
  entry->SetString("label", field_name);
  entry->SetString("status", field_status);
  entry->SetString("time", field_time);
  section_list->Append(entry.release());
}

void AddCookieEntry(base::ListValue* accounts_list,
                     const std::string& field_email,
                     const std::string& field_gaia_id,
                     const std::string& field_valid) {
  scoped_ptr<base::DictionaryValue> entry(new base::DictionaryValue());
  entry->SetString("email", field_email);
  entry->SetString("gaia_id", field_gaia_id);
  entry->SetString("valid", field_valid);
  accounts_list->Append(entry.release());
}

std::string SigninStatusFieldToLabel(UntimedSigninStatusField field) {
  switch (field) {
    case ACCOUNT_ID:
      return "Account Id";
    case GAIA_ID:
      return "Gaia Id";
    case USERNAME:
      return "Username";
    case UNTIMED_FIELDS_END:
      NOTREACHED();
      return std::string();
  }
  NOTREACHED();
  return std::string();
}

#if !defined (OS_CHROMEOS)
std::string SigninStatusFieldToLabel(TimedSigninStatusField field) {
  switch (field) {
    case AUTHENTICATION_RESULT_RECEIVED:
      return "Gaia Authentication Result";
    case REFRESH_TOKEN_RECEIVED:
      return "RefreshToken Received";
    case SIGNIN_STARTED:
      return "SigninManager Started";
    case SIGNIN_COMPLETED:
      return "SigninManager Completed";
    case TIMED_FIELDS_END:
      NOTREACHED();
      return "Error";
  }
  NOTREACHED();
  return "Error";
}
#endif // !defined (OS_CHROMEOS)

void SetPref(PrefService* prefs,
             TimedSigninStatusField field,
             const std::string& time,
             const std::string& value) {
  std::string value_pref = SigninStatusFieldToString(field) + ".value";
  std::string time_pref = SigninStatusFieldToString(field) + ".time";
  prefs->SetString(value_pref, value);
  prefs->SetString(time_pref, time);
}

void GetPref(PrefService* prefs,
             TimedSigninStatusField field,
             std::string* time,
             std::string* value) {
  std::string value_pref = SigninStatusFieldToString(field) + ".value";
  std::string time_pref = SigninStatusFieldToString(field) + ".time";
  *value = prefs->GetString(value_pref);
  *time = prefs->GetString(time_pref);
}

void ClearPref(PrefService* prefs, TimedSigninStatusField field) {
  std::string value_pref = SigninStatusFieldToString(field) + ".value";
  std::string time_pref = SigninStatusFieldToString(field) + ".time";
  prefs->ClearPref(value_pref);
  prefs->ClearPref(time_pref);
}

}  // anonymous namespace

AboutSigninInternals::AboutSigninInternals(
    ProfileOAuth2TokenService* token_service,
    AccountTrackerService* account_tracker,
    SigninManagerBase* signin_manager,
    SigninErrorController* signin_error_controller,
    GaiaCookieManagerService* cookie_manager_service)
    : token_service_(token_service),
      account_tracker_(account_tracker),
      signin_manager_(signin_manager),
      client_(NULL),
      signin_error_controller_(signin_error_controller),
      cookie_manager_service_(cookie_manager_service) {}

AboutSigninInternals::~AboutSigninInternals() {}

// static
void AboutSigninInternals::RegisterPrefs(
    user_prefs::PrefRegistrySyncable* user_prefs) {
  // SigninManager information for about:signin-internals.

  // TODO(rogerta): leaving untimed fields here for now because legacy
  // profiles still have these prefs.  In three or four version from M43
  // we can probably remove them.
  for (int i = UNTIMED_FIELDS_BEGIN; i < UNTIMED_FIELDS_END; ++i) {
    const std::string pref_path =
        SigninStatusFieldToString(static_cast<UntimedSigninStatusField>(i));
    user_prefs->RegisterStringPref(pref_path.c_str(), std::string());
  }

  for (int i = TIMED_FIELDS_BEGIN; i < TIMED_FIELDS_END; ++i) {
    const std::string value =
        SigninStatusFieldToString(static_cast<TimedSigninStatusField>(i)) +
        ".value";
    const std::string time =
        SigninStatusFieldToString(static_cast<TimedSigninStatusField>(i)) +
        ".time";
    user_prefs->RegisterStringPref(value.c_str(), std::string());
    user_prefs->RegisterStringPref(time.c_str(), std::string());
  }
}

void AboutSigninInternals::AddSigninObserver(
    AboutSigninInternals::Observer* observer) {
  signin_observers_.AddObserver(observer);
}

void AboutSigninInternals::RemoveSigninObserver(
    AboutSigninInternals::Observer* observer) {
  signin_observers_.RemoveObserver(observer);
}

void AboutSigninInternals::NotifySigninValueChanged(
    const TimedSigninStatusField& field,
    const std::string& value) {
  unsigned int field_index = field - TIMED_FIELDS_BEGIN;
  DCHECK(field_index >= 0 &&
         field_index < signin_status_.timed_signin_fields.size());

  Time now = Time::NowFromSystemTime();
  std::string time_as_str =
      base::UTF16ToUTF8(base::TimeFormatShortDateAndTime(now));
  TimedSigninStatusValue timed_value(value, time_as_str);

  signin_status_.timed_signin_fields[field_index] = timed_value;

  // Also persist these values in the prefs.
  SetPref(client_->GetPrefs(), field, value, time_as_str);

  // If the user is restarting a sign in process, clear the fields that are
  // to come.
  if (field == AUTHENTICATION_RESULT_RECEIVED) {
    ClearPref(client_->GetPrefs(), REFRESH_TOKEN_RECEIVED);
    ClearPref(client_->GetPrefs(), SIGNIN_STARTED);
    ClearPref(client_->GetPrefs(), SIGNIN_COMPLETED);
  }

  NotifyObservers();
}

void AboutSigninInternals::RefreshSigninPrefs() {
  // Return if no client exists. Can occur in unit tests.
  if (!client_)
    return;

  PrefService* pref_service = client_->GetPrefs();
  for (int i = TIMED_FIELDS_BEGIN; i < TIMED_FIELDS_END; ++i) {
    std::string time_str;
    std::string value_str;
    GetPref(pref_service, static_cast<TimedSigninStatusField>(i),
            &time_str, &value_str);
    TimedSigninStatusValue value(value_str, time_str);
    signin_status_.timed_signin_fields[i - TIMED_FIELDS_BEGIN] = value;
  }

  // TODO(rogerta): Get status and timestamps for oauth2 tokens.

  NotifyObservers();
}

void AboutSigninInternals::Initialize(SigninClient* client) {
  DCHECK(!client_);
  client_ = client;

  RefreshSigninPrefs();

  signin_error_controller_->AddObserver(this);
  signin_manager_->AddSigninDiagnosticsObserver(this);
  token_service_->AddDiagnosticsObserver(this);
  cookie_manager_service_->AddObserver(this);
}

void AboutSigninInternals::Shutdown() {
  signin_error_controller_->RemoveObserver(this);
  signin_manager_->RemoveSigninDiagnosticsObserver(this);
  token_service_->RemoveDiagnosticsObserver(this);
  cookie_manager_service_->RemoveObserver(this);
}

void AboutSigninInternals::NotifyObservers() {
  if (!signin_observers_.might_have_observers())
    return;

  // TODO(robliao): Remove ScopedTracker below once https://crbug.com/422460 is
  // fixed.
  tracked_objects::ScopedTracker tracking_profile(
      FROM_HERE_WITH_EXPLICIT_FUNCTION(
          "422460 AboutSigninInternals::NotifyObservers"));

  const std::string product_version = client_->GetProductVersion();

  // TODO(robliao): Remove ScopedTracker below once https://crbug.com/422460 is
  // fixed.
  tracked_objects::ScopedTracker tracking_profile05(
      FROM_HERE_WITH_EXPLICIT_FUNCTION(
          "422460 AboutSigninInternals::NotifyObservers 0.5"));

  scoped_ptr<base::DictionaryValue> signin_status_value =
      signin_status_.ToValue(account_tracker_,
                             signin_manager_,
                             signin_error_controller_,
                             token_service_,
                             product_version);

  // TODO(robliao): Remove ScopedTracker below once https://crbug.com/422460 is
  // fixed.
  tracked_objects::ScopedTracker tracking_profile1(
      FROM_HERE_WITH_EXPLICIT_FUNCTION(
          "422460 AboutSigninInternals::NotifyObservers1"));

  FOR_EACH_OBSERVER(AboutSigninInternals::Observer,
                    signin_observers_,
                    OnSigninStateChanged(signin_status_value.get()));
}

scoped_ptr<base::DictionaryValue> AboutSigninInternals::GetSigninStatus() {
  return signin_status_.ToValue(account_tracker_,
                                signin_manager_,
                                signin_error_controller_,
                                token_service_,
                                client_->GetProductVersion()).Pass();
}

void AboutSigninInternals::OnAccessTokenRequested(
    const std::string& account_id,
    const std::string& consumer_id,
    const OAuth2TokenService::ScopeSet& scopes) {
  // TODO(robliao): Remove ScopedTracker below once https://crbug.com/422460 is
  // fixed.
  tracked_objects::ScopedTracker tracking_profile(
      FROM_HERE_WITH_EXPLICIT_FUNCTION(
          "422460 AboutSigninInternals::OnAccessTokenRequested"));

  TokenInfo* token = signin_status_.FindToken(account_id, consumer_id, scopes);
  if (token) {
    *token = TokenInfo(consumer_id, scopes);
  } else {
    token = new TokenInfo(consumer_id, scopes);
    signin_status_.token_info_map[account_id].push_back(token);
  }

  NotifyObservers();
}

void AboutSigninInternals::OnFetchAccessTokenComplete(
    const std::string& account_id,
    const std::string& consumer_id,
    const OAuth2TokenService::ScopeSet& scopes,
    GoogleServiceAuthError error,
    base::Time expiration_time) {
  TokenInfo* token = signin_status_.FindToken(account_id, consumer_id, scopes);
  if (!token) {
    DVLOG(1) << "Can't find token: " << account_id << ", " << consumer_id;
    return;
  }

  token->receive_time = base::Time::Now();
  token->error = error;
  token->expiration_time = expiration_time;

  NotifyObservers();
}

void AboutSigninInternals::OnTokenRemoved(
    const std::string& account_id,
    const OAuth2TokenService::ScopeSet& scopes) {
  for (size_t i = 0; i < signin_status_.token_info_map[account_id].size();
       ++i) {
    TokenInfo* token = signin_status_.token_info_map[account_id][i];
    if (token->scopes == scopes)
      token->Invalidate();
  }
  NotifyObservers();
}

void AboutSigninInternals::OnRefreshTokenReceived(std::string status) {
  NotifySigninValueChanged(REFRESH_TOKEN_RECEIVED, status);
}

void AboutSigninInternals::OnAuthenticationResultReceived(std::string status) {
  NotifySigninValueChanged(AUTHENTICATION_RESULT_RECEIVED, status);
}

void AboutSigninInternals::OnErrorChanged() {
  NotifyObservers();
}

void AboutSigninInternals::GoogleSigninFailed(
    const GoogleServiceAuthError& error) {
  NotifyObservers();
}

void AboutSigninInternals::GoogleSigninSucceeded(const std::string& account_id,
                                                 const std::string& username,
                                                 const std::string& password) {
  NotifyObservers();
}

void AboutSigninInternals::GoogleSignedOut(const std::string& account_id,
                                           const std::string& username) {
  NotifyObservers();
}

void AboutSigninInternals::OnGaiaAccountsInCookieUpdated(
    const std::vector<gaia::ListedAccount>& gaia_accounts,
    const GoogleServiceAuthError& error) {
  if (error.state() != GoogleServiceAuthError::NONE)
    return;

  base::DictionaryValue cookie_status;
  base::ListValue* cookie_info = new base::ListValue();
  cookie_status.Set("cookie_info", cookie_info);

  for (size_t i = 0; i < gaia_accounts.size(); ++i) {
    AddCookieEntry(cookie_info,
                   gaia_accounts[i].raw_email,
                   gaia_accounts[i].gaia_id,
                   gaia_accounts[i].valid ? "Valid" : "Invalid");
  }

  if (gaia_accounts.size() == 0) {
    AddCookieEntry(cookie_info,
                   "No Accounts Present.",
                   std::string(),
                   std::string());
  }

  // Update the observers that the cookie's accounts are updated.
  FOR_EACH_OBSERVER(AboutSigninInternals::Observer,
                    signin_observers_,
                    OnCookieAccountsFetched(&cookie_status));
}

AboutSigninInternals::TokenInfo::TokenInfo(
    const std::string& consumer_id,
    const OAuth2TokenService::ScopeSet& scopes)
    : consumer_id(consumer_id),
      scopes(scopes),
      request_time(base::Time::Now()),
      error(GoogleServiceAuthError::AuthErrorNone()),
      removed_(false) {}

AboutSigninInternals::TokenInfo::~TokenInfo() {}

bool AboutSigninInternals::TokenInfo::LessThan(const TokenInfo* a,
                                               const TokenInfo* b) {
  if (a->request_time == b->request_time) {
    if (a->consumer_id == b->consumer_id) {
      return a->scopes < b->scopes;
    }
    return a->consumer_id < b->consumer_id;
  }
  return a->request_time < b->request_time;
}

void AboutSigninInternals::TokenInfo::Invalidate() { removed_ = true; }

base::DictionaryValue* AboutSigninInternals::TokenInfo::ToValue() const {
  scoped_ptr<base::DictionaryValue> token_info(new base::DictionaryValue());
  token_info->SetString("service", consumer_id);

  std::string scopes_str;
  for (OAuth2TokenService::ScopeSet::const_iterator it = scopes.begin();
       it != scopes.end();
       ++it) {
    scopes_str += *it + "<br/>";
  }
  token_info->SetString("scopes", scopes_str);
  token_info->SetString("request_time", GetTimeStr(request_time).c_str());

  if (removed_) {
    token_info->SetString("status", "Token was revoked.");
  } else if (!receive_time.is_null()) {
    if (error == GoogleServiceAuthError::AuthErrorNone()) {
      bool token_expired = expiration_time < base::Time::Now();
      std::string expiration_time_string = GetTimeStr(expiration_time);
      if (expiration_time.is_null()) {
        token_expired = false;
        expiration_time_string = "Expiration time not available";
      }
      std::string status_str = "";
      if (token_expired)
        status_str = "<p style=\"color: #ffffff; background-color: #ff0000\">";
      base::StringAppendF(&status_str, "Received token at %s. Expire at %s",
                          GetTimeStr(receive_time).c_str(),
                          expiration_time_string.c_str());
      if (token_expired)
        base::StringAppendF(&status_str, "</p>");
      token_info->SetString("status", status_str);
    } else {
      token_info->SetString(
          "status",
          base::StringPrintf("Failure: %s", error.ToString().c_str()));
    }
  } else {
    token_info->SetString("status", "Waiting for response");
  }

  return token_info.release();
}

AboutSigninInternals::SigninStatus::SigninStatus()
    : timed_signin_fields(TIMED_FIELDS_COUNT) {}

AboutSigninInternals::SigninStatus::~SigninStatus() {
  for (TokenInfoMap::iterator it = token_info_map.begin();
       it != token_info_map.end();
       ++it) {
    STLDeleteElements(&it->second);
  }
}

AboutSigninInternals::TokenInfo* AboutSigninInternals::SigninStatus::FindToken(
    const std::string& account_id,
    const std::string& consumer_id,
    const OAuth2TokenService::ScopeSet& scopes) {
  for (size_t i = 0; i < token_info_map[account_id].size(); ++i) {
    TokenInfo* tmp = token_info_map[account_id][i];
    if (tmp->consumer_id == consumer_id && tmp->scopes == scopes)
      return tmp;
  }
  return NULL;
}

scoped_ptr<base::DictionaryValue> AboutSigninInternals::SigninStatus::ToValue(
    AccountTrackerService* account_tracker,
    SigninManagerBase* signin_manager,
    SigninErrorController* signin_error_controller,
    ProfileOAuth2TokenService* token_service,
    const std::string& product_version) {
  // TODO(robliao): Remove ScopedTracker below once https://crbug.com/422460 is
  // fixed.
  tracked_objects::ScopedTracker tracking_profile1(
      FROM_HERE_WITH_EXPLICIT_FUNCTION(
          "422460 AboutSigninInternals::SigninStatus::ToValue1"));

  scoped_ptr<base::DictionaryValue> signin_status(new base::DictionaryValue());
  base::ListValue* signin_info = new base::ListValue();
  signin_status->Set("signin_info", signin_info);

  // A summary of signin related info first.
  base::ListValue* basic_info = AddSection(signin_info, "Basic Information");
  AddSectionEntry(basic_info, "Chrome Version", product_version);
  AddSectionEntry(basic_info, "Webview Based Signin?",
      switches::IsEnableWebviewBasedSignin() == true ? "On" : "Off");
  AddSectionEntry(basic_info, "New Avatar Menu?",
      switches::IsNewAvatarMenu() == true ? "On" : "Off");
  AddSectionEntry(basic_info, "New Profile Management?",
      switches::IsNewProfileManagement() == true ? "On" : "Off");
  AddSectionEntry(basic_info, "Account Consistency?",
      switches::IsEnableAccountConsistency() == true ? "On" : "Off");
  AddSectionEntry(basic_info, "Signin Status",
      signin_manager->IsAuthenticated() ? "Signed In" : "Not Signed In");

  // TODO(robliao): Remove ScopedTracker below once https://crbug.com/422460 is
  // fixed.
  tracked_objects::ScopedTracker tracking_profile2(
      FROM_HERE_WITH_EXPLICIT_FUNCTION(
          "422460 AboutSigninInternals::SigninStatus::ToValue2"));

  if (signin_manager->IsAuthenticated()) {
    std::string account_id = signin_manager->GetAuthenticatedAccountId();
    AddSectionEntry(basic_info,
                    SigninStatusFieldToLabel(
                        static_cast<UntimedSigninStatusField>(ACCOUNT_ID)),
                    account_id);
    AddSectionEntry(basic_info,
                    SigninStatusFieldToLabel(
                        static_cast<UntimedSigninStatusField>(GAIA_ID)),
                    account_tracker->GetAccountInfo(account_id).gaia);
    AddSectionEntry(basic_info,
                    SigninStatusFieldToLabel(
                        static_cast<UntimedSigninStatusField>(USERNAME)),
                    signin_manager->GetAuthenticatedAccountInfo().email);
    if (signin_error_controller->HasError()) {
      const std::string error_account_id =
          signin_error_controller->error_account_id();
      const std::string error_username =
          account_tracker->GetAccountInfo(error_account_id).email;
      AddSectionEntry(basic_info, "Auth Error",
          signin_error_controller->auth_error().ToString());
      AddSectionEntry(basic_info, "Auth Error Account Id", error_account_id);
      AddSectionEntry(basic_info, "Auth Error Username", error_username);
    } else {
      AddSectionEntry(basic_info, "Auth Error", "None");
    }
  }

#if !defined(OS_CHROMEOS)
  // TODO(robliao): Remove ScopedTracker below once https://crbug.com/422460 is
  // fixed.
  tracked_objects::ScopedTracker tracking_profile3(
      FROM_HERE_WITH_EXPLICIT_FUNCTION(
          "422460 AboutSigninInternals::SigninStatus::ToValue3"));

  // Time and status information of the possible sign in types.
  base::ListValue* detailed_info =
      AddSection(signin_info, "Last Signin Details");
  for (int i = TIMED_FIELDS_BEGIN; i < TIMED_FIELDS_END; ++i) {
    const std::string status_field_label =
        SigninStatusFieldToLabel(static_cast<TimedSigninStatusField>(i));

    AddSectionEntry(detailed_info,
                    status_field_label,
                    timed_signin_fields[i - TIMED_FIELDS_BEGIN].first,
                    timed_signin_fields[i - TIMED_FIELDS_BEGIN].second);
  }
#endif // !defined(OS_CHROMEOS)

  // TODO(robliao): Remove ScopedTracker below once https://crbug.com/422460 is
  // fixed.
  tracked_objects::ScopedTracker tracking_profile4(
      FROM_HERE_WITH_EXPLICIT_FUNCTION(
          "422460 AboutSigninInternals::SigninStatus::ToValue4"));

  // Token information for all services.
  base::ListValue* token_info = new base::ListValue();
  signin_status->Set("token_info", token_info);
  for (TokenInfoMap::iterator it = token_info_map.begin();
       it != token_info_map.end();
       ++it) {
    // TODO(robliao): Remove ScopedTracker below once https://crbug.com/422460
    // is fixed.
    tracked_objects::ScopedTracker tracking_profile41(
        FROM_HERE_WITH_EXPLICIT_FUNCTION(
            "422460 AboutSigninInternals::SigninStatus::ToValue41"));

    base::ListValue* token_details = AddSection(token_info, it->first);

    // TODO(robliao): Remove ScopedTracker below once https://crbug.com/422460
    // is fixed.
    tracked_objects::ScopedTracker tracking_profile42(
        FROM_HERE_WITH_EXPLICIT_FUNCTION(
            "422460 AboutSigninInternals::SigninStatus::ToValue42"));

    std::sort(it->second.begin(), it->second.end(), TokenInfo::LessThan);
    const std::vector<TokenInfo*>& tokens = it->second;

    // TODO(robliao): Remove ScopedTracker below once https://crbug.com/422460
    // is fixed.
    tracked_objects::ScopedTracker tracking_profile43(
        FROM_HERE_WITH_EXPLICIT_FUNCTION(
            "422460 AboutSigninInternals::SigninStatus::ToValue43"));

    for (size_t i = 0; i < tokens.size(); ++i) {
      base::DictionaryValue* token_info = tokens[i]->ToValue();
      token_details->Append(token_info);
    }
  }

  base::ListValue* account_info = new base::ListValue();
  signin_status->Set("accountInfo", account_info);
  const std::vector<std::string>& accounts_in_token_service =
      token_service->GetAccounts();

  if(accounts_in_token_service.size() == 0) {
    base::DictionaryValue* no_token_entry = new base::DictionaryValue();
    no_token_entry->SetString("accountId", "No token in Token Service.");
    account_info->Append(no_token_entry);
  }

  for(const std::string& account_id : accounts_in_token_service) {
    base::DictionaryValue* entry = new base::DictionaryValue();
    entry->SetString("accountId", account_id);
    account_info->Append(entry);
  }

  return signin_status.Pass();
}
