/*	WebFileDatabase.m
	Copyright 2002, Apple, Inc. All rights reserved.
*/

#import "WebFileDatabase.h"

#import <fcntl.h>
#import <fts.h>
#import <pthread.h>
#import <string.h>
#import <sys/stat.h>
#import <sys/types.h>
#import <sys/mman.h>

#import "WebFoundationLogging.h"
#import "WebLRUFileList.h"
#import "WebNSFileManagerExtras.h"
#import "WebSystemBits.h"

#if ERROR_DISABLED
#define BEGIN_EXCEPTION_HANDLER
#define END_EXCEPTION_HANDLER
#else
#define BEGIN_EXCEPTION_HANDLER NS_DURING
#define END_EXCEPTION_HANDLER \
    NS_HANDLER \
        ERROR("Uncaught exception: %@ [%@] [%@]", [localException class], [localException reason], [localException userInfo]); \
    NS_ENDHANDLER
#endif

static pthread_once_t databaseInitControl = PTHREAD_ONCE_INIT;
static NSNumber *WebFileDirectoryPOSIXPermissions;
static NSNumber *WebFilePOSIXPermissions;
static NSRunLoop *syncRunLoop;

#define UniqueFilePathSize (34)
static void UniqueFilePathForKey(id key, char *buffer);

typedef enum
{
    WebFileDatabaseSetObjectOp,
    WebFileDatabaseRemoveObjectOp,
} WebFileDatabaseOpcode;

enum
{
    MAX_UNSIGNED_LENGTH = 20, // long enough to hold the string representation of a 64-bit unsigned number
    SYNC_IDLE_THRESHOLD = 10,
};

// interface WebFileDatabaseOp -------------------------------------------------------------

@interface WebFileDatabaseOp : NSObject
{
    WebFileDatabaseOpcode opcode;
    id key;
    id object; 
}

+(id)opWithCode:(WebFileDatabaseOpcode)opcode key:(id)key object:(id)object;
-(id)initWithCode:(WebFileDatabaseOpcode)opcode key:(id)key object:(id)object;

-(WebFileDatabaseOpcode)opcode;
-(id)key;
-(id)object;
-(void)perform:(WebFileDatabase *)target;

@end


// implementation WebFileDatabaseOp -------------------------------------------------------------

@implementation WebFileDatabaseOp

+(id)opWithCode:(WebFileDatabaseOpcode)theOpcode key:(id)theKey object:(id)theObject
{
    return [[[WebFileDatabaseOp alloc] initWithCode:theOpcode key:theKey object:theObject] autorelease];
}

-(id)initWithCode:(WebFileDatabaseOpcode)theOpcode key:(id)theKey object:(id)theObject
{
    ASSERT(theKey);

    if ((self = [super init])) {
        
        opcode = theOpcode;
        key = [theKey retain];
        object = [theObject retain];
        
        return self;
    }
  
    return nil;
}

-(WebFileDatabaseOpcode)opcode
{
    return opcode;
}

-(id)key
{
    return key;
}

-(id)object
{
    return object;
}

-(void)perform:(WebFileDatabase *)target
{
    ASSERT(target);

    switch (opcode) {
        case WebFileDatabaseSetObjectOp:
            [target performSetObject:object forKey:key];
            break;
        case WebFileDatabaseRemoveObjectOp:
            [target performRemoveObjectForKey:key];
            break;
        default:
            ASSERT_NOT_REACHED();
            break;
    }
}

-(void)dealloc
{
    [key release];
    [object release];
    
    [super dealloc];
}

@end


// interface WebFileDatabasePrivate -----------------------------------------------------------

@interface WebFileDatabase (WebFileDatabasePrivate)

-(void)_createLRUList:(id)arg;
-(void)_truncateToSizeLimit:(unsigned)size;

@end

// implementation WebFileDatabasePrivate ------------------------------------------------------

@implementation WebFileDatabase (WebFileDatabasePrivate)

static void UniqueFilePathForKey(id key, char *buffer)
{
    const char *s;
    UInt32 hash1;
    UInt32 hash2;
    CFIndex len;
    CFIndex cnt;

    s = [[[[key description] lowercaseString] stringByStandardizingPath] lossyCString];
    len = strlen(s);

    // compute first hash    
    hash1 = len;
    for (cnt = 0; cnt < len; cnt++) {
        hash1 += (hash1 << 8) + s[cnt];
    }
    hash1 += (hash1 << (len & 31));

    // compute second hash    
    hash2 = len;
    for (cnt = 0; cnt < len; cnt++) {
        hash2 = (37 * hash2) ^ s[cnt];
    }

    snprintf(buffer, UniqueFilePathSize, "%.2lu/%.2lu/%.10lu-%.10lu.cache", ((hash1 & 0xff) >> 4), ((hash2 & 0xff) >> 4), hash1, hash2);
}

