/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "wtf/PartitionAlloc.h"

#include "wtf/BitwiseOperations.h"
#include "wtf/OwnPtr.h"
#include "wtf/PassOwnPtr.h"
#include <gtest/gtest.h>
#include <stdlib.h>
#include <string.h>

#if OS(POSIX)
#include <sys/mman.h>

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif
#endif // OS(POSIX)

#if !defined(MEMORY_TOOL_REPLACES_ALLOCATOR)

namespace {

static const size_t kTestMaxAllocation = 4096;
static SizeSpecificPartitionAllocator<kTestMaxAllocation> allocator;
static PartitionAllocatorGeneric genericAllocator;

static const size_t kTestAllocSize = sizeof(void*);
#ifdef NDEBUG
static const size_t kPointerOffset = 0;
static const size_t kExtraAllocSize = 0;
#else
static const size_t kPointerOffset = sizeof(uintptr_t);
static const size_t kExtraAllocSize = sizeof(uintptr_t) * 2;
#endif
static const size_t kRealAllocSize = kTestAllocSize + kExtraAllocSize;
static const size_t kTestBucketIndex = kRealAllocSize >> WTF::kBucketShift;

static void TestSetup()
{
    allocator.init();
    genericAllocator.init();
}

static void TestShutdown()
{
    // We expect no leaks in the general case. We have a test for leak
    // detection.
    EXPECT_TRUE(allocator.shutdown());
    EXPECT_TRUE(genericAllocator.shutdown());
}

static WTF::PartitionPage* GetFullPage(size_t size)
{
    size_t realSize = size + kExtraAllocSize;
    size_t bucketIdx = realSize >> WTF::kBucketShift;
    WTF::PartitionBucket* bucket = &allocator.root()->buckets()[bucketIdx];
    size_t numSlots = (bucket->numSystemPagesPerSlotSpan * WTF::kSystemPageSize) / realSize;
    void* first = 0;
    void* last = 0;
    size_t i;
    for (i = 0; i < numSlots; ++i) {
        void* ptr = partitionAlloc(allocator.root(), size);
        EXPECT_TRUE(ptr);
        if (!i)
            first = WTF::partitionCookieFreePointerAdjust(ptr);
        else if (i == numSlots - 1)
            last = WTF::partitionCookieFreePointerAdjust(ptr);
    }
    EXPECT_EQ(WTF::partitionPointerToPage(first), WTF::partitionPointerToPage(last));
    if (bucket->numSystemPagesPerSlotSpan == WTF::kNumSystemPagesPerPartitionPage)
        EXPECT_EQ(reinterpret_cast<size_t>(first) & WTF::kPartitionPageBaseMask, reinterpret_cast<size_t>(last) & WTF::kPartitionPageBaseMask);
    EXPECT_EQ(numSlots, static_cast<size_t>(bucket->activePagesHead->numAllocatedSlots));
    EXPECT_EQ(0, partitionPageFreelistHead(bucket->activePagesHead));
    EXPECT_TRUE(bucket->activePagesHead);
    EXPECT_TRUE(bucket->activePagesHead != &WTF::PartitionRootBase::gSeedPage);
    return bucket->activePagesHead;
}

static void FreeFullPage(WTF::PartitionPage* page)
{
    size_t size = page->bucket->slotSize;
    size_t numSlots = (page->bucket->numSystemPagesPerSlotSpan * WTF::kSystemPageSize) / size;
    EXPECT_EQ(numSlots, static_cast<size_t>(abs(page->numAllocatedSlots)));
    char* ptr = reinterpret_cast<char*>(partitionPageToPointer(page));
    size_t i;
    for (i = 0; i < numSlots; ++i) {
        partitionFree(ptr + kPointerOffset);
        ptr += size;
    }
}

// Check that the most basic of allocate / free pairs work.
TEST(WTF_PartitionAlloc, Basic)
{
    TestSetup();
    WTF::PartitionBucket* bucket = &allocator.root()->buckets()[kTestBucketIndex];

    EXPECT_FALSE(bucket->freePagesHead);
    EXPECT_EQ(&WTF::PartitionRootBase::gSeedPage, bucket->activePagesHead);
    EXPECT_EQ(0, bucket->activePagesHead->u2.activePageNext);

    void* ptr = partitionAlloc(allocator.root(), kTestAllocSize);
    EXPECT_TRUE(ptr);
    EXPECT_EQ(kPointerOffset, reinterpret_cast<size_t>(ptr) & WTF::kPartitionPageOffsetMask);
    // Check that the offset appears to include a guard page.
    EXPECT_EQ(WTF::kPartitionPageSize + kPointerOffset, reinterpret_cast<size_t>(ptr) & WTF::kSuperPageOffsetMask);

    partitionFree(ptr);
    // Expect that the last active page does not get tossed to the freelist.
    EXPECT_FALSE(bucket->freePagesHead);

    TestShutdown();
}

// Check that we can detect a memory leak.
TEST(WTF_PartitionAlloc, SimpleLeak)
{
    TestSetup();
    void* leakedPtr = partitionAlloc(allocator.root(), kTestAllocSize);
    (void)leakedPtr;
    void* leakedPtr2 = partitionAllocGeneric(genericAllocator.root(), kTestAllocSize);
    (void)leakedPtr2;
    EXPECT_FALSE(allocator.shutdown());
    EXPECT_FALSE(genericAllocator.shutdown());
}

// Test multiple allocations, and freelist handling.
TEST(WTF_PartitionAlloc, MultiAlloc)
{
    TestSetup();

    char* ptr1 = reinterpret_cast<char*>(partitionAlloc(allocator.root(), kTestAllocSize));
    char* ptr2 = reinterpret_cast<char*>(partitionAlloc(allocator.root(), kTestAllocSize));
    EXPECT_TRUE(ptr1);
    EXPECT_TRUE(ptr2);
    ptrdiff_t diff = ptr2 - ptr1;
    EXPECT_EQ(static_cast<ptrdiff_t>(kRealAllocSize), diff);

    // Check that we re-use the just-freed slot.
    partitionFree(ptr2);
    ptr2 = reinterpret_cast<char*>(partitionAlloc(allocator.root(), kTestAllocSize));
    EXPECT_TRUE(ptr2);
    diff = ptr2 - ptr1;
    EXPECT_EQ(static_cast<ptrdiff_t>(kRealAllocSize), diff);
    partitionFree(ptr1);
    ptr1 = reinterpret_cast<char*>(partitionAlloc(allocator.root(), kTestAllocSize));
    EXPECT_TRUE(ptr1);
    diff = ptr2 - ptr1;
    EXPECT_EQ(static_cast<ptrdiff_t>(kRealAllocSize), diff);

    char* ptr3 = reinterpret_cast<char*>(partitionAlloc(allocator.root(), kTestAllocSize));
    EXPECT_TRUE(ptr3);
    diff = ptr3 - ptr1;
    EXPECT_EQ(static_cast<ptrdiff_t>(kRealAllocSize * 2), diff);

    partitionFree(ptr1);
    partitionFree(ptr2);
    partitionFree(ptr3);

    TestShutdown();
}

// Test a bucket with multiple pages.
TEST(WTF_PartitionAlloc, MultiPages)
{
    TestSetup();
    WTF::PartitionBucket* bucket = &allocator.root()->buckets()[kTestBucketIndex];

    WTF::PartitionPage* page = GetFullPage(kTestAllocSize);
    FreeFullPage(page);
    EXPECT_FALSE(bucket->freePagesHead);
    EXPECT_EQ(page, bucket->activePagesHead);
    EXPECT_EQ(0, page->u2.activePageNext);
    EXPECT_EQ(0, page->numAllocatedSlots);

    page = GetFullPage(kTestAllocSize);
    WTF::PartitionPage* page2 = GetFullPage(kTestAllocSize);

    EXPECT_EQ(page2, bucket->activePagesHead);
    EXPECT_EQ(0, page2->u2.activePageNext);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(partitionPageToPointer(page)) & WTF::kSuperPageBaseMask, reinterpret_cast<uintptr_t>(partitionPageToPointer(page2)) & WTF::kSuperPageBaseMask);

