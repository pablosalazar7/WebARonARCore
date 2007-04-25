/*
 * Copyright (C) 2005 Apple Computer, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import <WebKit/WebPluginDatabase.h>

#import <JavaScriptCore/Assertions.h>
#import <WebKit/WebBasePluginPackage.h>
#import <WebKit/WebDataSourcePrivate.h>
#import <WebKit/WebFrame.h>
#import <WebKit/WebFrameViewInternal.h>
#import <WebKit/WebHTMLRepresentation.h>
#import <WebKit/WebHTMLView.h>
#import <WebKit/WebKitLogging.h>
#import <WebKit/WebNetscapePluginPackage.h>
#import <WebKit/WebPluginPackage.h>
#import <WebKit/WebViewPrivate.h>
#import <WebKitSystemInterface.h>

@interface WebPluginDatabase (Internal)
+ (NSArray *)_defaultPlugInPaths;
- (NSArray *)_plugInPaths;
- (void)_addPlugin:(WebBasePluginPackage *)plugin;
- (void)_removePlugin:(WebBasePluginPackage *)plugin;
- (NSMutableSet *)_scanForNewPlugins;
@end

@implementation WebPluginDatabase

static WebPluginDatabase *sharedDatabase = nil;

+ (WebPluginDatabase *)sharedDatabase 
{
    if (!sharedDatabase) {
        sharedDatabase = [[WebPluginDatabase alloc] init];
        [sharedDatabase setPlugInPaths:[self _defaultPlugInPaths]];
        [sharedDatabase refresh];
    }
    
    return sharedDatabase;
}

- (WebBasePluginPackage *)pluginForKey:(NSString *)key withEnumeratorSelector:(SEL)enumeratorSelector
{
    WebBasePluginPackage *plugin, *CFMPlugin=nil, *machoPlugin=nil, *webPlugin=nil;
    NSEnumerator *pluginEnumerator = [plugins objectEnumerator];
    key = [key lowercaseString];

    while ((plugin = [pluginEnumerator nextObject]) != nil) {
        if ([[[plugin performSelector:enumeratorSelector] allObjects] containsObject:key]) {
            if ([plugin isKindOfClass:[WebPluginPackage class]]) {
                if (!webPlugin)
                    webPlugin = plugin;
            } else if([plugin isKindOfClass:[WebNetscapePluginPackage class]]) {
                WebExecutableType executableType = [(WebNetscapePluginPackage *)plugin executableType];
                if (executableType == WebCFMExecutableType) {
                    if (!CFMPlugin)
                        CFMPlugin = plugin;
                } else if (executableType == WebMachOExecutableType) {
                    if (!machoPlugin)
                        machoPlugin = plugin;
                } else {
                    ASSERT_NOT_REACHED();
                }
            } else {
                ASSERT_NOT_REACHED();
            }
        }
    }

    // Allow other plug-ins to win over QT because if the user has installed a plug-in that can handle a type
    // that the QT plug-in can handle, they probably intended to override QT.
    if (webPlugin && ![webPlugin isQuickTimePlugIn])
        return webPlugin;
    else if (machoPlugin && ![machoPlugin isQuickTimePlugIn])
        return machoPlugin;
    else if (CFMPlugin && ![CFMPlugin isQuickTimePlugIn])
        return CFMPlugin;
    else if (webPlugin)
        return webPlugin;
    else if (machoPlugin)
        return machoPlugin;
    else if (CFMPlugin)
        return CFMPlugin;
    return nil;
}

- (WebBasePluginPackage *)pluginForMIMEType:(NSString *)MIMEType
{
    return [self pluginForKey:[MIMEType lowercaseString]
       withEnumeratorSelector:@selector(MIMETypeEnumerator)];
}

- (WebBasePluginPackage *)pluginForExtension:(NSString *)extension
{
    WebBasePluginPackage *plugin = [self pluginForKey:[extension lowercaseString]
                               withEnumeratorSelector:@selector(extensionEnumerator)];
    if (!plugin) {
        // If no plug-in was found from the extension, attempt to map from the extension to a MIME type
        // and find the a plug-in from the MIME type. This is done in case the plug-in has not fully specified
        // an extension <-> MIME type mapping.
        NSString *MIMEType = WKGetMIMETypeForExtension(extension);
        if ([MIMEType length] > 0)
            plugin = [self pluginForMIMEType:MIMEType];
    }
    return plugin;
}

- (NSArray *)plugins
{
    return [plugins allValues];
}

static NSArray *additionalWebPlugInPaths;

+ (void)setAdditionalWebPlugInPaths:(NSArray *)additionalPaths
{
    if (additionalPaths == additionalWebPlugInPaths)
        return;
    
    [additionalWebPlugInPaths release];
    additionalWebPlugInPaths = [additionalPaths copy];

    // One might be tempted to add additionalWebPlugInPaths to the global WebPluginDatabase here.
    // For backward compatibility with earlier versions of the +setAdditionalWebPlugInPaths: SPI,
    // we need to save a copy of the additional paths and not cause a refresh of the plugin DB
    // at this time.
    // See Radars 4608487 and 4609047.
}

- (void)setPlugInPaths:(NSArray *)newPaths
{
    if (plugInPaths == newPaths)
        return;
        
    [plugInPaths release];
    plugInPaths = [newPaths copy];
}

- (void)close
{
    NSEnumerator *pluginEnumerator = [[self plugins] objectEnumerator];
    WebBasePluginPackage *plugin;
    while ((plugin = [pluginEnumerator nextObject]) != nil)
        [self _removePlugin:plugin];
    [plugins release];
    plugins = nil;
}

- (id)init
{
    if (!(self = [super init]))
        return nil;
        
    registeredMIMETypes = [[NSMutableSet alloc] init];
    
    return self;
}

- (void)dealloc
{
    [plugInPaths release];
    [plugins release];
    [registeredMIMETypes release];
    
    [super dealloc];
}

- (void)refresh
{
    // This method does a bit of autoreleasing, so create an autorelease pool to ensure that calling
    // -refresh multiple times does not bloat the default pool.
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    
    // Create map from plug-in path to WebBasePluginPackage
    if (!plugins)
        plugins = [[NSMutableDictionary alloc] initWithCapacity:12];

    // Find all plug-ins on disk
    NSMutableSet *newPlugins = [self _scanForNewPlugins];

    // Find plug-ins to remove from database (i.e., plug-ins that no longer exist on disk)
    NSMutableSet *pluginsToRemove = [NSMutableSet set];
    NSEnumerator *pluginEnumerator = [plugins objectEnumerator];
    WebBasePluginPackage *plugin;
    while ((plugin = [pluginEnumerator nextObject]) != nil) {
        // Any plug-ins that were removed from disk since the last refresh should be removed from
        // the database.
        if (![newPlugins containsObject:plugin])
            [pluginsToRemove addObject:plugin];
            
        // Remove every member of 'plugins' from 'newPlugins'.  After this loop exits, 'newPlugins'
        // will be the set of new plug-ins that should be added to the database.
        [newPlugins removeObject:plugin];
    }

#if !LOG_DISABLED
    if ([newPlugins count] > 0)
        LOG(Plugins, "New plugins:\n%@", newPlugins);
    if ([pluginsToRemove count] > 0)
        LOG(Plugins, "Removed plugins:\n%@", pluginsToRemove);
#endif

    // Remove plugins from database
    pluginEnumerator = [pluginsToRemove objectEnumerator];
    while ((plugin = [pluginEnumerator nextObject]) != nil) 
        [self _removePlugin:plugin];
    
    // Add new plugins to database
    pluginEnumerator = [newPlugins objectEnumerator];
    while ((plugin = [pluginEnumerator nextObject]) != nil)
        [self _addPlugin:plugin];

    // Build a list of MIME types.
    NSMutableSet *MIMETypes = [[NSMutableSet alloc] init];
    pluginEnumerator = [plugins objectEnumerator];
    while ((plugin = [pluginEnumerator nextObject]) != nil)
        [MIMETypes addObjectsFromArray:[[plugin MIMETypeEnumerator] allObjects]];
    
    // Register plug-in views and representations.
    NSEnumerator *MIMEEnumerator = [MIMETypes objectEnumerator];
    NSString *MIMEType;
    while ((MIMEType = [MIMEEnumerator nextObject]) != nil) {
        [registeredMIMETypes addObject:MIMEType];

        if ([WebView canShowMIMETypeAsHTML:MIMEType])
            // Don't allow plug-ins to override our core HTML types.
            continue;
        plugin = [self pluginForMIMEType:MIMEType];
        if ([plugin isJavaPlugIn])
            // Don't register the Java plug-in for a document view since Java files should be downloaded when not embedded.
            continue;
        if ([plugin isQuickTimePlugIn] && [[WebFrameView _viewTypesAllowImageTypeOmission:NO] objectForKey:MIMEType])
            // Don't allow the QT plug-in to override any types because it claims many that we can handle ourselves.
            continue;
        
        if (self == sharedDatabase)
            [WebView registerViewClass:[WebHTMLView class] representationClass:[WebHTMLRepresentation class] forMIMEType:MIMEType];
    }
    [MIMETypes release];
    
    [pool drain];
}

- (BOOL)isMIMETypeRegistered:(NSString *)MIMEType
{
    return [registeredMIMETypes containsObject:MIMEType];
}

@end

@implementation WebPluginDatabase (Internal)

+ (NSArray *)_defaultPlugInPaths
{
    // Plug-ins are found in order of precedence.
    // If there are duplicates, the first found plug-in is used.
    // For example, if there is a QuickTime.plugin in the users's home directory
    // that is used instead of the /Library/Internet Plug-ins version.
    // The purpose is to allow non-admin users to update their plug-ins.
    return [NSArray arrayWithObjects:
        [NSHomeDirectory() stringByAppendingPathComponent:@"Library/Internet Plug-Ins"],
        @"/Library/Internet Plug-Ins",
        [[NSBundle mainBundle] builtInPlugInsPath],
        nil];
}

- (NSArray *)_plugInPaths
{
    if (self == sharedDatabase && additionalWebPlugInPaths) {
        // Add additionalWebPlugInPaths to the global WebPluginDatabase.  We do this here for
        // backward compatibility with earlier versions of the +setAdditionalWebPlugInPaths: SPI,
        // which simply saved a copy of the additional paths and did not cause the plugin DB to 
        // refresh.  See Radars 4608487 and 4609047.
        NSMutableArray *modifiedPlugInPaths = [[plugInPaths mutableCopy] autorelease];
        [modifiedPlugInPaths addObjectsFromArray:additionalWebPlugInPaths];
        return modifiedPlugInPaths;
    } else
        return plugInPaths;
}

- (void)_addPlugin:(WebBasePluginPackage *)plugin
{
    ASSERT(plugin);
    NSString *pluginPath = [plugin path];
    ASSERT(pluginPath);
    [plugins setObject:plugin forKey:pluginPath];
    [plugin wasAddedToPluginDatabase:self];
}

- (void)_removePlugin:(WebBasePluginPackage *)plugin
{    
    ASSERT(plugin);

    // Unregister plug-in's MIME type registrations
    NSEnumerator *MIMETypeEnumerator = [plugin MIMETypeEnumerator];
    NSString *MIMEType;
    while ((MIMEType = [MIMETypeEnumerator nextObject])) {
        if ([registeredMIMETypes containsObject:MIMEType]) {
            if (self == sharedDatabase)
                [WebView _unregisterViewClassAndRepresentationClassForMIMEType:MIMEType];
            [registeredMIMETypes removeObject:MIMEType];
        }
    }

    // Remove plug-in from database
    NSString *pluginPath = [plugin path];
    ASSERT(pluginPath);
    [plugin retain];
    [plugins removeObjectForKey:pluginPath];
    [plugin wasRemovedFromPluginDatabase:self];
    [plugin release];
}

- (NSMutableSet *)_scanForNewPlugins
{
    NSMutableSet *newPlugins = [NSMutableSet set];
    NSEnumerator *directoryEnumerator = [[self _plugInPaths] objectEnumerator];
    NSMutableSet *uniqueFilenames = [[NSMutableSet alloc] init];
    NSFileManager *fileManager = [NSFileManager defaultManager];
    NSString *pluginDirectory;
    while ((pluginDirectory = [directoryEnumerator nextObject]) != nil) {
        // Get contents of each plug-in directory
        NSEnumerator *filenameEnumerator = [[fileManager directoryContentsAtPath:pluginDirectory] objectEnumerator];
        NSString *filename;
        while ((filename = [filenameEnumerator nextObject]) != nil) {
            // Unique plug-ins by filename
            if ([uniqueFilenames containsObject:filename])
                continue;
            [uniqueFilenames addObject:filename];
            
            // Create a plug-in package for this path
            NSString *pluginPath = [pluginDirectory stringByAppendingPathComponent:filename];
            WebBasePluginPackage *pluginPackage = [plugins objectForKey:pluginPath];
            if (!pluginPackage)
                pluginPackage = [WebBasePluginPackage pluginWithPath:pluginPath];
            if (pluginPackage)
                [newPlugins addObject:pluginPackage];
        }
    }
    [uniqueFilenames release];
    
    return newPlugins;
}

@end
