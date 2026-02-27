//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <p3d/context.hpp>
#include <p3d/view.hpp>
#include <p3d/camera.hpp>
#include <p3d/shader.hpp>
#include <p3d/matrixstack.hpp>
#include <p3d/utility.hpp>
#include <pddi/pddiext.hpp>
#include <p3d/memory.hpp>

#include <radmemorymonitor.hpp>

//  class to trap momory allocations from PDDI and pass them on to the radcore memormy manager
class RadcoreMemAdapt : public pddiExtMemRegistration::CallBack
{
    public:
        void Alloc(void* mem, int estSize)
        {
            //
            // this is not neccessary since underlaying memory system is already hooked into
            // memorymonitor
            //
            // radMemoryMonitorDeclareAllocation( mem, estSize); 
        }

        void Free(void* mem)
        {
            //
            // this is not neccessary since underlaying memory system is already hooked into
            // memorymonitor
            //
            // radMemoryMonitorRescindAllocation(mem);
        }
};

static RadcoreMemAdapt s_memCallback;

//-------------------------------------------------------------------
tContext::tContext(pddiDevice* dev, pddiDisplay* disp, pddiRenderContext* con)
{

    p3d::UsePermanentMem( true );
    p3d::MemSetup();
    FrameCount = 0;

    View = NULL;     // Current used tView object

    nLights = 0;

    RenderDevice = dev;
    RenderDisplay = disp;
    RenderContext = con;

    RenderDevice->AddRef();
    RenderDisplay->AddRef();
    RenderContext->AddRef();

    // TODO : reset this properly when context is changed for multiple contexts
    dev->SetCurrentContext(RenderContext);

    clearColour.Set(0, 0, 0);
    clearDepth = 1.0f;
    clearStencil = 0;

    RenderContext->SetFillMode(PDDI_FILL_SOLID);
    RenderContext->EnableZBuffer(true);

    // initialize matrix stack
    CVM.Identity();
    CVMI.Identity();

    stack = new tMatrixStack(RenderContext);

    clearMask = 0;
    clearColour.Set(0,0,0);

    Inventory = new tInventory;
    Inventory->AddRef();

    loadManager = new tLoadManager; 
    loadManager->AddRef();

    skinning = (pddiExtHardwareSkinning*)RenderContext->GetExtension(PDDI_EXT_HARDWARE_SKINNING);
    vertexProgram = (pddiExtVertexProgram*)RenderContext->GetExtension(PDDI_EXT_VERTEX_PROGRAM);

    pddiExtMemRegistration* reg = (pddiExtMemRegistration*)RenderContext->GetExtension(PDDI_EXT_MEM_REGISTRATION);
    reg->Register(&s_memCallback);

    inFrame = false;

    p3d::UsePermanentMem( false );
}

tContext::~tContext(void)
{
    Inventory->Release();
    Inventory = NULL;

    pddiObject* temp;

    // NULL out all objects before calling destructors, in case
    // Destructors "accidentaly" generate windows messages

    if(RenderContext)
    {
        temp = RenderContext;
        RenderContext = NULL;
        temp->Release();
    }

    if(RenderDisplay) 
    {
        temp = RenderDisplay;
        RenderDisplay = NULL;
        temp->Release();
    }

    if(RenderDevice)  
    {
        temp = RenderDevice;
        RenderDevice = NULL;
        temp->Release();
    }

    delete stack;
    loadManager->Release();
    p3d::MemShutdown();
}

void tContext::Setup(void)
{
}

void tContext::Shutdown(void)
{
}

//-------------------------------------------------------------------
void tContext::BeginFrame(void)
{
    FrameCount++;
    p3d::pddi->BeginFrame();

    if(clearMask)
    {
        pddiRect fullScreen(0,0,p3d::display->GetWidth(),p3d::display->GetHeight());
        p3d::pddi->SetClearColour(clearColour);
        p3d::pddi->SetClearDepth(clearDepth);
        p3d::pddi->SetClearStencil(clearStencil);
        p3d::pddi->SetScissor(&fullScreen);
        p3d::pddi->Clear(clearMask);
    }

    p3d::stack->LoadIdentity();
    inFrame = true;
}


