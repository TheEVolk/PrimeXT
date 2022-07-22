#include "render_features.h"
#include "gl_local.h"
#include "gl_pipeline.h"

namespace render::features {
CVelocityMapRenderFeature velocityMapRenderFeature;

CVelocityMapRenderFeature::CVelocityMapRenderFeature()
{
}

void CVelocityMapRenderFeature::init()
{
  this->texture = CREATE_TEXTURE("*velocity_texture", glState.width, glState.height, NULL, TF_NOMIPMAP);
  pipeline->addMrtTarget("velocity", this->texture, TF_NOMIPMAP);
}
}