-(void)_createLRUList:(id)arg
{
    WebSetThreadPriority(WebMinThreadPriority + 1); // make this a little higher priority than the syncRunLoop thread

    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    BEGIN_EXCEPTION_HANDLER
    
    WebLRUFileList *fileList = WebLRUFileListCreate();
    WebLRUFileListRebuildFileDataUsingRootDirectory(fileList, [path fileSystemRepresentation]);
    lru = fileList;

    END_EXCEPTION_HANDLER

    LOG(WebFileDatabaseActivity, "lru list created");

    [pool release];
}

-(void)_truncateToSizeLimit:(unsigned)size
{
    NSFileManager *defaultManager;
    
    if (!lru || size > [self usage]) {
        return;
    }

    if (size == 0) {
        [self removeAllObjects];
    }
    else {
        defaultManager = [NSFileManager defaultManager];
        [mutex lock];
        while ([self usage] > size) {
            char uniqueKey[UniqueFilePathSize];
            if (!WebLRUFileListGetPathOfOldestFile(lru, uniqueKey, UniqueFilePathSize)) {
                break;
            }
            NSString *filePath = [[NSString alloc] initWithFormat:@"%@/%s", path, uniqueKey];
            [defaultManager _web_removeFileOnlyAtPath:filePath];
            WebLRUFileListRemoveOldestFileFromList(lru);
        }
        [mutex unlock];
    }
}

@end


// implementation WebFileDatabase -------------------------------------------------------------

@implementation WebFileDatabase

// creation functions ---------------------------------------------------------------------------
#pragma mark creation functions

+(void)_syncLoop:(id)arg
{
    WebSetThreadPriority(WebMinThreadPriority);

    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSPort *placeholderPort;

    BEGIN_EXCEPTION_HANDLER

    syncRunLoop = [NSRunLoop currentRunLoop];

    while (YES) {
        BEGIN_EXCEPTION_HANDLER
        // we specifically use an NSRunLoop here to get autorelease pool support
        placeholderPort = [NSPort port];
        [syncRunLoop addPort:placeholderPort forMode:NSDefaultRunLoopMode];
        [syncRunLoop run];
        [syncRunLoop removePort:placeholderPort forMode:NSDefaultRunLoopMode];
        END_EXCEPTION_HANDLER
    }

    END_EXCEPTION_HANDLER

    [pool release];
}

static void databaseInit()
{
    // set file perms to owner read/write/execute only
    WebFileDirectoryPOSIXPermissions = [[NSNumber numberWithInt:(WEB_UREAD | WEB_UWRITE | WEB_UEXEC)] retain];

    // set file perms to owner read/write only
    WebFilePOSIXPermissions = [[NSNumber numberWithInt:(WEB_UREAD | WEB_UWRITE)] retain];

    [NSThread detachNewThreadSelector:@selector(_syncLoop:) toTarget:[WebFileDatabase class] withObject:nil];
}

-(id)initWithPath:(NSString *)thePath
{
    pthread_once(&databaseInitControl, databaseInit);

    [super initWithPath:thePath];
    
    if (self == nil || thePath == nil) {
        [self dealloc];
        return nil;
    }

    ops = [[NSMutableArray alloc] init];
    setCache = [[NSMutableDictionary alloc] init];
    removeCache = [[NSMutableSet alloc] init];
    timer = nil;
    mutex = [[NSRecursiveLock alloc] init];
    
    return self;
}

-(void)dealloc
{
    [mutex lock];
    // this locking gives time for writes that are happening now to finish
    [mutex unlock];
    [self close];
    [timer invalidate];
    [timer release];
    [ops release];
    [setCache release];
    [removeCache release];
    [mutex release];

    [super dealloc];
}

-(void)setTimer
{
    if (timer == nil) {
        NSDate *fireDate = [[NSDate alloc] initWithTimeIntervalSinceNow:SYNC_IDLE_THRESHOLD];
        timer = [[NSTimer alloc] initWithFireDate:fireDate interval:SYNC_IDLE_THRESHOLD target:self selector:@selector(lazySync:) userInfo:nil repeats:YES];
        [fireDate release];
        [syncRunLoop addTimer:timer forMode:NSDefaultRunLoopMode];
    }
}

// database functions ---------------------------------------------------------------------------
#pragma mark database functions

-(void)setObject:(id)object forKey:(id)key
{
    WebFileDatabaseOp *op;

    ASSERT(object);
    ASSERT(key);

    touch = CFAbsoluteTimeGetCurrent();

    LOG(WebFileDatabaseActivity, "%p - %@", object, key);
    
    [mutex lock];
    
    [setCache setObject:object forKey:key];
    op = [[WebFileDatabaseOp alloc] initWithCode:WebFileDatabaseSetObjectOp key:key object:object];
    [ops addObject:op];
    [op release];
    [self setTimer];
    
    [mutex unlock];
}

