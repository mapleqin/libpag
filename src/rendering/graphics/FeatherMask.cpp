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
#include "tgfx/core/Mask.h"
#include "tgfx/gpu/Canvas.h"
#include "tgfx/gpu/Surface.h"

#define FEATHER_MASK_EXPEND 30

namespace pag {
std::shared_ptr<Graphic> FeatherMask::MakeFrom(ID assetID, const std::vector<MaskData*>& masks,
                                               Frame layerFrame) {
  if (masks.empty()) {
    return nullptr;
  }
  return std::shared_ptr<Graphic>(new FeatherMask(assetID, masks, layerFrame));
}

FeatherMask::FeatherMask(ID assetID, const std::vector<MaskData*>& masks, Frame layerFrame)
    : assetID(assetID), masks(masks), layerFrame(layerFrame) {
}

void FeatherMask::measureBounds(tgfx::Rect*) const {
}

bool FeatherMask::hitTest(RenderCache*, float, float) {
  return true;
}

bool FeatherMask::getPath(tgfx::Path*) const {
  return false;
}

void FeatherMask::prepare(RenderCache*) const {
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
  return tgfx::Rect::MakeLTRB(0, 0, maxRight, maxBottom);
}

std::unique_ptr<Snapshot> DrawFeatherMask(const std::vector<MaskData*>& masks, Frame layerFrame,
                                          RenderCache* cache, float scaleFactor) {
  bool isFirst = true;
  auto totalBounds = MeasureFeatherMaskBounds(masks, layerFrame);
  totalBounds.outset(FEATHER_MASK_EXPEND, FEATHER_MASK_EXPEND);
  auto surface = tgfx::Surface::Make(cache->getContext(),
                                     static_cast<int>(ceilf(totalBounds.width() * scaleFactor)),
                                     static_cast<int>(ceilf(totalBounds.height() * scaleFactor)));
  auto canvas = surface->getCanvas();
  auto totalMatrix = canvas->getMatrix();
  if (surface == nullptr) {
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
    auto bounds = maskPath.getBounds();
    auto width = static_cast<int>(ceilf(bounds.width() * scaleFactor));
    auto height = static_cast<int>(ceilf(bounds.height() * scaleFactor));
    
    if (isFirst) {
      isFirst = false;
    }
  }
  auto bounds = path.getBounds();
  auto width = static_cast<int>(ceilf(bounds.width() * scaleFactor));
  auto height = static_cast<int>(ceilf(bounds.height() * scaleFactor));
  auto mask = tgfx::Mask::Make(width, height);
  if (mask == nullptr) {
    return nullptr;
  }
  auto matrix = tgfx::Matrix::MakeScale(scaleFactor);
  matrix.preTranslate(-bounds.x(), -bounds.y());
  mask->setMatrix(matrix);
  mask->fillPath(path);
  auto drawingMatrix = tgfx::Matrix::I();
  matrix.invert(&drawingMatrix);
  auto texture = mask->makeTexture(cache->getContext());
  if (texture == nullptr) {
    return nullptr;
  }
  return std::make_unique<Snapshot>(texture, drawingMatrix);
  return nullptr;
}
  
void FeatherMask::draw(tgfx::Canvas* canvas, RenderCache* renderCache) const {
  tgfx::Paint paint;
  auto snapshot = renderCache->getSnapshot(assetID);
  if (!snapshot) {
    snapshot = DrawFeatherMask(masks, layerFrame, renderCache, 1.0).get();
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
