/*	IFURLFileDatabase.m
	Copyright 2002, Apple, Inc. All rights reserved.
*/

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <pthread.h>

#import "IFURLFileDatabase.h"
#import "IFNSFileManagerExtensions.h"
#import "IFURLCacheLoaderConstantsPrivate.h"
#import "WebFoundationDebug.h"

static NSNumber *IFURLFileDirectoryPosixPermissions;
static NSNumber *IFURLFilePosixPermissions;

typedef enum
{
    IFURLFileDatabaseSetObjectOp,
    IFURLFileDatabaseRemoveObjectOp,
} IFURLFileDatabaseOpCode;

enum
{
    SYNC_INTERVAL = 5,
    SYNC_IDLE_THRESHOLD = 5,
};

// interface IFURLFileReader -------------------------------------------------------------

@interface IFURLFileReader : NSObject
{
    NSData *data;
    caddr_t mappedBytes;
    size_t mappedLength;
}

- (id)initWithPath:(NSString *)path;
- (NSData *)data;

@end

// implementation IFURLFileReader -------------------------------------------------------------

/*
 * FIXME: This is a bad hack which really should go away.
 * Here we're using private API to hold us over until
 * this API is made public (which is planned).
 */
@interface NSData (IFExtensions)
- (id)initWithBytes:(void *)bytes length:(unsigned)length copy:(BOOL)copy freeWhenDone:(BOOL)freeBytes bytesAreVM:(BOOL)vm;
@end

static NSMutableSet *notMappableFileNameSet = nil;
static NSLock *mutex;
static pthread_once_t cacheFileReaderControl = PTHREAD_ONCE_INIT;

static void URLFileReaderInit()
{
    mutex = [[NSLock alloc] init];
    notMappableFileNameSet = [[NSMutableSet alloc] init];    
}

@implementation IFURLFileReader

- (id)initWithPath:(NSString *)path
{
    int fd;
    struct stat statInfo;
    const char *fileSystemPath;
    BOOL fileNotMappable;

    pthread_once(&cacheFileReaderControl, URLFileReaderInit);

    self = [super init];
    
    data = nil;
    mappedBytes = NULL;
    mappedLength = 0;

    NS_DURING
        fileSystemPath = [path fileSystemRepresentation];
    NS_HANDLER
        fileSystemPath = NULL;
    NS_ENDHANDLER

    [mutex lock];
    fileNotMappable = [notMappableFileNameSet containsObject:path];
    [mutex unlock];

    if (fileNotMappable) {
        data = [NSData dataWithContentsOfFile:path];
    }
    else if (fileSystemPath && (fd = open(fileSystemPath, O_RDONLY, 0)) >= 0) {
        // File exists. Retrieve the file size.
        if (fstat(fd, &statInfo) == 0) {
            // Map the file into a read-only memory region.
            mappedBytes = mmap(NULL, statInfo.st_size, PROT_READ, 0, fd, 0);
            if (mappedBytes == MAP_FAILED) {
                // map has failed but file exists
                // add file to set of paths known not to be mappable
                // then, read file from file system
                [mutex lock];
                [notMappableFileNameSet addObject:path];
                [mutex unlock];
                
                mappedBytes = NULL;
                data = [NSData dataWithContentsOfFile:path];
            }
            else {
                // On success, create data object using mapped bytes.
                mappedLength = statInfo.st_size;
                data = [[NSData alloc] initWithBytes:mappedBytes length:mappedLength copy:NO freeWhenDone:YES bytesAreVM:YES];
                // ok data creation failed but we know file exists
                // be stubborn....try to read bytes again
                if (!data) {
                    munmap(mappedBytes, mappedLength);    
                    data = [NSData dataWithContentsOfFile:path];
                }
            }
        }
        close(fd);
    }
    
    if (data) {
        if (mappedBytes) {
            WEBFOUNDATIONDEBUGLEVEL(WebFoundationLogDiskCacheActivity, "mmaped disk cache file - %s", [path lossyCString]);
        }
        else {
            WEBFOUNDATIONDEBUGLEVEL(WebFoundationLogDiskCacheActivity, "fs read disk cache file - %s", [path lossyCString]);
        }
        return self;
    }
    else {
        WEBFOUNDATIONDEBUGLEVEL(WebFoundationLogDiskCacheActivity, "no disk cache file - %s", [path lossyCString]);
        [self dealloc];
        return nil;
    }
}

- (NSData *)data
{
    return data;
}

- (void)dealloc
{
    if (mappedBytes) {
        munmap(mappedBytes, mappedLength); 
    }
    
    [data release];
    
    [super dealloc];
}

@end

// interface IFURLFileDatabaseOp -------------------------------------------------------------

@interface IFURLFileDatabaseOp : NSObject
{
    IFURLFileDatabaseOpCode opcode;
    id key;
    id object; 
}

+(id)opWithCode:(IFURLFileDatabaseOpCode)opcode key:(id)key object:(id)object;
-(id)initWithCode:(IFURLFileDatabaseOpCode)opcode key:(id)key object:(id)object;