    // Fully free the non-current page. It should not be freelisted because
    // there is no other immediately useable page. The other page is full.
    FreeFullPage(page);
    EXPECT_EQ(0, page->numAllocatedSlots);
    EXPECT_FALSE(bucket->freePagesHead);
    EXPECT_EQ(page, bucket->activePagesHead);

    // Allocate a new page, it should pull from the freelist.
    page = GetFullPage(kTestAllocSize);
    EXPECT_FALSE(bucket->freePagesHead);
    EXPECT_EQ(page, bucket->activePagesHead);

    FreeFullPage(page);
    FreeFullPage(page2);
    EXPECT_EQ(0, page->numAllocatedSlots);
    EXPECT_EQ(0, page2->numAllocatedSlots);
    EXPECT_TRUE(partitionPageIsFree(page2));

    TestShutdown();
}

// Test some finer aspects of internal page transitions.
TEST(WTF_PartitionAlloc, PageTransitions)
{
    TestSetup();
    WTF::PartitionBucket* bucket = &allocator.root()->buckets()[kTestBucketIndex];

    WTF::PartitionPage* page1 = GetFullPage(kTestAllocSize);
    EXPECT_EQ(page1, bucket->activePagesHead);
    EXPECT_EQ(0, page1->u2.activePageNext);
    WTF::PartitionPage* page2 = GetFullPage(kTestAllocSize);
    EXPECT_EQ(page2, bucket->activePagesHead);
    EXPECT_EQ(0, page2->u2.activePageNext);

    // Bounce page1 back into the non-full list then fill it up again.
    char* ptr = reinterpret_cast<char*>(partitionPageToPointer(page1)) + kPointerOffset;
    partitionFree(ptr);
    EXPECT_EQ(page1, bucket->activePagesHead);
    (void) partitionAlloc(allocator.root(), kTestAllocSize);
    EXPECT_EQ(page1, bucket->activePagesHead);
    EXPECT_EQ(page2, bucket->activePagesHead->u2.activePageNext);

    // Allocating another page at this point should cause us to scan over page1
    // (which is both full and NOT our current page), and evict it from the
    // freelist. Older code had a O(n^2) condition due to failure to do this.
    WTF::PartitionPage* page3 = GetFullPage(kTestAllocSize);
    EXPECT_EQ(page3, bucket->activePagesHead);
    EXPECT_EQ(0, page3->u2.activePageNext);

    // Work out a pointer into page2 and free it.
    ptr = reinterpret_cast<char*>(partitionPageToPointer(page2)) + kPointerOffset;
    partitionFree(ptr);
    // Trying to allocate at this time should cause us to cycle around to page2
    // and find the recently freed slot.
    char* newPtr = reinterpret_cast<char*>(partitionAlloc(allocator.root(), kTestAllocSize));
    EXPECT_EQ(ptr, newPtr);
    EXPECT_EQ(page2, bucket->activePagesHead);
    EXPECT_EQ(page3, page2->u2.activePageNext);

    // Work out a pointer into page1 and free it. This should pull the page
    // back into the list of available pages.
    ptr = reinterpret_cast<char*>(partitionPageToPointer(page1)) + kPointerOffset;
    partitionFree(ptr);
    // This allocation should be satisfied by page1.
    newPtr = reinterpret_cast<char*>(partitionAlloc(allocator.root(), kTestAllocSize));
    EXPECT_EQ(ptr, newPtr);
    EXPECT_EQ(page1, bucket->activePagesHead);
    EXPECT_EQ(page2, page1->u2.activePageNext);

    FreeFullPage(page3);
    FreeFullPage(page2);
    FreeFullPage(page1);

    // Allocating whilst in this state exposed a bug, so keep the test.
    ptr = reinterpret_cast<char*>(partitionAlloc(allocator.root(), kTestAllocSize));
    partitionFree(ptr);

    TestShutdown();
}

