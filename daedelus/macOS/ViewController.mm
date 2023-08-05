#import <MetalKit/MetalKit.h>
#import <QuartzCore/QuartzCore.h>
#import "./ViewController.h"
#include "../Engine/NativeRenderer.hh"
#include "../Engine/Input.hh"
#include "../Scenes/S13E01/Scene.hh"

@implementation ViewController
{
    MTKView *_view;
    NativeRenderer *_renderer;
    CFTimeInterval _startTime;
    
}

static CVReturn DisplayLinkCallback(CVDisplayLinkRef displayLink, const CVTimeStamp *inNow, const CVTimeStamp *inOutputTime, CVOptionFlags flagsIn, CVOptionFlags *flagsOut, void *displayLinkContext) {
    // Cast the context back to your view controller
    //ViewController *viewController = (__bridge ViewController *)displayLinkContext;
    
    Scenes::S13E01::onIdle(CACurrentMediaTime());
    
    return kCVReturnSuccess;
}

- (void)viewWillAppear
{
    [super viewWillAppear];
    CVDisplayLinkCreateWithActiveCGDisplays(&_displayLink);
    Scenes::S13E01::onInit(CACurrentMediaTime());
    CVDisplayLinkSetOutputCallback(_displayLink, &DisplayLinkCallback, (__bridge void *)self);
    CVDisplayLinkStart(_displayLink);
}

- (void)viewWillDisappear {
    [super viewWillDisappear];
    
    CVDisplayLinkStop(_displayLink);
    CVDisplayLinkRelease(_displayLink);
    _displayLink = NULL;
}

- (void)mouseDown:(NSEvent *)event {
    Scenes::S13E01::onMouseClicked(Engine::Input::ButtonState::Down,
                                   simd::float2{(float)event.locationInWindow.x, (float)event.locationInWindow.y});
}

- (void)mouseUp:(NSEvent *)event {
    Scenes::S13E01::onMouseClicked(Engine::Input::ButtonState::Up,
                                   simd::float2{(float)event.locationInWindow.x, (float)event.locationInWindow.y});
}

- (void)mouseDragged:(NSEvent *)event {
    Scenes::S13E01::onMouseMoved(simd::float2{(float)event.locationInWindow.x, (float)event.locationInWindow.y});
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    _view = (MTKView *)self.view;
    _view.device = MTLCreateSystemDefaultDevice();
    _view.preferredFramesPerSecond = 30;
    NSAssert(_view.device, @"Metal is not supported on this device");

    __weak ViewController* weakSelf = self;
    dispatch_async( dispatch_get_global_queue( QOS_CLASS_USER_INITIATED, 0 ), ^(){
        ViewController* strongSelf;
        if ( (strongSelf = weakSelf) )
        {
            NativeRenderer *renderer = new Scenes::S13E01::Renderer((__bridge MTK::View*)strongSelf->_view);
            NSAssert(renderer, @"Renderer failed initialization");
            
            strongSelf->_renderer = renderer;
            
            dispatch_async( dispatch_get_main_queue(), ^(){
                ViewController* innerStrongSelf;
                if ( (innerStrongSelf = weakSelf) ) {
                    // Initialize our renderer with the view size
                    auto view = (__bridge MTK::View*)innerStrongSelf->_view;
                    innerStrongSelf->_renderer->drawableSizeWillChange(view, view->drawableSize());
                    view->setDelegate(innerStrongSelf->_renderer);
                }
            } );
        }
    } );
}
@end

