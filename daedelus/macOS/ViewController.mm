#import <MetalKit/MetalKit.h>
#import <QuartzCore/QuartzCore.h>
#import "./ViewController.h"
#include "../Engine/NativeRenderer.hh"
#include "../Engine/Input.hh"
#include "../Scenes/S13E01/Scene.hh"
#include "../Scenes/S13E02/Scene.hh"

@implementation ViewController
{
    MTKView *_view;
    int _currentScene;
    Engine::Renderer *_renderer;
    CFTimeInterval _startTime;
    Engine::Scene *_scenes[2];
}

static CVReturn DisplayLinkCallback(CVDisplayLinkRef displayLink, const CVTimeStamp *inNow, const CVTimeStamp *inOutputTime, CVOptionFlags flagsIn, CVOptionFlags *flagsOut, void *displayLinkContext) {
    // Cast the context back to your view controller
    ViewController *viewController = (__bridge ViewController *)displayLinkContext;
    
    viewController->_scenes[viewController->_currentScene]->onIdle(CACurrentMediaTime());
    
    return kCVReturnSuccess;
}

- (void)viewWillAppear
{
    [super viewWillAppear];
    CVDisplayLinkCreateWithActiveCGDisplays(&_displayLink);
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
    _scenes[_currentScene]->onMouseClicked(Engine::Input::ButtonState::Down,
                                   simd::float2{(float)event.locationInWindow.x, (float)event.locationInWindow.y});
}

- (void)mouseUp:(NSEvent *)event {
    _scenes[_currentScene]->onMouseClicked(Engine::Input::ButtonState::Up,
                                   simd::float2{(float)event.locationInWindow.x, (float)event.locationInWindow.y});
}

- (void)mouseDragged:(NSEvent *)event {
    _scenes[_currentScene]->onMouseMoved(simd::float2{(float)event.locationInWindow.x, (float)event.locationInWindow.y});
}

- (void)sceneSelected:(NSMenuItem *)sender {
    NSMenu *scenesMenu = [sender menu];
    NSInteger index = [scenesMenu indexOfItem:sender];
    NSLog(@"Selected item index: %ld", index);
    [self initializeSceneWithIndex:(int)index];
}

- (void)initializeSceneWithIndex:(int)index {
    auto view = (__bridge MTK::View*)_view;
    if (_renderer != nullptr) {
        view->setDelegate(nullptr);
        _renderer->~Renderer();
    }
    _currentScene = index;
    _scenes[_currentScene]->onInit(CACurrentMediaTime());
    auto *renderer = _scenes[_currentScene]->createRenderer((__bridge MTK::View*)_view);
    NSAssert(renderer, @"Renderer failed initialization");
    _renderer = renderer;
    
    // Initialize our renderer with the view size
    _renderer->drawableSizeWillChange(view, view->drawableSize());
    view->setDelegate(_renderer);
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    _view = (MTKView *)self.view;
    _view.device = MTLCreateSystemDefaultDevice();
    _view.preferredFramesPerSecond = 30;
    NSAssert(_view.device, @"Metal is not supported on this device");
        
    NSMenu *applicationMenu = [[NSApplication sharedApplication] mainMenu];
    NSMenu *scenesMenu = [[applicationMenu itemWithTitle:@"Scenes"] submenu];
    
    _scenes[0] = new Scenes::S13E01::Scene();
    _scenes[1] = new Scenes::S13E02::Scene();
    
    NSArray *options = @[@"S13E01", @"S13E02"];

    for (NSString *option in options) {
        NSMenuItem *menuItem = [[NSMenuItem alloc] initWithTitle:option action:@selector(sceneSelected:) keyEquivalent:@""];
        [scenesMenu addItem:menuItem];
    }
    
    [self initializeSceneWithIndex:1];
}
@end

