#import "KWQView.h"

#import <qwidget.h>

@implementation KWQView

- initWithFrame: (NSRect) r widget: (QWidget *)w 
{
    [super initWithFrame: r];
    widget = w;
}


- (void)drawRect:(NSRect)rect {
    NSLog (@"drawRect\n", nil);
    widget->paint((void *)0);
}


- (BOOL)isFlipped 
{
	return YES;
}

@end
