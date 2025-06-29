#pragma once

#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>
#include <gst/gst.h>
#include <memory>
#include <napi.h>
#include <string>

class Element : public Napi::ObjectWrap<Element> {
public:
  static Napi::Object CreateFromGstElement(
    Napi::Env env,
    GstElement *element
  );

  Element(const Napi::CallbackInfo &info);
  virtual ~Element() = default;

  Napi::Value get_element_property(const Napi::CallbackInfo &info);
  Napi::Value set_element_property(const Napi::CallbackInfo &info);
  Napi::Value add_pad_probe(const Napi::CallbackInfo &info);
  Napi::Value set_pad(const Napi::CallbackInfo &info);
  Napi::Value get_pad(const Napi::CallbackInfo &info);

  Napi::Value get_sample(const Napi::CallbackInfo &info);
  Napi::Value on_sample(const Napi::CallbackInfo &info);

  Napi::Value push(const Napi::CallbackInfo &info);

private:
  std::unique_ptr<GstElement, decltype(&gst_object_unref)> element;
};