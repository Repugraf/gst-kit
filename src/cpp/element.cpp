#include "element.hpp"
#include "async-workers.hpp"
#include "type-conversion.hpp"
#include <chrono>
#include <cstring>
#include <gst/rtp/gstrtpbuffer.h>
#include <memory>
#include <string>
#include <thread>

Napi::Object Element::CreateFromGstElement(Napi::Env env, GstElement *element) {
  Napi::Function func = DefineClass(env, "Element", {});
  return func.New({Napi::External<GstElement>::New(env, element)});
}

Element::Element(const Napi::CallbackInfo &info) :
    Napi::ObjectWrap<Element>(info), element(nullptr, gst_object_unref) {
  std::string element_type = "element";
  if (info.Length() > 0 && info[0].IsExternal()) {
    GstElement *elem = info[0].As<Napi::External<GstElement>>().Data();
    element.reset(elem);

    // Set element type based on GStreamer element type
    if (GST_IS_APP_SINK(elem))
      element_type = "app-sink-element";
    else if (GST_IS_APP_SRC(elem))
      element_type = "app-src-element";
  }

  // Set properties as enumerable instance properties
  Napi::Env env = info.Env();
  Napi::Object thisObj = info.This().As<Napi::Object>();

  auto get_sample_method = Napi::Function::New(
    env, [this](const Napi::CallbackInfo &info) -> Napi::Value { return this->get_sample(info); },
    "getSample"
  );
  auto on_sample_method = Napi::Function::New(
    env, [this](const Napi::CallbackInfo &info) -> Napi::Value { return this->on_sample(info); },
    "onSample"
  );
  auto get_element_property_method = Napi::Function::New(
    env,
    [this](const Napi::CallbackInfo &info) -> Napi::Value {
      return this->get_element_property(info);
    },
    "getElementProperty"
  );
  auto set_element_property_method = Napi::Function::New(
    env,
    [this](const Napi::CallbackInfo &info) -> Napi::Value {
      return this->set_element_property(info);
    },
    "setElementProperty"
  );
  auto add_pad_probe_method = Napi::Function::New(
    env,
    [this](const Napi::CallbackInfo &info) -> Napi::Value { return this->add_pad_probe(info); },
    "addPadProbe"
  );
  auto set_pad_method = Napi::Function::New(
    env, [this](const Napi::CallbackInfo &info) -> Napi::Value { return this->set_pad(info); }, "setPad"
  );
  auto get_pad_method = Napi::Function::New(
    env, [this](const Napi::CallbackInfo &info) -> Napi::Value { return this->get_pad(info); }, "getPad"
  );
  auto push_method = Napi::Function::New(
    env, [this](const Napi::CallbackInfo &info) -> Napi::Value { return this->push(info); }, "push"
  );

  std::vector property_descriptors = {
    Napi::PropertyDescriptor::Value("type", Napi::String::New(env, element_type), napi_enumerable),
    Napi::PropertyDescriptor::Value(
      "getElementProperty", get_element_property_method, napi_enumerable
    ),
    Napi::PropertyDescriptor::Value(
      "setElementProperty", set_element_property_method, napi_enumerable
    ),
    Napi::PropertyDescriptor::Value("addPadProbe", add_pad_probe_method, napi_enumerable),
    Napi::PropertyDescriptor::Value("setPad", set_pad_method, napi_enumerable),
    Napi::PropertyDescriptor::Value("getPad", get_pad_method, napi_enumerable)
  };

  if (element || GST_IS_APP_SINK(element.get())) {
    property_descriptors.push_back(
      Napi::PropertyDescriptor::Value("getSample", get_sample_method, napi_enumerable)
    );
    property_descriptors.push_back(
      Napi::PropertyDescriptor::Value("onSample", on_sample_method, napi_enumerable)
    );
  }

  if (element && GST_IS_APP_SRC(element.get())) {
    property_descriptors.push_back(
      Napi::PropertyDescriptor::Value("push", push_method, napi_enumerable)
    );
  }

  thisObj.DefineProperties(property_descriptors);
}