// Test some corner cases relating to page transitions in the internal
// free page list metadata bucket.
TEST(WTF_PartitionAlloc, FreePageListPageTransitions)
{
    TestSetup();
    WTF::PartitionBucket* bucket = &allocator.root()->buckets()[kTestBucketIndex];

    size_t numToFillFreeListPage = WTF::kPartitionPageSize / (sizeof(WTF::PartitionPage) + kExtraAllocSize);
    // The +1 is because we need to account for the fact that the current page
    // never gets thrown on the freelist.
    ++numToFillFreeListPage;
    OwnPtr<WTF::PartitionPage*[]> pages = adoptArrayPtr(new WTF::PartitionPage*[numToFillFreeListPage]);

    size_t i;
    for (i = 0; i < numToFillFreeListPage; ++i) {
        pages[i] = GetFullPage(kTestAllocSize);
    }
    EXPECT_EQ(pages[numToFillFreeListPage - 1], bucket->activePagesHead);
    for (i = 0; i < numToFillFreeListPage; ++i)
        FreeFullPage(pages[i]);
    EXPECT_EQ(0, bucket->activePagesHead->numAllocatedSlots);
    EXPECT_EQ(0, bucket->activePagesHead->u2.activePageNext);

    // Allocate / free in a different bucket size so we get control of a
    // different free page list. We need two pages because one will be the last
    // active page and not get freed.
    WTF::PartitionPage* page1 = GetFullPage(kTestAllocSize * 2);
    WTF::PartitionPage* page2 = GetFullPage(kTestAllocSize * 2);
    FreeFullPage(page1);
    FreeFullPage(page2);

    // If we re-allocate all kTestAllocSize allocations, we'll pull all the
    // free pages and end up freeing the first page for free page objects.
    // It's getting a bit tricky but a nice re-entrancy is going on:
    // alloc(kTestAllocSize) -> pulls page from free page list ->
    // free(PartitionFreepagelistEntry) -> last entry in page freed ->
    // alloc(PartitionFreepagelistEntry).
    for (i = 0; i < numToFillFreeListPage; ++i) {
        pages[i] = GetFullPage(kTestAllocSize);
    }
    EXPECT_EQ(pages[numToFillFreeListPage - 1], bucket->activePagesHead);

    // As part of the final free-up, we'll test another re-entrancy:
    // free(kTestAllocSize) -> last entry in page freed ->
    // alloc(PartitionFreepagelistEntry) -> pulls page from free page list ->
    // free(PartitionFreepagelistEntry)
    for (i = 0; i < numToFillFreeListPage; ++i)
        FreeFullPage(pages[i]);
    EXPECT_EQ(0, bucket->activePagesHead->numAllocatedSlots);
    EXPECT_EQ(0, bucket->activePagesHead->u2.activePageNext);

    TestShutdown();
}

// Test a large series of allocations that cross more than one underlying
// 64KB super page allocation.
TEST(WTF_PartitionAlloc, MultiPageAllocs)
{
    TestSetup();
    // This is guaranteed to cross a super page boundary because the first
    // partition page "slot" will be taken up by a guard page.
    size_t numPagesNeeded = WTF::kNumPartitionPagesPerSuperPage;
    // The super page should begin and end in a guard so we one less page in
    // order to allocate a single page in the new super page.
    --numPagesNeeded;

    EXPECT_GT(numPagesNeeded, 1u);
    OwnPtr<WTF::PartitionPage*[]> pages;
    pages = adoptArrayPtr(new WTF::PartitionPage*[numPagesNeeded]);
    uintptr_t firstSuperPageBase = 0;
    size_t i;
    for (i = 0; i < numPagesNeeded; ++i) {
        pages[i] = GetFullPage(kTestAllocSize);
        void* storagePtr = partitionPageToPointer(pages[i]);
        if (!i)
            firstSuperPageBase = reinterpret_cast<uintptr_t>(storagePtr) & WTF::kSuperPageBaseMask;
        if (i == numPagesNeeded - 1) {
            uintptr_t secondSuperPageBase = reinterpret_cast<uintptr_t>(storagePtr) & WTF::kSuperPageBaseMask;
            uintptr_t secondSuperPageOffset = reinterpret_cast<uintptr_t>(storagePtr) & WTF::kSuperPageOffsetMask;
            EXPECT_FALSE(secondSuperPageBase == firstSuperPageBase);
            // Check that we allocated a guard page for the second page.
            EXPECT_EQ(WTF::kPartitionPageSize, secondSuperPageOffset);
        }
    }
    for (i = 0; i < numPagesNeeded; ++i)
        FreeFullPage(pages[i]);

    TestShutdown();
}

