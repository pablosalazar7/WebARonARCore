// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sync/syncable/parent_child_index.h"

#include "base/stl_util.h"

#include "sync/syncable/entry_kernel.h"
#include "sync/syncable/syncable_id.h"

namespace syncer {
namespace syncable {

bool ChildComparator::operator()(const EntryKernel* a,
                                 const EntryKernel* b) const {
  const UniquePosition& a_pos = a->ref(UNIQUE_POSITION);
  const UniquePosition& b_pos = b->ref(UNIQUE_POSITION);

  if (a_pos.IsValid() && b_pos.IsValid()) {
    // Position is important to this type.
    return a_pos.LessThan(b_pos);
  } else if (a_pos.IsValid() && !b_pos.IsValid()) {
    // TODO(rlarocque): Remove this case.
    // An item with valid position as sibling of one with invalid position.
    // We should not support this, but the tests rely on it.  For now, just
    // move all invalid position items to the right.
    return true;
  } else if (!a_pos.IsValid() && b_pos.IsValid()) {
    // TODO(rlarocque): Remove this case.
    // Mirror of the above case.
    return false;
  } else {
    // Position doesn't matter.
    DCHECK(!a->ref(UNIQUE_POSITION).IsValid());
    DCHECK(!b->ref(UNIQUE_POSITION).IsValid());
    // Sort by META_HANDLE to ensure consistent order for testing.
    return a->ref(META_HANDLE) < b->ref(META_HANDLE);
  }
}

ParentChildIndex::ParentChildIndex() {
  // Pre-allocate these two vectors to the number of model types.
  model_type_root_ids_.resize(MODEL_TYPE_COUNT);
  type_root_child_sets_.resize(MODEL_TYPE_COUNT);
}

ParentChildIndex::~ParentChildIndex() {
  for (int i = 0; i < MODEL_TYPE_COUNT; i++) {
    // Normally all OrderedChildSet instances in |type_root_child_sets_|
    // are shared with |parent_children_map_|. Null out shared instances and
    // ScopedVector will take care of the ones that are not shared (if any).
    if (!model_type_root_ids_[i].IsNull()) {
      DCHECK_EQ(type_root_child_sets_[i],
                parent_children_map_.find(model_type_root_ids_[i])->second);
      type_root_child_sets_[i] = nullptr;
    }
  }

  STLDeleteContainerPairSecondPointers(parent_children_map_.begin(),
                                       parent_children_map_.end());
}

bool ParentChildIndex::ShouldInclude(const EntryKernel* entry) {
  // This index excludes deleted items and the root item.  The root
  // item is excluded so that it doesn't show up as a child of itself.
  return !entry->ref(IS_DEL) && !entry->ref(ID).IsRoot();
}

bool ParentChildIndex::Insert(EntryKernel* entry) {
  DCHECK(ShouldInclude(entry));

  OrderedChildSet* siblings = nullptr;
  const Id& parent_id = entry->ref(PARENT_ID);
  ModelType model_type = entry->GetModelType();

  if (ShouldUseParentId(parent_id, model_type)) {
    // Hierarchical type, lookup child set in the map.
    DCHECK(!parent_id.IsNull());
    ParentChildrenMap::iterator it = parent_children_map_.find(parent_id);
    if (it != parent_children_map_.end()) {
      siblings = it->second;
    } else {
      siblings = new OrderedChildSet();
      parent_children_map_.insert(std::make_pair(parent_id, siblings));
    }
  } else {
    // Non-hierarchical type, return a pre-defined collection by type.
    siblings = GetOrCreateModelTypeChildSet(model_type);
  }

  // If this is one of type root folder for a non-hierarchical type, associate
  // its ID with the model type and the type's pre-defined child set with the
  // type root ID.
  // TODO(stanisc): crbug/438313: Just TypeSupportsHierarchy condition should
  // theoretically be sufficient but in practice many tests don't properly
  // initialize entries so TypeSupportsHierarchy ends up failing. Consider
  // tweaking TypeSupportsHierarchy and fixing all related test code.
  if (parent_id.IsRoot() && entry->ref(IS_DIR) &&
      syncer::IsRealDataType(model_type) &&
      !TypeSupportsHierarchy(model_type)) {
    const Id& type_root_id = entry->ref(ID);
    // Disassociate the type root child set with the previous ID if any.
    const Id& prev_type_root_id = model_type_root_ids_[model_type];
    if (!prev_type_root_id.IsNull()) {
      ParentChildrenMap::iterator it =
          parent_children_map_.find(prev_type_root_id);
      if (it != parent_children_map_.end()) {
        // If the entry exists in the map it must already have the same
        // model type specific child set. This child set will be re-inserted
        // in the map under the new ID below so it is safe to simply erase
        // the entry here.
        DCHECK_EQ(it->second, GetModelTypeChildSet(model_type));
        parent_children_map_.erase(it);
      }
    }
    // Store the new type root ID and associate the child set.
    // Note that the child set isn't owned by the map in this case.
    model_type_root_ids_[model_type] = type_root_id;
    parent_children_map_.insert(
        std::make_pair(type_root_id, GetOrCreateModelTypeChildSet(model_type)));
  }

  // Finally, insert the entry in the child set.
  return siblings->insert(entry).second;
}

// Like the other containers used to help support the syncable::Directory, this
// one does not own any EntryKernels.  This function removes references to the
// given EntryKernel but does not delete it.
void ParentChildIndex::Remove(EntryKernel* e) {
  OrderedChildSet* siblings = nullptr;
  ModelType model_type = e->GetModelType();
  const Id& parent_id = e->ref(PARENT_ID);
  bool should_erase = false;
  ParentChildrenMap::iterator it;

  if (ShouldUseParentId(parent_id, model_type)) {
    // Hierarchical type, lookup child set in the map.
    DCHECK(!parent_id.IsNull());
    it = parent_children_map_.find(parent_id);
    DCHECK(it != parent_children_map_.end());
    siblings = it->second;
    should_erase = true;
  } else {
    // Non-hierarchical type, return a pre-defined child set by type.
    siblings = type_root_child_sets_[model_type];
  }

  OrderedChildSet::iterator j = siblings->find(e);
  DCHECK(j != siblings->end());

  // Erase the entry from the child set.
  siblings->erase(j);
  // If the set is now empty and isn't shareable with |type_root_child_sets_|,
  // erase it from the map.
  if (siblings->empty() && should_erase) {
    delete siblings;
    parent_children_map_.erase(it);
  }
}

bool ParentChildIndex::Contains(EntryKernel *e) const {
  const OrderedChildSet* siblings = GetChildSet(e);
  return siblings && siblings->count(e) > 0;
}

const OrderedChildSet* ParentChildIndex::GetChildren(const Id& id) const {
  DCHECK(!id.IsNull());

  ParentChildrenMap::const_iterator parent = parent_children_map_.find(id);
  if (parent == parent_children_map_.end()) {
    return nullptr;
  }

  OrderedChildSet* children = parent->second;
  // The expectation is that the function returns nullptr instead of an empty
  // child set
  if (children && children->empty())
    children = nullptr;
  return children;
}

const OrderedChildSet* ParentChildIndex::GetChildren(EntryKernel* e) const {
  return GetChildren(e->ref(ID));
}

const OrderedChildSet* ParentChildIndex::GetSiblings(EntryKernel* e) const {
  // This implies the entry is in the index.
  DCHECK(Contains(e));
  const OrderedChildSet* siblings = GetChildSet(e);
  DCHECK(siblings && !siblings->empty());
  return siblings;
}

/* static */
bool ParentChildIndex::ShouldUseParentId(const Id& parent_id,
                                         ModelType model_type) {
  // For compatibility with legacy unit tests, in addition to hierarchical
  // entries, this returns true any entries directly under root and for entries
  // of UNSPECIFIED model type.
  return parent_id.IsRoot() || TypeSupportsHierarchy(model_type) ||
         !syncer::IsRealDataType(model_type);
}

const OrderedChildSet* ParentChildIndex::GetChildSet(EntryKernel* e) const {
  ModelType model_type = e->GetModelType();

  const Id& parent_id = e->ref(PARENT_ID);
  if (ShouldUseParentId(parent_id, model_type)) {
    // Hierarchical type, lookup child set in the map.
    ParentChildrenMap::const_iterator it = parent_children_map_.find(parent_id);
    if (it == parent_children_map_.end())
      return nullptr;
    return it->second;
  }

  // Non-hierarchical type, return a collection indexed by type.
  return GetModelTypeChildSet(model_type);
}

const OrderedChildSet* ParentChildIndex::GetModelTypeChildSet(
    ModelType model_type) const {
  return type_root_child_sets_[model_type];
}

OrderedChildSet* ParentChildIndex::GetOrCreateModelTypeChildSet(
    ModelType model_type) {
  if (!type_root_child_sets_[model_type])
    type_root_child_sets_[model_type] = new OrderedChildSet();
  return type_root_child_sets_[model_type];
}

const Id& ParentChildIndex::GetModelTypeRootId(ModelType model_type) const {
  return model_type_root_ids_[model_type];
}

}  // namespace syncable
}  // namespace syncer
