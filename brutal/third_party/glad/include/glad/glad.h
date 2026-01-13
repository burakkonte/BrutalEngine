/*
    GLAD - OpenGL Loader (Minimal version for OpenGL 3.3 Core)
    Generated for Brutal Engine
*/
#ifndef GLAD_H
#define GLAD_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#define APIENTRY __stdcall
#define WINGDIAPI __declspec(dllimport)
#else
#define APIENTRY
#define WINGDIAPI
#endif

typedef void GLvoid;
typedef unsigned int GLenum;
typedef float GLfloat;
typedef int GLint;
typedef int GLsizei;
typedef unsigned int GLbitfield;
typedef double GLdouble;
typedef unsigned int GLuint;
typedef unsigned char GLboolean;
typedef unsigned char GLubyte;
typedef char GLchar;
typedef signed long long GLsizeiptr;
typedef signed long long GLintptr;

#define GL_FALSE 0
#define GL_TRUE 1
#define GL_NONE 0
#define GL_NO_ERROR 0
#define GL_DEPTH_BUFFER_BIT 0x00000100
#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_TRIANGLES 0x0004
#define GL_LINES 0x0001
#define GL_SRC_ALPHA 0x0302
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#define GL_FRONT 0x0404
#define GL_BACK 0x0405
#define GL_CW 0x0900
#define GL_CCW 0x0901
#define GL_CULL_FACE 0x0B44
#define GL_DEPTH_TEST 0x0B71
#define GL_BLEND 0x0BE2
#define GL_SCISSOR_TEST 0x0C11
#define GL_UNSIGNED_BYTE 0x1401
#define GL_UNSIGNED_INT 0x1405
#define GL_FLOAT 0x1406
#define GL_RGB 0x1907
#define GL_RGBA 0x1908
#define GL_NEAREST 0x2600
#define GL_LINEAR 0x2601
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_TEXTURE_2D 0x0DE1
#define GL_REPEAT 0x2901
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_INFO_LOG_LENGTH 0x8B84
#define GL_ACTIVE_UNIFORMS 0x8B86
#define GL_ARRAY_BUFFER 0x8892
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#define GL_STREAM_DRAW 0x88E0
#define GL_STATIC_DRAW 0x88E4
#define GL_DYNAMIC_DRAW 0x88E8
#define GL_VERSION 0x1F02
#define GL_RENDERER 0x1F01
#define GL_VENDOR 0x1F00
#define GL_TEXTURE0 0x84C0
#define GL_R8 0x8229
#define GL_RED 0x1903

// Function pointers
typedef void (APIENTRY *PFNGLCLEARCOLORPROC)(GLfloat, GLfloat, GLfloat, GLfloat);
typedef void (APIENTRY *PFNGLCLEARPROC)(GLbitfield);
typedef void (APIENTRY *PFNGLENABLEPROC)(GLenum);
typedef void (APIENTRY *PFNGLDISABLEPROC)(GLenum);
typedef void (APIENTRY *PFNGLVIEWPORTPROC)(GLint, GLint, GLsizei, GLsizei);
typedef void (APIENTRY *PFNGLBLENDFUNCPROC)(GLenum, GLenum);
typedef void (APIENTRY *PFNGLCULLFACEPROC)(GLenum);
typedef void (APIENTRY *PFNGLFRONTFACEPROC)(GLenum);
typedef void (APIENTRY *PFNGLFLUSHPROC)(void);
typedef const GLubyte* (APIENTRY *PFNGLGETSTRINGPROC)(GLenum);
typedef GLenum (APIENTRY *PFNGLGETERRORPROC)(void);
typedef void (APIENTRY* PFNGLSCISSORPROC)(GLint, GLint, GLsizei, GLsizei);
typedef void (APIENTRY *PFNGLDRAWARRAYSPROC)(GLenum, GLint, GLsizei);
typedef void (APIENTRY *PFNGLDRAWELEMENTSPROC)(GLenum, GLsizei, GLenum, const void*);
typedef void (APIENTRY *PFNGLLINEWIDTHPROC)(GLfloat);
typedef void (APIENTRY *PFNGLGENTEXTURESPROC)(GLsizei, GLuint*);
typedef void (APIENTRY *PFNGLDELETETEXTURESPROC)(GLsizei, const GLuint*);
typedef void (APIENTRY *PFNGLBINDTEXTUREPROC)(GLenum, GLuint);
typedef void (APIENTRY *PFNGLTEXIMAGE2DPROC)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*);
typedef void (APIENTRY *PFNGLTEXPARAMETERIPROC)(GLenum, GLenum, GLint);
typedef void (APIENTRY *PFNGLACTIVETEXTUREPROC)(GLenum);