// Test the generic allocation functions that can handle arbitrary sizes and
// reallocing etc.
TEST(WTF_PartitionAlloc, GenericAlloc)
{
    TestSetup();

    void* ptr = partitionAllocGeneric(genericAllocator.root(), 1);
    EXPECT_TRUE(ptr);
    partitionFreeGeneric(genericAllocator.root(), ptr);
    ptr = partitionAllocGeneric(genericAllocator.root(), WTF::kGenericMaxSlotBucketed + 1);
    EXPECT_TRUE(ptr);
    partitionFreeGeneric(genericAllocator.root(), ptr);

    ptr = partitionAllocGeneric(genericAllocator.root(), 1);
    EXPECT_TRUE(ptr);
    void* origPtr = ptr;
    char* charPtr = static_cast<char*>(ptr);
    *charPtr = 'A';

    // Change the size of the realloc, remaining inside the same bucket.
    void* newPtr = partitionReallocGeneric(genericAllocator.root(), ptr, 2);
    EXPECT_EQ(ptr, newPtr);
    newPtr = partitionReallocGeneric(genericAllocator.root(), ptr, 1);
    EXPECT_EQ(ptr, newPtr);
    newPtr = partitionReallocGeneric(genericAllocator.root(), ptr, WTF::kGenericSmallestBucket);
    EXPECT_EQ(ptr, newPtr);

    // Change the size of the realloc, switching buckets.
    newPtr = partitionReallocGeneric(genericAllocator.root(), ptr, WTF::kGenericSmallestBucket + 1);
    EXPECT_NE(newPtr, ptr);
    // Check that the realloc copied correctly.
    char* newCharPtr = static_cast<char*>(newPtr);
    EXPECT_EQ(*newCharPtr, 'A');
#ifndef NDEBUG
    // Subtle: this checks for an old bug where we copied too much from the
    // source of the realloc. The condition can be detected by a trashing of
    // the uninitialized value in the space of the upsized allocation.
    EXPECT_EQ(WTF::kUninitializedByte, static_cast<unsigned char>(*(newCharPtr + WTF::kGenericSmallestBucket)));
#endif
    *newCharPtr = 'B';
    // The realloc moved. To check that the old allocation was freed, we can
    // do an alloc of the old allocation size and check that the old allocation
    // address is at the head of the freelist and reused.
    void* reusedPtr = partitionAllocGeneric(genericAllocator.root(), 1);
    EXPECT_EQ(reusedPtr, origPtr);
    partitionFreeGeneric(genericAllocator.root(), reusedPtr);

    // Downsize the realloc.
    ptr = newPtr;
    newPtr = partitionReallocGeneric(genericAllocator.root(), ptr, 1);
    EXPECT_EQ(newPtr, origPtr);
    newCharPtr = static_cast<char*>(newPtr);
    EXPECT_EQ(*newCharPtr, 'B');
    *newCharPtr = 'C';

    // Upsize the realloc into the paged zone.
    ptr = newPtr;
    newPtr = partitionReallocGeneric(genericAllocator.root(), ptr, WTF::kGenericMaxSlotBucketed + 1);
    EXPECT_NE(newPtr, ptr);
    newCharPtr = static_cast<char*>(newPtr);
    EXPECT_EQ(*newCharPtr, 'C');
    *newCharPtr = 'D';

    // Upsize and downsize the realloc, remaining in the paged zone.
    ptr = newPtr;
    newPtr = partitionReallocGeneric(genericAllocator.root(), ptr, WTF::kGenericMaxSlotBucketed * 10);
    newCharPtr = static_cast<char*>(newPtr);
    EXPECT_EQ(*newCharPtr, 'D');
    *newCharPtr = 'E';
    ptr = newPtr;
    newPtr = partitionReallocGeneric(genericAllocator.root(), ptr, WTF::kGenericMaxSlotBucketed * 2);
    newCharPtr = static_cast<char*>(newPtr);
    EXPECT_EQ(*newCharPtr, 'E');
    *newCharPtr = 'F';

    // Upsize the realloc into a direct mapped mapping.
    ptr = newPtr;
    newPtr = partitionReallocGeneric(genericAllocator.root(), ptr, WTF::kGenericMaxPageBucketed + 1);
    EXPECT_NE(newPtr, ptr);
    newCharPtr = static_cast<char*>(newPtr);
    EXPECT_EQ(*newCharPtr, 'F');
    *newCharPtr = 'G';

    // Upsize and downsize the realloc, remaining in the direct mapped zone.
    ptr = newPtr;
    newPtr = partitionReallocGeneric(genericAllocator.root(), ptr, WTF::kGenericMaxPageBucketed * 2);
    newCharPtr = static_cast<char*>(newPtr);
    EXPECT_EQ(*newCharPtr, 'G');
    *newCharPtr = 'H';
    ptr = newPtr;
    newPtr = partitionReallocGeneric(genericAllocator.root(), ptr, WTF::kGenericMaxPageBucketed + 1);
    newCharPtr = static_cast<char*>(newPtr);
    EXPECT_EQ(*newCharPtr, 'H');
    *newCharPtr = 'I';

    // Downsize the realloc to the paged zone.
    ptr = newPtr;
    newPtr = partitionReallocGeneric(genericAllocator.root(), ptr, WTF::kGenericMaxSlotBucketed + 1);
    EXPECT_NE(newPtr, ptr);
    newCharPtr = static_cast<char*>(newPtr);
    EXPECT_EQ(*newCharPtr, 'I');
    *newCharPtr = 'J';

    // Downsize the realloc to the slotted zone.
    ptr = newPtr;
    newPtr = partitionReallocGeneric(genericAllocator.root(), ptr, 1);
    EXPECT_NE(newPtr, ptr);
    EXPECT_EQ(newPtr, origPtr);
    newCharPtr = static_cast<char*>(newPtr);
    EXPECT_EQ(*newCharPtr, 'J');

    partitionFreeGeneric(genericAllocator.root(), newPtr);
    TestShutdown();
}

