#include "type-conversion.hpp"
#include <gst/gst.h>

namespace TypeConversion {
  bool js_to_gvalue(
    Napi::Env env, const Napi::Value &js_value, GType target_type, GValue *out_value
  ) {
    g_value_init(out_value, target_type);

    switch (target_type) {
      case G_TYPE_STRING: {
        if (!js_value.IsString()) {
          g_value_unset(out_value);
          return false;
        }
        std::string str_value = js_value.As<Napi::String>().Utf8Value();
        g_value_set_string(out_value, str_value.c_str());
        return true;
      }
      case G_TYPE_BOOLEAN: {
        if (!js_value.IsBoolean()) {
          g_value_unset(out_value);
          return false;
        }
        g_value_set_boolean(out_value, js_value.As<Napi::Boolean>().Value());
        return true;
      }
      case G_TYPE_INT: {
        if (!js_value.IsNumber()) {
          g_value_unset(out_value);
          return false;
        }
        g_value_set_int(out_value, js_value.As<Napi::Number>().Int32Value());
        return true;
      }
      case G_TYPE_UINT: {
        if (!js_value.IsNumber()) {
          g_value_unset(out_value);
          return false;
        }
        g_value_set_uint(out_value, js_value.As<Napi::Number>().Uint32Value());
        return true;
      }
      case G_TYPE_FLOAT: {
        if (!js_value.IsNumber()) {
          g_value_unset(out_value);
          return false;
        }
        g_value_set_float(out_value, js_value.As<Napi::Number>().FloatValue());
        return true;
      }
      case G_TYPE_DOUBLE: {
        if (!js_value.IsNumber()) {
          g_value_unset(out_value);
          return false;
        }
        g_value_set_double(out_value, js_value.As<Napi::Number>().DoubleValue());
        return true;
      }
      case G_TYPE_UINT64: {
        if (js_value.IsNumber()) {
          g_value_set_uint64(out_value, js_value.As<Napi::Number>().Uint32Value());
          return true;
        } else if (js_value.IsBigInt()) {
          bool lossless;
          g_value_set_uint64(out_value, js_value.As<Napi::BigInt>().Uint64Value(&lossless));
          return true;
        } else {
          g_value_unset(out_value);
          return false;
        }
      }
      default: {
        // Handle special GStreamer types
        if (target_type == gst_caps_get_type()) {
          // Handle GstCaps - expect string that can be parsed into caps
          if (!js_value.IsString()) {
            g_value_unset(out_value);
            return false;
          }
          std::string caps_str = js_value.As<Napi::String>().Utf8Value();
          GstCaps *caps = gst_caps_from_string(caps_str.c_str());
          if (!caps) {
            g_value_unset(out_value);
            return false;
          }
          g_value_set_boxed(out_value, caps);
          gst_caps_unref(caps);
          return true;
        } else if (G_TYPE_IS_ENUM(target_type)) {
          // Handle enum types
          if (js_value.IsString()) {
            std::string enum_str = js_value.As<Napi::String>().Utf8Value();
            GEnumClass *enum_class = G_ENUM_CLASS(g_type_class_ref(target_type));
            GEnumValue *enum_value = g_enum_get_value_by_nick(enum_class, enum_str.c_str());
            if (!enum_value) {
              enum_value = g_enum_get_value_by_name(enum_class, enum_str.c_str());
            }
            if (enum_value) {
              g_value_set_enum(out_value, enum_value->value);
              g_type_class_unref(enum_class);
              return true;
            } else {
              g_type_class_unref(enum_class);
              g_value_unset(out_value);
              return false;
            }
          } else if (js_value.IsNumber()) {
            g_value_set_enum(out_value, js_value.As<Napi::Number>().Int32Value());
            return true;
          } else {
            g_value_unset(out_value);
            return false;
          }
        } else if (G_TYPE_IS_FLAGS(target_type)) {
          // Handle flags types
          if (js_value.IsString()) {
            std::string flags_str = js_value.As<Napi::String>().Utf8Value();
            GFlagsClass *flags_class = G_FLAGS_CLASS(g_type_class_ref(target_type));
            guint flags_value = 0;
            
            // Try to parse the string as flag names
            gchar **flag_names = g_strsplit(flags_str.c_str(), "+", -1);
            gboolean success = TRUE;
            
            for (gint i = 0; flag_names[i] != NULL; i++) {
              g_strstrip(flag_names[i]); // Remove whitespace
              GFlagsValue *flag_val = g_flags_get_value_by_nick(flags_class, flag_names[i]);
              if (!flag_val) {
                flag_val = g_flags_get_value_by_name(flags_class, flag_names[i]);
              }
              if (flag_val) {
                flags_value |= flag_val->value;
              } else {
                success = FALSE;
                break;
              }
            }
            
            g_strfreev(flag_names);
            g_type_class_unref(flags_class);
            
            if (success) {
              g_value_set_flags(out_value, flags_value);
              return true;
            } else {
              g_value_unset(out_value);
              return false;
            }
          } else if (js_value.IsNumber()) {
            g_value_set_flags(out_value, js_value.As<Napi::Number>().Uint32Value());
            return true;
          } else {
            g_value_unset(out_value);
            return false;
          }
        } else {
          // For other types, try to convert from string if possible
          if (js_value.IsString() && g_value_type_transformable(G_TYPE_STRING, target_type)) {
            GValue string_value = G_VALUE_INIT;
            g_value_init(&string_value, G_TYPE_STRING);
            std::string str_value = js_value.As<Napi::String>().Utf8Value();
            g_value_set_string(&string_value, str_value.c_str());

            if (g_value_transform(&string_value, out_value)) {
              g_value_unset(&string_value);
              return true;
            } else {
              g_value_unset(&string_value);
              g_value_unset(out_value);
              return false;
            }
          } else {
            g_value_unset(out_value);
            return false;
          }
        }
      }
    }
  };