-(IFURLFileDatabaseOpCode)opcode;
-(id)key;
-(id)object;
-(void)perform:(IFURLFileDatabase *)target;

@end


// implementation IFURLFileDatabaseOp -------------------------------------------------------------

@implementation IFURLFileDatabaseOp

+(id)opWithCode:(IFURLFileDatabaseOpCode)theOpcode key:(id)theKey object:(id)theObject
{
    return [[[IFURLFileDatabaseOp alloc] initWithCode:theOpcode key:theKey object:theObject] autorelease];
}

-(id)initWithCode:(IFURLFileDatabaseOpCode)theOpcode key:(id)theKey object:(id)theObject
{
    if ((self = [super init])) {
        
        opcode = theOpcode;
        key = [theKey retain];
        object = [theObject retain];
        
        return self;
    }
  
    [self release];
    return nil;
}

-(IFURLFileDatabaseOpCode)opcode
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

-(void)perform:(IFURLFileDatabase *)target
{
    switch (opcode) {
        case IFURLFileDatabaseSetObjectOp:
            [target performSetObject:object forKey:key];
            break;
        case IFURLFileDatabaseRemoveObjectOp:
            [target performRemoveObjectForKey:key];
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


// implementation IFURLFileDatabase -------------------------------------------------------------

@implementation IFURLFileDatabase

// creation functions ---------------------------------------------------------------------------
#pragma mark creation functions

+(void)initialize
{
    // set file perms to owner read/write/execute only
    IFURLFileDirectoryPosixPermissions = [[NSNumber numberWithInt:(IF_UREAD | IF_UWRITE | IF_UEXEC)] retain];

    // set file perms to owner read/write only
    IFURLFilePosixPermissions = [[NSNumber numberWithInt:(IF_UREAD | IF_UWRITE)] retain];
}

-(id)initWithPath:(NSString *)thePath
{
    if ((self = [super initWithPath:thePath])) {
    
        ops = [[NSMutableArray alloc] init];
        setCache = [[NSMutableDictionary alloc] init];
        removeCache = [[NSMutableSet alloc] init];
        timer = nil;
        mutex = [[NSLock alloc] init];

        return self;
    }
    
    [self release];
    return nil;
}

-(void)dealloc
{
    [self close];
    [self sync];
    
    [setCache release];
    [removeCache release];
    [mutex release];

    [super dealloc];
}

+(NSString *)uniqueFilePathForKey:(id)key
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

    // create the path and return it
    return [NSString stringWithFormat:@"%.2u/%.2u/%.10u-%.10u.cache", ((hash1 & 0xff) >> 4), ((hash2 & 0xff) >> 4), hash1, hash2];
}

-(void)setTimer
{
    if (timer == nil) {
        timer = [[NSTimer scheduledTimerWithTimeInterval:SYNC_INTERVAL target:self selector:@selector(lazySync:) userInfo:nil repeats:YES] retain];
    }
}

// database functions ---------------------------------------------------------------------------
#pragma mark database functions

-(void)setObject:(id)object forKey:(id)key
{
    IFURLFileDatabaseOp *op;

    touch = CFAbsoluteTimeGetCurrent();
    
    [mutex lock];
    
    [setCache setObject:object forKey:key];
    op = [[IFURLFileDatabaseOp alloc] initWithCode:IFURLFileDatabaseSetObjectOp key:key object:object];
    [ops addObject:op];
    [self setTimer];
    
    [mutex unlock];
}

-(void)removeObjectForKey:(id)key
{
    IFURLFileDatabaseOp *op;

    touch = CFAbsoluteTimeGetCurrent();
    
    [mutex lock];
    
    [removeCache addObject:key];
    op = [[IFURLFileDatabaseOp alloc] initWithCode:IFURLFileDatabaseRemoveObjectOp key:key object:nil];
    [ops addObject:op];
    [self setTimer];
    
    [mutex unlock];
}

-(void)removeAllObjects
{
    touch = CFAbsoluteTimeGetCurrent();

    [mutex lock];
    [setCache removeAllObjects];
    [removeCache removeAllObjects];
    [self close];
    [[NSFileManager defaultManager] backgroundRemoveFileAtPath:path];
    [self open];
    [mutex unlock];

    WEBFOUNDATIONDEBUGLEVEL(WebFoundationLogDiskCacheActivity, "removeAllObjects");
}

-(id)objectForKey:(id)key
{
    id result;
    id fileKey;
    id object;
    NSString *filePath;
    IFURLFileReader *fileReader;
    NSData *data;
    NSUnarchiver *unarchiver;
        
    result = nil;
    fileKey = nil;
    fileReader = nil;
    data = nil;
    unarchiver = nil;

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
    filePath = [[NSString alloc] initWithFormat:@"%@/%@", path, [IFURLFileDatabase uniqueFilePathForKey:key]];
    fileReader = [[IFURLFileReader alloc] initWithPath:filePath];
    if (fileReader && (data = [fileReader data])) {
        unarchiver = [[NSUnarchiver alloc] initForReadingWithData:data];
    }
    
    NS_DURING
        if (unarchiver) {
            fileKey = [unarchiver decodeObject];
            object = [unarchiver decodeObject];
            if (object && [fileKey isEqual:key]) {
                // make sure this object stays around until client has had a chance at it
                result = [object retain];
                [result autorelease];
            }
        }
    NS_HANDLER
        WEBFOUNDATIONDEBUGLEVEL(WebFoundationLogDiskCacheActivity, "cannot unarchive cache file - %s", DEBUG_OBJECT(key));
        result = nil;
    NS_ENDHANDLER

    [unarchiver release];
    [fileReader release];
    [filePath release];

    return result;
}

-(NSEnumerator *)keys
{
    // FIXME: [kocienda] Radar 2859370 (IFURLFileDatabase needs to implement keys method)
    return nil;
}


-(void)performSetObject:(id)object forKey:(id)key
{
    BOOL result;
    NSString *filePath;
    NSMutableData *data;
    NSDictionary *attributes;
    NSDictionary *directoryAttributes;
    NSArchiver *archiver;
    NSFileManager *defaultManager;

    WEBFOUNDATIONDEBUGLEVEL(WebFoundationLogDiskCacheActivity, "performSetObject - %s - %s",
        DEBUG_OBJECT(key), DEBUG_OBJECT([IFURLFileDatabase uniqueFilePathForKey:key]));

    result = NO;

    data = [NSMutableData data];
    archiver = [[NSArchiver alloc] initForWritingWithMutableData:data];
    [archiver encodeObject:key];
    [archiver encodeObject:object];
    
    attributes = [NSDictionary dictionaryWithObjectsAndKeys:
        [NSDate date], @"NSFileModificationDate",
        NSUserName(), @"NSFileOwnerAccountName",
        IFURLFilePosixPermissions, @"NSFilePosixPermissions",
        NULL
    ];

    directoryAttributes = [NSDictionary dictionaryWithObjectsAndKeys:
        [NSDate date], @"NSFileModificationDate",
        NSUserName(), @"NSFileOwnerAccountName",
        IFURLFileDirectoryPosixPermissions, @"NSFilePosixPermissions",
        NULL
    ];

    defaultManager = [NSFileManager defaultManager];

    filePath = [[NSString alloc] initWithFormat:@"%@/%@", path, [IFURLFileDatabase uniqueFilePathForKey:key]];

    result = [defaultManager createFileAtPath:filePath contents:data attributes:attributes];
    if (!result) {
        result = [defaultManager createFileAtPathWithIntermediateDirectories:filePath contents:data attributes:attributes directoryAttributes:directoryAttributes];
    }

    [archiver release];
    [filePath release];
}

-(void)performRemoveObjectForKey:(id)key
{
    NSString *filePath;

    WEBFOUNDATIONDEBUGLEVEL(WebFoundationLogDiskCacheActivity, "performRemoveObjectForKey - %s", DEBUG_OBJECT(key));

    filePath = [[NSString alloc] initWithFormat:@"%@/%@", path, [IFURLFileDatabase uniqueFilePathForKey:key]];
    [[NSFileManager defaultManager] removeFileAtPath:filePath handler:nil];
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
                IFURLFileDirectoryPosixPermissions, @"NSFilePosixPermissions",
                NULL
            ];
            
            // be optimistic that full subpath leading to directory exists
            isOpen = [manager createDirectoryAtPath:path attributes:attributes];
            if (!isOpen) {
                // perhaps the optimism did not pay off ...
                // try again, this time creating full subpath leading to directory
                isOpen = [manager createDirectoryAtPathWithIntermediateDirectories:path attributes:attributes];
            }
        }
    }
    
    return isOpen;
}

-(BOOL)close
{
    if (isOpen) {
        isOpen = NO;
    }
    
    return YES;
}

-(void)lazySync:(NSTimer *)theTimer
{
    IFURLFileDatabaseOp *op;

    while (touch + SYNC_IDLE_THRESHOLD < CFAbsoluteTimeGetCurrent() && [ops count] > 0) {
        [mutex lock];

        if (timer) {
            [timer invalidate];
            [timer autorelease];
            timer = nil;
        }
        
        op = [ops lastObject];
        if (op) {
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
}

-(void)sync
{
    NSArray *array;
    int opCount;
    int i;
    IFURLFileDatabaseOp *op;

    touch = CFAbsoluteTimeGetCurrent();
    
    array = nil;

    [mutex lock];
    if ([ops count] > 0) {
        array = [NSArray arrayWithArray:ops];
        [ops removeAllObjects];
    }
    if (timer) {
        [timer invalidate];
        [timer autorelease];
        timer = nil;
    }
    [setCache removeAllObjects];
    [removeCache removeAllObjects];
    [mutex unlock];

    opCount = [array count];
    for (i = 0; i < opCount; i++) {
        op = [array objectAtIndex:i];
        [op perform:self];
    }
}

@end