-(void)removeObjectForKey:(id)key
{
    WebFileDatabaseOp *op;

    ASSERT(key);

    touch = CFAbsoluteTimeGetCurrent();
    
    [mutex lock];
    
    [removeCache addObject:key];
    op = [[WebFileDatabaseOp alloc] initWithCode:WebFileDatabaseRemoveObjectOp key:key object:nil];
    [ops addObject:op];
    [op release];
    [self setTimer];
    
    [mutex unlock];
}

-(void)removeAllObjects
{
    touch = CFAbsoluteTimeGetCurrent();

    [mutex lock];
    [setCache removeAllObjects];
    [removeCache removeAllObjects];
    [ops removeAllObjects];
    [self close];
    [[NSFileManager defaultManager] _web_backgroundRemoveFileAtPath:path];
    [self open];
    [mutex unlock];

    LOG(WebFileDatabaseActivity, "removeAllObjects");
}

-(id)objectForKey:(id)key
{
    volatile id result;
    
    ASSERT(key);

    touch = CFAbsoluteTimeGetCurrent();

    // check caches
    [mutex lock];
    if ([removeCache containsObject:key]) {
        [mutex unlock];
        return nil;
    }
    if ((result = [setCache objectForKey:key])) {
        [mutex unlock];
        return result;
    }
    [mutex unlock];

    // go to disk
    char uniqueKey[UniqueFilePathSize];
    UniqueFilePathForKey(key, uniqueKey);
    NSString *filePath = [[NSString alloc] initWithFormat:@"%@/%s", path, uniqueKey];
    NSData *data = [[NSData alloc] initWithContentsOfFile:filePath];
    NSUnarchiver * volatile unarchiver = nil;

    NS_DURING
        if (data) {
            unarchiver = [[NSUnarchiver alloc] initForReadingWithData:data];
            if (unarchiver) {
                id fileKey = [unarchiver decodeObject];
                if ([fileKey isEqual:key]) {
                    id object = [unarchiver decodeObject];
                    if (object) {
                        // Decoded objects go away when the unarchiver does, so we need to
                        // retain this so we can return it to our caller.
                        result = [[object retain] autorelease];
                        if (lru) {
                            // if we can't update the list yet, that's too bad
                            // but not critically bad
                            WebLRUFileListTouchFileWithPath(lru, uniqueKey);
                        }
                        LOG(WebFileDatabaseActivity, "read disk cache file - %@", key);
                    }
                }
            }
        }
    NS_HANDLER
        LOG(WebFileDatabaseActivity, "cannot unarchive cache file - %@", key);
        result = nil;
    NS_ENDHANDLER

    [unarchiver release];
    [data release];
    [filePath release];

    return result;
}

-(void)performSetObject:(id)object forKey:(id)key
{
    NSString *filePath;
    NSMutableData *data;
    NSDictionary *attributes;
    NSDictionary *directoryAttributes;
    NSArchiver *archiver;
    NSFileManager *defaultManager;
    char uniqueKey[UniqueFilePathSize];
    BOOL result;

    ASSERT(object);
    ASSERT(key);

    UniqueFilePathForKey(key, uniqueKey);

    LOG(WebFileDatabaseActivity, "%@ - %s", key, uniqueKey);

    data = [NSMutableData data];
    archiver = [[NSArchiver alloc] initForWritingWithMutableData:data];
    [archiver encodeObject:key];
    [archiver encodeObject:object];
    
    attributes = [NSDictionary dictionaryWithObjectsAndKeys:
        NSUserName(), NSFileOwnerAccountName,
        WebFilePOSIXPermissions, NSFilePosixPermissions,
        NULL
    ];

    directoryAttributes = [NSDictionary dictionaryWithObjectsAndKeys:
        NSUserName(), NSFileOwnerAccountName,
        WebFileDirectoryPOSIXPermissions, NSFilePosixPermissions,
        NULL
    ];

    defaultManager = [NSFileManager defaultManager];

    filePath = [[NSString alloc] initWithFormat:@"%@/%s", path, uniqueKey];
    attributes = [defaultManager fileAttributesAtPath:filePath traverseLink:YES];

    // update usage and truncate before writing file
    // this has the effect of _always_ keeping disk usage under sizeLimit by clearing away space in anticipation of the write.
    WebLRUFileListSetFileData(lru, uniqueKey, [data length], CFAbsoluteTimeGetCurrent());
    [self _truncateToSizeLimit:[self sizeLimit]];

    result = [defaultManager _web_createFileAtPathWithIntermediateDirectories:filePath contents:data attributes:attributes directoryAttributes:directoryAttributes];

    if (!result) {
        WebLRUFileListRemoveFileWithPath(lru, uniqueKey);
    }

    [archiver release];
    [filePath release];    
}

