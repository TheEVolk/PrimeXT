#include "gl_feature.h"

namespace render {
struct render_context_t;
};

namespace render::features {
class CVelocityMapRenderFeature : CRenderFeature {
  public:
  int texture;
  CVelocityMapRenderFeature();
  void init();

  private:
};

class CHDRRenderFeature : CRenderFeature {
  public:
  CHDRRenderFeature();

  private:
};

class CMainRenderFeature : CRenderFeature {
  public:
  CMainRenderFeature();
  bool preFrameHandler(render_context_t* context);
  void setupFrameFlags(render_context_t* context);

  private:
};

extern CVelocityMapRenderFeature velocityMapRenderFeature;
extern CMainRenderFeature mainRenderFeature;
}