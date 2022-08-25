#include "gl_pipeline.h"
#include "gl_cvars.h"
#include "gl_debug.h"
#include "gl_local.h"
#include "gl_studio.h"
#include "gl_world.h"
#include "hud.h"
#include "r_weather.h"
#include "ref_params.h"
#include "screenfade.h"
#include "shake.h"
#include "tri.h"
#include "ui/imgui_manager.h"
#include "utils.h"
#include "gl_grass.h"
#include "dev_profiler.h"
#include "features/render_features.h"
#include <algorithm>
#include "gl_postprocess.h"

extern CBasePostEffects post;

#define SKY_FOG_FACTOR 16.0f // experimentally determined value (chislo s potolka)

// from gl_rmain
void R_BuildViewPassHierarchy(void);
void R_SetupViewCache(const ref_viewpass_t* rvp);
// TODO: to feature
void R_RenderVelocityMap();
void R_ResetGLstate(void);
void R_RenderScreenQuad();
void GL_PrintStats(int params);
void GL_ComputeSunParams(const Vector& skyVector);

extern CHud gHUD;

/*
  The whole task is that all rendering functions need to be output to separate
  Render Features. That is, shadows, mirrors, cameras, and so on.

  This class contains functions related to the world rendering cycle,
  but does not contain any additional utilities.
*/

namespace render {
CRenderPipeline* pipeline = new CRenderPipeline();

CRenderPipeline::CRenderPipeline()
{
  // preFrameFeatures
  this->rawItems.push_back({ ConveyorItemIntent::PRE_FRAME,
      (ConveyorHandler)&render::features::CMainRenderFeature::preFrameHandler,
      (CRenderFeature*)&render::features::mainRenderFeature });

  // preRenderFeatures
  // postRenderFeatures
  // postFrameFeatures

  // postFxFeatures
}

bool CRenderPipeline::renderScene(render_context_t* context)
{
  // printf("drawing scene\n");
  GlDebugScope _gs(__FUNCTION__);
  RI->params = context->params;
  tr.frametime = RP_NORMALPASS() ? tr.saved_frametime : 0.0;

  R_BuildViewPassHierarchy();
  R_SetupViewCache(context->rvp);

  R_CheckSkyPortal(tr.sky_camera);
  R_RenderSubview(); // prepare subview frames
  R_RenderShadowmaps(); // draw all the shadowmaps

  R_SetupGLstate();
  R_Clear(~0);

  R_DrawSkyBox();
  R_RenderSolidBrushList();
  R_RenderSolidStudioList();
  HUD_DrawNormalTriangles();
  R_RenderSurfOcclusionList();
  R_DrawParticles(false);
  R_RenderDebugStudioList(false);

  GL_CheckForErrors();

  // restore right depthrange here
  // GL_DepthRange(gldepthmin, gldepthmax);

  pglDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);

  R_RenderTransList();

  R_DrawParticles(true);
  R_DrawWeather();
  HUD_DrawTransparentTriangles();
  GLenum buffers[11] = {
    GL_COLOR_ATTACHMENT0_EXT,
    GL_COLOR_ATTACHMENT1_EXT,
    GL_COLOR_ATTACHMENT2_EXT,
    GL_COLOR_ATTACHMENT3_EXT,
    GL_COLOR_ATTACHMENT4_EXT,
    GL_COLOR_ATTACHMENT5_EXT,
    GL_COLOR_ATTACHMENT6_EXT,
    GL_COLOR_ATTACHMENT7_EXT,
    GL_COLOR_ATTACHMENT8_EXT,
    GL_COLOR_ATTACHMENT9_EXT,
    GL_COLOR_ATTACHMENT10_EXT
  };
  pglDrawBuffersARB(_mrtTargetsSize + 1, buffers);

  GL_CheckForErrors();

  R_ResetGLstate();

  GL_BindShader(NULL);

  return true;
}

int CRenderPipeline::renderFrame(render_context_t* context)
{
  GlDebugScope _gs(__FUNCTION__);

  if (_needBuildMrt) {
    this->buildMrt();
  }

  return this->processConveyor(context);
  return 1;
}

void CRenderPipeline::addConveyorItems(int* index, ConveyorItemIntent intent)
{
  printf("Finding...\n");
  std::for_each(this->rawItems.begin(), this->rawItems.end(), [=](conveyor_item_request_t& request) {
    if (request.intent != intent) {
      return;
    }

    this->_conveyor[*index] = {
      request.handler,
      request.object
    };

    *index = *index + 1;
  });
}