-(void)performRemoveObjectForKey:(id)key
{
    NSString *filePath;
    char uniqueKey[UniqueFilePathSize];
    
    ASSERT(key);

    LOG(WebFileDatabaseActivity, "%@", key);

    UniqueFilePathForKey(key, uniqueKey);
    filePath = [[NSString alloc] initWithFormat:@"%@/%s", path, uniqueKey];
    [[NSFileManager defaultManager] _web_removeFileOnlyAtPath:filePath];
    WebLRUFileListRemoveFileWithPath(lru, uniqueKey);
    [filePath release];
}

// database management functions ---------------------------------------------------------------------------
#pragma mark database management functions

-(BOOL)open
{
    NSFileManager *manager;
    NSDictionary *attributes;
    BOOL isDir;
    
    if (!isOpen) {
        manager = [NSFileManager defaultManager];
        if ([manager fileExistsAtPath:path isDirectory:&isDir]) {
            if (isDir) {
                isOpen = YES;
            }
        }
        else {
            attributes = [NSDictionary dictionaryWithObjectsAndKeys:
                [NSDate date], @"NSFileModificationDate",
                NSUserName(), @"NSFileOwnerAccountName",
                WebFileDirectoryPOSIXPermissions, @"NSFilePosixPermissions",
                NULL
            ];
            
	    isOpen = [manager _web_createDirectoryAtPathWithIntermediateDirectories:path attributes:attributes];
	}

        // remove any leftover turds
        [manager _web_deleteBackgroundRemoveLeftoverFiles:path];
        
        if (isOpen) {
            [NSThread detachNewThreadSelector:@selector(_createLRUList:) toTarget:self withObject:nil];
        }
    }
    
    return isOpen;
}

-(BOOL)close
{
    if (isOpen) {
        isOpen = NO;
        if (lru) {
            WebLRUFileListRelease(lru);
            lru = NULL;
        }
    }
    
    return YES;
}

-(void)lazySync:(NSTimer *)theTimer
{
    if (!lru) {
        // wait for lru to finish getting created        
        return;
    }

#ifndef NDEBUG
    CFTimeInterval mark = CFAbsoluteTimeGetCurrent();
#endif

    LOG(WebFileDatabaseActivity, ">>> BEFORE lazySync\n%@", WebLRUFileListDescription(lru));

    WebFileDatabaseOp *op;

    ASSERT(theTimer);

    while (touch + SYNC_IDLE_THRESHOLD < CFAbsoluteTimeGetCurrent() && [ops count] > 0) {
        [mutex lock];

        if (timer) {
            [timer invalidate];
            [timer autorelease];
            timer = nil;
        }
        
        op = [ops lastObject];
        if (op) {
            [op retain];
            [ops removeLastObject];
            [op perform:self];
            [setCache removeObjectForKey:[op key]];
            [removeCache removeObject:[op key]];
            [op release];
        }

        [mutex unlock];
    }

    // come back later to finish the work...
    if ([ops count] > 0) {
        [mutex lock];
        [self setTimer];
        [mutex unlock];
    }

    LOG(WebFileDatabaseActivity, "<<< AFTER lazySync\n%@", WebLRUFileListDescription(lru));

#ifndef NDEBUG
    CFTimeInterval now = CFAbsoluteTimeGetCurrent();
    LOG(WebFileDatabaseActivity, "lazySync ran in %.3f secs.", now - mark);
#endif
}

-(void)sync
{
    NSArray *array;

    if (!lru) {
        // wait for lru to finish getting created        
        return;
    }

    touch = CFAbsoluteTimeGetCurrent();

    LOG(WebFileDatabaseActivity, ">>> BEFORE sync\n%@", WebLRUFileListDescription(lru));
    
    [mutex lock];
    array = [ops copy];
    [ops removeAllObjects];
    [timer invalidate];
    [timer autorelease];
    timer = nil;
    [setCache removeAllObjects];
    [removeCache removeAllObjects];
    [mutex unlock];

    [array makeObjectsPerformSelector:@selector(perform:) withObject:self];
    [array release];

    LOG(WebFileDatabaseActivity, "<<< AFTER sync\n%@", WebLRUFileListDescription(lru));
}

-(unsigned)count
{
    if (lru)
        return WebLRUFileListCountItems(lru);
    
    return 0;
}

-(unsigned)usage
{
    if (lru)
        return WebLRUFileListGetTotalSize(lru);
    
    return 0;
}

-(void)setSizeLimit:(unsigned)limit
{
    sizeLimit = limit;
    if (limit < [self usage]) {
        [self _truncateToSizeLimit:limit];
    }
}

@end
