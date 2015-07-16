// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/declarative_webrequest/webrequest_rules_registry.h"

#include <algorithm>
#include <limits>
#include <utility>

#include "base/bind.h"
#include "base/stl_util.h"
#include "extensions/browser/api/declarative_webrequest/webrequest_condition.h"
#include "extensions/browser/api/declarative_webrequest/webrequest_constants.h"
#include "extensions/browser/api/web_request/web_request_api_helpers.h"
#include "extensions/browser/api/web_request/web_request_permissions.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/error_utils.h"
#include "extensions/common/extension.h"
#include "extensions/common/permissions/permissions_data.h"
#include "net/url_request/url_request.h"

using url_matcher::URLMatcherConditionSet;

namespace {

const char kActionCannotBeExecuted[] = "The action '*' can never be executed "
    "because there are is no time in the request life-cycle during which the "
    "conditions can be checked and the action can possibly be executed.";

const char kAllURLsPermissionNeeded[] =
    "To execute the action '*', you need to request host permission for all "
    "hosts.";

}  // namespace

namespace extensions {

WebRequestRulesRegistry::WebRequestRulesRegistry(
    content::BrowserContext* browser_context,
    RulesCacheDelegate* cache_delegate,
    int rules_registry_id)
    : RulesRegistry(browser_context,
                    declarative_webrequest_constants::kOnRequest,
                    content::BrowserThread::IO,
                    cache_delegate,
                    rules_registry_id),
      browser_context_(browser_context) {
  if (browser_context_)
    extension_info_map_ = ExtensionSystem::Get(browser_context_)->info_map();
}

std::set<const WebRequestRule*> WebRequestRulesRegistry::GetMatches(
    const WebRequestData& request_data_without_ids) const {
  RuleSet result;

  WebRequestDataWithMatchIds request_data(&request_data_without_ids);
  request_data.url_match_ids = url_matcher_.MatchURL(
      request_data.data->request->url());
  request_data.first_party_url_match_ids = url_matcher_.MatchURL(
      request_data.data->request->first_party_for_cookies());

  // 1st phase -- add all rules with some conditions without UrlFilter
  // attributes.
  for (const WebRequestRule* rule : rules_with_untriggered_conditions_) {
    if (rule->conditions().IsFulfilled(-1, request_data))
      result.insert(rule);
  }

  // 2nd phase -- add all rules with some conditions triggered by URL matches.
  AddTriggeredRules(request_data.url_match_ids, request_data, &result);
  AddTriggeredRules(request_data.first_party_url_match_ids,
                    request_data, &result);

  return result;
}

std::list<LinkedPtrEventResponseDelta> WebRequestRulesRegistry::CreateDeltas(
    const InfoMap* extension_info_map,
    const WebRequestData& request_data,
    bool crosses_incognito) {
  if (webrequest_rules_.empty())
    return std::list<LinkedPtrEventResponseDelta>();

  std::set<const WebRequestRule*> matches = GetMatches(request_data);

  // Sort all matching rules by their priority so that they can be processed
  // in decreasing order.
  typedef std::pair<WebRequestRule::Priority, WebRequestRule::GlobalRuleId>
      PriorityRuleIdPair;
  std::vector<PriorityRuleIdPair> ordered_matches;
  ordered_matches.reserve(matches.size());
  for (const WebRequestRule* rule : matches)
    ordered_matches.push_back(make_pair(rule->priority(), rule->id()));
  // Sort from rbegin to rend in order to get descending priority order.
  std::sort(ordered_matches.rbegin(), ordered_matches.rend());

  // Build a map that maps each extension id to the minimum required priority
  // for rules of that extension. Initially, this priority is -infinite and
  // will be increased when the rules are processed and raise the bar via
  // WebRequestIgnoreRulesActions.
  typedef std::string ExtensionId;
  typedef std::map<ExtensionId, WebRequestRule::Priority> MinPriorities;
  typedef std::map<ExtensionId, std::set<std::string> > IgnoreTags;
  MinPriorities min_priorities;
  IgnoreTags ignore_tags;
  for (const PriorityRuleIdPair& priority_rule_id_pair : ordered_matches) {
    const WebRequestRule::GlobalRuleId& rule_id = priority_rule_id_pair.second;
    const ExtensionId& extension_id = rule_id.first;
    min_priorities[extension_id] = std::numeric_limits<int>::min();
  }

  // Create deltas until we have passed the minimum priority.
  std::list<LinkedPtrEventResponseDelta> result;
  for (const PriorityRuleIdPair& priority_rule_id_pair : ordered_matches) {
    const WebRequestRule::Priority priority_of_rule =
        priority_rule_id_pair.first;
    const WebRequestRule::GlobalRuleId& rule_id = priority_rule_id_pair.second;
    const ExtensionId& extension_id = rule_id.first;
    const WebRequestRule* rule =
        webrequest_rules_[rule_id.first][rule_id.second].get();
    CHECK(rule);

    // Skip rule if a previous rule of this extension instructed to ignore
    // all rules with a lower priority than min_priorities[extension_id].
    int current_min_priority = min_priorities[extension_id];
    if (priority_of_rule < current_min_priority)
      continue;

    if (!rule->tags().empty() && !ignore_tags[extension_id].empty()) {
      bool ignore_rule = false;
      for (const std::string& tag : rule->tags())
        ignore_rule |= ContainsKey(ignore_tags[extension_id], tag);
      if (ignore_rule)
        continue;
    }

    std::list<LinkedPtrEventResponseDelta> rule_result;
    WebRequestAction::ApplyInfo apply_info = {
      extension_info_map, request_data, crosses_incognito, &rule_result,
      &ignore_tags[extension_id]
    };
    rule->Apply(&apply_info);
    result.splice(result.begin(), rule_result);

    min_priorities[extension_id] = std::max(current_min_priority,
                                            rule->GetMinimumPriority());
  }
  return result;
}

std::string WebRequestRulesRegistry::AddRulesImpl(
    const std::string& extension_id,
    const std::vector<linked_ptr<core_api::events::Rule>>& rules) {
  typedef std::pair<WebRequestRule::RuleId, linked_ptr<const WebRequestRule>>
      IdRulePair;
  typedef std::vector<IdRulePair> RulesVector;

  base::Time extension_installation_time =
      GetExtensionInstallationTime(extension_id);

  std::string error;
  RulesVector new_webrequest_rules;
  new_webrequest_rules.reserve(rules.size());
  const Extension* extension =
      extension_info_map_->extensions().GetByID(extension_id);
  RulesMap& registered_rules = webrequest_rules_[extension_id];

  for (const linked_ptr<core_api::events::Rule>& rule : rules) {
    const WebRequestRule::RuleId& rule_id(*rule->id);
    DCHECK(registered_rules.find(rule_id) == registered_rules.end());

    scoped_ptr<WebRequestRule> webrequest_rule(WebRequestRule::Create(
        url_matcher_.condition_factory(), browser_context(), extension,
        extension_installation_time, rule,
        base::Bind(&Checker, base::Unretained(extension)), &error));
    if (!error.empty()) {
      // We don't return here, because we want to clear temporary
      // condition sets in the url_matcher_.
      break;
    }

    new_webrequest_rules.push_back(
        IdRulePair(rule_id, make_linked_ptr(webrequest_rule.release())));
  }

  if (!error.empty()) {
    // Clean up temporary condition sets created during rule creation.
    url_matcher_.ClearUnusedConditionSets();
    return error;
  }

  // Wohoo, everything worked fine.
  registered_rules.insert(new_webrequest_rules.begin(),
                          new_webrequest_rules.end());

  // Create the triggers.
  for (const IdRulePair& id_rule_pair : new_webrequest_rules) {
    URLMatcherConditionSet::Vector url_condition_sets;
    const linked_ptr<const WebRequestRule>& rule = id_rule_pair.second;
    rule->conditions().GetURLMatcherConditionSets(&url_condition_sets);
    for (const scoped_refptr<URLMatcherConditionSet>& condition_set :
         url_condition_sets) {
      rule_triggers_[condition_set->id()] = rule.get();
    }
  }

  // Register url patterns in |url_matcher_| and
  // |rules_with_untriggered_conditions_|.
  URLMatcherConditionSet::Vector all_new_condition_sets;
  for (const IdRulePair& id_rule_pair : new_webrequest_rules) {
    const linked_ptr<const WebRequestRule>& rule = id_rule_pair.second;
    rule->conditions().GetURLMatcherConditionSets(&all_new_condition_sets);
    if (rule->conditions().HasConditionsWithoutUrls())
      rules_with_untriggered_conditions_.insert(rule.get());
  }
  url_matcher_.AddConditionSets(all_new_condition_sets);

  ClearCacheOnNavigation();

  if (browser_context_ && !registered_rules.empty()) {
    content::BrowserThread::PostTask(
        content::BrowserThread::UI, FROM_HERE,
        base::Bind(&extension_web_request_api_helpers::NotifyWebRequestAPIUsed,
                   browser_context_, extension->id()));
  }

  return std::string();
}

std::string WebRequestRulesRegistry::RemoveRulesImpl(
    const std::string& extension_id,
    const std::vector<std::string>& rule_identifiers) {
  // URLMatcherConditionSet IDs that can be removed from URLMatcher.
  std::vector<URLMatcherConditionSet::ID> remove_from_url_matcher;
  RulesMap& registered_rules = webrequest_rules_[extension_id];

  for (const std::string& identifier : rule_identifiers) {
    // Skip unknown rules.
    RulesMap::iterator webrequest_rules_entry =
        registered_rules.find(identifier);
    if (webrequest_rules_entry == registered_rules.end())
      continue;

    // Remove all triggers but collect their IDs.
    CleanUpAfterRule(webrequest_rules_entry->second.get(),
                     &remove_from_url_matcher);

    // Removes the owning references to (and thus deletes) the rule.
    registered_rules.erase(webrequest_rules_entry);
  }
  if (registered_rules.empty())
    webrequest_rules_.erase(extension_id);

  // Clear URLMatcher based on condition_set_ids that are not needed any more.
  url_matcher_.RemoveConditionSets(remove_from_url_matcher);

  ClearCacheOnNavigation();

  return std::string();
}

std::string WebRequestRulesRegistry::RemoveAllRulesImpl(
    const std::string& extension_id) {
  // First we get out all URLMatcherConditionSets and remove the rule references
  // from |rules_with_untriggered_conditions_|.
  std::vector<URLMatcherConditionSet::ID> remove_from_url_matcher;
  for (const std::pair<WebRequestRule::RuleId,
                       linked_ptr<const WebRequestRule>>& rule_id_rule_pair :
       webrequest_rules_[extension_id]) {
    CleanUpAfterRule(rule_id_rule_pair.second.get(), &remove_from_url_matcher);
  }
  url_matcher_.RemoveConditionSets(remove_from_url_matcher);

  webrequest_rules_.erase(extension_id);
  ClearCacheOnNavigation();
  return std::string();
}

void WebRequestRulesRegistry::CleanUpAfterRule(
    const WebRequestRule* rule,
    std::vector<URLMatcherConditionSet::ID>* remove_from_url_matcher) {
  URLMatcherConditionSet::Vector condition_sets;
  rule->conditions().GetURLMatcherConditionSets(&condition_sets);
  for (const scoped_refptr<URLMatcherConditionSet>& condition_set :
       condition_sets) {
    remove_from_url_matcher->push_back(condition_set->id());
    rule_triggers_.erase(condition_set->id());
  }
  rules_with_untriggered_conditions_.erase(rule);
}

bool WebRequestRulesRegistry::IsEmpty() const {
  // Easy first.
  if (!rule_triggers_.empty() && url_matcher_.IsEmpty())
    return false;

  // Now all the registered rules for each extensions.
  for (const std::pair<WebRequestRule::ExtensionId, RulesMap>&
           extension_id_rules_map_pair : webrequest_rules_) {
    if (!extension_id_rules_map_pair.second.empty())
      return false;
  }
  return true;
}

WebRequestRulesRegistry::~WebRequestRulesRegistry() {}

base::Time WebRequestRulesRegistry::GetExtensionInstallationTime(
    const std::string& extension_id) const {
  return extension_info_map_->GetInstallTime(extension_id);
}

void WebRequestRulesRegistry::ClearCacheOnNavigation() {
  extension_web_request_api_helpers::ClearCacheOnNavigation();
}

// static
bool WebRequestRulesRegistry::Checker(const Extension* extension,
                                      const WebRequestConditionSet* conditions,
                                      const WebRequestActionSet* actions,
                                      std::string* error) {
  return (StageChecker(conditions, actions, error) &&
          HostPermissionsChecker(extension, actions, error));
}

// static
bool WebRequestRulesRegistry::HostPermissionsChecker(
    const Extension* extension,
    const WebRequestActionSet* actions,
    std::string* error) {
  if (extension->permissions_data()->HasEffectiveAccessToAllHosts())
    return true;

  // Without the permission for all URLs, actions with the STRATEGY_DEFAULT
  // should not be registered, they would never be able to execute.
  for (const scoped_refptr<const WebRequestAction>& action :
       actions->actions()) {
    if (action->host_permissions_strategy() ==
        WebRequestAction::STRATEGY_DEFAULT) {
      *error = ErrorUtils::FormatErrorMessage(kAllURLsPermissionNeeded,
                                              action->GetName());
      return false;
    }
  }
  return true;
}

// static
bool WebRequestRulesRegistry::StageChecker(
    const WebRequestConditionSet* conditions,
    const WebRequestActionSet* actions,
    std::string* error) {
  // Actions and conditions can be checked and executed in specific stages
  // of each web request. A rule is inconsistent if there is an action that
  // can only be triggered in stages in which no condition can be evaluated.

  // In which stages there are conditions to evaluate.
  int condition_stages = 0;
  for (const linked_ptr<const WebRequestCondition>& condition :
       conditions->conditions()) {
    condition_stages |= condition->stages();
  }

  for (const scoped_refptr<const WebRequestAction>& action :
       actions->actions()) {
    // Test the intersection of bit masks, this is intentionally & and not &&.
    if (action->stages() & condition_stages)
      continue;
    // We only get here if no matching condition was found.
    *error = ErrorUtils::FormatErrorMessage(kActionCannotBeExecuted,
                                            action->GetName());
    return false;
  }
  return true;
}
void WebRequestRulesRegistry::AddTriggeredRules(
    const URLMatches& url_matches,
    const WebRequestCondition::MatchData& request_data,
    RuleSet* result) const {
  for (url_matcher::URLMatcherConditionSet::ID url_match : url_matches) {
    RuleTriggers::const_iterator rule_trigger = rule_triggers_.find(url_match);
    CHECK(rule_trigger != rule_triggers_.end());
    if (!ContainsKey(*result, rule_trigger->second) &&
        rule_trigger->second->conditions().IsFulfilled(url_match, request_data))
      result->insert(rule_trigger->second);
  }
}

}  // namespace extensions
