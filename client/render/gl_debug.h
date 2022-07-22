#pragma once
#include "vector.h"
#include "com_model.h"

#define GL_CheckForErrors() (GL_CheckForErrorsInternal((__FUNCTION__), (__LINE__)))

//
// gl_debug.cpp
//
void GL_DebugGroupPush(const char *markerName);
void GL_DebugGroupPop();
void GL_CheckForErrorsInternal(const char *filename, int line);
void DBG_PrintVertexVBOSizes();
void DBG_DrawLightFrustum();
void DBG_DrawGlassScissors();
void DrawLightProbes();
void R_ShowLightMaps();
void R_RenderLightProbeInternal(const Vector &origin, const Vector lightCube[]);
void DBG_DrawBBox(const Vector &mins, const Vector &maxs);
void DrawWirePoly(msurface_t *surf);
void DrawTangentSpaces();
void DrawWireFrame();
void DrawViewLeaf();
void DrawCubeMaps();

class GlDebugScope
{
public:
  inline GlDebugScope(const char *markerName) {
    GL_DebugGroupPush(markerName);
  }

  inline ~GlDebugScope() {
    GL_DebugGroupPop();
  }
};