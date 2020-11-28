// Copyright(c) 2017 POLYGONTEK
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http ://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Precompiled.h"
#include "RHI/RHIOpenGL.h"
#include "RGLInternal.h"
#include "Platform/PlatformTime.h"
#include "Profiler/Profiler.h"

#define USE_DISPLAY_LINK    1

BE_NAMESPACE_BEGIN

static int          majorVersion = 0;
static int          minorVersion = 0;

static CVar         gl_debug("gl_debug", "0", CVar::Flag::Bool, "");
static CVar         gl_debugLevel("gl_debugLevel", "3", CVar::Flag::Integer, "");
static CVar         gl_ignoreError("gl_ignoreError", "0", CVar::Flag::Bool, "");
static CVar         gl_finish("gl_finish", "0", CVar::Flag::Bool, "");

extern CVar         r_sRGB;

BE_NAMESPACE_END

static const float userContentScaleFactor = 2.0f;

@interface EAGLView : UIView
@end

@implementation EAGLView {
    BE1::GLContext *    glContext;
    GLuint              framebuffer;
    GLuint              colorbuffer;
    GLuint              depthbuffer;
    GLint               backingWidth;
    GLint               backingHeight;
    CADisplayLink *     displayLink;
    BOOL                displayLinkStarted;
    int                 displayLinkFrameInterval;
}

+ (Class)layerClass {
    return [CAEAGLLayer class];
}

- (void)setGLContext:(BE1::GLContext *)ctx {
    glContext = ctx;
}

- (GLuint)framebuffer {
    return framebuffer;
}

- (id)initWithFrame:(CGRect)frameRect {
    self = [super initWithFrame:frameRect];

    // Configure EAGLDrawable (CAEAGLLayer)
    CAEAGLLayer *eaglLayer = (CAEAGLLayer *)self.layer;
    eaglLayer.opaque = YES;
    NSMutableDictionary *dict = [NSMutableDictionary dictionary];
    [dict setValue:[NSNumber numberWithBool:NO] forKey:kEAGLDrawablePropertyRetainedBacking];
    const NSString *colorFormat = BE1::r_sRGB.GetBool() ? kEAGLColorFormatSRGBA8 : kEAGLColorFormatRGBA8;
    [dict setValue:colorFormat forKey:kEAGLDrawablePropertyColorFormat];
    eaglLayer.drawableProperties = dict;

    displayLinkStarted = NO;
    displayLinkFrameInterval = 1;

    return self;
}

- (void)dealloc {
    [self stopDisplayLink];
    
    [self freeFramebuffer];

#if !__has_feature(objc_arc)
    [super dealloc];
#endif
}

- (BOOL)initFramebuffer {
    const float nativeScale = [[UIScreen mainScreen] scale];

    self.contentScaleFactor = userContentScaleFactor;
    BE_LOG("Setting contentScaleFactor to %0.4f (optimal = %0.4f)", self.contentScaleFactor, nativeScale);

    if (self.contentScaleFactor == 1.0f || self.contentScaleFactor == 2.0f) {
        CAEAGLLayer *eaglLayer = (CAEAGLLayer *)self.layer;
        eaglLayer.magnificationFilter = kCAFilterNearest;
    }

    // Create FBO
    gglGenFramebuffers(1, &framebuffer);
    gglBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    // Create color buffer for display
    gglGenRenderbuffers(1, &colorbuffer);
    gglBindRenderbuffer(GL_RENDERBUFFER, colorbuffer);
    [glContext->eaglContext renderbufferStorage:GL_RENDERBUFFER fromDrawable:(CAEAGLLayer *)self.layer];
    gglFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorbuffer);

    // Get the size of the buffer
    gglGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &backingWidth);
    gglGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &backingHeight);

    // Create depth buffer
    gglGenRenderbuffers(1, &depthbuffer);
    gglBindRenderbuffer(GL_RENDERBUFFER, depthbuffer);
    gglRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, backingWidth, backingHeight);
    gglFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthbuffer);

    GLenum status = gglCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        BE_FATALERROR("failed to make complete framebuffer object 0x%x", status);
        gglDeleteFramebuffers(1, &framebuffer);
        gglDeleteRenderbuffers(1, &colorbuffer);
        gglDeleteRenderbuffers(1, &depthbuffer);
        return NO;
    }

    return YES;
}

