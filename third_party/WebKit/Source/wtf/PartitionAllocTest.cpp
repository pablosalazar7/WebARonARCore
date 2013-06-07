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

#include "wtf/CryptographicallyRandomNumber.h"
#include <gtest/gtest.h>
#include <stdlib.h>
#include <string.h>

#if OS(LINUX) && CPU(X86_64) && defined(NDEBUG)

namespace {

static PartitionRoot root;

static const int kTestAllocSize = sizeof(void*);

static void RandomNumberSource(unsigned char* buf, size_t len)
{
    memset(buf, '\0', len);
}

static void TestSetup()
{
    WTF::setRandomSource(RandomNumberSource);
    partitionAllocInit(&root);
}

static void TestShutdown()
{
    partitionAllocShutdown(&root);
}

static WTF::PartitionPageHeader* GetFullPage()
{
    size_t bucketIdx = kTestAllocSize >> WTF::kBucketShift;
    WTF::PartitionBucket* bucket = &root.buckets[bucketIdx];
    size_t numSlots = (WTF::kPageSize - sizeof(WTF::PartitionPageHeader)) / kTestAllocSize;
    void* first = 0;
    void* last = 0;
    size_t i;
    for (i = 0; i < numSlots; ++i) {
        void* ptr = partitionAlloc(&root, kTestAllocSize);
        EXPECT_TRUE(ptr);
        if (!i)
            first = ptr;
        else if (i == numSlots - 1)
            last = ptr;
    }
    EXPECT_EQ(reinterpret_cast<size_t>(first) & WTF::kPageMask, reinterpret_cast<size_t>(last) & WTF::kPageMask);
    EXPECT_EQ(numSlots, static_cast<size_t>(bucket->currPage->numAllocatedSlots));
    EXPECT_EQ(0, bucket->currPage->freelistHead);
    return bucket->currPage;
}

static void FreeFullPage(WTF::PartitionPageHeader* page)
{
    size_t numSlots = (WTF::kPageSize - sizeof(WTF::PartitionPageHeader)) / kTestAllocSize;
    EXPECT_EQ(numSlots, static_cast<size_t>(abs(page->numAllocatedSlots)));
    char* ptr = reinterpret_cast<char*>(page);
    ptr += sizeof(WTF::PartitionPageHeader);
    size_t i;
    for (i = 0; i < numSlots; ++i) {
        partitionFree(ptr);
        ptr += kTestAllocSize;
    }
    EXPECT_EQ(0, page->numAllocatedSlots);
}

// Check that the most basic of allocate / free pairs work.
TEST(WTF_PartitionAlloc, Basic)
{
    TestSetup();
    size_t bucketIdx = kTestAllocSize >> WTF::kBucketShift;
    WTF::PartitionBucket* bucket = &root.buckets[bucketIdx];

    EXPECT_EQ(0, bucket->freePages);
    EXPECT_EQ(&bucket->seedPage, bucket->currPage);
    EXPECT_EQ(&bucket->seedPage, bucket->currPage->next);
    EXPECT_EQ(&bucket->seedPage, bucket->currPage->prev);

    void* ptr = partitionAlloc(&root, kTestAllocSize);
    EXPECT_TRUE(ptr);
    EXPECT_EQ(sizeof(WTF::PartitionPageHeader), reinterpret_cast<size_t>(ptr) & ~WTF::kPageMask);

    partitionFree(ptr);
    // Expect that a just-freed page doesn't get tossed to the freelist.
    EXPECT_EQ(0, bucket->freePages);

    TestShutdown();
}

// Test multiple allocations, and freelist handling.
TEST(WTF_PartitionAlloc, MultiAlloc)
{
    TestSetup();

    char* ptr1 = reinterpret_cast<char*>(partitionAlloc(&root, kTestAllocSize));
    char* ptr2 = reinterpret_cast<char*>(partitionAlloc(&root, kTestAllocSize));
    EXPECT_TRUE(ptr1);
    EXPECT_TRUE(ptr2);
    ptrdiff_t diff = ptr2 - ptr1;
    EXPECT_EQ(kTestAllocSize, diff);

    // Check that we re-use the just-freed slot.
    partitionFree(ptr2);
    ptr2 = reinterpret_cast<char*>(partitionAlloc(&root, kTestAllocSize));
    EXPECT_TRUE(ptr2);
    diff = ptr2 - ptr1;
    EXPECT_EQ(kTestAllocSize, diff);
    partitionFree(ptr1);
    ptr1 = reinterpret_cast<char*>(partitionAlloc(&root, kTestAllocSize));
    EXPECT_TRUE(ptr1);
    diff = ptr2 - ptr1;
    EXPECT_EQ(kTestAllocSize, diff);

    char* ptr3 = reinterpret_cast<char*>(partitionAlloc(&root, kTestAllocSize));
    EXPECT_TRUE(ptr3);
    diff = ptr3 - ptr1;
    EXPECT_EQ(kTestAllocSize * 2, diff);

    partitionFree(ptr1);
    partitionFree(ptr2);
    partitionFree(ptr3);

    TestShutdown();
}

// Test a bucket with multiple pages.
TEST(WTF_PartitionAlloc, MultiPages)
{
    TestSetup();
    size_t bucketIdx = kTestAllocSize >> WTF::kBucketShift;
    WTF::PartitionBucket* bucket = &root.buckets[bucketIdx];

    WTF::PartitionPageHeader* page = GetFullPage();
    FreeFullPage(page);
    EXPECT_EQ(0, bucket->freePages);

    page = GetFullPage();
    WTF::PartitionPageHeader* page2 = GetFullPage();

    EXPECT_EQ(page2, bucket->currPage);

    // Fully free the non-current page, it should be freelisted.
    FreeFullPage(page);
    EXPECT_EQ(0, page->numAllocatedSlots);
    EXPECT_TRUE(bucket->freePages);
    EXPECT_EQ(page, bucket->freePages->page);
    EXPECT_EQ(page2, bucket->currPage);

    // Allocate a new page, it should pull from the freelist.
    page = GetFullPage();
    EXPECT_FALSE(bucket->freePages);
    EXPECT_EQ(page, bucket->currPage);

    FreeFullPage(page);
    FreeFullPage(page2);
    EXPECT_EQ(0, page->numAllocatedSlots);
    EXPECT_EQ(0, page2->numAllocatedSlots);

    TestShutdown();
}

// Test some finer aspects of internal page transitions.
TEST(WTF_PartitionAlloc, PageTransitions)
{
    TestSetup();
    size_t bucketIdx = kTestAllocSize >> WTF::kBucketShift;
    WTF::PartitionBucket* bucket = &root.buckets[bucketIdx];

    WTF::PartitionPageHeader* page1 = GetFullPage();
    WTF::PartitionPageHeader* page2 = GetFullPage();
    EXPECT_EQ(page2, bucket->currPage);
    EXPECT_EQ(page1, bucket->seedPage.next);
    // Allocating another page at this point should cause us to scan over page1
    // (which is both full and NOT our current page), and evict it from the
    // freelist. Older code had a O(n^2) condition due to failure to do this.
    WTF::PartitionPageHeader* page3 = GetFullPage();
    EXPECT_EQ(page3, bucket->currPage);
    EXPECT_EQ(page2, bucket->seedPage.next);
    EXPECT_EQ(page3, bucket->seedPage.next->next);
    EXPECT_EQ(&bucket->seedPage, bucket->seedPage.next->next->next);

    // Work out a pointer into page2 and free it.
    char* ptr = reinterpret_cast<char*>(page2) + sizeof(WTF::PartitionPageHeader);
    partitionFree(ptr);
    // Trying to allocate at this time should cause us to cycle around to page2
    // and find the recently freed slot.
    char* newPtr = reinterpret_cast<char*>(partitionAlloc(&root, kTestAllocSize));
    EXPECT_EQ(ptr, newPtr);
    EXPECT_EQ(page2, bucket->currPage);

    // Work out a pointer into page1 and free it. This should pull the page
    // back into the ring list of available pages.
    ptr = reinterpret_cast<char*>(page1) + sizeof(WTF::PartitionPageHeader);
    partitionFree(ptr);
    // This allocation should be satisfied by page1.
    newPtr = reinterpret_cast<char*>(partitionAlloc(&root, kTestAllocSize));
    EXPECT_EQ(ptr, newPtr);
    EXPECT_EQ(page1, bucket->currPage);

    FreeFullPage(page3);
    FreeFullPage(page2);
    FreeFullPage(page1);

    TestShutdown();
}

} // namespace

#endif // OS(LINUX) && CPU(X86_64)
