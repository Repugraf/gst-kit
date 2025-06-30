import { join, dirname } from "node:path";
import { fileURLToPath } from "node:url";
import { createRequire } from "node:module";

// Get the directory name of the current module
const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

// Get the project root directory (two levels up from __dirname)
const projectRoot = join(__dirname, "../../");

export type GStreamerPropertyValue = string | number | boolean | bigint;

// Sample object returned for GST_VALUE_HOLDS_SAMPLE properties
export type GStreamerSample = {
  buffer?: Buffer;
  flags?: number;
  caps?: {
    name?: string;
    // Additional structure fields (format, width, height, framerate, etc.)
    [key: string]: GStreamerPropertyValue | undefined;
  };
};

// GStreamer message object returned by busPop
export type GstMessage = {
  type: string;
  srcElementName?: string;
  timestamp: bigint;
  structureName?: string;
  // Structure fields are added dynamically based on message content
  [key: string]: GStreamerPropertyValue | undefined;

  // Error message specific fields
  errorMessage?: string;
  errorDomain?: string;
  errorCode?: number;
  debugInfo?: string;

  // Warning message specific fields
  warningMessage?: string;
  warningDomain?: string;
  warningCode?: number;

  // State change message specific fields
  oldState?: number;
  newState?: number;
  pendingState?: number;
};

// Extended return types including arrays, buffers, and samples
export type GStreamerPropertyReturnValue =
  | GStreamerPropertyValue
  | GStreamerPropertyValue[]
  | Record<string, GStreamerPropertyValue>
  | Buffer
  | GStreamerSample
  | null;

export type GStreamerPropertyResult =
  | { type: "primitive"; value: GStreamerPropertyValue }
  | { type: "bigint"; value: GStreamerPropertyValue }
  | { type: "array"; value: GStreamerPropertyValue[] }
  | { type: "object"; value: Record<string, GStreamerPropertyValue> }
  | { type: "buffer"; value: Buffer }
  | { type: "sample"; value: GStreamerSample }
  | null;

// State change result returned by play(), pause(), and stop()
export type StateChangeResult = {
  result: "success" | "async" | "no-preroll" | "failure" | "unknown";
  finalState: number;
  targetState: number;
};

export type RTPData = {
  timestamp: number;
  sequence: number;
  ssrc: number;
  payloadType: number;
};

export type GstPad = {
  name: string;
  direction: number; // 0 = unknown, 1 = src, 2 = sink
  caps: string | null;
};

export type BufferData = {
  // Raw buffer data
  buffer?: Buffer;

  // Timing information
  pts?: number; // Presentation timestamp (nanoseconds)
  dts?: number; // Decode timestamp (nanoseconds)
  duration?: number; // Buffer duration (nanoseconds)
  offset?: number;
  offsetEnd?: number;

  // Buffer flags
  flags: number;

  // Caps information (stream format)
  caps?: {
    name?: string;
    [key: string]: GStreamerPropertyValue | undefined;
  };

  // RTP-specific data (only present for RTP streams)
  rtp?: RTPData;
};

type ElementBase = {
  getElementProperty: (key: string) => GStreamerPropertyResult;
  setElementProperty: (key: string, value: GStreamerPropertyValue) => void;
  addPadProbe: (padName: string, callback: (bufferData: BufferData) => void) => () => void;
  setPad: (attribute: string, padName: string) => void;
  getPad: (padName: string) => GstPad | null;
};

type Element = {
  readonly type: "element";
} & ElementBase;

type AppSinkElement = {
  readonly type: "app-sink-element";
  getSample(timeoutMs?: number): Promise<GStreamerSample | null>;
  onSample(callback: (sample: GStreamerSample) => void): () => void;
} & ElementBase;

type AppSrcElement = {
  readonly type: "app-src-element";
  push(buffer: Buffer, pts?: Buffer | number): void;
} & ElementBase;

interface Pipeline {
  play(timeoutMs?: number): Promise<StateChangeResult>;
  pause(timeoutMs?: number): Promise<StateChangeResult>;
  stop(timeoutMs?: number): Promise<StateChangeResult>;
  playing(): boolean;
  getElementByName(name: string): Element | AppSinkElement | AppSrcElement | null;
  queryPosition(): number;
  queryDuration(): number;
  busPop(timeoutMs?: number): Promise<GstMessage | null>;
  seek(positionSeconds: number): boolean;
}

// Define the interface for the native addon
interface NativeAddon {
  Pipeline: new (pipeline: string) => Pipeline;
  GStreamerPropertyValue: GStreamerPropertyValue;
  GStreamerSample: GStreamerSample;
  GStreamerPropertyReturnValue: GStreamerPropertyReturnValue;
}

// Create require function for ESM
const require = createRequire(import.meta.url);

// Load the native addon
const nativeAddon: NativeAddon = require(join(projectRoot, "build/Release/gst_kit.node"));

/**
 * https://gstreamer.freedesktop.org/documentation/gstreamer/gstbuffer.html?gi-language=c#GstBufferFlags
 * */
export const GstBufferFlags = {
  GST_BUFFER_FLAG_LIVE: 16,
  GST_BUFFER_FLAG_DECODE_ONLY: 32,
  GST_BUFFER_FLAG_DISCONT: 64,
  GST_BUFFER_FLAG_RESYNC: 128,
  GST_BUFFER_FLAG_CORRUPTED: 256,
  GST_BUFFER_FLAG_MARKER: 512,
  GST_BUFFER_FLAG_HEADER: 1024,
  GST_BUFFER_FLAG_GAP: 2048,
  GST_BUFFER_FLAG_DROPPABLE: 4096,
  GST_BUFFER_FLAG_DELTA_UNIT: 8192,
  GST_BUFFER_FLAG_TAG_MEMORY: 16384,
  GST_BUFFER_FLAG_SYNC_AFTER: 32768,
  GST_BUFFER_FLAG_NON_DROPPABLE: 65536,
  GST_BUFFER_FLAG_LAST: 1048576,
} as const;

const { Pipeline: PipelineClass } = nativeAddon;

export { PipelineClass as Pipeline };

export default { ...nativeAddon, GstBufferFlags };
