/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Tencent is pleased to support the open source community by making libpag available.
//
//  Copyright (C) 2021 THL A29 Limited, a Tencent company. All rights reserved.
//
//  Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
//  except in compliance with the License. You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  unless required by applicable law or agreed to in writing, software distributed under the
//  license is distributed on an "as is" basis, without warranties or conditions of any kind,
//  either express or implied. see the license for the specific language governing permissions
//  and limitations under the license.
//
/////////////////////////////////////////////////////////////////////////////////////////////////

#include "FeatherMask.h"
#include "pag/file.h"
#include "rendering/caches/RenderCache.h"
#include "rendering/utils/PathUtil.h"
#include "rendering/filters/gaussianblur/GaussianBlurFilter.h"
#include "tgfx/core/Mask.h"
#include "tgfx/gpu/Canvas.h"
#include "tgfx/gpu/Surface.h"
#include "Trace.h"

namespace pag {
std::shared_ptr<Graphic> FeatherMask::MakeFrom(ID assetID, const std::vector<MaskData*>& masks,
                                               Frame layerFrame) {
  if (masks.empty()) {
    return nullptr;
  }
  return std::shared_ptr<Graphic>(new FeatherMask(assetID, masks, layerFrame));
}

tgfx::Rect MeasureFeatherMaskBounds(const std::vector<MaskData*>& masks, Frame layerFrame) {
  float maxRight = 0.0;
  float maxBottom = 0.0;
  for (auto mask : masks) {
    auto maskPath = ToPath(*(mask->maskPath->getValueAt(layerFrame)));
    auto bounds = maskPath.getBounds();
    if (bounds.right > maxRight) {
      maxRight = bounds.right;
    }
    if (bounds.bottom > maxBottom) {
      maxBottom = bounds.bottom;
    }
  }
  return tgfx::Rect::MakeLTRB(0, 0, maxRight * (1.0 + BLUR_EXPEND), maxBottom * (1.0 + BLUR_EXPEND));
}

FeatherMask::FeatherMask(ID assetID, const std::vector<MaskData*>& masks, Frame layerFrame)
    : assetID(assetID), masks(std::move(masks)), layerFrame(layerFrame) {
      bounds = MeasureFeatherMaskBounds(masks, layerFrame);
}

void FeatherMask::measureBounds(tgfx::Rect* rect) const {
  *rect = bounds;
}

bool FeatherMask::hitTest(RenderCache*, float, float) {
  return true;
}

bool FeatherMask::getPath(tgfx::Path*) const {
  return false;
}

void FeatherMask::prepare(RenderCache*) const {
}

std::unique_ptr<Snapshot> FeatherMask::DrawFeatherMask(const std::vector<MaskData*>& masks,
                                                       Frame layerFrame,
                                                       RenderCache* cache, float scaleFactor) const {
  bool isFirst = true;
  auto surface = tgfx::Surface::Make(cache->getContext(),
                                     static_cast<int>(ceilf(bounds.right * scaleFactor)),
                                     static_cast<int>(ceilf(bounds.bottom * scaleFactor)),
                                     true);
  auto canvas = surface->getCanvas();
  canvas->setMatrix(tgfx::Matrix::MakeTrans(bounds.x(), bounds.y()));
  auto totalMatrix = canvas->getMatrix();
  if (surface == nullptr) {
    return nullptr;
  }
  tgfx::Paint paint;
  auto effect = new FastBlurEffect();
  effect->blurriness = new Property<float>;
  effect->repeatEdgePixels = new Property<bool>;
  effect->blurDimensions = new Property<Enum>;
  effect->repeatEdgePixels->value = false;
  effect->blurDimensions->value = BlurDimensionsDirection::All;
  effect->blurriness->value = 70.0;
  auto filter = new GaussianBlurFilter(effect);
  if (!filter->initialize(cache->getContext())) {
    delete effect;
    delete filter;
    return nullptr;
  }
  for (auto& mask : masks) {
    auto path = mask->maskPath->getValueAt(layerFrame);
    if (path == nullptr || !path->isClosed() || mask->maskMode == MaskMode::None) {
      continue;
    }
    auto maskPath = ToPath(*path);
    auto expansion = mask->maskExpansion->getValueAt(layerFrame);
    ExpandPath(&maskPath, expansion);
    auto inverted = mask->inverted;
    if (isFirst) {
      if (mask->maskMode == MaskMode::Subtract) {
        inverted = !inverted;
      }
    }
    if (inverted) {
      maskPath.toggleInverseFillType();
    }
    auto maskBounds = maskPath.getBounds();
    auto width = static_cast<int>(ceilf(maskBounds.width() * scaleFactor));
    auto height = static_cast<int>(ceilf(maskBounds.height() * scaleFactor));
    auto maskSurface = tgfx::Surface::Make(cache->getContext(), width, height, true);
    auto maskCanvas = maskSurface->getCanvas();
    maskCanvas->setMatrix(tgfx::Matrix::MakeTrans(-maskBounds.x(), -maskBounds.y()));
    maskCanvas->drawPath(maskPath, paint);
    auto maskTexture = maskSurface->getTexture();
//    Trace(maskTexture.get());
    auto filterSource = ToFilterSource(maskTexture.get(), {1.0, 1.0});
    auto filterTarget = ToFilterTarget(surface.get(), tgfx::Matrix::MakeTrans(maskBounds.x(), maskBounds.y()));
    filter->update(layerFrame, maskBounds, bounds, {1.0, 1.0});
    filter->draw(cache->getContext(), filterSource.get(), filterTarget.get());
//    Trace(surface->getTexture().get());
  }
  
  auto texture = surface->getTexture();
  if (texture == nullptr) {
    return nullptr;
  }
  auto matrix = tgfx::Matrix::MakeScale(scaleFactor);
//  matrix.preTranslate(bounds.x(), bounds.y());
  auto drawingMatrix = tgfx::Matrix::I();
  matrix.invert(&drawingMatrix);
  
  delete effect;
  
  return std::make_unique<Snapshot>(texture, drawingMatrix);
}

void FeatherMask::draw(tgfx::Canvas* canvas, RenderCache* renderCache) const {
  tgfx::Paint paint;
  auto snapshot = renderCache->getSnapshot(assetID);
  if (!snapshot) {
    snapshot = DrawFeatherMask(masks, layerFrame, renderCache, 1.0).release();
  }
  canvas->drawTexture(snapshot->getTexture());
}

std::unique_ptr<Snapshot> FeatherMask::makeSnapshot(RenderCache* cache, float scaleFactor) const {
  if (masks.empty()) {
    return nullptr;
  }
  return DrawFeatherMask(masks, layerFrame, cache, scaleFactor);
}
}  // namespace pag
