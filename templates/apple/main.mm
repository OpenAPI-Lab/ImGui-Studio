#import <TargetConditionals.h>
#import <MetalKit/MetalKit.h>
#if TARGET_OS_OSX
#import <Cocoa/Cocoa.h>
#include "imgui_impl_osx.h"
#else
#import <UIKit/UIKit.h>
#endif

#include "ui.hpp"
#include "studio_apple_config.hpp"
#include "imgui_impl_metal.h"
#include <algorithm>
#if STUDIO_APPLE_IMAGES
#include "studio_metal_images.hpp"
#endif

#if TARGET_OS_OSX
@interface StudioController : NSViewController <MTKViewDelegate>
#else
@interface StudioController : UIViewController <MTKViewDelegate>
#endif
- (void)stopRendering;
@end

@implementation StudioController {
    MTKView* _canvas;
    id<MTLCommandQueue> _queue;
    ImGuiContext* _context;
    StudioExportState _state;
#if STUDIO_APPLE_IMAGES
    StudioMetalImages _images;
#endif
#if TARGET_OS_OSX
    BOOL _platformReady;
#else
    __weak UITouch* _activeTouch;
    CFTimeInterval _previousFrame;
#endif
}

- (void)loadView {
    const CGRect frame = CGRectMake(0, 0, 1200, 720);
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    _canvas = [[MTKView alloc] initWithFrame:frame device:device];
    _canvas.paused = YES;
    _canvas.clearColor = MTLClearColorMake(0.08, 0.09, 0.11, 1.0);
#if TARGET_OS_OSX
    self.view = _canvas;
#else
    self.view = [[UIView alloc] initWithFrame:frame];
    self.view.backgroundColor = UIColor.blackColor;
    _canvas.translatesAutoresizingMaskIntoConstraints = NO;
    [self.view addSubview:_canvas];
    UILayoutGuide* safe = self.view.safeAreaLayoutGuide;
    [NSLayoutConstraint activateConstraints:@[
        [_canvas.topAnchor constraintEqualToAnchor:safe.topAnchor],
        [_canvas.bottomAnchor constraintEqualToAnchor:safe.bottomAnchor],
        [_canvas.leadingAnchor constraintEqualToAnchor:safe.leadingAnchor],
        [_canvas.trailingAnchor constraintEqualToAnchor:safe.trailingAnchor]
    ]];
#endif
    _queue = [device newCommandQueue];
    if (!_queue) {
        NSLog(@"Unable to initialize Metal.");
        return;
    }

    IMGUI_CHECKVERSION();
    _context = ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;
    ApplyStudioStyle();
    ImGui_ImplMetal_Init(device);
#if STUDIO_APPLE_IMAGES
    _images.device = device;
    if (!LoadStudioImages(_state, uploadMetalImage, &_images)) NSLog(@"Some images were unavailable.");
#endif
#if !TARGET_OS_OSX
    _canvas.delegate = self;
    _canvas.paused = NO;
#endif
}

#if TARGET_OS_OSX
- (void)viewDidAppear {
    [super viewDidAppear];
    if (!_context || _platformReady) return;
    // The AppKit backend captures the window and installs input handlers.
    ImGui::SetCurrentContext(_context);
    _platformReady = ImGui_ImplOSX_Init(_canvas);
    if (_platformReady) {
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        _canvas.delegate = self;
        _canvas.paused = NO;
    }
}
#endif

- (void)stopRendering {
    _canvas.paused = YES;
    _canvas.delegate = nil;
    if (!_context) return;
    ImGui::SetCurrentContext(_context);
#if STUDIO_APPLE_IMAGES
    ReleaseStudioImages(_state, releaseMetalImage, &_images);
#endif
#if TARGET_OS_OSX
    if (_platformReady) ImGui_ImplOSX_Shutdown();
    _platformReady = NO;
#endif
    ImGui_ImplMetal_Shutdown();
    ImGui::DestroyContext(_context);
    _context = nullptr;
}

- (void)dealloc {
    [self stopRendering];
}