void CRenderPipeline::buildMrt()
{
  // MSAA
  for (int i = 0; i < _mrtTargetsSize; i++) {
    GL_AttachColorTextureToFBO(tr.screen_temp_fbo_msaa, _mrtTargets[i].msaaTexture, i + 1);
    GL_AttachColorTextureToFBO(tr.screen_temp_fbo, _mrtTargets[i].texture, i + 1);
  }

  GL_CheckFBOStatus(tr.screen_temp_fbo_msaa);
  GL_CheckFBOStatus(tr.screen_temp_fbo);
  _needBuildMrt = false;
}

void CRenderPipeline::buildConveyor()
{
  if (this->_conveyorSize) {
    free(this->_conveyor);
  }

  this->_conveyorSize = 3 + this->rawItems.size();
  this->_conveyor = (conveyor_item_t*)malloc(this->_conveyorSize * sizeof(conveyor_item_t));

  printf("Index...\n");
  int index = 0;
  this->addConveyorItems(&index, ConveyorItemIntent::PRE_FRAME);
  this->_conveyor[index] = { (ConveyorHandler)&CRenderPipeline::startFrame, (CRenderFeature*)this };
  index++;
  this->addConveyorItems(&index, ConveyorItemIntent::PRE_RENDER);
  this->_conveyor[index] = { (ConveyorHandler)&CRenderPipeline::renderScene, (CRenderFeature*)this };
  index++;
  this->addConveyorItems(&index, ConveyorItemIntent::POST_RENDER);
  this->_conveyor[index] = { (ConveyorHandler)&CRenderPipeline::endFrame, (CRenderFeature*)this };
  index++;
  this->addConveyorItems(&index, ConveyorItemIntent::POST_FRAME);
  printf("Builded %i items\n", this->_conveyorSize);
}

bool CRenderPipeline::processConveyor(render_context_t* context)
{
  if (this->_needBuildConveyor) {
    this->buildConveyor();
    this->_needBuildConveyor = false;
  }

  if (this->_conveyorSize == 0) {
    return false;
  }

  for (int i = 0; i < this->_conveyorSize; i++) {
    render::conveyor_item_t item = this->_conveyor[i];
    if (!(item.object->*item.handler)(context)) {
      // printf("False: index %i.\n", i);
      return false;
    }
  }

  return true;
}