Napi::Value Element::get_element_property(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() < 1 || !info[0].IsString()) {
    Napi::TypeError::New(env, "Property name must be a string").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  if (!element.get()) {
    Napi::TypeError::New(env, "Element is null or not initialized").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  std::string property_name = info[0].As<Napi::String>().Utf8Value();

  GParamSpec *spec =
    g_object_class_find_property(G_OBJECT_GET_CLASS(element.get()), property_name.c_str());

  if (!spec) {
    return env.Null();
  }

  GValue value = G_VALUE_INIT;
  g_value_init(&value, G_PARAM_SPEC_VALUE_TYPE(spec));
  g_object_get_property(G_OBJECT(element.get()), property_name.c_str(), &value);

  Napi::Value result = TypeConversion::gvalue_to_js_with_type(env, &value);

  g_value_unset(&value);
  return result;
}

Napi::Value Element::set_element_property(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() < 2) {
    Napi::TypeError::New(env, "setElementProperty requires property name and value")
      .ThrowAsJavaScriptException();
    return env.Undefined();
  }

  if (!info[0].IsString()) {
    Napi::TypeError::New(env, "Property name must be a string").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  if (!element.get()) {
    Napi::TypeError::New(env, "Element is null or not initialized").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  std::string property_name = info[0].As<Napi::String>().Utf8Value();
  Napi::Value property_value = info[1];

  GParamSpec *spec =
    g_object_class_find_property(G_OBJECT_GET_CLASS(element.get()), property_name.c_str());

  if (!spec) {
    Napi::TypeError::New(env, ("Property '" + property_name + "' not found").c_str())
      .ThrowAsJavaScriptException();
    return env.Undefined();
  }

  // Check if property is writable
  if (!(spec->flags & G_PARAM_WRITABLE)) {
    Napi::TypeError::New(env, ("Property '" + property_name + "' is not writable").c_str())
      .ThrowAsJavaScriptException();
    return env.Undefined();
  }

  GValue value = G_VALUE_INIT;
  GType prop_type = G_PARAM_SPEC_VALUE_TYPE(spec);

  // Convert JavaScript value to GValue
  if (!TypeConversion::js_to_gvalue(env, property_value, prop_type, &value)) {
    std::string error_msg = TypeConversion::get_conversion_error_message(prop_type, property_value);

    // Handle special error cases
    if (prop_type == gst_caps_get_type() && property_value.IsString()) {
      error_msg = "Invalid caps string";
    } else if (G_TYPE_IS_ENUM(prop_type) && property_value.IsString()) {
      std::string enum_str = property_value.As<Napi::String>().Utf8Value();
      error_msg = "Invalid enum value: " + enum_str;
    }

    Napi::TypeError::New(env, error_msg.c_str()).ThrowAsJavaScriptException();
    return env.Undefined();
  }

  // Set the property
  g_object_set_property(G_OBJECT(element.get()), property_name.c_str(), &value);
  g_value_unset(&value);

  return env.Undefined();
}

Napi::Value Element::get_sample(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  // Validate that we have an app sink element
  if (!element || !GST_IS_APP_SINK(element.get())) {
    Napi::TypeError::New(env, "getSample() can only be called on app-sink-element")
      .ThrowAsJavaScriptException();
    return env.Undefined();
  }

  // Default timeout is 1000ms (1 second)
  guint64 timeout_ms = 1000;

  // Check if timeout parameter is provided
  if (info.Length() > 0 && info[0].IsNumber()) {
    timeout_ms = info[0].As<Napi::Number>().Uint32Value();
  }

  // Create worker and get its promise
  // Note: N-API AsyncWorker manages its own memory - it will be automatically
  // deleted when the work completes (OnOK or OnError is called)
  PullSampleWorker *worker = new PullSampleWorker(env, GST_APP_SINK(element.get()), timeout_ms);
  Napi::Promise promise = worker->GetPromise().Promise();
  worker->Queue();

  return promise;
}

// Structure to hold sample callback data
struct SampleCallbackContext {
  Napi::ThreadSafeFunction callback;
  gulong signal_id;
  GstAppSink *app_sink;
};

// Signal callback for new-sample
static void new_sample_callback(GstAppSink *app_sink, gpointer user_data) {
  SampleCallbackContext *context = static_cast<SampleCallbackContext *>(user_data);

  // Try to pull the sample
  GstSample *sample = gst_app_sink_try_pull_sample(app_sink, 0); // Non-blocking

  if (sample) {
    // Call the JavaScript callback with the sample
    context->callback.NonBlockingCall([=](Napi::Env env, Napi::Function js_callback) {
      Napi::Object sampleData = TypeConversion::gst_sample_to_js(env, sample);
      js_callback.Call({sampleData});

      // Unref the sample when done
      gst_sample_unref(sample);
    });
  }
}

Napi::Value Element::on_sample(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  // Validate that we have an app sink element
  if (!element || !GST_IS_APP_SINK(element.get())) {
    Napi::TypeError::New(env, "onSample() can only be called on app-sink-element")
      .ThrowAsJavaScriptException();
    return env.Undefined();
  }

  if (info.Length() < 1 || !info[0].IsFunction()) {
    Napi::TypeError::New(env, "Expected 1 argument: callback function")
      .ThrowAsJavaScriptException();
    return env.Undefined();
  }

  Napi::Function callback = info[0].As<Napi::Function>();

  // Enable signal emission on the appsink
  g_object_set(element.get(), "emit-signals", TRUE, NULL);

  // Create a thread-safe function for the callback
  Napi::ThreadSafeFunction tsfn =
    Napi::ThreadSafeFunction::New(env, callback, "SampleCallback", 0, 1);

  // Create context for the signal
  SampleCallbackContext *context = new SampleCallbackContext{tsfn, 0, GST_APP_SINK(element.get())};

  // Connect to the "new-sample" signal
  context->signal_id =
    g_signal_connect(element.get(), "new-sample", G_CALLBACK(new_sample_callback), context);

  // Return an unsubscribe function
  return Napi::Function::New(env, [context](const Napi::CallbackInfo &info) -> Napi::Value {
    // Disconnect the signal (this will stop the callbacks)
    g_signal_handler_disconnect(context->app_sink, context->signal_id);

    // Clean up the thread-safe function
    context->callback.Release();
    delete context;

    return info.Env().Undefined();
  });
}

// Structure to hold probe data
struct PadProbeContext {
  Napi::ThreadSafeFunction callback;
  gulong probe_id;
  GstPad *pad;
  GstElement *element;
};

// Pad probe callback for comprehensive buffer data extraction
static GstPadProbeReturn
pad_probe_callback(GstPad *pad, GstPadProbeInfo *info, gpointer user_data) {
  PadProbeContext *context = static_cast<PadProbeContext *>(user_data);

  if (GST_PAD_PROBE_INFO_TYPE(info) & GST_PAD_PROBE_TYPE_BUFFER) {
    GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER(info);

    if (buffer) {
      // Copy buffer data immediately to avoid lifetime issues
      GstMapInfo map;
      uint8_t *buffer_data_copy = nullptr;
      gsize buffer_data_size = 0;
      if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        buffer_data_copy = new uint8_t[map.size];
        std::memcpy(buffer_data_copy, map.data, map.size);
        buffer_data_size = map.size;
        gst_buffer_unmap(buffer, &map);
      }

      // Copy buffer metadata immediately
      guint64 pts = GST_BUFFER_PTS_IS_VALID(buffer) ? GST_BUFFER_PTS(buffer) : GST_CLOCK_TIME_NONE;
      guint64 dts = GST_BUFFER_DTS_IS_VALID(buffer) ? GST_BUFFER_DTS(buffer) : GST_CLOCK_TIME_NONE;
      guint64 duration =
        GST_BUFFER_DURATION_IS_VALID(buffer) ? GST_BUFFER_DURATION(buffer) : GST_CLOCK_TIME_NONE;
      guint64 offset =
        GST_BUFFER_OFFSET_IS_VALID(buffer) ? GST_BUFFER_OFFSET(buffer) : GST_BUFFER_OFFSET_NONE;
      guint64 offset_end = GST_BUFFER_OFFSET_END_IS_VALID(buffer) ? GST_BUFFER_OFFSET_END(buffer)
                                                                  : GST_BUFFER_OFFSET_NONE;
      guint32 flags = GST_BUFFER_FLAGS(buffer);

      // Copy caps data immediately
      GstCaps *caps = gst_pad_get_current_caps(pad);
      gchar *caps_string = caps ? gst_caps_to_string(caps) : nullptr;
      if (caps) {
        gst_caps_unref(caps);
      }

      // Copy RTP data if available
      bool has_rtp = false;
      guint32 rtp_timestamp = 0;
      guint16 rtp_sequence = 0;
      guint32 rtp_ssrc = 0;
      guint8 rtp_payload_type = 0;

      GstRTPBuffer rtp_buffer = GST_RTP_BUFFER_INIT;
      if (gst_rtp_buffer_map(buffer, GST_MAP_READ, &rtp_buffer)) {
        has_rtp = true;
        rtp_timestamp = gst_rtp_buffer_get_timestamp(&rtp_buffer);
        rtp_sequence = gst_rtp_buffer_get_seq(&rtp_buffer);
        rtp_ssrc = gst_rtp_buffer_get_ssrc(&rtp_buffer);
        rtp_payload_type = gst_rtp_buffer_get_payload_type(&rtp_buffer);
        gst_rtp_buffer_unmap(&rtp_buffer);
      }

      // Call the JavaScript callback with copied data
      context->callback.NonBlockingCall([=](Napi::Env env, Napi::Function js_callback) {
        Napi::Object buffer_data = Napi::Object::New(env);

        // Raw buffer data
        if (buffer_data_copy) {
          buffer_data.Set(
            "buffer", Napi::Buffer<uint8_t>::Copy(env, buffer_data_copy, buffer_data_size)
          );
          delete[] buffer_data_copy; // Clean up copied data
        }

        // Timing information
        if (pts != GST_CLOCK_TIME_NONE) {
          buffer_data.Set("pts", Napi::Number::New(env, static_cast<double>(pts)));
        }
        if (dts != GST_CLOCK_TIME_NONE) {
          buffer_data.Set("dts", Napi::Number::New(env, static_cast<double>(dts)));
        }
        if (duration != GST_CLOCK_TIME_NONE) {
          buffer_data.Set("duration", Napi::Number::New(env, static_cast<double>(duration)));
        }
        if (offset != GST_BUFFER_OFFSET_NONE) {
          buffer_data.Set("offset", Napi::Number::New(env, static_cast<double>(offset)));
        }
        if (offset_end != GST_BUFFER_OFFSET_NONE) {
          buffer_data.Set("offsetEnd", Napi::Number::New(env, static_cast<double>(offset_end)));
        }

        // Buffer flags
        buffer_data.Set("flags", Napi::Number::New(env, flags));

        // Caps information
        if (caps_string) {
          Napi::Object caps_obj = Napi::Object::New(env);
          caps_obj.Set("name", Napi::String::New(env, caps_string));
          buffer_data.Set("caps", caps_obj);
          g_free(caps_string); // Clean up caps string
        }

        // RTP data if available
        if (has_rtp) {
          Napi::Object rtpData = Napi::Object::New(env);
          rtpData.Set("timestamp", Napi::Number::New(env, rtp_timestamp));
          rtpData.Set("sequence", Napi::Number::New(env, rtp_sequence));
          rtpData.Set("ssrc", Napi::Number::New(env, rtp_ssrc));
          rtpData.Set("payloadType", Napi::Number::New(env, rtp_payload_type));

          buffer_data.Set("rtp", rtpData);
        }

        js_callback.Call({buffer_data});
      });
    }
  }

  return GST_PAD_PROBE_OK;
}

Napi::Value Element::add_pad_probe(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  if (info.Length() < 2) {
    Napi::TypeError::New(env, "Expected 2 arguments: padName and callback")
      .ThrowAsJavaScriptException();
    return env.Null();
  }

  if (!info[0].IsString()) {
    Napi::TypeError::New(env, "First argument must be a string (pad name)")
      .ThrowAsJavaScriptException();
    return env.Null();
  }

  if (!info[1].IsFunction()) {
    Napi::TypeError::New(env, "Second argument must be a function (callback)")
      .ThrowAsJavaScriptException();
    return env.Null();
  }

  std::string pad_name = info[0].As<Napi::String>().Utf8Value();
  Napi::Function callback = info[1].As<Napi::Function>();

  // Get the pad from the element
  GstPad *pad = gst_element_get_static_pad(element.get(), pad_name.c_str());
  if (!pad) {
    Napi::Error::New(env, "Failed to get pad: " + pad_name).ThrowAsJavaScriptException();
    return env.Null();
  }

  // Create a thread-safe function for the callback
  Napi::ThreadSafeFunction tsfn =
    Napi::ThreadSafeFunction::New(env, callback, "PadProbeCallback", 0, 1);

  // Create context for the probe
  PadProbeContext *context = new PadProbeContext{tsfn, 0, pad, element.get()};

  // Add the probe
  context->probe_id = gst_pad_add_probe(
    pad, GST_PAD_PROBE_TYPE_BUFFER, pad_probe_callback, context, [](gpointer data) {
      // Cleanup callback
      PadProbeContext *context = static_cast<PadProbeContext *>(data);
      context->callback.Release();
      gst_object_unref(context->pad);
      delete context;
    }
  );

  // Return an unsubscribe function
  return Napi::Function::New(env, [context](const Napi::CallbackInfo &info) -> Napi::Value {
    // Remove the probe (this will trigger the destructor callback which cleans up)
    gst_pad_remove_probe(context->pad, context->probe_id);

    return info.Env().Undefined();
  });
}

Napi::Value Element::push(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  // Validate that we have an app source element
  if (!element || !GST_IS_APP_SRC(element.get())) {
    Napi::TypeError::New(env, "push() can only be called on app-src-element")
      .ThrowAsJavaScriptException();
    return env.Undefined();
  }

  if (info.Length() < 1) {
    Napi::TypeError::New(env, "push() requires at least 1 argument: buffer")
      .ThrowAsJavaScriptException();
    return env.Undefined();
  }

  if (!info[0].IsBuffer()) {
    Napi::TypeError::New(env, "First argument must be a Buffer").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  // Get buffer data from Node.js Buffer
  Napi::Buffer<uint8_t> node_buffer = info[0].As<Napi::Buffer<uint8_t>>();
  uint8_t *buffer_data = node_buffer.Data();
  size_t buffer_length = node_buffer.Length();

  // Create GStreamer buffer
  GstBuffer *gst_buffer = gst_buffer_new_allocate(nullptr, buffer_length, nullptr);
  if (!gst_buffer) {
    Napi::Error::New(env, "Failed to allocate GStreamer buffer").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  // Fill the buffer with data
  gst_buffer_fill(gst_buffer, 0, buffer_data, buffer_length);

  // Handle optional PTS (presentation timestamp) parameter
  if (info.Length() > 1) {
    if (info[1].IsBuffer()) {
      // Handle PTS as buffer (compatible with original NAN implementation)
      Napi::Buffer<uint8_t> pts_buffer = info[1].As<Napi::Buffer<uint8_t>>();
      if (pts_buffer.Length() >= 8) {
        uint8_t *pts_data = pts_buffer.Data();
        // Read as big-endian uint64
        guint64 pts = GST_READ_UINT64_BE(pts_data);
        GST_BUFFER_PTS(gst_buffer) = pts;
      }
    } else if (info[1].IsNumber()) {
      // Handle PTS as number (more convenient JavaScript API)
      guint64 pts = static_cast<guint64>(info[1].As<Napi::Number>().Int64Value());
      GST_BUFFER_PTS(gst_buffer) = pts;
    }
  }

  // Push buffer to app source
  GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(element.get()), gst_buffer);

  // Check for errors
  if (ret != GST_FLOW_OK) {
    // Buffer is consumed by push_buffer even on error, so don't unref it
    std::string error_msg = "Failed to push buffer: ";
    switch (ret) {
      case GST_FLOW_FLUSHING:
        error_msg += "Element is flushing";
        break;
      case GST_FLOW_EOS:
        error_msg += "End of stream";
        break;
      case GST_FLOW_NOT_LINKED:
        error_msg += "Source pad not linked";
        break;
      case GST_FLOW_ERROR:
        error_msg += "Generic error";
        break;
      default:
        error_msg += "Unknown error (" + std::to_string(ret) + ")";
        break;
    }
    Napi::Error::New(env, error_msg).ThrowAsJavaScriptException();
    return env.Undefined();
  }

  return env.Undefined();
}

Napi::Value Element::set_pad(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  
  if (info.Length() < 2 || !info[0].IsString() || !info[1].IsString()) {
    Napi::TypeError::New(env, "setPad() requires two string arguments (attribute, padName)")
      .ThrowAsJavaScriptException();
    return env.Undefined();
  }

  if (!element.get()) {
    Napi::Error::New(env, "Element is null or not initialized").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  std::string attribute = info[0].As<Napi::String>().Utf8Value();
  std::string padName = info[1].As<Napi::String>().Utf8Value();

  GstPad *pad = gst_element_get_static_pad(element.get(), padName.c_str());
  if (!pad) {
    Napi::Error::New(env, "Pad not found: " + padName).ThrowAsJavaScriptException();
    return env.Undefined();
  }

  g_object_set(G_OBJECT(element.get()), attribute.c_str(), pad, NULL);
  
  gst_object_unref(pad);
  
  return env.Undefined();
}

Napi::Value Element::get_pad(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  
  if (info.Length() < 1 || !info[0].IsString()) {
    Napi::TypeError::New(env, "getPad() requires one string argument (padName)")
      .ThrowAsJavaScriptException();
    return env.Null();
  }

  if (!element.get()) {
    return env.Null();
  }

  std::string padName = info[0].As<Napi::String>().Utf8Value();

  GstPad *pad = gst_element_get_static_pad(element.get(), padName.c_str());
  
  if (!pad) {
    return env.Null();
  }

  // Create an object with pad information
  Napi::Object padObj = Napi::Object::New(env);
  padObj.Set("name", Napi::String::New(env, GST_PAD_NAME(pad)));
  padObj.Set("direction", Napi::Number::New(env, GST_PAD_DIRECTION(pad)));
  
  // Get pad caps if available
  GstCaps *caps = gst_pad_get_current_caps(pad);
  if (caps) {
    gchar *caps_str = gst_caps_to_string(caps);
    padObj.Set("caps", Napi::String::New(env, caps_str));
    g_free(caps_str);
    gst_caps_unref(caps);
  } else {
    padObj.Set("caps", env.Null());
  }
  
  gst_object_unref(pad);
  
  return padObj;
}
