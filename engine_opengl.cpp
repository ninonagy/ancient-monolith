// engine_opengl.cpp

#define STBI_NO_STDIO
#define STBI_ASSERT Assert
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STBRP_ASSERT Assert
#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"

#define STBTT_malloc(x, u) ((void)(u), Platform.AllocateMemory(x))
#define STBTT_free(x, u) ((void)(u), Platform.DeallocateMemory(x))
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

internal opengl_info
OpenGLGetInfo(b32 ModernContext)
{
    opengl_info Result = {};
    
    Result.ModernContext = ModernContext;
    Result.Vendor = (char *)glGetString(GL_VENDOR);
    Result.Renderer = (char *)glGetString(GL_RENDERER);
    Result.Version = (char *)glGetString(GL_VERSION);
    
    Result.MajorVersion = Result.MinorVersion = 0;

    glGetIntegerv(GL_MAJOR_VERSION, &Result.MajorVersion);
    glGetIntegerv(GL_MINOR_VERSION, &Result.MinorVersion);

    if(Result.ModernContext)
    {
        Result.ShadingLanguageVersion = (char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
    }
    else
    {
        // NOTE(inso): we could get shaders on legacy 2.1 if the extensions are present.
        Result.ShadingLanguageVersion = "none";
    }

    return Result;
}

#define CheckGLError() CheckGLError_(__FILE__,__LINE__);

internal void
CheckGLError_(char *File, int Line)
{
    GLenum Error (glGetError());

    while(Error != GL_NO_ERROR)
    {
        char *ErrorString;

        switch(Error)
        {
            case GL_INVALID_OPERATION:      ErrorString="INVALID_OPERATION";      break;
            case GL_INVALID_ENUM:           ErrorString="INVALID_ENUM";           break;
            case GL_INVALID_VALUE:          ErrorString="INVALID_VALUE";          break;
            case GL_OUT_OF_MEMORY:          ErrorString="OUT_OF_MEMORY";          break;
            //case GL_INVALID_FRAMEBUFFER_OPERATION:  ErrorString="INVALID_FRAMEBUFFER_OPERATION";  break;
        }

        Platform.Log(ErrorString);
        Error = glGetError();
    }
}

internal void
OpenGLErrorCallback(GLenum Source, GLenum Type, GLuint Id,
                    GLenum Severity, GLsizei Length,
                    char* Message, void* UserParam)
{
    if(Severity != DEBUG_SEVERITY_HIGH)
    {
        return;
    }

#if 0
    if(Platform.OutputDebugString)
    {
        Platform.OutputDebugString(Message);
    }
#endif
#if 1
    if(Platform.Log)
    {
        Platform.Log(Message);
    }
#endif
}

internal void
OpenGLCheckExtensions(opengl_info *Info)
{
    GLint NumExtensions;
    glGetIntegerv(GL_NUM_EXTENSIONS, &NumExtensions);
    for(int i = 0; i < NumExtensions; ++i)
    {
        char* Extension = (char*)gl.glGetStringi(GL_EXTENSIONS, i);
        if(!Extension)
            continue;

#if ENGINE_DEBUG
        Platform.Log(Extension);

        if(StringsAreEqual(Extension, "GL_KHR_debug"))
        {
            Platform.Log("Found GL_KHR_debug");
            Info->KHR_debug = true;
        }
        else if(StringsAreEqual(Extension, "GL_ARB_debug_output"))
        {
            Platform.Log("Found GL_ARB_debug_output");
            Info->ARB_debug_output = true;
        }
#endif
    }
}

#define OpenGLLoadAndCheck(Type, Name)                                                                                 \
    {                                                                                                                  \
        gl.Name = (Type *)GL_LOAD_PROC((GL_LOAD_PROC_CAST) #Name);                                                     \
        if(!gl.Name)                                                                                                   \
        {                                                                                                              \
            Platform.Log("Unable to load gl function: " #Name);                                                        \
        }                                                                                                              \
    }

internal void
LoadOpenGLFunctions(opengl_info *Info)
{
    OpenGLLoadAndCheck(gl_gen_buffers, glGenBuffers);
    OpenGLLoadAndCheck(gl_bind_buffer, glBindBuffer);
    OpenGLLoadAndCheck(gl_buffer_data, glBufferData);
    OpenGLLoadAndCheck(gl_map_buffer_range, glMapBufferRange);
    OpenGLLoadAndCheck(gl_unmap_buffer, glUnmapBuffer);
    OpenGLLoadAndCheck(gl_create_shader, glCreateShader);
    OpenGLLoadAndCheck(gl_shader_source, glShaderSource);
    OpenGLLoadAndCheck(gl_compile_shader, glCompileShader);
    OpenGLLoadAndCheck(gl_get_shaderiv, glGetShaderiv);
    OpenGLLoadAndCheck(gl_get_shader_info_log, glGetShaderInfoLog);
    OpenGLLoadAndCheck(gl_create_program, glCreateProgram);
    OpenGLLoadAndCheck(gl_attach_shader, glAttachShader);
    OpenGLLoadAndCheck(gl_bind_frag_data_location, glBindFragDataLocation);
    OpenGLLoadAndCheck(gl_link_program, glLinkProgram);
    OpenGLLoadAndCheck(gl_use_program, glUseProgram);
    OpenGLLoadAndCheck(gl_get_program_iv, glGetProgramiv);
    OpenGLLoadAndCheck(gl_get_program_info_log, glGetProgramInfoLog);
    OpenGLLoadAndCheck(gl_get_attrib_location, glGetAttribLocation);
    OpenGLLoadAndCheck(gl_vertex_attrib_pointer, glVertexAttribPointer);
    OpenGLLoadAndCheck(gl_enable_vertex_attrib_array, glEnableVertexAttribArray);
    OpenGLLoadAndCheck(gl_disable_vertex_attrib_array, glDisableVertexAttribArray);
    OpenGLLoadAndCheck(gl_gen_vertex_arrays, glGenVertexArrays);
    OpenGLLoadAndCheck(gl_bind_vertex_array, glBindVertexArray);
    OpenGLLoadAndCheck(gl_get_uniform_location, glGetUniformLocation);
    OpenGLLoadAndCheck(gl_delete_vertex_arrays, glDeleteVertexArrays);
    OpenGLLoadAndCheck(gl_delete_buffers, glDeleteBuffers);
    OpenGLLoadAndCheck(gl_delete_shader, glDeleteShader);
    OpenGLLoadAndCheck(gl_delete_program, glDeleteProgram);
    OpenGLLoadAndCheck(gl_detach_shader, glDetachShader);
    OpenGLLoadAndCheck(gl_uniform_matrix4fv, glUniformMatrix4fv);
    OpenGLLoadAndCheck(gl_buffer_sub_data, glBufferSubData);
    OpenGLLoadAndCheck(gl_active_texture, glActiveTexture);
    OpenGLLoadAndCheck(gl_uniform1i, glUniform1i);
    OpenGLLoadAndCheck(gl_uniform4f, glUniform4f);

    // NOTE(kiljacken): Everything following this comment is an extension, and might be null

    // NOTE(inso): On linux you should only check these if you know they were in the extension string(s)
    // if it's not available glXGetProcAddress can still return non-null but it'll break.
    // see https://dri.freedesktop.org/wiki/glXGetProcAddressNeverReturnsNULL/

#if ENGINE_DEBUG
    if(Info->KHR_debug || Info->MajorVersion > 4 || (Info->MajorVersion == 4 && Info->MinorVersion >= 3))
    {
        OpenGLLoadAndCheck(gl_debug_message_callback, glDebugMessageCallback);
    }
    else if(Info->ARB_debug_output)
    {
        OpenGLLoadAndCheck(gl_debug_message_callback_arb, glDebugMessageCallbackARB);
    }
#endif
}

internal void
InitOpenGL(b32 ModernContext)
{
    opengl_info Info = OpenGLGetInfo(ModernContext);

    Platform.Log(Info.Vendor);
    Platform.Log(Info.Renderer);
    Platform.Log(Info.Version);
    Platform.Log(Info.ShadingLanguageVersion);
    
    // NOTE(inso): Need to load this func early since CheckExtensions needs it.
    OpenGLLoadAndCheck(gl_get_stringi, glGetStringi);

    OpenGLCheckExtensions(&Info);
    LoadOpenGLFunctions(&Info);

#if ENGINE_DEBUG
    if(Info.KHR_debug || Info.MajorVersion > 4 || (Info.MajorVersion == 4 && Info.MinorVersion >= 3))
    {
        gl.glDebugMessageCallback((gl_debug_proc*)&OpenGLErrorCallback, NULL);
        glEnable(GL_DEBUG_OUTPUT);
        Platform.Log("Using GL_KHR_debug for gl debug output");
    }
    else if(Info.ARB_debug_output)
    {
        gl.glDebugMessageCallbackARB((gl_debug_proc*)&OpenGLErrorCallback, NULL);
        glEnable(GL_DEBUG_OUTPUT);
        Platform.Log("Using GL_ARB_debug_output for gl debug output");
    }
    else
    {
        Platform.Log("No gl debug output extension available");
    }
#endif
/*
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    
    glEnable(GL_MULTISAMPLE);
    
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CW);
    
    int SampleData;
    glGetIntegerv(GL_SAMPLE_BUFFERS, &SampleData);
    glGetIntegerv(GL_SAMPLES, &SampleData);
*/
}

inline bool
CheckForReload(char *FilePath, int64 *OldFileWriteTime)
{
    TIMED_BLOCK();

    int64 NewFileWriteTime = Platform.GetFileTime(FilePath);
    if(*OldFileWriteTime != NewFileWriteTime)
    {
        OldFileWriteTime = &NewFileWriteTime;
        return true;
    }

    return false;
}

internal shader
OpenGLLoadShader(char *FilePath, uint32 ShaderType)
{
    TIMED_BLOCK();

    shader Result = {};

    read_file_result File = Platform.ReadEntireFile(FilePath);
    Result.OldFileWriteTime = Platform.GetFileTime(FilePath);

    if(File.Contents)
    {
        Result.ID = gl.glCreateShader(ShaderType);

        gl.glShaderSource(Result.ID, 1, (GLchar**)&File.Contents, 0);
        gl.glCompileShader(Result.ID);

        GLint ShaderCompileStatus = 0;
        gl.glGetShaderiv(Result.ID, GL_COMPILE_STATUS, &ShaderCompileStatus);

        if(ShaderCompileStatus == GL_FALSE)
        {
            GLint ShaderInfoLogLength = 0;
            gl.glGetShaderiv(Result.ID, GL_INFO_LOG_LENGTH, &ShaderInfoLogLength);

            if(ShaderInfoLogLength > 1)
            {
                GLchar* ShaderInfoLog = (GLchar *)Platform.AllocateMemory(ShaderInfoLogLength);
                gl.glGetShaderInfoLog(Result.ID, ShaderInfoLogLength, &ShaderInfoLogLength, ShaderInfoLog);
                Platform.Log(FilePath);
                Platform.Log(ShaderInfoLog);
                Platform.DeallocateMemory(ShaderInfoLog);
            }
            else
            {
                Platform.Log("LoadShader: Your info log was too short");
            }
        }

        Platform.FreeFileMemory(File.Contents);
    }
    else
    {
        Platform.Log(FilePath);
        Platform.Log("Shader returned no contents");
    }

    return (Result);
}

internal void
OpenGLLinkShader(GLuint ShaderProgram, GLuint ShaderOne, GLuint ShaderTwo)
{
    TIMED_BLOCK();

    gl.glAttachShader(ShaderProgram, ShaderOne);
    gl.glAttachShader(ShaderProgram, ShaderTwo);
    gl.glLinkProgram(ShaderProgram);

    GLint ProgramLinkStatus = 0;
    gl.glGetProgramiv(ShaderProgram, GL_LINK_STATUS, &ProgramLinkStatus);
    if(ProgramLinkStatus == GL_FALSE)
    {
        GLint ProgramInfoLogLength = 0;
        gl.glGetProgramiv(ShaderProgram, GL_INFO_LOG_LENGTH, &ProgramInfoLogLength);
        if(ProgramInfoLogLength > 1)
        {
            GLchar* ProgramInfoLog = (GLchar *)Platform.AllocateMemory(ProgramInfoLogLength);
            gl.glGetProgramInfoLog(ShaderProgram, ProgramInfoLogLength, &ProgramInfoLogLength, ProgramInfoLog);
            Platform.Log(ProgramInfoLog);
            Platform.DeallocateMemory(ProgramInfoLog);
        }
        else
        {
            Platform.Log("LinkShader: Your info log was too short");
        }
    }
    else
    {
        gl.glDetachShader(ShaderProgram, ShaderOne);
        gl.glDetachShader(ShaderProgram, ShaderTwo);
    }
}

internal bool
OpenGLCheckLoadShader(shader *VertexShader,
                      shader *FragmentShader,
                      GLuint *ShaderProgram,
                      char* VertexShaderPath,
                      char* FragmentShaderPath)
{
    TIMED_BLOCK();

    bool Result = false;

    if(CheckForReload(VertexShaderPath, &VertexShader->OldFileWriteTime))
    {
        if(VertexShader->ID)
        {
            gl.glDeleteShader(VertexShader->ID);
        }

        *VertexShader = OpenGLLoadShader(VertexShaderPath, GL_VERTEX_SHADER);
        VertexShader->IsInitialized = false;
    }

    if(CheckForReload(FragmentShaderPath, &FragmentShader->OldFileWriteTime))
    {
        if(FragmentShader->ID)
        {
            gl.glDeleteShader(FragmentShader->ID);
        }

        *FragmentShader = OpenGLLoadShader(FragmentShaderPath, GL_FRAGMENT_SHADER);
        FragmentShader->IsInitialized = false;
    }

    if(!FragmentShader->IsInitialized || !VertexShader->IsInitialized)
    {
        Result = true;
        if(*ShaderProgram)
        {
            gl.glDeleteProgram(*ShaderProgram);
        }

        *ShaderProgram = gl.glCreateProgram();
        gl.glBindFragDataLocation(*ShaderProgram, 0, "outColor");

        OpenGLLinkShader(*ShaderProgram, VertexShader->ID, FragmentShader->ID);
        VertexShader->IsInitialized = true;
        FragmentShader->IsInitialized = true;
    }

    return Result;
}

internal bool
OpenGLSetupVAO(GLuint *VaoId, GLuint *VboId, memory_index Size)
{
    TIMED_BLOCK();

    bool Result = false;
    if(!*VaoId)
    {
        gl.glGenVertexArrays(1, VaoId);
        Result = true;
    }
    gl.glBindVertexArray(*VaoId);

    if(!*VboId)
    {
        gl.glGenBuffers(1, VboId);
    }

    // TODO: Find a way for buffers that needs to change size!
    gl.glBindBuffer(GL_ARRAY_BUFFER, *VboId);
    gl.glBufferData(GL_ARRAY_BUFFER, Size, 0, GL_STREAM_DRAW);

    return (Result);
}

internal void
OpenGLUpdateView(render_buffer *RenderBuffer, u32 Width, u32 Height)
{
    TIMED_BLOCK();
    
    glViewport(0, 0, Width, Height);
    RenderBuffer->ProjectionMatrix = Orthographic(0.0f, (float)Width, 0.0f, (float)Height);
    //GLState.ProjectionMatrix = Perspective((float)Width/(float)Height, 90.0f);
}

internal void
OpenGLRenderScene(render_buffer *RenderBuffer)
{
    TIMED_BLOCK();

    uint32 TotalCount[RenderCommandTypeCount] = {0};
    while(RenderBuffer->Current < RenderBuffer->End)
    {
        render_command_type Type = GetNextType(RenderBuffer);
        GetNextCommand(RenderBuffer, Type);
        ++TotalCount[Type];
    }
    BeginRenderBuffer(RenderBuffer);

    glClearColor(RenderBuffer->ClearColor.r, 
                 RenderBuffer->ClearColor.g, 
                 RenderBuffer->ClearColor.b, 
                 RenderBuffer->ClearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if(!GLState.WhiteTexture)
    {
        glGenTextures(1, &GLState.WhiteTexture);
        glBindTexture(GL_TEXTURE_2D, GLState.WhiteTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        uint32 Pixels[] = {0xffffffff};
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, Pixels);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    if(OpenGLCheckLoadShader(&GLState.SimpleVertexShader,
                             &GLState.SimpleFragmentShader,
                             &GLState.SimpleShaderProgram,
                             "shaders/simple_vert.shader",
                             "shaders/simple_frag.shader"))
    {
        GLState.SimpleShaderMVP = gl.glGetUniformLocation(GLState.SimpleShaderProgram, "MVP");
        GLState.SimpleShaderColor = gl.glGetUniformLocation(GLState.SimpleShaderProgram, "color");
    }

    if(OpenGLCheckLoadShader(&GLState.TextVertexShader,
                             &GLState.TextFragmentShader,
                             &GLState.TextShaderProgram,
                             "shaders/text_vert.shader",
                             "shaders/text_frag.shader"))
    {
        GLState.TextShaderMVP   = gl.glGetUniformLocation(GLState.TextShaderProgram, "MVP");
        GLState.TextShaderColor = gl.glGetUniformLocation(GLState.TextShaderProgram, "color");
    }

    if(OpenGLSetupVAO(&GLState.MeshVertexArray,
                      &GLState.MeshVertexBuffer,
                      RenderBuffer->MeshVerticesCount * 2 * 2 * sizeof(float)))
    {
        GLint PosAttrib = gl.glGetAttribLocation(GLState.SimpleShaderProgram, "position");
        gl.glVertexAttribPointer(PosAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(0));
        gl.glEnableVertexAttribArray(PosAttrib);

        GLint TexCoordAttrib = gl.glGetAttribLocation(GLState.SimpleShaderProgram, "vertTexCoord");
        gl.glVertexAttribPointer(TexCoordAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        gl.glEnableVertexAttribArray(TexCoordAttrib);
    }

    if(OpenGLSetupVAO(&GLState.SimpleVertexArray,
                      &GLState.SimpleVertexBuffer,
                      TotalCount[RenderCommand_DrawRect] * 6 * 4 * sizeof(float)))
    {
        GLint PosAttrib = gl.glGetAttribLocation(GLState.SimpleShaderProgram, "position");
        gl.glVertexAttribPointer(PosAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(0));
        gl.glEnableVertexAttribArray(PosAttrib);

        GLint TexCoordAttrib = gl.glGetAttribLocation(GLState.SimpleShaderProgram, "vertTexCoord");
        gl.glVertexAttribPointer(TexCoordAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        gl.glEnableVertexAttribArray(TexCoordAttrib);
    }

    if(OpenGLSetupVAO(&GLState.OutlineVertexArray,
                      &GLState.OutlineVertexBuffer,
                      TotalCount[RenderCommand_DrawRectOutline] * 8 * 4 * sizeof(float)))
    {
        GLint PosAttrib = gl.glGetAttribLocation(GLState.SimpleShaderProgram, "position");
        gl.glVertexAttribPointer(PosAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(0));
        gl.glEnableVertexAttribArray(PosAttrib);

        GLint TexCoordAttrib = gl.glGetAttribLocation(GLState.SimpleShaderProgram, "vertTexCoord");
        gl.glVertexAttribPointer(TexCoordAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        gl.glEnableVertexAttribArray(TexCoordAttrib);
    }

    if(OpenGLSetupVAO(&GLState.LineVertexArray,
                      &GLState.LineVertexBuffer,
                      TotalCount[RenderCommand_DrawLine] * 2 * 4 * sizeof(float)))
    {
        GLint PosAttrib = gl.glGetAttribLocation(GLState.SimpleShaderProgram, "position");
        gl.glVertexAttribPointer(PosAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(0));
        gl.glEnableVertexAttribArray(PosAttrib);

        GLint TexCoordAttrib = gl.glGetAttribLocation(GLState.SimpleShaderProgram, "vertTexCoord");
        gl.glVertexAttribPointer(TexCoordAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        gl.glEnableVertexAttribArray(TexCoordAttrib);
    }

    real32 FocusLength = RenderBuffer->FocusLength;
    real32 ScaleValue = RenderBuffer->MetersToPixels;
    RenderBuffer->ViewMatrix = Identity();
    RenderBuffer->ProjectionMatrix = RenderBuffer->ProjectionMatrix * 
                                     Scale(v2(ScaleValue, ScaleValue));
    mat4 ViewProjectionMatrix = RenderBuffer->ProjectionMatrix * RenderBuffer->ViewMatrix;
    
    u32 PreviousMeshSize = 0;
    u32 PreviousVerticesCount = 0;
    u32 RectCount = 0;
    u32 OutlineRectCount = 0;
    u32 LineCount = 0;

    while(RenderBuffer->Current < RenderBuffer->End)
    {
        render_command_type Type = GetNextType(RenderBuffer);
        void *Data = GetNextCommand(RenderBuffer, Type);

        switch(Type)
        {
            case RenderCommand_DrawMesh:
            {
                render_command_draw_mesh *Command = (render_command_draw_mesh *)Data;

                mat4 TranslationM = Translate(v3(0, 0, RenderBuffer->CameraP.z));
                //mat4 ScaleM = Scale(Command->Dim);
                //mat4 RotateM = Rotate(Command->Angle);
                mat4 ModelMatrix = TranslationM;//* RotateM * ScaleM;
                mat4 MVP = ViewProjectionMatrix * ModelMatrix;

                //mat4 MVP = ViewProjectionMatrix;

                gl.glBindVertexArray(GLState.MeshVertexArray);
                gl.glBindBuffer(GL_ARRAY_BUFFER, GLState.MeshVertexBuffer);
                
                uint32 MeshSize = Command->Mesh.NumberOfVertices*2*2*sizeof(float);
                float *Vertices = (float *)gl.glMapBufferRange(GL_ARRAY_BUFFER, PreviousMeshSize, MeshSize, GL_MAP_WRITE_BIT);

                Assert(Vertices)
                if(Vertices)
                {
                    CopyMemory(Vertices, (float *)Command->Mesh.Vertices, MeshSize);
                }
                
                gl.glUnmapBuffer(GL_ARRAY_BUFFER);

                gl.glActiveTexture(GL_TEXTURE0);
                if(Command->TextureSlot)
                {
                    GLuint TextureHandle = *(GLuint*)&Command->TextureSlot->ImplData;
                    glBindTexture(GL_TEXTURE_2D, TextureHandle);
                }
                else
                {
                    glBindTexture(GL_TEXTURE_2D, GLState.WhiteTexture);
                }

                gl.glUseProgram(GLState.SimpleShaderProgram);

                gl.glUniformMatrix4fv(GLState.SimpleShaderMVP, 1, false, (float *)MVP.Elements);
                gl.glUniform4f(GLState.SimpleShaderColor,
                               Command->Color.r, 
                               Command->Color.g, 
                               Command->Color.b, 
                               Command->Color.a);
            #if 1
                glDrawArrays(GL_TRIANGLES, PreviousVerticesCount, Command->Mesh.NumberOfVertices);
            #else
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                glDrawArrays(GL_TRIANGLES, PreviousVerticesCount, Command->Mesh.NumberOfVertices);
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            #endif

                PreviousMeshSize += MeshSize;
                PreviousVerticesCount += Command->Mesh.NumberOfVertices;
            } break;

            case RenderCommand_DrawRect:
            {
                render_command_draw_rect *Command = (render_command_draw_rect *)Data;

                // TODO: Set DimScale name different
                // TODO: Make function for this!
                real32 DimScale = FocusLength / (FocusLength + (Command->P.z - RenderBuffer->CameraP.z));
                mat4 TranslationM = Translate(Command->P);
                mat4 ScaleM = Scale(Command->Dim * DimScale);
                mat4 RotateM = Rotate(Command->Angle);
                mat4 ModelMatrix = TranslationM * RotateM * ScaleM;
                mat4 MVP = ViewProjectionMatrix * ModelMatrix;

                if(Command->TextureSlot)
                {
                    vec2 HalfPixel =
                        v2(0.5f/(float)Command->TextureSlot->Width, 0.5f/(float)Command->TextureSlot->Height);
                    Command->MinUV = Command->MinUV + HalfPixel;
                    Command->MaxUV = Command->MaxUV - HalfPixel;
                }

                float Vertices[] = {
                    -0.5f, -0.5f, Command->MinUV.x, Command->MinUV.y, -0.5f, 0.5f, Command->MinUV.x, Command->MaxUV.y,
                    0.5f,  0.5f,  Command->MaxUV.x, Command->MaxUV.y,
                    
                    -0.5f, -0.5f, Command->MinUV.x, Command->MinUV.y, 0.5f,  0.5f, Command->MaxUV.x, Command->MaxUV.y,
                    0.5f,  -0.5f, Command->MaxUV.x, Command->MinUV.y,
                };

                gl.glBindVertexArray(GLState.SimpleVertexArray);
                gl.glBindBuffer(GL_ARRAY_BUFFER, GLState.SimpleVertexBuffer);
                gl.glBufferSubData(GL_ARRAY_BUFFER, RectCount*sizeof(Vertices), sizeof(Vertices), Vertices);

                gl.glActiveTexture(GL_TEXTURE0);
                if(Command->TextureSlot)
                {
                    GLuint TextureHandle = *(GLuint*)&Command->TextureSlot->ImplData;
                    glBindTexture(GL_TEXTURE_2D, TextureHandle);
                }
                else
                {
                    glBindTexture(GL_TEXTURE_2D, GLState.WhiteTexture);
                }

                gl.glUseProgram(GLState.SimpleShaderProgram);

                gl.glUniformMatrix4fv(GLState.SimpleShaderMVP, 1, false, (float *)MVP.Elements);
                gl.glUniform4f(GLState.SimpleShaderColor,
                               Command->Color.r, 
                               Command->Color.g, 
                               Command->Color.b, 
                               Command->Color.a);
                               
                glDrawArrays(GL_TRIANGLES, RectCount*6, 6);

                ++RectCount;
            } break;

            case RenderCommand_DrawRectOutline:
            {
                render_command_draw_rect_outline *Command = 
                    (render_command_draw_rect_outline *)Data;

                mat4 TranslationM = Translate(Command->P);
                mat4 ScaleM = Scale(Command->Dim);
                mat4 RotateM = Rotate(Command->Angle);
                mat4 ModelMatrix = TranslationM * RotateM * ScaleM;
                mat4 MVP = ViewProjectionMatrix * ModelMatrix;

                float Vertices[] = {
                    -0.5f, -0.5f, 0.0f, 0.0f, 0.5f,  -0.5f, 1.0f, 0.0f,

                    0.5f,  -0.5f, 1.0f, 0.0f, 0.5f,  0.5f,  1.0f, 1.0f,

                    0.5f,  0.5f,  1.0f, 1.0f, -0.5f, 0.5f,  0.0f, 1.0f,

                    -0.5f, 0.5f,  0.0f, 1.0f, -0.5f, -0.5f, 0.0f, 0.0f,
                };

                gl.glBindVertexArray(GLState.OutlineVertexArray);
                gl.glBindBuffer(GL_ARRAY_BUFFER, GLState.OutlineVertexBuffer);
                gl.glBufferSubData(GL_ARRAY_BUFFER, OutlineRectCount*sizeof(Vertices),
                                   sizeof(Vertices), Vertices);

                gl.glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, GLState.WhiteTexture);

                gl.glUseProgram(GLState.SimpleShaderProgram);

                gl.glUniformMatrix4fv(GLState.SimpleShaderMVP, 1, false, (float*)MVP.Elements);
                gl.glUniform4f(GLState.SimpleShaderColor, 
                               Command->Color.r, 
                               Command->Color.g, 
                               Command->Color.b, 
                               Command->Color.a);

                glDrawArrays(GL_LINES, OutlineRectCount * 8, 8);

                ++OutlineRectCount;
            } break;

            case RenderCommand_DrawLine:
            {
                render_command_draw_line *Command = (render_command_draw_line *)Data;

                mat4 TranslationM = Translate(v3(0, 0, RenderBuffer->CameraP.z));
                mat4 MVP = ViewProjectionMatrix * TranslationM;

                float Vertices[] = {
                    Command->Start.x, Command->Start.y, 0.0f, 0.0f, Command->End.x, Command->End.y, 1.0f, 1.0f
                };

                gl.glBindVertexArray(GLState.LineVertexArray);
                gl.glBindBuffer(GL_ARRAY_BUFFER, GLState.LineVertexBuffer);
                gl.glBufferSubData(GL_ARRAY_BUFFER, LineCount * sizeof(Vertices), sizeof(Vertices), Vertices);

                gl.glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, GLState.WhiteTexture);

                gl.glUseProgram(GLState.SimpleShaderProgram);

                gl.glUniformMatrix4fv(GLState.SimpleShaderMVP, 1, false, (float*)MVP.Elements);
                gl.glUniform4f(GLState.SimpleShaderColor, 
                            Command->Color.r,
                            Command->Color.g,
                            Command->Color.b,
                            Command->Color.a);

                glDrawArrays(GL_LINES, LineCount*2, 2);

                ++LineCount;
            } break;

            /*case RenderCommand_DrawPoint
            {
                render_command_draw_point *Command = (render_command_draw_point *)Data;

                glDrawArrays(GL_POINTS, );
            } break;*/
            
            case RenderCommand_LoadTexture:
            {
                render_command_load_texture* Command = 
                    (render_command_load_texture *)Data;

                GLuint* TextureHandle_ = (GLuint*)Command->TextureSlot->ImplData;
                if(!*TextureHandle_)
                {
                    glGenTextures(1, TextureHandle_);
                }

                GLuint TextureHandle = *TextureHandle_;

                glBindTexture(GL_TEXTURE_2D, TextureHandle);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, Command->Interpolate ? GL_LINEAR : GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, Command->Interpolate ? GL_LINEAR : GL_NEAREST);

                read_file_result ImageFile = Platform.ReadEntireFile(Command->Filename);

                int Width, Height, Comps;
                uint8* IData = stbi_load_from_memory(
                    (uint8*)ImageFile.Contents, ImageFile.ContentsSize, &Width, &Height, &Comps, 4);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, IData);
                stbi_image_free(IData);
                Platform.FreeFileMemory(ImageFile.Contents);

                Command->TextureSlot->Width  = Width;
                Command->TextureSlot->Height = Height;
            } break;

            case RenderCommand_LoadTextureRaw:
            {
                render_command_load_texture_raw *Command = 
                    (render_command_load_texture_raw *)Data;

                GLuint* TextureHandle_ = (GLuint*)Command->TextureSlot->ImplData;
                if(!*TextureHandle_)
                {
                    glGenTextures(1, TextureHandle_);
                }

                GLuint TextureHandle = *TextureHandle_;
                glBindTexture(GL_TEXTURE_2D, TextureHandle);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, Command->Interpolate ? GL_LINEAR : GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, Command->Interpolate ? GL_LINEAR : GL_NEAREST);
                glTexImage2D(GL_TEXTURE_2D,
                             0,
                             GL_RGBA,
                             Command->Width,
                             Command->Height,
                             0,
                             GL_RGBA,
                             GL_UNSIGNED_BYTE,
                             (GLvoid *)Command->Data);
                glBindTexture(GL_TEXTURE_2D, 0);

                Command->TextureSlot->Width  = Command->Width;
                Command->TextureSlot->Height = Command->Height;
            } break;

            case RenderCommand_LoadFont:
            {
                render_command_load_font *Command = (render_command_load_font *)Data;
                font_texture *Dest = (font_texture *)Command->FontSlot->Data;

                read_file_result FontFile = Platform.ReadEntireFile(Command->FileName);
                if(FontFile.Contents)
                {
                    uint8 *TextureData = (uint8 *)Platform.AllocateMemory(FONT_ATLAS_WIDTH * FONT_ATLAS_HEIGHT);

                    stbtt_pack_context Pack;
                    stbtt_PackBegin(&Pack, TextureData, FONT_ATLAS_WIDTH, FONT_ATLAS_HEIGHT, 0, 1, 0);
                    stbtt_PackSetOversampling(&Pack, Command->OversampleX, Command->OversampleY);
                    stbtt_PackFontRange(&Pack, (uint8*)FontFile.Contents, 0, Command->Height, 33, 94, Dest->Chars);
                    stbtt_PackEnd(&Pack);

                    if(!Dest->Handle)
                    {
                        glGenTextures(1, &Dest->Handle);
                    }
                    glBindTexture(GL_TEXTURE_2D, Dest->Handle);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glTexImage2D(GL_TEXTURE_2D,
                                 0,
                                 GL_RED,
                                 FONT_ATLAS_WIDTH,
                                 FONT_ATLAS_HEIGHT,
                                 0,
                                 GL_RED,
                                 GL_UNSIGNED_BYTE,
                                 TextureData);
                    glBindTexture(GL_TEXTURE_2D, 0);

                    Platform.DeallocateMemory(TextureData);
                    Platform.FreeFileMemory(FontFile.Contents);
                }
                else
                {
                    Platform.Log("Unable to read font file!");
                }
            } break;

            case RenderCommand_DrawString:
            {
                render_command_draw_string *Command = (render_command_draw_string *)Data;
                uint8 *String = (uint8 *)Command + sizeof(render_command_draw_string);

                font_texture *Font = (font_texture *)Command->FontSlot->Data;

                mat4 TranslationM = Translate(Command->P);
                mat4 ScaleMat = Scale(v2(Command->Scale, Command->Scale));
                mat4 MVP = ViewProjectionMatrix * TranslationM * ScaleMat;

                GLuint vao;
                gl.glGenVertexArrays(1, &vao);
                gl.glBindVertexArray(vao);

                GLuint vbo;
                gl.glGenBuffers(1, &vbo);
                gl.glBindBuffer(GL_ARRAY_BUFFER, vbo);

                gl.glBufferData(GL_ARRAY_BUFFER, 256 * 6 * 4 * sizeof(float), 0, GL_STREAM_DRAW);

                gl.glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, Font->Handle);

                gl.glUseProgram(GLState.TextShaderProgram);
                GLint PosAttrib = gl.glGetAttribLocation(GLState.TextShaderProgram, "position");
                gl.glVertexAttribPointer(PosAttrib, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)(0));
                gl.glEnableVertexAttribArray(PosAttrib);

                GLint TexCoordAttrib = gl.glGetAttribLocation(GLState.TextShaderProgram, "vertTexCoord");
                gl.glVertexAttribPointer(TexCoordAttrib, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)(2*sizeof(float)));
                gl.glEnableVertexAttribArray(TexCoordAttrib);

                gl.glUniformMatrix4fv(GLState.TextShaderMVP, 1, false, (float*)MVP.Elements);
                gl.glUniform4f(GLState.TextShaderColor, Command->Color.r, Command->Color.g, Command->Color.b, Command->Color.a);

                vec2 Pos = {0};
                u32 Chars = 0;
                u32 CharCount = 0;
                for(uint8 *Current = String;
                    *Current && CharCount < Command->CharCount;
                    ++Current, ++CharCount)
                {
                    if(*Current > 32 && *Current < 127)
                    {
                        int Index = *Current - 33;
                        stbtt_aligned_quad Quad;
                        stbtt_GetPackedQuad(Font->Chars, FONT_ATLAS_WIDTH, FONT_ATLAS_HEIGHT, Index, &Pos.x, &Pos.y, &Quad, 0);

                        // float x0,y0,s0,t0; // top-left     
                        // float x1,y1,s1,t1; // bottom-right

                        float Vertices[] = {
                            Quad.x0, Quad.y0, Quad.s0, Quad.t0,
                            Quad.x1, Quad.y0, Quad.s1, Quad.t0,
                            Quad.x1, Quad.y1, Quad.s1, Quad.t1,

                            Quad.x0, Quad.y0, Quad.s0, Quad.t0,
                            Quad.x1, Quad.y1, Quad.s1, Quad.t1,
                            Quad.x0, Quad.y1, Quad.s0, Quad.t1,
                        };

                        gl.glBufferSubData(GL_ARRAY_BUFFER, Chars*sizeof(Vertices), sizeof(Vertices), Vertices);
                        ++Chars;
                    }
                    else if(*Current == ' ')
                    {
                        // TODO(kiljacken): We might want to be smarter about this
                        Pos.x += Font->Chars[0].xadvance;
                    }
                }

                glDrawArrays(GL_TRIANGLES, 0, Chars*6);
                gl.glBindBuffer(GL_ARRAY_BUFFER, 0);
                gl.glBindVertexArray(0);

                gl.glDeleteBuffers(1, &vbo);
                gl.glDeleteVertexArrays(1, &vao);
            } break;

            case RenderCommand_SetCameraMatrix:
            {
                render_command_set_camera_matrix *Command = 
                    (render_command_set_camera_matrix *)Data;
                
                RenderBuffer->ViewMatrix = Command->Matrix;
                ViewProjectionMatrix = RenderBuffer->ProjectionMatrix * 
                                       RenderBuffer->ViewMatrix;
            } break;

            default:
            {
                InvalidCodePath;
            } break;
        }
    }

    RenderBuffer->MeshVerticesCount = 0;
}