// Test the generic allocation functions can handle some specific sizes of
// interest.
TEST(WTF_PartitionAlloc, GenericAllocSizes)
{
    TestSetup();

    void* ptr = partitionAllocGeneric(genericAllocator.root(), 0);
    EXPECT_TRUE(ptr);
    partitionFreeGeneric(genericAllocator.root(), ptr);

    // kPartitionPageSize is interesting because it results in just one
    // allocation per page, which tripped up some corner cases.
    size_t size = WTF::kPartitionPageSize - kExtraAllocSize;
    ptr = partitionAllocGeneric(genericAllocator.root(), size);
    EXPECT_TRUE(ptr);
    void* ptr2 = partitionAllocGeneric(genericAllocator.root(), size);
    EXPECT_TRUE(ptr2);
    partitionFreeGeneric(genericAllocator.root(), ptr);
    // Should have freelisted at this point.
    WTF::PartitionPage* page = WTF::partitionPointerToPage(WTF::partitionCookieFreePointerAdjust(ptr));
    EXPECT_TRUE(page->bucket->freePagesHead);
    partitionFreeGeneric(genericAllocator.root(), ptr2);

    size = WTF::kGenericMaxSlotBucketed - kExtraAllocSize;
    ptr = partitionAllocGeneric(genericAllocator.root(), size);
    EXPECT_TRUE(ptr);
    memset(ptr, 'A', size);
    ptr2 = partitionAllocGeneric(genericAllocator.root(), size);
    EXPECT_TRUE(ptr2);
    void* ptr3 = partitionAllocGeneric(genericAllocator.root(), size);
    EXPECT_TRUE(ptr3);
    void* ptr4 = partitionAllocGeneric(genericAllocator.root(), size);
    EXPECT_TRUE(ptr4);

    page = WTF::partitionPointerToPage(WTF::partitionCookieFreePointerAdjust(ptr));
    EXPECT_NE(page, WTF::partitionPointerToPage(WTF::partitionCookieFreePointerAdjust(ptr3)));

    partitionFreeGeneric(genericAllocator.root(), ptr);
    partitionFreeGeneric(genericAllocator.root(), ptr3);
    partitionFreeGeneric(genericAllocator.root(), ptr2);
    // Should have freelisted at this point.
    EXPECT_TRUE(page->bucket->freePagesHead);
    ptr3 = partitionAllocGeneric(genericAllocator.root(), size);
    EXPECT_TRUE(ptr3);
    void* newPtr = partitionAllocGeneric(genericAllocator.root(), size);
    EXPECT_EQ(ptr, newPtr);
#if OS(LINUX) && defined(NDEBUG)
    // On Linux, we have a guarantee that freelisting a page should cause its
    // contents to be nulled out. We check for null here to detect an bug we
    // had where a large slot size was causing us to not properly free all
    // resources back to the system.
    // We only run the check in optimized builds because the debug build
    // writes over the allocated area with an "uninitialized" byte pattern.
    EXPECT_EQ(0, *(reinterpret_cast<char*>(newPtr) + (size - 1)));
#endif
    partitionFreeGeneric(genericAllocator.root(), newPtr);
    partitionFreeGeneric(genericAllocator.root(), ptr3);
    partitionFreeGeneric(genericAllocator.root(), ptr4);

    // Can we allocate a massive (512MB) size?
    ptr = partitionAllocGeneric(genericAllocator.root(), 512 * 1024 * 1024);
    partitionFreeGeneric(genericAllocator.root(), ptr);

    // Check a more reasonable, but still direct mapped, size.
    // Chop a system page and a byte off to test for rounding errors.
    size = 20 * 1024 * 1024;
    size -= WTF::kSystemPageSize;
    size -= 1;
    ptr = partitionAllocGeneric(genericAllocator.root(), size);
    char* charPtr = reinterpret_cast<char*>(ptr);
    *(charPtr + (size - 1)) = 'A';
    partitionFreeGeneric(genericAllocator.root(), ptr);

    // Check allocating and freeing a couple of paged sizes.
    ptr = partitionAllocGeneric(genericAllocator.root(), WTF::kGenericMaxSlotBucketed + 1);
    ptr2 = partitionAllocGeneric(genericAllocator.root(), WTF::kGenericMaxSlotBucketed + 1);
    partitionFreeGeneric(genericAllocator.root(), ptr2);
    partitionFreeGeneric(genericAllocator.root(), ptr);
    // And again but free them the other way around.
    ptr = partitionAllocGeneric(genericAllocator.root(), WTF::kGenericMaxSlotBucketed + 1);
    ptr2 = partitionAllocGeneric(genericAllocator.root(), WTF::kGenericMaxSlotBucketed + 1);
    partitionFreeGeneric(genericAllocator.root(), ptr);
    partitionFreeGeneric(genericAllocator.root(), ptr2);

    // Can we free null?
    partitionFreeGeneric(genericAllocator.root(), 0);

    TestShutdown();
}

