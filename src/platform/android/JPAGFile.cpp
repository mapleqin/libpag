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

#include "JNIHelper.h"
#include "JPAGImage.h"
#include "JPAGLayerHandle.h"
#include "NativePlatform.h"
#include "PAGText.h"

namespace pag {
static jfieldID PAGFile_nativeContext;
}

using namespace pag;

std::shared_ptr<pag::PAGFile> getPAGFile(JNIEnv* env, jobject thiz) {
  auto nativeContext =
      reinterpret_cast<JPAGLayerHandle*>(env->GetLongField(thiz, PAGFile_nativeContext));
  if (nativeContext == nullptr) {
    return nullptr;
  }
  return std::static_pointer_cast<PAGFile>(nativeContext->get());
}

extern "C" {

JNIEXPORT void Java_org_libpag_PAGFile_nativeInit(JNIEnv* env, jclass clazz) {
  PAGFile_nativeContext = env->GetFieldID(clazz, "nativeContext", "J");
  // 调用堆栈源头从C++触发而不是Java触发的情况下，FindClass
  // 可能会失败，因此要提前初始化这部分反射方法。
  NativePlatform::InitJNI(env);
}

JNIEXPORT jint Java_org_libpag_PAGFile_MaxSupportedTagLevel(JNIEnv*, jclass) {
  return pag::PAGFile::MaxSupportedTagLevel();
}

JNIEXPORT jobject Java_org_libpag_PAGFile_LoadFromPath(JNIEnv* env, jclass, jstring pathObj) {
  if (pathObj == nullptr) {
    LOGE("PAGFile.LoadFromPath() Invalid path specified.");
    return NULL;
  }
  auto path = SafeConvertToStdString(env, pathObj);
  if (path.empty()) {
    return NULL;
  }
  LOGI("PAGFile.LoadFromPath() start: %s", path.c_str());
  auto pagFile = PAGFile::Load(path);
  if (pagFile == nullptr) {
    LOGE("PAGFile.LoadFromPath() Invalid pag file : %s", path.c_str());
    return NULL;
  }
  return ToPAGLayerJavaObject(env, pagFile);
}

JNIEXPORT jobject Java_org_libpag_PAGFile_LoadFromBytes(JNIEnv* env, jclass, jbyteArray bytes,
                                                        jint length) {
  if (bytes == nullptr) {
    LOGE("PAGFile.LoadFromBytes() Invalid image bytes specified.");
    return NULL;
  }
  auto data = env->GetByteArrayElements(bytes, nullptr);
  auto pagFile = PAGFile::Load(data, static_cast<size_t>(length));
  env->ReleaseByteArrayElements(bytes, data, 0);
  if (pagFile == nullptr) {
    LOGE("PAGFile.LoadFromBytes() Invalid image bytes specified.");
    return NULL;
  }
  return ToPAGLayerJavaObject(env, pagFile);
}

JNIEXPORT jobject Java_org_libpag_PAGFile_LoadFromAssets(JNIEnv* env, jclass, jobject managerObj,
                                                         jstring pathObj) {
  auto path = SafeConvertToStdString(env, pathObj);
  auto byteData = ReadBytesFromAssets(env, managerObj, pathObj);
  if (byteData == nullptr) {
    LOGE("PAGFile.LoadFromAssets() Can't find the file name from asset manager : %s", path.c_str());
    return NULL;
  }
  LOGI("PAGFile.LoadFromAssets() start: %s", path.c_str());
  auto pagFile = PAGFile::Load(byteData->data(), byteData->length(), "assets://" + path);
  if (pagFile == nullptr) {
    LOGE("PAGFile.LoadFromAssets() Invalid pag file : %s", path.c_str());
    return NULL;
  }
  return ToPAGLayerJavaObject(env, pagFile);
}

JNIEXPORT jint Java_org_libpag_PAGFile_tagLevel(JNIEnv* env, jobject thiz) {
  auto pagFile = getPAGFile(env, thiz);
  if (pagFile == nullptr) {
    return 0;
  }
  return pagFile->tagLevel();
}

JNIEXPORT jint Java_org_libpag_PAGFile_numTexts(JNIEnv* env, jobject thiz) {
  auto pagFile = getPAGFile(env, thiz);
  if (pagFile == nullptr) {
    return 0;
  }
  return pagFile->numTexts();
}

JNIEXPORT jint Java_org_libpag_PAGFile_numImages(JNIEnv* env, jobject thiz) {
  auto pagFile = getPAGFile(env, thiz);
  if (pagFile == nullptr) {
    return 0;
  }
  return pagFile->numImages();
}

JNIEXPORT jint Java_org_libpag_PAGFile_numVideos(JNIEnv* env, jobject thiz) {
  auto pagFile = getPAGFile(env, thiz);
  if (pagFile == nullptr) {
    return 0;
  }
  return pagFile->numVideos();
}

JNIEXPORT jstring Java_org_libpag_PAGFile_path(JNIEnv* env, jobject thiz) {
  auto pagFile = getPAGFile(env, thiz);
  if (pagFile == nullptr) {
    return 0;
  }
  auto path = pagFile->path();
  return SafeConvertToJString(env, path.c_str());
}

JNIEXPORT jobject Java_org_libpag_PAGFile_getTextData(JNIEnv* env, jobject thiz, jint index) {
  auto pagFile = getPAGFile(env, thiz);
  if (pagFile == nullptr) {
    return nullptr;
  }
  auto textDocument = pagFile->getTextData(index);
  return ToPAGTextObject(env, textDocument);
}

JNIEXPORT void Java_org_libpag_PAGFile_replaceText(JNIEnv* env, jobject thiz, jint index,
                                                   jobject textData) {
  auto pagFile = getPAGFile(env, thiz);
  if (pagFile == nullptr) {
    return;
  }
  auto textDocument = ToTextDocument(env, textData);
  pagFile->replaceText(index, textDocument);
}

JNIEXPORT void Java_org_libpag_PAGFile_nativeReplaceImage(JNIEnv* env, jobject thiz, jint index,
                                                          jlong imageObject) {
  auto pagFile = getPAGFile(env, thiz);
  if (pagFile == nullptr) {
    return;
  }
  auto image = reinterpret_cast<JPAGImage*>(imageObject);
  if (image != nullptr) {
    pagFile->replaceImage(index, image->get());
  } else {
    pagFile->replaceImage(index, nullptr);
  }
}

JNIEXPORT jobjectArray Java_org_libpag_PAGFile_getLayersByEditableIndex(JNIEnv* env, jobject thiz,
                                                                        jint editableIndex,
                                                                        jint layerType) {
  auto pagFile = getPAGFile(env, thiz);
  if (pagFile == nullptr) {
    return ToPAGLayerJavaObjectList(env, {});
  }
  auto layers =
      pagFile->getLayersByEditableIndex(editableIndex, static_cast<pag::LayerType>(layerType));
  return ToPAGLayerJavaObjectList(env, layers);
}

JNIEXPORT jint Java_org_libpag_PAGFile_timeStretchMode(JNIEnv* env, jobject thiz) {
  auto pagFile = getPAGFile(env, thiz);
  if (pagFile == nullptr) {
    return 0;
  }
  return pagFile->timeStretchMode();
}

JNIEXPORT void Java_org_libpag_PAGFile_setTimeStretchMode(JNIEnv* env, jobject thiz, jint mode) {
  auto pagFile = getPAGFile(env, thiz);
  if (pagFile == nullptr) {
    return;
  }
  pagFile->setTimeStretchMode(static_cast<Enum>(mode));
}

JNIEXPORT void Java_org_libpag_PAGFile_setDuration(JNIEnv* env, jobject thiz, jlong duration) {
  auto pagFile = getPAGFile(env, thiz);
  if (pagFile == nullptr) {
    return;
  }
  pagFile->setDuration(duration);
}

JNIEXPORT jobject Java_org_libpag_PAGFile_copyOriginal(JNIEnv* env, jobject thiz) {
  auto pagFile = getPAGFile(env, thiz);
  if (pagFile == nullptr) {
    return NULL;
  }
  auto newFile = pagFile->copyOriginal();
  return ToPAGLayerJavaObject(env, newFile);
}
}