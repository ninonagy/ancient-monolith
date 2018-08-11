#pragma once

struct texture_slot
{
    struct texture_slot* NextFree;
    uint32 Width;
    uint32 Height;
    uint8 ImplData[64];
};

struct font_slot
{
    uint8 Data[4096];
};

#define RENDERER_TEXTURE_SLOTS 16

typedef struct render_buffer
{
    memory_index Size;
    uint8 *Base;
    uint8 *Current;
    uint8 *End;
    
    u32 TextureCount;
    texture_slot *FreeTextureSlot;
    texture_slot TextureSlots[RENDERER_TEXTURE_SLOTS];
    
    texture_slot *OverlayTexture;
    texture_slot *GradientTexture;
    texture_slot *ShapeTexture;

    font_slot FontSlots[3];

    real32 FrameRateValue; // FrameTimeValue

    // Transform
    vec3 CameraP;
    real32 FocusLength;
    real32 MetersToPixels;
    vec2 ScreenCenter;
    mat4 ViewMatrix;
    mat4 ProjectionMatrix;
    
    uint32 Width;
    uint32 Height;
    vec4 ClearColor;

    uint32 MeshVerticesCount;

} render_buffer;

enum render_command_type
{
    RenderCommand_DrawMesh,
    RenderCommand_DrawRect,
    RenderCommand_DrawRectOutline,
    RenderCommand_DrawLine,
    RenderCommand_LoadTexture,
    RenderCommand_LoadTextureRaw,
    RenderCommand_LoadFont,
    RenderCommand_DrawString,
    RenderCommand_SetCameraMatrix,

    RenderCommandTypeCount
};

#define ForEachType(Action)                                                         \
    Action(RenderCommand_DrawMesh, render_command_draw_mesh);                       \
    Action(RenderCommand_DrawRect, render_command_draw_rect);                       \
    Action(RenderCommand_DrawRectOutline, render_command_draw_rect_outline);        \
    Action(RenderCommand_DrawLine, render_command_draw_line);                       \
    Action(RenderCommand_LoadTexture, render_command_load_texture);                 \
    Action(RenderCommand_LoadTextureRaw, render_command_load_texture_raw);                                        \
    Action(RenderCommand_LoadFont, render_command_load_font);                       \
    Action(RenderCommand_SetCameraMatrix, render_command_set_camera_matrix);

struct render_command_header
{
    render_command_type Type;
};

struct vertex
{
    vec2 P;
    vec2 UV;
};

typedef struct triangle_mesh
{
    u32 NumberOfVertices;
    vertex *Vertices;
} triangle_mesh;

struct render_command_draw_mesh
{
    //vec3 P;
    //real32 Angle;
    //bool32 IsLocal;
    triangle_mesh Mesh;
    texture_slot *TextureSlot;
    vec4 Color;
};

struct render_command_draw_rect
{
    vec3 P;
    vec2 Dim;
    real32 Angle;
    texture_slot *TextureSlot;
    vec4 Color;
    vec2 MinUV;
    vec2 MaxUV;
};

struct render_command_draw_rect_outline
{
    vec3 P;
    vec2 Dim;
    real32 Angle;
    texture_slot *TextureSlot;
    vec4 Color;
};

struct render_command_draw_line
{
    vec2 Start;
    vec2 End;
    vec4 Color;
};

struct render_command_load_texture
{
    texture_slot *TextureSlot;
    bool32 Interpolate;
    char *Filename;
};

struct render_command_load_texture_raw
{
    texture_slot *TextureSlot;
    bool Interpolate;
    uint8 *Data;
    uint32 Width;
    uint32 Height;
};

struct render_command_load_font
{
    font_slot *FontSlot;
    char *FileName;
    real32 Height;
    uint8 OversampleX;
    uint8 OversampleY;
};

struct render_command_draw_string
{
    font_slot *FontSlot;
    vec2 P;
    real32 Scale;
    vec4 Color;
    uint16 CharCount;
};

#define draw_rect render_command_draw_rect

struct render_command_set_camera_matrix
{
    mat4 Matrix;
};

#define BeginRenderBuffer(RenderBuffer) (RenderBuffer)->Current = (RenderBuffer)->Base
#define EndRenderBuffer(RenderBuffer)                                                   \
    (RenderBuffer)->End = (RenderBuffer)->Current;                                      \
    BeginRenderBuffer(RenderBuffer)