// Tests the handing out of freelists for partial pages.
TEST(WTF_PartitionAlloc, PartialPageFreelists)
{
    TestSetup();

    size_t bigSize = allocator.root()->maxAllocation - kExtraAllocSize;
    EXPECT_EQ(WTF::kSystemPageSize - WTF::kAllocationGranularity, bigSize + kExtraAllocSize);
    size_t bucketIdx = (bigSize + kExtraAllocSize) >> WTF::kBucketShift;
    WTF::PartitionBucket* bucket = &allocator.root()->buckets()[bucketIdx];
    EXPECT_EQ(0, bucket->freePagesHead);

    void* ptr = partitionAlloc(allocator.root(), bigSize);
    EXPECT_TRUE(ptr);

    WTF::PartitionPage* page = WTF::partitionPointerToPage(WTF::partitionCookieFreePointerAdjust(ptr));
    size_t totalSlots = (page->bucket->numSystemPagesPerSlotSpan * WTF::kSystemPageSize) / (bigSize + kExtraAllocSize);
    EXPECT_EQ(4u, totalSlots);
    // The freelist should have one entry, because we were able to exactly fit
    // one object slot and one freelist pointer (the null that the head points
    // to) into a system page.
    EXPECT_TRUE(partitionPageFreelistHead(page));
    EXPECT_EQ(1, page->numAllocatedSlots);
    EXPECT_EQ(2, page->numUnprovisionedSlots);

    void* ptr2 = partitionAlloc(allocator.root(), bigSize);
    EXPECT_TRUE(ptr2);
    EXPECT_FALSE(partitionPageFreelistHead(page));
    EXPECT_EQ(2, page->numAllocatedSlots);
    EXPECT_EQ(2, page->numUnprovisionedSlots);

    void* ptr3 = partitionAlloc(allocator.root(), bigSize);
    EXPECT_TRUE(ptr3);
    EXPECT_TRUE(partitionPageFreelistHead(page));
    EXPECT_EQ(3, page->numAllocatedSlots);
    EXPECT_EQ(0, page->numUnprovisionedSlots);

    void* ptr4 = partitionAlloc(allocator.root(), bigSize);
    EXPECT_TRUE(ptr4);
    EXPECT_FALSE(partitionPageFreelistHead(page));
    EXPECT_EQ(4, page->numAllocatedSlots);
    EXPECT_EQ(0, page->numUnprovisionedSlots);

    void* ptr5 = partitionAlloc(allocator.root(), bigSize);
    EXPECT_TRUE(ptr5);

    WTF::PartitionPage* page2 = WTF::partitionPointerToPage(WTF::partitionCookieFreePointerAdjust(ptr5));
    EXPECT_EQ(1, page2->numAllocatedSlots);

    // Churn things a little whilst there's a partial page freelist.
    partitionFree(ptr);
    ptr = partitionAlloc(allocator.root(), bigSize);
    void* ptr6 = partitionAlloc(allocator.root(), bigSize);

    partitionFree(ptr);
    partitionFree(ptr2);
    partitionFree(ptr3);
    partitionFree(ptr4);
    partitionFree(ptr5);
    partitionFree(ptr6);
    EXPECT_TRUE(bucket->freePagesHead);
    EXPECT_EQ(page, bucket->freePagesHead);
    EXPECT_TRUE(partitionPageFreelistHead(page2));
    EXPECT_EQ(0, page2->numAllocatedSlots);

    // And test a couple of sizes that do not cross kSystemPageSize with a single allocation.
    size_t mediumSize = (WTF::kSystemPageSize / 2) - kExtraAllocSize;
    bucketIdx = (mediumSize + kExtraAllocSize) >> WTF::kBucketShift;
    bucket = &allocator.root()->buckets()[bucketIdx];
    EXPECT_EQ(0, bucket->freePagesHead);

    ptr = partitionAlloc(allocator.root(), mediumSize);
    EXPECT_TRUE(ptr);
    page = WTF::partitionPointerToPage(WTF::partitionCookieFreePointerAdjust(ptr));
    EXPECT_EQ(1, page->numAllocatedSlots);
    totalSlots = (page->bucket->numSystemPagesPerSlotSpan * WTF::kSystemPageSize) / (mediumSize + kExtraAllocSize);
    size_t firstPageSlots = WTF::kSystemPageSize / (mediumSize + kExtraAllocSize);
    EXPECT_EQ(2u, firstPageSlots);
    EXPECT_EQ(totalSlots - firstPageSlots, page->numUnprovisionedSlots);

    partitionFree(ptr);

    size_t smallSize = (WTF::kSystemPageSize / 4) - kExtraAllocSize;
    bucketIdx = (smallSize + kExtraAllocSize) >> WTF::kBucketShift;
    bucket = &allocator.root()->buckets()[bucketIdx];
    EXPECT_EQ(0, bucket->freePagesHead);

    ptr = partitionAlloc(allocator.root(), smallSize);
    EXPECT_TRUE(ptr);
    page = WTF::partitionPointerToPage(WTF::partitionCookieFreePointerAdjust(ptr));
    EXPECT_EQ(1, page->numAllocatedSlots);
    totalSlots = (page->bucket->numSystemPagesPerSlotSpan * WTF::kSystemPageSize) / (smallSize + kExtraAllocSize);
    firstPageSlots = WTF::kSystemPageSize / (smallSize + kExtraAllocSize);
    EXPECT_EQ(totalSlots - firstPageSlots, page->numUnprovisionedSlots);

    partitionFree(ptr);
    EXPECT_TRUE(partitionPageFreelistHead(page));
    EXPECT_EQ(0, page->numAllocatedSlots);

    size_t verySmallSize = (WTF::kAllocationGranularity * 2) - kExtraAllocSize;
    bucketIdx = (verySmallSize + kExtraAllocSize) >> WTF::kBucketShift;
    bucket = &allocator.root()->buckets()[bucketIdx];
    EXPECT_EQ(0, bucket->freePagesHead);

    ptr = partitionAlloc(allocator.root(), verySmallSize);
    EXPECT_TRUE(ptr);
    page = WTF::partitionPointerToPage(WTF::partitionCookieFreePointerAdjust(ptr));
    EXPECT_EQ(1, page->numAllocatedSlots);
    totalSlots = (page->bucket->numSystemPagesPerSlotSpan * WTF::kSystemPageSize) / (verySmallSize + kExtraAllocSize);
    firstPageSlots = WTF::kSystemPageSize / (verySmallSize + kExtraAllocSize);
    EXPECT_EQ(totalSlots - firstPageSlots, page->numUnprovisionedSlots);

    partitionFree(ptr);
    EXPECT_TRUE(partitionPageFreelistHead(page));
    EXPECT_EQ(0, page->numAllocatedSlots);

    // And try an allocation size (against the generic allocator) that is
    // larger than a system page.
    size_t pageAndAHalfSize = (WTF::kSystemPageSize + (WTF::kSystemPageSize / 2)) - kExtraAllocSize;
    ptr = partitionAllocGeneric(genericAllocator.root(), pageAndAHalfSize);
    EXPECT_TRUE(ptr);
    page = WTF::partitionPointerToPage(WTF::partitionCookieFreePointerAdjust(ptr));
    EXPECT_EQ(1, page->numAllocatedSlots);
    EXPECT_TRUE(partitionPageFreelistHead(page));
    totalSlots = (page->bucket->numSystemPagesPerSlotSpan * WTF::kSystemPageSize) / (pageAndAHalfSize + kExtraAllocSize);
    EXPECT_EQ(totalSlots - 2, page->numUnprovisionedSlots);
    partitionFreeGeneric(genericAllocator.root(), ptr);

    // And then make sure than exactly the page size only faults one page.
    size_t pageSize = WTF::kSystemPageSize - kExtraAllocSize;
    ptr = partitionAllocGeneric(genericAllocator.root(), pageSize);
    EXPECT_TRUE(ptr);
    page = WTF::partitionPointerToPage(WTF::partitionCookieFreePointerAdjust(ptr));
    EXPECT_EQ(1, page->numAllocatedSlots);
    EXPECT_FALSE(partitionPageFreelistHead(page));
    totalSlots = (page->bucket->numSystemPagesPerSlotSpan * WTF::kSystemPageSize) / (pageSize + kExtraAllocSize);
    EXPECT_EQ(totalSlots - 1, page->numUnprovisionedSlots);
    partitionFreeGeneric(genericAllocator.root(), ptr);

    TestShutdown();
}