- (void)freeFramebuffer {
    gglDeleteFramebuffers(1, &framebuffer);
    gglDeleteRenderbuffers(1, &colorbuffer);
    gglDeleteRenderbuffers(1, &depthbuffer);
}

- (CGSize)backingPixelSize {
    CGSize size;
    size.width = backingWidth;
    size.height = backingHeight;
    return size;
}

- (void)setAnimationFrameInterval:(int)interval {
#if USE_DISPLAY_LINK
    if (interval <= 0 || interval == displayLinkFrameInterval) {
        return;
    }
    
    displayLinkFrameInterval = interval;
    
    if (displayLinkStarted) {
        [self stopDisplayLink];
        [self startDisplayLink];
    }
#endif
}

- (void)startDisplayLink {
#if USE_DISPLAY_LINK
    if (displayLinkStarted) {
        return;
    }
    
    displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(drawView:)];
    [displayLink setFrameInterval:displayLinkFrameInterval];
    //[displayLink setPreferredFramesPerSecond:60];
    [displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
    
    displayLinkStarted = YES;
#endif
}

- (void)stopDisplayLink {
#if USE_DISPLAY_LINK
    if (!displayLinkStarted) {
        return;
    }
    
    [displayLink invalidate];
    displayLink = nil;
    displayLinkStarted = NO;
#endif
}

- (void)layoutSubviews {
    gglBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    // Allocate color buffer backing based on the current layer size
    gglBindRenderbuffer(GL_RENDERBUFFER, colorbuffer);
    [glContext->eaglContext renderbufferStorage:GL_RENDERBUFFER fromDrawable:(CAEAGLLayer *)self.layer];
    gglGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &backingWidth);
    gglGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &backingHeight);

    GLenum depthbufferStorage;
    gglBindRenderbuffer(GL_RENDERBUFFER, depthbuffer);
    gglGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_INTERNAL_FORMAT, (GLint *)&depthbufferStorage);
    gglRenderbufferStorage(GL_RENDERBUFFER, depthbufferStorage, backingWidth, backingHeight);
    gglFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthbuffer);

    GLenum status = gglCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        BE_FATALERROR("Failed to make complete framebuffer object 0x%x", status);
    }

    [self drawView:nil];
}

- (void)drawView:(id)sender {
    glContext->displayFunc(glContext->handle, glContext->displayFuncDataPtr);
}

- (BOOL)swapBuffers {
    gglBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    gglBindRenderbuffer(GL_RENDERBUFFER, colorbuffer);

    // Discard the unncessary depth/stencil buffer
    const GLenum discards[]  = { GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT };
    //gglDiscardFramebufferEXT(GL_READ_FRAMEBUFFER, 2, discards);
    gglInvalidateFramebuffer(GL_READ_FRAMEBUFFER, 2, discards);

    BOOL succeeded = [glContext->eaglContext presentRenderbuffer:GL_RENDERBUFFER];
    return succeeded;
}

#ifdef ENABLE_IMGUI

- (BOOL)isFirstResponder {
    return YES;
}

// This touch mapping is super cheesy/hacky. We treat any touch on the screen
// as if it were a depressed left mouse button, and we don't bother handling
// multitouch correctly at all. This causes the "cursor" to behave very erratically
// when there are multiple active touches. But for demo purposes, single-touch
// interaction actually works surprisingly well.
- (BOOL)updateIOWithTouchEvent:(UIEvent *)event {
    UITouch *anyTouch = event.allTouches.anyObject;
    CGPoint touchLocation = [anyTouch locationInView:self];

    ImGuiIO &io = ImGui::GetIO();
    io.MousePos = ImVec2(touchLocation.x, touchLocation.y);
    
    BOOL hasActiveTouch = NO;
    for (UITouch *touch in event.allTouches) {
        if (touch.phase != UITouchPhaseEnded && touch.phase != UITouchPhaseCancelled) {
            hasActiveTouch = YES;
            break;
        }
    }
    io.MouseDown[0] = hasActiveTouch;
    //ImGui::UpdateHoveredWindowAndCaptureFlags();
    //return io.WantCaptureMouse;
    return false;
}

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    if ([self updateIOWithTouchEvent:event]) {
        return;
    }
    
    [super touchesBegan:touches withEvent:event];
}

- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    if ([self updateIOWithTouchEvent:event]) {
        return;
    }

    [super touchesEnded:touches withEvent:event];
}

- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    if ([self updateIOWithTouchEvent:event]) {
        return;
    }

    [super touchesMoved:touches withEvent:event];
}

#endif

@end // @implementation EAGLView

//-------------------------------------------------------------------------------------------------------

BE_NAMESPACE_BEGIN

static void GetGLVersion(int *major, int *minor) {
    const char *verstr = (const char *)glGetString(GL_VERSION);
    if (!verstr || sscanf(verstr, "%d.%d", major, minor) != 2) {
        *major = *minor = 0;
    }
}

void OpenGLRHI::InitMainContext(WindowHandle windowHandle, const Settings *settings) {
    mainContext = new GLContext;
    mainContext->state = new GLState;
    
    // Create EAGLContext
    mainContext->eaglContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];
    if (!mainContext->eaglContext) {
        BE_FATALERROR("Couldn't create main EAGLContext");
    }
    
    // Make current context
    [EAGLContext setCurrentContext:mainContext->eaglContext];
    
    GetGLVersion(&majorVersion, &minorVersion);

    int decimalVersion = majorVersion * 10 + minorVersion;
    if (decimalVersion < 30) {
        //BE_ERR("Minimum OpenGL extensions missing !!\nRequired OpenGL 3.3 or higher graphic card");
    }

    // gglXXX 함수 바인딩 및 확장 flag 초기화
    ggl_init(gl_debug.GetBool());
    
    // default FBO
    mainContext->defaultFramebuffer = 0;

    // Create default VAO for main context
    gglGenVertexArrays(1, &mainContext->defaultVAO);

#ifdef ENABLE_IMGUI
    ImGuiCreateContext(mainContext);
#endif
}

void OpenGLRHI::FreeMainContext() {
#ifdef ENABLE_IMGUI
    ImGuiDestroyContext(mainContext);
#endif

    // Delete default VAO for main context
    gglDeleteVertexArrays(1, &mainContext->defaultVAO);

    // Sets the current context to nil.
    [EAGLContext setCurrentContext:nil];

#if !__has_feature(objc_arc)
    [mainContext->eaglContext release];
#endif

    SAFE_DELETE(mainContext->state);
    SAFE_DELETE(mainContext);
}

RHI::Handle OpenGLRHI::CreateContext(RHI::WindowHandle windowHandle, bool useSharedContext) {
    GLContext *ctx = new GLContext;

    int handle = contextList.FindNull();
    if (handle == -1) {
        handle = contextList.Append(ctx);
    } else {
        contextList[handle] = ctx;
    }

    ctx->handle = (Handle)handle;
    ctx->onDemandDrawing = false;
    ctx->rootView = (__bridge UIView *)windowHandle;

    if (!useSharedContext) {
        ctx->state = mainContext->state;
        ctx->eaglContext = mainContext->eaglContext;
    } else {
        ctx->state = new GLState;
        ctx->eaglContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3 sharegroup:[mainContext->eaglContext sharegroup]];
        if (!ctx->eaglContext) {
            BE_FATALERROR("Couldn't create main EAGLContext");
        }
    }
    
    CGRect contentRect = [ctx->rootView bounds];
    ctx->eaglView = [[EAGLView alloc] initWithFrame:CGRectMake(0, 0, contentRect.size.width, contentRect.size.height)];
    ctx->eaglView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;

    [ctx->eaglView setGLContext:ctx];

    [ctx->eaglView initFramebuffer];

    [ctx->rootView addSubview:ctx->eaglView];

#ifdef ENABLE_IMGUI
    ctx->imGuiContext = mainContext->imGuiContext;
    ctx->imGuiLastTime = PlatformTime::Seconds();