bool CRenderPipeline::startFrame(render_context_t* context)
{
  if (CVAR_TO_BOOL(gl_hdr))
    GL_BindDrawbuffer(tr.screen_temp_fbo_msaa);

  bool allow_dynamic_sun = false;
  static float cached_lighting = 0.0f;
  static float shadowmap_size = 0.0f;
  static int waterlevel_old;
  float levelTime = r_sun_daytime->value;
  float blurAmount = 0.0;

  r_speeds_msg[0] = r_depth_msg[0] = '\0';
  tr.fCustomRendering = false;
  tr.fGamePaused = RENDER_GET_PARM(PARAM_GAMEPAUSED, 0);
  memset(&r_stats, 0, sizeof(r_stats));
  tr.sunlight = NULL;

  if (r_buildstats.total_buildtime > 0.0 && RENDER_GET_PARM(PARM_CLIENT_ACTIVE, 0)) {
    // display time to building some stuff: lighting, VBO, shaders etc
    if (r_buildstats.compile_shaders > 0.0) {
      ALERT(at_aiconsole,
          "^3GL_BackendStartFrame: ^7shaders compiled in %.1f msec\n",
          r_buildstats.compile_shaders * 1000.0);
    }
    if (r_buildstats.create_buffer_object > 0.0) {
      double time = (r_buildstats.create_buffer_object - r_buildstats.compile_shaders) * 1000.0;
      ALERT(at_aiconsole,
          "^3GL_BackendStartFrame: ^7VBO created in %1.f msec\n", time);
    }
    if (r_buildstats.create_light_cache > 0.0) {
      ALERT(at_aiconsole,
          "^3GL_BackendStartFrame: ^7VL cache created in %.1f msec\n",
          r_buildstats.create_light_cache * 1000.0);
    }
    if (r_buildstats.create_buffer_object > 0.0 || r_buildstats.create_light_cache > 0.0) {
      ALERT(at_aiconsole,
          "^3GL_BackendStartFrame: ^7total building time = %g seconds\n",
          r_buildstats.total_buildtime);
    }
    memset(&r_buildstats, 0, sizeof(r_buildstats));
  }

  if (!g_fRenderInitialized)
    return 0;

  if (!FBitSet(context->params, RP_DRAW_WORLD))
    GL_Setup3D();

  if (CVAR_TO_BOOL(r_finish) && FBitSet(context->params, RP_DRAW_WORLD))
    pglFinish();

  // setup light factors
  tr.ambientFactor = bound(AMBIENT_EPSILON, r_lighting_ambient->value,
      r_lighting_modulate->value);
  float cachedFactor = bound(tr.ambientFactor, r_lighting_modulate->value, 1.0f);

  if (cachedFactor != tr.diffuseFactor) {
    tr.diffuseFactor = cachedFactor;
    R_StudioClearLightCache();
  }

  if (cached_lighting != r_lighting_extended->value) {
    cached_lighting = r_lighting_extended->value;
    R_StudioClearLightCache();
  }

  if (shadowmap_size != r_shadowmap_size->value) {
    shadowmap_size = r_shadowmap_size->value;
    R_InitShadowTextures();
  }

  // FIXME: allow player overview for custom renderer
  if (!FBitSet(context->params, RP_DRAW_WORLD))
    return 0;

  // use engine renderer
  if (cv_renderer->value == 0) {
    glState.depthmin = -1.0f;
    glState.depthmax = -1.0f;
    glState.depthmask = -1;
    return 0;
  }

  // keep world in actual state
  GET_ENTITY(0)->curstate.messagenum = r_currentMessageNum;

  if (levelTime != -1.0f && r_sun_allowed->value == 1.0f) {
    Vector skyVector;
    matrix4x4 a;

    float time = levelTime;
    float timeAng = (((time + 12.0f) / 24.0f) * M_PI * 2.0f);
    float longitude = RAD2DEG(0.5f * M_PI) - 90.0f;

    a.CreateRotate(RAD2DEG(timeAng) - 180.0f, 1.0, 0.5f, 0.0f);
    a.ConcatRotate(longitude, -0.5f, 1.0f, 0.0f);
    skyVector = a.VectorRotate(Vector(0.0f, 0.0f, 1.0f)).Normalize();
    GL_ComputeSunParams(skyVector);
    allow_dynamic_sun = true;

    gEngfuncs.Con_NPrintf(7, "Day Time: %i:%02.f\n", (int)time,
        (time - (int)time) * 59.0f);
    gEngfuncs.Con_NPrintf(8, "Sun Ambient: %f\n", tr.sun_ambient);
    gEngfuncs.Con_NPrintf(9, "Ambient Factor %f\n", tr.ambientFactor);
  } else {
    Vector skyVector;

    skyVector.x = tr.movevars->skyvec_x;
    skyVector.y = tr.movevars->skyvec_y;
    skyVector.z = tr.movevars->skyvec_z;
    skyVector = skyVector.Normalize();

    GL_ComputeSunParams(skyVector);

    // hidden test mode
    if (r_sun_allowed->value == 2.0)
      allow_dynamic_sun = true;
  }

  tr.sky_ambient.x = tr.movevars->skycolor_r;
  tr.sky_ambient.y = tr.movevars->skycolor_g;
  tr.sky_ambient.z = tr.movevars->skycolor_b;

  // NOTE: sunlight must be added first in list
  if (FBitSet(world->features, WORLD_HAS_SKYBOX) && allow_dynamic_sun) {
    tr.sunlight = CL_AllocDlight(SUNLIGHT_KEY); // alloc sun source
    R_SetupLightParams(tr.sunlight, g_vecZero, g_vecZero, 32768.0f, 90.0f,
        LIGHT_DIRECTIONAL);
    R_SetupLightTexture(tr.sunlight, tr.whiteTexture);
    tr.sunlight->die = GET_CLIENT_TIME();
    tr.sunlight->color = tr.sun_diffuse;
    tr.sun_light_enabled = true;
  }

  tr.gravity = tr.movevars->gravity;
  tr.farclip = tr.movevars->zmax;

  if (tr.farclip == 0.0f || worldmodel == NULL)
    tr.farclip = 4096.0f;

  tr.cached_vieworigin = context->rvp->vieworigin;
  tr.cached_viewangles = context->rvp->viewangles;
  tr.waterlevel = tr.viewparams.waterlevel;

  if ((tr.waterlevel >= 3) != (waterlevel_old >= 3)) {
    waterlevel_old = tr.waterlevel;
    tr.glsl_valid_sequence++; // now all uber-shaders are invalidate and
                              // possible need for recompile
    tr.params_changed = true;
  }

  if (tr.waterlevel >= 3) {
    int waterent = WATER_ENTITY(RI->view.origin);

    // FIXME: how to allow fog on a world water?
    if (waterent > 0 && waterent < tr.viewparams.max_entities)
      tr.waterentity = GET_ENTITY(waterent);
    else
      tr.waterentity = NULL;
  } else
    tr.waterentity = NULL;

  R_GrassSetupFrame();

  // check for fog
  if (tr.waterentity) {
    entity_state_t* state = &tr.waterentity->curstate;

    if (state->rendercolor.r || state->rendercolor.g || state->rendercolor.b) {
      // enable global exponential color fog
      tr.fogColor[0] = (state->rendercolor.r) / 255.0f;
      tr.fogColor[1] = (state->rendercolor.g) / 255.0f;
      tr.fogColor[2] = (state->rendercolor.b) / 255.0f;
      tr.fogDensity = (state->renderamt) * 0.000025f;
      tr.fogSkyDensity = tr.fogDensity * SKY_FOG_FACTOR;
      tr.fogEnabled = true;
    }
  } else if (tr.movevars->fog_settings != 0) {
    // enable global exponential color fog
    // apply gamma-correction because user sets color in sRGB space
    tr.fogColor[0] = pow(
        (tr.movevars->fog_settings & 0xFF000000 >> 24) / 255.0f, 1.f / 2.2f);
    tr.fogColor[1] = pow((tr.movevars->fog_settings & 0xFF0000 >> 16) / 255.0f, 1.f / 2.2f);
    tr.fogColor[2] = pow((tr.movevars->fog_settings & 0xFF00 >> 8) / 255.0f, 1.f / 2.2f);
    tr.fogDensity = (tr.movevars->fog_settings & 0xFF) * 0.000025f;
    tr.fogSkyDensity = tr.fogDensity * SKY_FOG_FACTOR;
    tr.fogEnabled = true;
  } else {
    tr.fogColor[0] = 0.0f;
    tr.fogColor[1] = 0.0f;
    tr.fogColor[2] = 0.0f;
    tr.fogDensity = 0.0f;
    tr.fogSkyDensity = 0.0f;
    tr.fogEnabled = false;
  }

  // apply the underwater warp
  if (tr.waterlevel >= 3) {
    float f = sin(tr.time * 0.4f * (M_PI * 1.7f));
    context->rvp->fov_x += f;
    context->rvp->fov_y -= f;
  }

  // apply the blur effect
  if (blurAmount > 0.0f) {
    float f = sin(tr.time * 0.4 * (M_PI * 1.7f)) * blurAmount * 20.0f;
    context->rvp->fov_x += f;
    context->rvp->fov_y += f;

    screenfade_t fade;

    f = (sin(tr.time * 0.5f * (M_PI * 1.7f)) * 127) + 128;
    fade.fadeFlags = FFADE_STAYOUT;
    fade.fader = fade.fadeg = fade.fadeb = 0;
    fade.fadeReset = tr.time + 0.1f;
    fade.fadeEnd = tr.time + 0.1f;
    fade.fadealpha = bound(0, (byte)f, blurAmount * 255);

    gEngfuncs.pfnSetScreenFade(&fade);
  }

  Mod_ResortFaces();
  GL_LoadAndRebuildCubemaps(context->params);
  tr.fCustomRendering = true;
  r_stats.debug_surface = NULL;

  if (r_speeds->value == 6.0f) {
    Vector vecSrc = RI->view.origin;
    Vector vecEnd = vecSrc + GetVForward() * RI->view.farClip;
    pmtrace_t trace;

    gEngfuncs.pEventAPI->EV_SetTraceHull(2);
    gEngfuncs.pEventAPI->EV_PlayerTrace((float*)&vecSrc, (float*)&vecEnd,
        PM_NORMAL, -1, &trace);
    r_stats.debug_surface = gEngfuncs.pEventAPI->EV_TraceSurface(
        trace.ent, (float*)&vecSrc, (float*)&vecEnd);
  }

  // setup light animation tables
  R_AnimateLight();

  return 1;
}