//-------------------------------------------------------------------
void tContext::EndFrame(bool swapBuffers /*=true*/)
{
    p3d::pddi->EndFrame();
    inFrame = false;
    
    // nv: this is very bad 
    // p3d::loadManager->SwitchTask();
    // p3d::loadManager->TriggerCallbacks();

    if(swapBuffers)
    {
        SwapBuffers();
    }
}

//------------------------------------------------------------------------
void tContext::SetView(tView *view)
{
    View = view;
}

//------------------------------------------------------------------------
tView *tContext::GetView()
{
    return View;
}

//------------------------------------------------------------------------
// Display the current output buffer.
void tContext::SwapBuffers()
{
    // flip buffers
    RenderDisplay->SwapBuffers();
}

//------------------------------------------------------------------------
tMatrixStack* tContext::GetMatrixStack(p3dMatrixType type)
{
    return stack;
}

//------------------------------------------------------------------------
void tContext::LoadViewMatrix(const rmt::Matrix &matrix, const rmt::Matrix &inverseMatrix)
{
    CVM = matrix;
    CVMI = inverseMatrix;
    stack->Load(CVM);
}

// Get address of the current view matrix
rmt::Matrix* tContext::GetViewMatrix()
{   
    return &CVM;
}

//------------------------------------------------------------------------
// Get the Current World Matrix
rmt::Matrix *tContext::GetWorldMatrix()
{   
    CWM.Mult(*stack->GetMatrix(), CVMI);
    return &CWM;
}

void tContext::ObjectToWorld(const rmt::Vector& object, rmt::Vector* world)
{
    *world = object;
    world->Transform(*GetWorldMatrix());
}

void tContext::WorldToObject(const rmt::Vector& world, rmt::Vector* object)
{
    *object = world;
    rmt::Matrix m = *GetWorldMatrix();
    m.Invert();
    object->Transform(m);
}

void tContext::ObjectToView(const rmt::Vector& object, rmt::Vector* view)
{
    tView* v = GetView();
    if(!v) return;

    tCamera* c = v->GetCamera();
    if(!c) return;

    rmt::Vector tmpworld;
    ObjectToWorld(object,&tmpworld);
    c->WorldToView(tmpworld, view);
}

void tContext::ObjectToDevice(const rmt::Vector& object, rmt::Vector* device)
{
    tView* v = GetView();
    if(!v) return;

    tCamera* c = v->GetCamera();
    if(!c) return;

    rmt::Vector tmpworld, tmpview, tmpscreen;

    ObjectToWorld(object,&tmpworld);
    c->WorldToView(tmpworld, &tmpview);
    v->ViewToScreen(tmpview, &tmpscreen);
    ScreenToDevice(tmpscreen, device);
}

void tContext::WorldToView(const rmt::Vector& world, rmt::Vector* view)
{
    tView* v = GetView();
    if(!v) return;

    tCamera* c = v->GetCamera();
    if(!c) return;

    c->WorldToView(world, view);
}

void tContext::WorldToDevice(const rmt::Vector& world, rmt::Vector* device)
{
    tView* v = GetView();
    if(!v) return;

    tCamera* c = v->GetCamera();
    if(!c) return;

    rmt::Vector tmpview, tmpscreen;

    c->WorldToView(world, &tmpview);
    v->ViewToScreen(tmpview, &tmpscreen);
    ScreenToDevice(tmpscreen, device);
}


void tContext::ScreenToDevice(const rmt::Vector& screen, rmt::Vector* device)
{
    device->x = (screen.x + 0.5f) * p3d::display->GetWidth();
    device->y = (1.0f-(screen.y + 0.5f)) * p3d::display->GetHeight();
    device->z = screen.z;
}

void tContext::DeviceToScreen(const rmt::Vector& device, rmt::Vector* screen)
{
    screen->x = (device.x / float(p3d::display->GetWidth())) - 0.5f;
    screen->y = (1.0f - (device.y / float(p3d::display->GetHeight()))) - 0.5f;
    screen->z = device.z;
}

pddiExtHardwareSkinning* tContext::GetHardwareSkinning(void)
{
    return skinning;
}

pddiExtVertexProgram* tContext::GetVertexProgram(void)
{
    return vertexProgram;
}


