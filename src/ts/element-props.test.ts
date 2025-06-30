import { describe, it, expect } from "vitest";
import { Pipeline } from "./";

describe.concurrent("Element Properties", () => {
  it("should get prop value", async () => {
    const caps = "video/x-raw,format=(string)GRAY8";
    const pipeline = new Pipeline(`videotestsrc ! capsfilter name=target caps=${caps} ! fakesink`);

    await pipeline.play();

    const element = pipeline.getElementByName("target");

    if (!element) throw new Error("Element not found");

    const prop = element.getElementProperty("caps");

    expect(prop).not.toBeNull();
    expect(prop?.type).toBe("primitive");
    caps.split(",").forEach(cap => expect(prop?.value).toContain(cap));

    await pipeline.stop();
  });

  it("should set string property", () => {
    const pipeline = new Pipeline("videotestsrc name=target ! fakesink");
    const element = pipeline.getElementByName("target");

    if (!element) throw new Error("Element not found");

    // Test setting pattern property
    const originalPattern = element.getElementProperty("pattern");
    element.setElementProperty("pattern", "ball");
    const newPattern = element.getElementProperty("pattern");

    expect(newPattern?.type).toBe("primitive");
    expect(newPattern?.value).toBe("ball");
    expect(newPattern?.value).not.toBe(originalPattern?.value);
  });

  it("should set boolean property", () => {
    const pipeline = new Pipeline("videotestsrc name=target ! fakesink");
    const element = pipeline.getElementByName("target");

    if (!element) throw new Error("Element not found");

    // Test setting is-live property
    const originalIsLive = element.getElementProperty("is-live");
    element.setElementProperty("is-live", true);
    const newIsLive = element.getElementProperty("is-live");

    expect(newIsLive?.type).toBe("primitive");
    expect(newIsLive?.value).toBe(true);
    expect(newIsLive?.value).not.toBe(originalIsLive?.value);
  });

  it("should set integer property", () => {
    const pipeline = new Pipeline("videotestsrc name=target ! fakesink");
    const element = pipeline.getElementByName("target");

    if (!element) throw new Error("Element not found");

    // Test setting num-buffers property
    element.setElementProperty("num-buffers", 100);
    const numBuffers = element.getElementProperty("num-buffers");

    expect(numBuffers?.type).toBe("primitive");
    expect(numBuffers?.value).toBe(100);
  });

  it("should set bigint property", () => {
    const pipeline = new Pipeline("videotestsrc ! queue name=q ! fakesink");
    const queue = pipeline.getElementByName("q");

    if (!queue) throw new Error("Element not found");

    // Test setting num-buffers property with bigint
    queue.setElementProperty("max-size-time", 1000000000n);
    const maxSizeTime = queue.getElementProperty("max-size-time");
    expect(maxSizeTime?.type).toBe("bigint");
    expect(maxSizeTime?.value).toBe(1000000000n);

    // Test setting num-buffers property with number
    queue.setElementProperty("max-size-time", 1000);
    const maxSizeTime2 = queue.getElementProperty("max-size-time");
    expect(maxSizeTime2?.type).toBe("bigint");
    expect(maxSizeTime2?.value).toBe(1000n);
  });

  it("should set caps property", () => {
    const pipeline = new Pipeline("videotestsrc ! capsfilter name=filter ! fakesink");
    const element = pipeline.getElementByName("filter");

    if (!element) throw new Error("Element not found");

    // Test setting caps property
    const capsString = "video/x-raw,width=640,height=480,framerate=30/1";
    element.setElementProperty("caps", capsString);
    const caps = element.getElementProperty("caps");

    expect(caps?.type).toBe("primitive");
    expect(caps?.value).toContain("width=(int)640");
    expect(caps?.value).toContain("height=(int)480");
    expect(caps?.value).toContain("framerate=(fraction)30/1");
  });

  it("should throw error for non-existent property", () => {
    const pipeline = new Pipeline("videotestsrc name=target ! fakesink");
    const element = pipeline.getElementByName("target");

    if (!element) throw new Error("Element not found");

    expect(() => {
      element.setElementProperty("nonexistent-property", "value");
    }).toThrow("Property 'nonexistent-property' not found");
  });

  it("should throw error for wrong value type", () => {
    const pipeline = new Pipeline("videotestsrc name=target ! fakesink");
    const element = pipeline.getElementByName("target");

    if (!element) throw new Error("Element not found");

    expect(() => {
      element.setElementProperty("is-live", "not-a-boolean");
    }).toThrow("Expected boolean value");
  });

  it("should throw error for insufficient arguments", () => {
    const pipeline = new Pipeline("videotestsrc name=target ! fakesink");
    const element = pipeline.getElementByName("target");

    if (!element) throw new Error("Element not found");

    expect(() => {
      (element as any).setElementProperty("pattern");
    }).toThrow("setElementProperty requires property name and value");
  });

  it("should handle property lifecycle correctly", () => {
    const pipeline = new Pipeline("videotestsrc name=target ! fakesink");
    const element = pipeline.getElementByName("target");

    if (!element) throw new Error("Element not found");

    // 1. Property not supported at all - should throw
    expect(() => {
      element.setElementProperty("completely-fake-property", "value");
    }).toThrow("Property 'completely-fake-property' not found");

    // 2. Property supported but not yet set - should set it
    const originalPattern = element.getElementProperty("pattern");
    element.setElementProperty("pattern", "ball");
    const newPattern = element.getElementProperty("pattern");
    expect(newPattern?.type).toBe("primitive");
    expect(newPattern?.value).toBe("ball");
    expect(newPattern?.value).not.toBe(originalPattern?.value);

    // 3. Property exists (already set) - should replace it with new value
    element.setElementProperty("pattern", "smpte");
    const replacedPattern = element.getElementProperty("pattern");
    expect(replacedPattern?.type).toBe("primitive");
    expect(replacedPattern?.value).toBe("smpte");
    expect(replacedPattern?.value).not.toBe("ball");
  });

  it("should handle numeric property replacement", () => {
    const pipeline = new Pipeline("videotestsrc name=target ! fakesink");
    const element = pipeline.getElementByName("target");

    if (!element) throw new Error("Element not found");

    // Set initial value
    element.setElementProperty("num-buffers", 50);
    expect(element.getElementProperty("num-buffers")?.value).toBe(50);

    // Replace with new value
    element.setElementProperty("num-buffers", 100);
    expect(element.getElementProperty("num-buffers")?.value).toBe(100);
  });

  it("should set basic caps on capsfilter", () => {
    const pipeline = new Pipeline("videotestsrc ! capsfilter name=target ! fakesink");
    const element = pipeline.getElementByName("target");

    if (!element) throw new Error("Element not found");

    // Set basic video caps
    const capsString = "video/x-raw,width=320,height=240";
    element.setElementProperty("caps", capsString);
    const retrievedCaps = element.getElementProperty("caps");

    expect(retrievedCaps?.type).toBe("primitive");
    expect(retrievedCaps?.value).toContain("video/x-raw");
    expect(retrievedCaps?.value).toContain("width=(int)320");
    expect(retrievedCaps?.value).toContain("height=(int)240");
  });

  it("should set complex caps with multiple parameters", () => {
    const pipeline = new Pipeline("videotestsrc ! capsfilter name=target ! fakesink");
    const element = pipeline.getElementByName("target");

    if (!element) throw new Error("Element not found");

    // Set complex caps with format, dimensions, and framerate
    const capsString = "video/x-raw,format=RGB,width=640,height=480,framerate=25/1";
    element.setElementProperty("caps", capsString);
    const retrievedCaps = element.getElementProperty("caps");

    expect(retrievedCaps?.type).toBe("primitive");
    expect(retrievedCaps?.value).toContain("video/x-raw");
    expect(retrievedCaps?.value).toContain("format=(string)RGB");
    expect(retrievedCaps?.value).toContain("width=(int)640");
    expect(retrievedCaps?.value).toContain("height=(int)480");
    expect(retrievedCaps?.value).toContain("framerate=(fraction)25/1");
  });

  it("should update caps with different formats", () => {
    const pipeline = new Pipeline("videotestsrc ! capsfilter name=target ! fakesink");
    const element = pipeline.getElementByName("target");

    if (!element) throw new Error("Element not found");

    // Set initial caps
    element.setElementProperty("caps", "video/x-raw,format=RGBA,width=800,height=600");
    let caps = element.getElementProperty("caps");
    expect(caps?.type).toBe("primitive");
    expect(caps?.value).toContain("format=(string)RGBA");
    expect(caps?.value).toContain("width=(int)800");

    // Update caps with different format and dimensions
    element.setElementProperty("caps", "video/x-raw,format=YUV420,width=1920,height=1080");
    caps = element.getElementProperty("caps");
    expect(caps?.type).toBe("primitive");
    expect(caps?.value).toContain("format=(string)YUV420");
    expect(caps?.value).toContain("width=(int)1920");
    expect(caps?.value).toContain("height=(int)1080");
  });

  it("should set caps with fractional framerate", () => {
    const pipeline = new Pipeline("videotestsrc ! capsfilter name=target ! fakesink");
    const element = pipeline.getElementByName("target");

    if (!element) throw new Error("Element not found");

    // Set caps with common video framerates
    const framerates = ["30/1", "25/1", "24000/1001", "60/1"];

    for (const framerate of framerates) {
      const capsString = `video/x-raw,width=1280,height=720,framerate=${framerate}`;
      element.setElementProperty("caps", capsString);
      const retrievedCaps = element.getElementProperty("caps");

      expect(retrievedCaps?.type).toBe("primitive");
      expect(retrievedCaps?.value).toContain(`framerate=(fraction)${framerate}`);
    }
  });

  it("should throw error for invalid caps string", () => {
    const pipeline = new Pipeline("videotestsrc ! capsfilter name=target ! fakesink");
    const element = pipeline.getElementByName("target");

    if (!element) throw new Error("Element not found");

    expect(() => {
      element.setElementProperty("caps", "completely-invalid-caps-format!!!");
    }).toThrow("Invalid caps string");
  });

  it("should handle audio caps on capsfilter", () => {
    const pipeline = new Pipeline("audiotestsrc ! capsfilter name=target ! fakesink");
    const element = pipeline.getElementByName("target");

    if (!element) throw new Error("Element not found");

    // Set audio caps
    const capsString = "audio/x-raw,rate=44100,channels=2";
    element.setElementProperty("caps", capsString);
    const retrievedCaps = element.getElementProperty("caps");

    expect(retrievedCaps?.type).toBe("primitive");
    expect(retrievedCaps?.value).toContain("audio/x-raw");
    expect(retrievedCaps?.value).toContain("rate=(int)44100");
    expect(retrievedCaps?.value).toContain("channels=(int)2");
  });
});