// Modern OpenGL functions
typedef GLuint (APIENTRY *PFNGLCREATESHADERPROC)(GLenum);
typedef void (APIENTRY *PFNGLSHADERSOURCEPROC)(GLuint, GLsizei, const GLchar**, const GLint*);
typedef void (APIENTRY *PFNGLCOMPILESHADERPROC)(GLuint);
typedef void (APIENTRY *PFNGLGETSHADERIVPROC)(GLuint, GLenum, GLint*);
typedef void (APIENTRY *PFNGLGETSHADERINFOLOGPROC)(GLuint, GLsizei, GLsizei*, GLchar*);
typedef void (APIENTRY *PFNGLDELETESHADERPROC)(GLuint);
typedef GLuint (APIENTRY *PFNGLCREATEPROGRAMPROC)(void);
typedef void (APIENTRY *PFNGLATTACHSHADERPROC)(GLuint, GLuint);
typedef void (APIENTRY *PFNGLLINKPROGRAMPROC)(GLuint);
typedef void (APIENTRY *PFNGLGETPROGRAMIVPROC)(GLuint, GLenum, GLint*);
typedef void (APIENTRY *PFNGLGETPROGRAMINFOLOGPROC)(GLuint, GLsizei, GLsizei*, GLchar*);
typedef void (APIENTRY *PFNGLDELETEPROGRAMPROC)(GLuint);
typedef void (APIENTRY *PFNGLUSEPROGRAMPROC)(GLuint);
typedef GLint (APIENTRY *PFNGLGETUNIFORMLOCATIONPROC)(GLuint, const GLchar*);
typedef void (APIENTRY *PFNGLUNIFORM1IPROC)(GLint, GLint);
typedef void (APIENTRY *PFNGLUNIFORM1FPROC)(GLint, GLfloat);
typedef void (APIENTRY *PFNGLUNIFORM2FPROC)(GLint, GLfloat, GLfloat);
typedef void (APIENTRY *PFNGLUNIFORM3FPROC)(GLint, GLfloat, GLfloat, GLfloat);
typedef void (APIENTRY *PFNGLUNIFORM4FPROC)(GLint, GLfloat, GLfloat, GLfloat, GLfloat);
typedef void (APIENTRY *PFNGLUNIFORMMATRIX4FVPROC)(GLint, GLsizei, GLboolean, const GLfloat*);
typedef void (APIENTRY *PFNGLGENVERTEXARRAYSPROC)(GLsizei, GLuint*);
typedef void (APIENTRY *PFNGLDELETEVERTEXARRAYSPROC)(GLsizei, const GLuint*);
typedef void (APIENTRY *PFNGLBINDVERTEXARRAYPROC)(GLuint);
typedef void (APIENTRY *PFNGLGENBUFFERSPROC)(GLsizei, GLuint*);
typedef void (APIENTRY *PFNGLDELETEBUFFERSPROC)(GLsizei, const GLuint*);
typedef void (APIENTRY *PFNGLBINDBUFFERPROC)(GLenum, GLuint);
typedef void (APIENTRY *PFNGLBUFFERDATAPROC)(GLenum, GLsizeiptr, const void*, GLenum);
typedef void (APIENTRY *PFNGLBUFFERSUBDATAPROC)(GLenum, GLintptr, GLsizeiptr, const void*);
typedef void (APIENTRY *PFNGLENABLEVERTEXATTRIBARRAYPROC)(GLuint);
typedef void (APIENTRY *PFNGLVERTEXATTRIBPOINTERPROC)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*);

// Global function pointers
extern PFNGLCLEARCOLORPROC glClearColor;
extern PFNGLCLEARPROC glClear;
extern PFNGLENABLEPROC glEnable;
extern PFNGLDISABLEPROC glDisable;
extern PFNGLVIEWPORTPROC glViewport;
extern PFNGLBLENDFUNCPROC glBlendFunc;
extern PFNGLCULLFACEPROC glCullFace;
extern PFNGLFRONTFACEPROC glFrontFace;
extern PFNGLFLUSHPROC glFlush;
extern PFNGLGETSTRINGPROC glGetString;
extern PFNGLGETERRORPROC glGetError;
extern PFNGLSCISSORPROC glScissor;
extern PFNGLDRAWARRAYSPROC glDrawArrays;
extern PFNGLDRAWELEMENTSPROC glDrawElements;
extern PFNGLLINEWIDTHPROC glLineWidth;
extern PFNGLGENTEXTURESPROC glGenTextures;
extern PFNGLDELETETEXTURESPROC glDeleteTextures;
extern PFNGLBINDTEXTUREPROC glBindTexture;
extern PFNGLTEXIMAGE2DPROC glTexImage2D;
extern PFNGLTEXPARAMETERIPROC glTexParameteri;
extern PFNGLACTIVETEXTUREPROC glActiveTexture;
extern PFNGLCREATESHADERPROC glCreateShader;
extern PFNGLSHADERSOURCEPROC glShaderSource;
extern PFNGLCOMPILESHADERPROC glCompileShader;
extern PFNGLGETSHADERIVPROC glGetShaderiv;
extern PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
extern PFNGLDELETESHADERPROC glDeleteShader;
extern PFNGLCREATEPROGRAMPROC glCreateProgram;
extern PFNGLATTACHSHADERPROC glAttachShader;
extern PFNGLLINKPROGRAMPROC glLinkProgram;
extern PFNGLGETPROGRAMIVPROC glGetProgramiv;
extern PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
extern PFNGLDELETEPROGRAMPROC glDeleteProgram;
extern PFNGLUSEPROGRAMPROC glUseProgram;
extern PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
extern PFNGLUNIFORM1IPROC glUniform1i;
extern PFNGLUNIFORM1FPROC glUniform1f;
extern PFNGLUNIFORM2FPROC glUniform2f;
extern PFNGLUNIFORM3FPROC glUniform3f;
extern PFNGLUNIFORM4FPROC glUniform4f;
extern PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;
extern PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
extern PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays;
extern PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
extern PFNGLGENBUFFERSPROC glGenBuffers;
extern PFNGLDELETEBUFFERSPROC glDeleteBuffers;
extern PFNGLBINDBUFFERPROC glBindBuffer;
extern PFNGLBUFFERDATAPROC glBufferData;
extern PFNGLBUFFERSUBDATAPROC glBufferSubData;
extern PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
extern PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;

// Loader function
int gladLoadGL(void);

#ifdef __cplusplus
}
#endif

#endif // GLAD_H