- (void)drawInMTKView:(MTKView*)view {
    if (!_context || view.bounds.size.width <= 0 || view.bounds.size.height <= 0) return;
    MTLRenderPassDescriptor* pass = view.currentRenderPassDescriptor;
    id<CAMetalDrawable> drawable = view.currentDrawable;
    id<MTLCommandBuffer> commands = [_queue commandBuffer];
    if (!pass || !drawable || !commands) return;
    id<MTLRenderCommandEncoder> encoder = [commands renderCommandEncoderWithDescriptor:pass];
    if (!encoder) return;
    ImGui::SetCurrentContext(_context);
#if TARGET_OS_OSX
    ImGui_ImplOSX_NewFrame(view);
#else
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(view.bounds.size.width, view.bounds.size.height);
    io.DisplayFramebufferScale = ImVec2(view.drawableSize.width / view.bounds.size.width,
                                       view.drawableSize.height / view.bounds.size.height);
    const CFTimeInterval now = CACurrentMediaTime();
    io.DeltaTime = _previousFrame > 0 ? std::clamp(static_cast<float>(now - _previousFrame), 0.0001f, 0.1f) : 1.0f / 60.0f;
    _previousFrame = now;
#endif
    ImGui_ImplMetal_NewFrame(pass);
    ImGui::NewFrame();
    RenderStudioUI(_state);
    ImGui::Render();
    ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), commands, encoder);
    [encoder endEncoding];
    [commands presentDrawable:drawable];
    [commands commit];
}

- (void)mtkView:(MTKView*)view drawableSizeWillChange:(CGSize)size {
    // The frame uses the current bounds and drawable size.
}

#if !TARGET_OS_OSX
- (void)updateTouch:(UITouch*)touch {
    ImGui::SetCurrentContext(_context);
    const CGPoint point = [touch locationInView:_canvas];
    ImGuiIO& io = ImGui::GetIO();
    io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);
    io.AddMousePosEvent(point.x, point.y);
}

- (void)touchesBegan:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
    if (!_context || _activeTouch) return;
    UITouch* touch = touches.anyObject;
    _activeTouch = touch;
    [self updateTouch:touch];
    ImGui::GetIO().AddMouseButtonEvent(0, true);
}

- (void)touchesMoved:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
    UITouch* touch = _activeTouch;
    if (_context && touch && [touches containsObject:touch]) [self updateTouch:touch];
}

- (void)touchesEnded:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
    UITouch* touch = _activeTouch;
    if (!_context || !touch || ![touches containsObject:touch]) return;
    [self updateTouch:touch];
    ImGui::GetIO().AddMouseButtonEvent(0, false);
    _activeTouch = nil;
}

- (void)touchesCancelled:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
    [self touchesEnded:touches withEvent:event];
}
#endif
@end

#if TARGET_OS_OSX
@interface StudioAppDelegate : NSObject <NSApplicationDelegate>
@property(nonatomic, strong) NSWindow* window;
@end

@implementation StudioAppDelegate
- (void)applicationDidFinishLaunching:(NSNotification*)notification {
    NSMenu* menu = [[NSMenu alloc] init];
    NSMenuItem* applicationItem = [menu addItemWithTitle:@"" action:nil keyEquivalent:@""];
    applicationItem.submenu = [[NSMenu alloc] init];
    [applicationItem.submenu addItemWithTitle:NSLocalizedString(@"Quit", nil)
                                      action:@selector(terminate:) keyEquivalent:@"q"];
    NSApp.mainMenu = menu;

    self.window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 1200, 720)
        styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                  NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable
        backing:NSBackingStoreBuffered defer:NO];
    self.window.title = @"Studio Export";
    self.window.releasedWhenClosed = NO;
    self.window.contentViewController = [[StudioController alloc] initWithNibName:nil bundle:nil];
    [self.window center];
    [self.window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)application {
    return YES;
}

- (void)applicationWillTerminate:(NSNotification*)notification {
    [(StudioController*)self.window.contentViewController stopRendering];
}
@end

int main() {
    @autoreleasepool {
        NSApplication* app = [NSApplication sharedApplication];
        [app setActivationPolicy:NSApplicationActivationPolicyRegular];
        __attribute__((objc_precise_lifetime)) StudioAppDelegate* delegate = [[StudioAppDelegate alloc] init];
        app.delegate = delegate;
        [app run];
        app.delegate = nil;
    }
    return 0;
}
#else
@interface StudioAppDelegate : UIResponder <UIApplicationDelegate>
@end
@implementation StudioAppDelegate
@end

@interface StudioSceneDelegate : UIResponder <UIWindowSceneDelegate>
@property(nonatomic, strong) UIWindow* window;
@end

@implementation StudioSceneDelegate
- (void)scene:(UIScene*)scene willConnectToSession:(UISceneSession*)session
       options:(UISceneConnectionOptions*)options {
    if (![scene isKindOfClass:UIWindowScene.class]) return;
    self.window = [[UIWindow alloc] initWithWindowScene:(UIWindowScene*)scene];
    self.window.rootViewController = [[StudioController alloc] initWithNibName:nil bundle:nil];
    [self.window makeKeyAndVisible];
}

- (void)sceneDidDisconnect:(UIScene*)scene {
    [(StudioController*)self.window.rootViewController stopRendering];
    self.window = nil;
}
@end

int main(int argc, char** argv) {
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass(StudioAppDelegate.class));
    }
}
#endif