#endif

    SetContext((Handle)handle);

    ctx->defaultFramebuffer = ctx->eaglView.framebuffer;

    if (useSharedContext) {
        // Create default VAO for shared context
        gglGenVertexArrays(1, &ctx->defaultVAO);
    } else {
        ctx->defaultVAO = mainContext->defaultVAO;
    }

    SetDefaultState();

    return (Handle)handle;
}

void OpenGLRHI::DestroyContext(Handle ctxHandle) {
    GLContext *ctx = contextList[ctxHandle];

    [ctx->eaglView stopDisplayLink];
    [ctx->eaglView removeFromSuperview];
    
    if (ctx->eaglContext != mainContext->eaglContext) {
        // Delete default VAO for shared context
        gglDeleteVertexArrays(1, &ctx->defaultVAO);

#if !__has_feature(objc_arc)
        [ctx->eaglContext release];
#endif

        delete ctx->state;
    }

    if (currentContext == ctx) {
        currentContext = mainContext;

        [EAGLContext setCurrentContext:mainContext->eaglContext];
    }

    delete ctx;
    contextList[ctxHandle] = nullptr;
}

void OpenGLRHI::ActivateSurface(Handle ctxHandle, WindowHandle windowHandle) {
    GLContext *ctx = ctxHandle == NullContext ? mainContext : contextList[ctxHandle];
}

void OpenGLRHI::DeactivateSurface(Handle ctxHandle) {
    GLContext *ctx = ctxHandle == NullContext ? mainContext : contextList[ctxHandle];
}

void OpenGLRHI::SetContext(Handle ctxHandle) {
    EAGLContext *currentContext = [EAGLContext currentContext];
    GLContext *ctx = ctxHandle == NullContext ? mainContext : contextList[ctxHandle];

    if (currentContext != ctx->eaglContext) {
        // This ensures that previously submitted commands are delivered to the graphics hardware in a timely fashion.
        glFlush();
    }

    [EAGLContext setCurrentContext:ctx->eaglContext];

    this->currentContext = ctx;

#ifdef ENABLE_IMGUI
    ImGui::SetCurrentContext(ctx->imGuiContext);
#endif
}

void OpenGLRHI::SetContextDisplayFunc(Handle ctxHandle, DisplayContextFunc displayFunc, void *dataPtr, bool onDemandDrawing) {
    GLContext *ctx = ctxHandle == NullContext ? mainContext : contextList[ctxHandle];

    ctx->displayFunc = displayFunc;
    ctx->displayFuncDataPtr = dataPtr;
    ctx->onDemandDrawing = onDemandDrawing;

#if USE_DISPLAY_LINK
    [ctx->eaglView startDisplayLink];
#endif
}

void OpenGLRHI::DisplayContext(Handle ctxHandle) {
    GLContext *ctx = ctxHandle == NullContext ? mainContext : contextList[ctxHandle];

    [ctx->eaglView drawView:nil];
}

RHI::WindowHandle OpenGLRHI::GetWindowHandleFromContext(Handle ctxHandle) {
    const GLContext *ctx = ctxHandle == NullContext ? mainContext : contextList[ctxHandle];

    return (__bridge WindowHandle)ctx->rootView;
}

void OpenGLRHI::GetDisplayMetrics(Handle ctxHandle, DisplayMetrics *displayMetrics) const {
    const GLContext *ctx = ctxHandle == NullContext ? mainContext : contextList[ctxHandle];

    // FIXME: Cache the metrics result in advance
    CGSize viewSize = [ctx->eaglView bounds].size;
    displayMetrics->screenWidth = viewSize.width;
    displayMetrics->screenHeight = viewSize.height;
    
    CGSize backingSize = [ctx->eaglView backingPixelSize];
    displayMetrics->backingWidth = backingSize.width;
    displayMetrics->backingHeight = backingSize.height;

    if (@available(iOS 11, *)) {
        UIEdgeInsets insets = [[ctx->eaglView superview] safeAreaInsets];
        displayMetrics->safeAreaInsets.Set(insets.left, insets.top, insets.right, insets.bottom);
    } else {
        displayMetrics->safeAreaInsets.Set(0, 0, 0, 0);
    }
}

bool OpenGLRHI::IsFullscreen() const {
    return true;
}

bool OpenGLRHI::SetFullscreen(Handle ctxHandle, int width, int height) {
    return true;
}

