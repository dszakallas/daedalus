#import <Cocoa/Cocoa.h>
#import <QuartzCore/CoreVideo.h>
#import <MetalKit/MetalKit.h>
#import <QuartzCore/QuartzCore.h>
#import <GameController/GameController.h>
#include <memory>
#include "../Engine/Engine.hh"
#include "../Engine/Input.hh"
#include "../Scenes/S13E01/Scene.hh"
#include "../Scenes/S13E02/Scene.hh"
#include "../Scenes/NavigateCube/Scene.hh"

#pragma mark - AppDelegate
#pragma region AppDelegate {
@interface AppDelegate : NSObject <NSApplicationDelegate>

@end

@implementation AppDelegate

- (BOOL) applicationShouldTerminateAfterLastWindowClosed:(NSApplication*) sender
{
    return YES;
}

@end
#pragma endregion AppDelegate }

#pragma mark - ViewController
#pragma region ViewController {
@interface ViewController : NSViewController

@property (nonatomic, assign) CVDisplayLinkRef displayLink;

@end


@implementation ViewController
{
    MTKView *_view;
    int _currentScene;
    Engine::Renderer *_renderer;
    CFTimeInterval _startTime;
    std::unique_ptr<Engine::Scene> _scenes[3];
}

static CVReturn DisplayLinkCallback(CVDisplayLinkRef displayLink, const CVTimeStamp *inNow, const CVTimeStamp *inOutputTime, CVOptionFlags flagsIn, CVOptionFlags *flagsOut, void *displayLinkContext)
{
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

- (void)mouseDown:(NSEvent *)event
{
    _scenes[_currentScene]->onMouseClicked(
                                           Engine::Input::MouseButton::Left,
                                           Engine::Input::ButtonState::Down,
                                           simd::float2{(float)event.locationInWindow.x, (float)event.locationInWindow.y});
}

- (void)mouseUp:(NSEvent *)event
{
    _scenes[_currentScene]->onMouseClicked(
                                           Engine::Input::MouseButton::Left,
                                           Engine::Input::ButtonState::Up,
                                           simd::float2{(float)event.locationInWindow.x, (float)event.locationInWindow.y});
}

// TODO: Use mouseMoved: instead
- (void)mouseDragged:(NSEvent *)event
{
    _scenes[_currentScene]->onMouseMoved(simd::float2{(float)event.locationInWindow.x, (float)event.locationInWindow.y});
}

- (void)keyDown:(NSEvent *)event
{
    BOOL handled = NO;
    handled = _scenes[_currentScene]->onKey((Engine::Input::KeyboardButton)event.keyCode, Engine::Input::ButtonState::Down);
    if (!handled) {
        [super keyDown:event];
    }
}

- (void)sceneSelected:(NSMenuItem *)sender
{
    NSMenu *scenesMenu = [sender menu];
    NSInteger index = [scenesMenu indexOfItem:sender];
    NSLog(@"Selected item index: %ld", index);
    [self initializeSceneWithIndex:(int)index];
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (void)initializeSceneWithIndex:(int)index
{
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
    
    // Set up graphics device
    _view.device = MTLCreateSystemDefaultDevice();
    _view.preferredFramesPerSecond = 30;
    NSAssert(_view.device, @"Metal is not supported on this device");
    
    // Set up menu
    NSMenu *applicationMenu = [[NSApplication sharedApplication] mainMenu];
    NSMenu *scenesMenu = [[applicationMenu itemWithTitle:@"Scenes"] submenu];
    
    _scenes[0] = std::make_unique<Scenes::S13E01::Scene>();
    _scenes[1] = std::make_unique<Scenes::S13E02::Scene>();
    _scenes[2] = std::make_unique<Scenes::NavigateCube::Scene>();
    
    NSArray *options = @[@"S13E01", @"S13E02", @"NavigateCube"];

    for (NSString *option in options) {
        NSMenuItem *menuItem = [[NSMenuItem alloc] initWithTitle:option action:@selector(sceneSelected:) keyEquivalent:@""];
        [scenesMenu addItem:menuItem];
    }
    
    // Set up scenes
    [self initializeSceneWithIndex:1];
}

- (void)viewDidAppear
{
    [super viewDidAppear];
    [self.view.window makeFirstResponder:self];
}

@end

#pragma endregion ViewController }
