#include "gl_local.h"
#include "render_features.h"
#include "gl_pipeline.h"
#include "utils.h"

namespace render::features {
CMainRenderFeature mainRenderFeature;

CMainRenderFeature::CMainRenderFeature()
{
  // this->texture = CREATE_TEXTURE("*velocity_texture", glState.width, glState.height, NULL, TF_SCREEN);
  // GL_AttachColorTextureToFBO(this->fbo, this->texture, 0, 0, 0);
  //  pglBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo->id);

  // GL_CheckFBOStatus(this->fbo);
}

bool CMainRenderFeature::preFrameHandler(render_context_t* context)
{
  context->params |= (context->rvp->flags & RF_DRAW_WORLD) ? RP_DRAW_WORLD : RP_NONE;

  if (context->rvp->flags & RF_DRAW_CUBEMAP) {
    // now all uber-shaders are invalidate
    // and possible need for recompile
    tr.params_changed = true;
    tr.glsl_valid_sequence++;
    tr.fClearScreen = true;

    context->params |= world->build_default_cubemap ? RP_SKYVIEW : RP_ENVVIEW;
    return true;
  }

  context->params |= context->rvp->flags & RF_DRAW_OVERVIEW ? RP_DRAW_OVERVIEW : RP_NONE;
  context->params |= CL_IsThirdPerson() ? RP_THIRDPERSON : RP_NONE;
  return true;
}

void CMainRenderFeature::setupFrameFlags(render_context_t* context)
{
}
}