void OpenGLRHI::ResetFullscreen(Handle ctxHandle) {
}

void OpenGLRHI::GetGammaRamp(unsigned short ramp[768]) const {
}

void OpenGLRHI::SetGammaRamp(unsigned short ramp[768]) const {
}

bool OpenGLRHI::SwapBuffers() {
    if (!gl_ignoreError.GetBool()) {
        CheckError("OpenGLRHI::SwapBuffers");
    }

    if (gl_finish.GetBool()) {
        glFinish();
    }

    bool succeeded = [currentContext->eaglView swapBuffers];
    if (succeeded) {
        return false;
    }
    
    if (gl_debug.IsModified()) {
        ggl_rebind(gl_debug.GetBool());
        gl_debug.ClearModified();
    }

    return true;
}

void OpenGLRHI::SwapInterval(int interval) const {
    [currentContext->eaglView setAnimationFrameInterval:interval];
}

void OpenGLRHI::ImGuiCreateContext(GLContext *ctx) {
    // Setup Dear ImGui context.
    ctx->imGuiContext = ImGui::CreateContext();
    ImGui::SetCurrentContext(ctx->imGuiContext);

    ImGui::GetStyle().TouchExtraPadding = ImVec2(4.0F, 4.0F);

    // Setup Dear ImGui style.
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    ImGuiIO &io = ImGui::GetIO();

    io.IniFilename = nullptr;

    // Setup back-end capabilities flags.
    //io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;         // We can honor GetMouseCursor() values (optional)
    //io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;          // We can honor io.WantSetMousePos requests (optional, rarely used)
    //io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;    // We can create multi-viewports on the Platform side (optional)
    //io.BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport; // We can set io.MouseHoveredViewport correctly (optional, not easy)
    io.BackendPlatformName = "OpenGLRHI-IOS";

    ImGui_ImplOpenGL_Init("#version 300 es");
}

void OpenGLRHI::ImGuiDestroyContext(GLContext *ctx) {
    ImGui_ImplOpenGL_Shutdown();

    ImGui::DestroyContext(ctx->imGuiContext);
}

void OpenGLRHI::ImGuiBeginFrame(Handle ctxHandle) {
    BE_PROFILE_CPU_SCOPE_STATIC("OpenGLRHI::ImGuiBeginFrame");

    ImGui_ImplOpenGL_ValidateFrame();

    GLContext *ctx = ctxHandle == NullContext ? mainContext : contextList[ctxHandle];

    DisplayMetrics dm;
    GetDisplayMetrics(ctxHandle, &dm);

    // Setup display size
    ImGuiIO &io = ImGui::GetIO();
    io.DisplaySize = ImVec2(dm.screenWidth, dm.screenHeight);
    io.DisplayFramebufferScale = ImVec2((float)dm.backingWidth / dm.screenWidth, (float)dm.backingHeight / dm.screenHeight);

    // Setup time step
    double currentTime = PlatformTime::Seconds();
    io.DeltaTime = currentTime - ctx->imGuiLastTime;
    ctx->imGuiLastTime = currentTime;
    
    ImGui::NewFrame();
}

void OpenGLRHI::ImGuiRender() {
    BE_PROFILE_CPU_SCOPE_STATIC("OpenGLRHI::ImGuiRender");
    BE_PROFILE_GPU_SCOPE_STATIC("OpenGLRHI::ImGuiRender");

    ImGui::Render();

    bool sRGBWriteEnabled = OpenGL::SupportsFrameBufferSRGB() && IsSRGBWriteEnabled();
    if (sRGBWriteEnabled) {
        SetSRGBWrite(false);
    }

    ImGui_ImplOpenGL_RenderDrawData(ImGui::GetDrawData());

    if (sRGBWriteEnabled) {
        SetSRGBWrite(true);
    }
}

void OpenGLRHI::ImGuiEndFrame() {
    BE_PROFILE_CPU_SCOPE_STATIC("OpenGLRHI::ImGuiEndFrame");

    ImGui::EndFrame();

    // HACK: invalidate touch position after one frame.
    ImGuiIO &io = ImGui::GetIO();
    if (!io.MouseDown[0]) {
        io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
    }
}

BE_NAMESPACE_END
