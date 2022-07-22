#pragma once

//struct gl_drawbuffer_s;
// extern gl_drawbuffer_s gl_drawbuffer_t;

namespace render
{
  /*
    Render Feature allows you to add new passes and processing effects to the RenderPipeline
  */
  class CRenderFeature
  {
    public:
    int visframe;

    private:
    // std::pair<gl_drawbuffer_t*, int> createScreenBuffer(const std::string &name);
  };
}