bool CRenderPipeline::endFrame(render_context_t* context)
{
  mstudiolight_t light;
  bool hdr_rendering = CVAR_TO_BOOL(gl_hdr);
  tr.frametime = tr.saved_frametime;
  GL_DebugGroupPush(__FUNCTION__);

  // go into 2D mode (in case we draw PlayerSetup between two 2d calls)
  if (!FBitSet(context->params, RP_DRAW_WORLD))
    GL_Setup2D();

  if (tr.show_uniforms_peak) {
    ALERT(at_aiconsole, "peak used uniforms: %i\n",
        glConfig.peak_used_uniforms);
    tr.show_uniforms_peak = false;
  }

  DrawLightProbes(); // 3D
  DrawCubeMaps(); // 3D
  DrawViewLeaf(); // 3D
  DrawWireFrame(); // 3D
  DrawTangentSpaces(); // 3D
  DrawWirePoly(r_stats.debug_surface); // 3D
  DBG_DrawLightFrustum(); // 3D

  R_PushRefState();
  RI->params = context->params;
  RI->view.fov_x = context->rvp->fov_x;
  RI->view.fov_y = context->rvp->fov_y;

  if (!CVAR_TO_BOOL(cv_deferred))
    R_DrawViewModel(); // 3D

  if (hdr_rendering) {
    // copy image from multisampling framebuffer to screen framebuffer
    pglBindFramebuffer(GL_DRAW_FRAMEBUFFER, tr.screen_temp_fbo->id);
    pglBindFramebuffer(GL_READ_FRAMEBUFFER, tr.screen_temp_fbo_msaa->id);
    pglBlitFramebuffer(0, 0, glState.width, glState.height, 0, 0, glState.width,
        glState.height,
        GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);

    pglReadBuffer(GL_COLOR_ATTACHMENT0_EXT + 1);
    pglDrawBuffer(GL_COLOR_ATTACHMENT0_EXT + 1);
    pglBlitFramebuffer(0, 0, glState.width, glState.height, 0, 0, glState.width,
        glState.height,
        GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);

    pglReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
    pglDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
    GL_BindFBO(tr.screen_temp_fbo->id);
    RenderAverageLuminance();
  }

  RenderDOF(); // 2D
  RenderNerveGasBlur(); // 2D
  RenderUnderwaterBlur(); // 2D
  if (hdr_rendering) {
    RenderBloom();
    RenderTonemap(); // should be last step!!!
  }

  R_RenderDebugStudioList(true); // 3D
  RenderSunShafts(); // 2D
  RenderMonochrome(); // 2D
  RenderMotionBlur(); // 2D
  R_ShowLightMaps(); // 2D

  if (hdr_rendering) {
    R_RenderScreenQuad();
  }

  GL_CleanupDrawState();
  g_ImGuiManager.NewFrame();

  // restore state for correct rendering other stuff
  GL_Setup3D();
  GL_Blend(GL_TRUE);
  GL_CleanupDrawState();
  R_PopRefState();

  // fill the r_speeds message
  GL_PrintStats(context->params);

  R_UnloadFarGrass();
  GL_DebugGroupPop();
  tr.params_changed = false;
  tr.realframecount++;
  RI->view.worldProjectionMatrix.CopyToArray(post.prev_model_view_project);

  return true;
}

void CRenderPipeline::SetMrtTarget(const char* name, int texture, MrtFlags flags = MrtFlags::MRT_NONE)
{
  int index = -1;
  for (int i = 0; i < _mrtTargetsSize; i++) {
    printf("Check: %s %s\n", name, _mrtTargets[i].name);
    if (!Q_strcmp(name, _mrtTargets[i].name)) {
      index = i;
    }
  }

  if (index == -1) {
    index = _mrtTargetsSize;
    _mrtTargetsSize++;
  }

  _needBuildMrt = true;
  if (false) {
    _mrtTargets[index] = (MrtTarget) { name, texture, 0, flags };
    return;
  }

  if (index != _mrtTargetsSize - 1) {
    FREE_TEXTURE(_mrtTargets[index].msaaTexture);
  }

  char msaaName[32];
  sprintf(msaaName, "*%s_msaa", name);
  int msaaTexture = CREATE_TEXTURE(msaaName, glState.width, glState.height, NULL, TF_MULTISAMPLE);
  _mrtTargets[index] = (MrtTarget) { name, msaaTexture, texture, flags };
}
} // namespace render