  Napi::Value gvalue_to_js(Napi::Env env, const GValue *gvalue) {
    auto g_value_type = G_VALUE_TYPE(gvalue);
    if (g_value_type == G_TYPE_STRING) {
      return Napi::String::New(env, g_value_get_string(gvalue));
    } else if (g_value_type == G_TYPE_BOOLEAN) {
      return Napi::Boolean::New(env, g_value_get_boolean(gvalue));
    } else if (g_value_type == G_TYPE_INT) {
      return Napi::Number::New(env, g_value_get_int(gvalue));
    } else if (g_value_type == G_TYPE_UINT) {
      return Napi::Number::New(env, g_value_get_uint(gvalue));
    } else if (g_value_type == G_TYPE_FLOAT) {
      return Napi::Number::New(env, g_value_get_float(gvalue));
    } else if (g_value_type == G_TYPE_DOUBLE) {
      return Napi::Number::New(env, g_value_get_double(gvalue));
    } else if (g_value_type == G_TYPE_UINT64) {
      return Napi::BigInt::New(env, g_value_get_uint64(gvalue));
    } else if (GST_VALUE_HOLDS_ARRAY(gvalue)) {
      int size = gst_value_array_get_size(gvalue);
      Napi::Array array = Napi::Array::New(env, size);

      for (int i = 0; i < size; i++) {
        const GValue *element = gst_value_array_get_value(gvalue, i);
        array.Set(i, gvalue_to_js(env, element));
      }
      return array;
    } else if (GST_VALUE_HOLDS_BUFFER(gvalue)) {
      GstBuffer *buf = gst_value_get_buffer(gvalue);
      if (!buf) {
        return env.Null();
      }

      GstMapInfo map;
      if (gst_buffer_map(buf, &map, GST_MAP_READ)) {
        Napi::Buffer<uint8_t> buffer = Napi::Buffer<uint8_t>::Copy(env, map.data, map.size);
        gst_buffer_unmap(buf, &map);
        return buffer;
      }
      return env.Null();
    } else if (GST_VALUE_HOLDS_SAMPLE(gvalue)) {
      GstSample *sample = gst_value_get_sample(gvalue);
      return sample ? gst_sample_to_js(env, sample) : env.Null();
    } else if (G_TYPE_IS_ENUM(G_VALUE_TYPE(gvalue))) {
      gint enum_val = g_value_get_enum(gvalue);
      GEnumClass *enum_class = G_ENUM_CLASS(g_type_class_ref(G_VALUE_TYPE(gvalue)));
      GEnumValue *enum_value = g_enum_get_value(enum_class, enum_val);
      if (enum_value && enum_value->value_nick) {
        Napi::Value result = Napi::String::New(env, enum_value->value_nick);
        g_type_class_unref(enum_class);
        return result;
      } else if (enum_value && enum_value->value_name) {
        Napi::Value result = Napi::String::New(env, enum_value->value_name);
        g_type_class_unref(enum_class);
        return result;
      } else {
        g_type_class_unref(enum_class);
        return Napi::Number::New(env, enum_val);
      }
    } else if (G_TYPE_IS_FLAGS(G_VALUE_TYPE(gvalue))) {
      guint flags_val = g_value_get_flags(gvalue);
      GFlagsClass *flags_class = G_FLAGS_CLASS(g_type_class_ref(G_VALUE_TYPE(gvalue)));
      
      // Manual flags to string conversion
      std::string flags_str = "";
      if (flags_val == 0) {
        flags_str = "none";
      } else {
        for (guint i = 0; i < flags_class->n_values; i++) {
          GFlagsValue *flag_val = &flags_class->values[i];
          if (flag_val->value != 0 && (flags_val & flag_val->value) == flag_val->value) {
            if (!flags_str.empty()) {
              flags_str += "+";
            }
            if (flag_val->value_nick) {
              flags_str += flag_val->value_nick;
            } else if (flag_val->value_name) {
              flags_str += flag_val->value_name;
            } else {
              flags_str += std::to_string(flag_val->value);
            }
          }
        }
      }
      
      if (!flags_str.empty()) {
        Napi::Value result = Napi::String::New(env, flags_str);
        g_type_class_unref(flags_class);
        return result;
      } else {
        // Fall back to numeric value
        g_type_class_unref(flags_class);
        return Napi::Number::New(env, flags_val);
      }
    }

    // Handle GstFraction values
    else if (G_VALUE_HOLDS(gvalue, GST_TYPE_FRACTION)) {
      gint numerator = gst_value_get_fraction_numerator(gvalue);
      gint denominator = gst_value_get_fraction_denominator(gvalue);
      std::string fraction_str = std::to_string(numerator) + "/" + std::to_string(denominator);
      return Napi::String::New(env, fraction_str);
    }

    // Handle GstCaps and other complex types by converting to string
    else if (G_VALUE_HOLDS_BOXED(gvalue)) {
      gpointer boxed_value = g_value_get_boxed(gvalue);
      if (boxed_value && GST_IS_CAPS(boxed_value)) {
        GstCaps *caps = GST_CAPS(boxed_value);
        gchar *caps_str = gst_caps_to_string(caps);
        if (caps_str) {
          Napi::Value result = Napi::String::New(env, caps_str);
          g_free(caps_str);
          return result;
        }
      } else if (boxed_value && GST_IS_STRUCTURE(boxed_value)) {
        // Handle GstStructure (like stats property)
        const GstStructure *structure = GST_STRUCTURE(boxed_value);
        return gst_structure_to_js(env, structure);
      }
    }

    std::string type_name = g_type_name(G_VALUE_TYPE(gvalue));
    std::string error_msg = "Cannot convert GValue of type '" + type_name + "' to JavaScript value";
    Napi::TypeError::New(env, error_msg.c_str()).ThrowAsJavaScriptException();

    return env.Undefined();
  };