// Test some of the fragmentation-resistant properties of the allocator.
TEST(WTF_PartitionAlloc, PageRefilling)
{
    TestSetup();
    WTF::PartitionBucket* bucket = &allocator.root()->buckets()[kTestBucketIndex];

    // Grab two full pages and a non-full page.
    WTF::PartitionPage* page1 = GetFullPage(kTestAllocSize);
    WTF::PartitionPage* page2 = GetFullPage(kTestAllocSize);
    void* ptr = partitionAlloc(allocator.root(), kTestAllocSize);
    EXPECT_TRUE(ptr);
    EXPECT_NE(page1, bucket->activePagesHead);
    EXPECT_NE(page2, bucket->activePagesHead);
    WTF::PartitionPage* page = WTF::partitionPointerToPage(WTF::partitionCookieFreePointerAdjust(ptr));
    EXPECT_EQ(1, page->numAllocatedSlots);

    // Work out a pointer into page2 and free it; and then page1 and free it.
    char* ptr2 = reinterpret_cast<char*>(WTF::partitionPageToPointer(page1)) + kPointerOffset;
    partitionFree(ptr2);
    ptr2 = reinterpret_cast<char*>(WTF::partitionPageToPointer(page2)) + kPointerOffset;
    partitionFree(ptr2);

    // If we perform two allocations from the same bucket now, we expect to
    // refill both the nearly full pages.
    (void) partitionAlloc(allocator.root(), kTestAllocSize);
    (void) partitionAlloc(allocator.root(), kTestAllocSize);
    EXPECT_EQ(1, page->numAllocatedSlots);

    FreeFullPage(page2);
    FreeFullPage(page1);
    partitionFree(ptr);

    TestShutdown();
}

// Basic tests to ensure that allocations work for partial page buckets.
TEST(WTF_PartitionAlloc, PartialPages)
{
    TestSetup();

    // Find a size that is backed by a partial partition page.
    size_t size = sizeof(void*);
    WTF::PartitionBucket* bucket = 0;
    while (size < kTestMaxAllocation) {
        bucket = &allocator.root()->buckets()[size >> WTF::kBucketShift];
        if (bucket->numSystemPagesPerSlotSpan % WTF::kNumSystemPagesPerPartitionPage)
            break;
        size += sizeof(void*);
    }
    EXPECT_LT(size, kTestMaxAllocation);

    WTF::PartitionPage* page1 = GetFullPage(size);
    WTF::PartitionPage* page2 = GetFullPage(size);
    FreeFullPage(page2);
    FreeFullPage(page1);

    TestShutdown();
}

