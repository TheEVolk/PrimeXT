/*
  The Render Pipeline is responsible for the entire image rendering process.
*/
#pragma once
#include "gl_local.h"

namespace render {
enum MrtFlags {
  MRT_NONE = 0,
  MRT_NOTRANSPARENT = BIT(0), // not use in transparent
  MRT_ALLPASS = BIT(1) // using on other render passess
};

inline MrtFlags operator|(MrtFlags a, MrtFlags b)
{
  return static_cast<MrtFlags>(static_cast<int>(a) | static_cast<int>(b));
}

inline MrtFlags& operator|=(MrtFlags& a, MrtFlags b)
{
  return a = a | b;
}

struct render_context_t {
  ref_viewpass_t* rvp;
  RefParams params;
};

class CRenderFeature;
typedef bool (CRenderFeature::*ConveyorHandler)(render_context_t* context);

struct conveyor_item_t {
  ConveyorHandler handler;
  CRenderFeature* object;
};

enum ConveyorItemIntent {
  PRE_FRAME,
  PRE_RENDER,
  POST_RENDER,
  POST_FRAME
};

struct conveyor_item_request_t {
  ConveyorItemIntent intent;
  ConveyorHandler handler;
  CRenderFeature* object;
};

struct MrtTarget {
  const char* name;
  int msaaTexture;
  int texture;
  MrtFlags flags;
};

class CRenderPipeline {
  public:
  CRenderPipeline();
  void pushFeatureItem(conveyor_item_request_t& request);

  // render
  bool renderScene(render_context_t* context);
  int renderFrame(render_context_t* context);
  bool startFrame(render_context_t* context);
  bool endFrame(render_context_t* context);
  void SetMrtTarget(const char* name, int texture, MrtFlags flags);

  private:
      MrtTarget _mrtTargets[16];
      unsigned char _mrtTargetsSize = 0;
      bool _needBuildMrt = false;
      // rawItems
      std::vector<conveyor_item_request_t> rawItems;
      // builded
      conveyor_item_t* _conveyor;
      unsigned char _conveyorSize = 0;
      bool _needBuildConveyor = true;

      void addConveyorItems(int* index, ConveyorItemIntent intent);
      void buildConveyor();
      void buildMrt();
      bool processConveyor(render_context_t* context);
};

extern CRenderPipeline* pipeline;
}