  Napi::Value gvalue_to_js_with_type(Napi::Env env, const GValue *gvalue) {
    Napi::Value js_value = gvalue_to_js(env, gvalue);

    if (js_value.IsNull()) {
      return env.Null();
    }
    
    // Create the standardized result object
    Napi::Object result = Napi::Object::New(env);
    
    // Determine the type and set both type and value
    if (js_value.IsString() || js_value.IsNumber() || js_value.IsBoolean()) {
      result.Set("type", Napi::String::New(env, "primitive"));
    } else if (js_value.IsBigInt()) {
      result.Set("type", Napi::String::New(env, "bigint"));
    } else if (js_value.IsArray()) {
      result.Set("type", Napi::String::New(env, "array"));
    } else if (js_value.IsBuffer()) {
      result.Set("type", Napi::String::New(env, "buffer"));
    } else if (js_value.IsObject()) {
      // Check if it's a GStreamer sample by looking for specific properties
      Napi::Object obj = js_value.As<Napi::Object>();
      if (obj.Has("buffer") && obj.Has("caps") && obj.Has("flags")) {
        result.Set("type", Napi::String::New(env, "sample"));
      } else {
        result.Set("type", Napi::String::New(env, "object"));
      }
    } else {
      // Default to primitive for unknown types
      result.Set("type", Napi::String::New(env, "primitive"));
    }
    
    result.Set("value", js_value);
    return result;
  }