// Test correct handling if our mapping collides with another.
TEST(WTF_PartitionAlloc, MappingCollision)
{
    TestSetup();
    // The -2 is because the first and last partition pages in a super page are
    // guard pages.
    size_t numPartitionPagesNeeded = WTF::kNumPartitionPagesPerSuperPage - 2;
    OwnPtr<WTF::PartitionPage*[]> firstSuperPagePages = adoptArrayPtr(new WTF::PartitionPage*[numPartitionPagesNeeded]);
    OwnPtr<WTF::PartitionPage*[]> secondSuperPagePages = adoptArrayPtr(new WTF::PartitionPage*[numPartitionPagesNeeded]);

    size_t i;
    for (i = 0; i < numPartitionPagesNeeded; ++i)
        firstSuperPagePages[i] = GetFullPage(kTestAllocSize);

    char* pageBase = reinterpret_cast<char*>(WTF::partitionPageToPointer(firstSuperPagePages[0]));
    EXPECT_EQ(WTF::kPartitionPageSize, reinterpret_cast<uintptr_t>(pageBase) & WTF::kSuperPageOffsetMask);
    pageBase -= WTF::kPartitionPageSize;
    // Map a single system page either side of the mapping for our allocations,
    // with the goal of tripping up alignment of the next mapping.
    void* map1 = WTF::allocPages(pageBase - WTF::kPageAllocationGranularity, WTF::kPageAllocationGranularity, WTF::kPageAllocationGranularity);
    EXPECT_TRUE(map1);
    void* map2 = WTF::allocPages(pageBase + WTF::kSuperPageSize, WTF::kPageAllocationGranularity, WTF::kPageAllocationGranularity);
    EXPECT_TRUE(map2);
    WTF::setSystemPagesInaccessible(map1, WTF::kPageAllocationGranularity);
    WTF::setSystemPagesInaccessible(map2, WTF::kPageAllocationGranularity);

    for (i = 0; i < numPartitionPagesNeeded; ++i)
        secondSuperPagePages[i] = GetFullPage(kTestAllocSize);

    WTF::freePages(map1, WTF::kPageAllocationGranularity);
    WTF::freePages(map2, WTF::kPageAllocationGranularity);

    pageBase = reinterpret_cast<char*>(partitionPageToPointer(secondSuperPagePages[0]));
    EXPECT_EQ(WTF::kPartitionPageSize, reinterpret_cast<uintptr_t>(pageBase) & WTF::kSuperPageOffsetMask);
    pageBase -= WTF::kPartitionPageSize;
    // Map a single system page either side of the mapping for our allocations,
    // with the goal of tripping up alignment of the next mapping.
    map1 = WTF::allocPages(pageBase - WTF::kPageAllocationGranularity, WTF::kPageAllocationGranularity, WTF::kPageAllocationGranularity);
    EXPECT_TRUE(map1);
    map2 = WTF::allocPages(pageBase + WTF::kSuperPageSize, WTF::kPageAllocationGranularity, WTF::kPageAllocationGranularity);
    EXPECT_TRUE(map2);
    WTF::setSystemPagesInaccessible(map1, WTF::kPageAllocationGranularity);
    WTF::setSystemPagesInaccessible(map2, WTF::kPageAllocationGranularity);

    WTF::PartitionPage* pageInThirdSuperPage = GetFullPage(kTestAllocSize);
    WTF::freePages(map1, WTF::kPageAllocationGranularity);
    WTF::freePages(map2, WTF::kPageAllocationGranularity);

    EXPECT_EQ(0u, reinterpret_cast<uintptr_t>(partitionPageToPointer(pageInThirdSuperPage)) & WTF::kPartitionPageOffsetMask);

    // And make sure we really did get a page in a new superpage.
    EXPECT_NE(reinterpret_cast<uintptr_t>(partitionPageToPointer(firstSuperPagePages[0])) & WTF::kSuperPageBaseMask, reinterpret_cast<uintptr_t>(partitionPageToPointer(pageInThirdSuperPage)) & WTF::kSuperPageBaseMask);
    EXPECT_NE(reinterpret_cast<uintptr_t>(partitionPageToPointer(secondSuperPagePages[0])) & WTF::kSuperPageBaseMask, reinterpret_cast<uintptr_t>(partitionPageToPointer(pageInThirdSuperPage)) & WTF::kSuperPageBaseMask);

    FreeFullPage(pageInThirdSuperPage);
    for (i = 0; i < numPartitionPagesNeeded; ++i) {
        FreeFullPage(firstSuperPagePages[i]);
        FreeFullPage(secondSuperPagePages[i]);
    }

    TestShutdown();
}

// Make sure that malloc(-1) dies.
// In the past, we had an integer overflow that would alias malloc(-1) to
// malloc(0), which is not good.
TEST(WTF_PartitionAllocDeathTest, LargeAllocs)
{
    TestSetup();
    // Largest alloc.
    EXPECT_DEATH(partitionAllocGeneric(genericAllocator.root(), static_cast<size_t>(-1)), "");
    // And the smallest allocation we expect to die.
    EXPECT_DEATH(partitionAllocGeneric(genericAllocator.root(), static_cast<size_t>(INT_MAX) + 1), "");

    TestShutdown();
}

// Check that our immediate double-free detection works.
TEST(WTF_PartitionAllocDeathTest, DoubleFree)
{
    TestSetup();

    void* ptr = partitionAllocGeneric(genericAllocator.root(), kTestAllocSize);
    EXPECT_TRUE(ptr);
    partitionFreeGeneric(genericAllocator.root(), ptr);

    EXPECT_DEATH(partitionFreeGeneric(genericAllocator.root(), ptr), "");

    TestShutdown();
}

// Check that guard pages are present where expected.
TEST(WTF_PartitionAllocDeathTest, GuardPages)
{
    TestSetup();

    // This large size will result in a direct mapped allocation with guard
    // pages at either end.
    size_t size = (WTF::kGenericMaxPageBucketed + WTF::kSystemPageSize) - kExtraAllocSize;
    void* ptr = partitionAllocGeneric(genericAllocator.root(), size);
    EXPECT_TRUE(ptr);
    char* charPtr = reinterpret_cast<char*>(ptr) - kPointerOffset;

    EXPECT_DEATH(*(charPtr - 1) = 'A', "");
    EXPECT_DEATH(*(charPtr + size + kExtraAllocSize) = 'A', "");

    partitionFreeGeneric(genericAllocator.root(), ptr);

    TestShutdown();
}

// Tests that the countLeadingZeros() functions work to our satisfaction.
// It doesn't seem worth the overhead of a whole new file for these tests, so
// we'll put them here since partitionAllocGeneric will depend heavily on these
// functions working correctly.
TEST(WTF_PartitionAlloc, CLZWorks)
{
    EXPECT_EQ(32u, WTF::countLeadingZeros32(0));
    EXPECT_EQ(31u, WTF::countLeadingZeros32(1));
    EXPECT_EQ(1u, WTF::countLeadingZeros32(1 << 30));
    EXPECT_EQ(0u, WTF::countLeadingZeros32(1 << 31));

#if CPU(64BIT)
    EXPECT_EQ(64u, WTF::countLeadingZerosSizet(0ull));
    EXPECT_EQ(63u, WTF::countLeadingZerosSizet(1ull));
    EXPECT_EQ(32u, WTF::countLeadingZerosSizet(1ull << 31));
    EXPECT_EQ(1u, WTF::countLeadingZerosSizet(1ull << 62));
    EXPECT_EQ(0u, WTF::countLeadingZerosSizet(1ull << 63));
#else
    EXPECT_EQ(32u, WTF::countLeadingZerosSizet(0));
    EXPECT_EQ(31u, WTF::countLeadingZerosSizet(1));
    EXPECT_EQ(1u, WTF::countLeadingZerosSizet(1 << 30));
    EXPECT_EQ(0u, WTF::countLeadingZerosSizet(1 << 31));
#endif
}

} // namespace

#endif // !defined(MEMORY_TOOL_REPLACES_ALLOCATOR)
