#pragma once

#import <Cocoa/Cocoa.h>
#import <QuartzCore/CoreVideo.h>

@interface ViewController : NSViewController
@property (nonatomic, assign) CVDisplayLinkRef displayLink;
@end