  std::string get_conversion_error_message(GType target_type, const Napi::Value &js_value) {
    std::string js_type;
    if (js_value.IsString())
      js_type = "string";
    else if (js_value.IsNumber())
      js_type = "number";
    else if (js_value.IsBoolean())
      js_type = "boolean";
    else if (js_value.IsNull())
      js_type = "null";
    else if (js_value.IsUndefined())
      js_type = "undefined";
    else if (js_value.IsObject())
      js_type = "object";
    else
      js_type = "unknown";

    switch (target_type) {
      case G_TYPE_STRING:
        return "Expected string value, got " + js_type;
      case G_TYPE_BOOLEAN:
        return "Expected boolean value, got " + js_type;
      case G_TYPE_INT:
      case G_TYPE_UINT:
      case G_TYPE_FLOAT:
      case G_TYPE_DOUBLE:
        return "Expected number value, got " + js_type;
      default:
        if (target_type == gst_caps_get_type()) {
          return "Expected string value for caps property, got " + js_type;
        } else if (G_TYPE_IS_ENUM(target_type)) {
          return "Expected string or number value for enum property, got " + js_type;
        } else if (G_TYPE_IS_FLAGS(target_type)) {
          return "Expected string or number value for flags property, got " + js_type;
        } else {
          return "Cannot convert " + js_type + " to " + std::string(g_type_name(target_type));
        }
    }
  }

  Napi::Object gst_sample_to_js(Napi::Env env, GstSample *sample) {
    if (!sample) {
      Napi::TypeError::New(env, "Sample is null").ThrowAsJavaScriptException();
      return Napi::Object::New(env);
    }

    Napi::Object result = Napi::Object::New(env);

    // Add buffer from sample
    GstBuffer *buf = gst_sample_get_buffer(sample);
    if (buf) {
      GstMapInfo map;
      if (gst_buffer_map(buf, &map, GST_MAP_READ)) {
        Napi::Buffer<uint8_t> buffer = Napi::Buffer<uint8_t>::Copy(env, map.data, map.size);
        result.Set("buffer", buffer);
        gst_buffer_unmap(buf, &map);
      }

      // Add flags from buffer
      GstBufferFlags flags = gst_buffer_get_flags(buf);
      result.Set("flags", Napi::Number::New(env, static_cast<uint32_t>(flags)));
    }

    // Add caps from sample
    GstCaps *caps = gst_sample_get_caps(sample);
    if (caps) {
      Napi::Object caps_obj = Napi::Object::New(env);
      const GstStructure *structure = gst_caps_get_structure(caps, 0);
      if (structure) {
        // Add structure name
        const gchar *name = gst_structure_get_name(structure);
        caps_obj.Set("name", Napi::String::New(env, name));

        // Add individual structure fields
        auto callback_data = std::make_pair(env, &caps_obj);
        gst_structure_foreach(
          structure,
          [](GQuark field_id, const GValue *value, gpointer user_data) -> gboolean {
            auto *data = static_cast<std::pair<Napi::Env, Napi::Object *> *>(user_data);
            Napi::Env env = data->first;
            Napi::Object *obj = data->second;

            const char *field_name = g_quark_to_string(field_id);
            try {
              Napi::Value js_value = gvalue_to_js(env, value);
              obj->Set(field_name, js_value);
            } catch (...) {
              // Skip fields that can't be converted
            }

            return TRUE;
          },
          &callback_data
        );
      }
      result.Set("caps", caps_obj);
    }

    return result;
  }

  Napi::Object gst_structure_to_js(Napi::Env env, const GstStructure *structure) {
    if (!structure) {
      return Napi::Object::New(env);
    }

    Napi::Object result = Napi::Object::New(env);

    // Add structure name
    const gchar *name = gst_structure_get_name(structure);
    if (name) {
      result.Set("name", Napi::String::New(env, name));
    }

    // Add individual structure fields
    auto callback_data = std::make_pair(env, &result);
    gst_structure_foreach(
      structure,
      [](GQuark field_id, const GValue *value, gpointer user_data) -> gboolean {
        auto *data = static_cast<std::pair<Napi::Env, Napi::Object *> *>(user_data);
        Napi::Env env = data->first;
        Napi::Object *obj = data->second;

        const char *field_name = g_quark_to_string(field_id);
        try {
          Napi::Value js_value = gvalue_to_js(env, value);
          obj->Set(field_name, js_value);
        } catch (...) {
          // Skip fields that can't be converted
        }

        return TRUE;
      },
      &callback_data
    );

    return result;
  }
}
