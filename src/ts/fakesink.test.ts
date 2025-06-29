import { describe, expect, it } from "vitest";
import { Pipeline, type GStreamerSample } from ".";

describe.concurrent("FakeSink", () => {
  it("should capture last sample when enabled", async () => {
    const pipeline = new Pipeline("videotestsrc ! fakesink enable-last-sample=true name=sink");
    const fakesink = pipeline.getElementByName("sink");

    if (!fakesink) throw new Error("FakeSink element not found");

    await pipeline.play();

    // Get the last sample
    const sampleResult = fakesink.getElementProperty("last-sample");

    pipeline.stop();

    expect(sampleResult).not.toBeNull();
    expect(sampleResult?.type).toBe("sample");

    const sample = sampleResult?.value as GStreamerSample;
    expect(sample).toHaveProperty("buffer");
    expect(sample).toHaveProperty("caps");
    expect(sample).toHaveProperty("flags");

    if (sample && typeof sample === "object" && "buffer" in sample && "caps" in sample) {
      expect(sample.buffer).toBeInstanceOf(Buffer);
      expect(sample.buffer?.length).toBeGreaterThan(0);
      expect(sample.caps).toHaveProperty("name");
      expect(typeof sample.flags).toBe("number");
    }
  });

  it("should return null when last-sample is disabled", async () => {
    const pipeline = new Pipeline("videotestsrc ! fakesink enable-last-sample=false name=sink");
    const fakesink = pipeline.getElementByName("sink");

    if (!fakesink) throw new Error("FakeSink element not found");

    await pipeline.play();

    // Wait for some frames to be processed
    await new Promise(resolve => setTimeout(resolve, 50));

    // Try to get the last sample (should be null since it's disabled)
    const sampleResult = fakesink.getElementProperty("last-sample");

    pipeline.stop();

    expect(sampleResult).toBeNull();
  });

  it("should handle pipeline state changes correctly", async () => {
    const pipeline = new Pipeline("videotestsrc ! fakesink enable-last-sample=true name=sink");
    const fakesink = pipeline.getElementByName("sink");

    if (!fakesink) throw new Error("FakeSink element not found");

    // Pipeline should not be playing initially
    expect(pipeline.playing()).toBe(false);

    await pipeline.play();
    expect(pipeline.playing()).toBe(true);

    await pipeline.stop();
    expect(pipeline.playing()).toBe(false);
  });

  it("should work with limited number of buffers", async () => {
    const numBuffers = 5;
    const pipeline = new Pipeline(
      `videotestsrc num-buffers=${numBuffers} ! fakesink enable-last-sample=true name=sink`
    );
    const fakesink = pipeline.getElementByName("sink");

    if (!fakesink) throw new Error("FakeSink element not found");

    await pipeline.play();

    // Wait for all buffers to be processed
    await new Promise(resolve => setTimeout(resolve, 50));

    const sampleResult = fakesink.getElementProperty("last-sample");

    pipeline.stop();

    expect(sampleResult).not.toBeNull();
    expect(sampleResult?.type).toBe("sample");

    const sample = sampleResult?.value as GStreamerSample;
    if (sample && typeof sample === "object" && "buffer" in sample) {
      expect(sample.buffer).toBeInstanceOf(Buffer);
      expect(sample.buffer?.length).toBeGreaterThan(0);
    }
  });

  it("should capture samples with different video formats", async () => {
    const pipeline = new Pipeline(
      "videotestsrc ! videoconvert ! video/x-raw,format=RGB ! fakesink enable-last-sample=true name=sink"
    );
    const fakesink = pipeline.getElementByName("sink");

    if (!fakesink) throw new Error("FakeSink element not found");

    await pipeline.play();

    await new Promise(resolve => setTimeout(resolve, 50));

    const sampleResult = fakesink.getElementProperty("last-sample");

    pipeline.stop();

    expect(sampleResult).not.toBeNull();
    expect(sampleResult?.type).toBe("sample");

    const sample = sampleResult?.value as GStreamerSample;
    if (sample && typeof sample === "object" && "caps" in sample) {
      const caps = sample.caps;
      expect(caps?.name).toContain("video/x-raw");
    }
  });
});
