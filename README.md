# GStreamer Kit

A modern Node.js binding for GStreamer, providing high-level APIs for multimedia streaming and processing. This project modernizes the legacy node-gstreamer-superficial library with contemporary technologies and improved runtime support.

## Project Goals & Modernization

This project represents a complete modernization of the old [node-gstreamer-superficial](https://www.npmjs.com/package/gstreamer-superficial) library, featuring:

### Modern Runtime Support

- **N-API instead of NAN**: Uses Node-API (N-API) for better runtime stability and version independence
- **Multi-runtime compatibility**: Supports Node.js 16+, Bun 1.0+, and other V8-based runtimes
- **Version independence**: Not bound to specific V8 versions, ensuring longevity

### Modern Build System

- **Rollup bundling**: Generates both CommonJS and ESM modules for maximum compatibility
- **TypeScript-first**: Complete TypeScript support with full type definitions
- **CMake build system**: Robust C++ compilation with proper dependency management
- **Modern testing**: Uses Vitest for fast, concurrent testing instead of legacy test frameworks

### Enhanced Developer Experience

- **Full TypeScript support**: Complete type definitions for all APIs
- **ESM/CJS dual packaging**: Works with both `import` and `require` statements
- **Comprehensive testing**: Extensive test coverage with modern test runner
- **Better documentation**: Clear examples and API documentation

## Installation

```bash
npm install @gst/kit
```

### System Requirements

- **Runtime**: Node.js 16+ or Bun 1.0+ (Deno is not supported)
- **System**: GStreamer 1.14 or higher (1.26+ recommended)
- **Build Tools**: CMake 3.10 or higher, pkg-config
- **Dependencies**: GStreamer development packages and plugins

### Platform-Specific Installation Guide

#### Ubuntu/Debian (Recommended for Production)

**Complete Installation:**

```bash
# Update package list
sudo apt-get update

# Install GStreamer core development packages
sudo apt-get install -y \
  libgstreamer1.0-dev \
  libgstreamer-plugins-base1.0-dev \
  libgstreamer-plugins-bad1.0-dev \
  pkg-config

# Install GStreamer plugins (essential for most use cases)
sudo apt-get install -y \
  gstreamer1.0-plugins-base \
  gstreamer1.0-plugins-good \
  gstreamer1.0-plugins-bad \
  gstreamer1.0-plugins-ugly \
  gstreamer1.0-libav

# Install build tools
sudo apt-get install -y cmake build-essential
```

**Verification:**

```bash
pkg-config --cflags --libs gstreamer-1.0
gst-launch-1.0 --version
```

#### macOS (Homebrew)

**Complete Installation:**

```bash
# Install GStreamer and plugins
brew install gstreamer gst-plugins-base gst-plugins-good gst-plugins-bad gst-plugins-ugly pkg-config

# Install build tools
brew install cmake

# Set environment variables (add to ~/.zshrc or ~/.bash_profile)
export PKG_CONFIG_PATH="$(brew --prefix)/lib/pkgconfig:$PKG_CONFIG_PATH"
export LIBRARY_PATH="$(brew --prefix)/lib:$LIBRARY_PATH"
export LD_LIBRARY_PATH="$(brew --prefix)/lib:$LD_LIBRARY_PATH"
```

**Verification:**

```bash
pkg-config --cflags --libs gstreamer-1.0
gst-launch-1.0 --version
```

#### Windows

**Complete Installation:**

1. **Install Build Tools and CMake:**

   ```powershell
   # Install Visual Studio Build Tools 2019/2022 (Community edition is free)
   # Download from: https://visualstudio.microsoft.com/downloads/
   
   # Install CMake
   # Download from: https://cmake.org/download/
   # Or via chocolatey: choco install cmake
   
   # Install pkg-config
   choco install pkgconfiglite
   ```

2. **Install GStreamer 1.26.2:**

   ```powershell
   # Download both runtime and development MSI packages from:
   # https://gstreamer.freedesktop.org/download/
   
   # For 64-bit systems, download and install:
   # - gstreamer-1.0-msvc-x86_64-1.26.2.msi (runtime)
   # - gstreamer-1.0-devel-msvc-x86_64-1.26.2.msi (development)
   
   # Install both MSI files by double-clicking or using:
   # msiexec /i gstreamer-1.0-msvc-x86_64-1.26.2.msi /quiet
   # msiexec /i gstreamer-1.0-devel-msvc-x86_64-1.26.2.msi /quiet
   ```

3. **Set Environment Variables:**

   ```powershell
   # Add to system PATH (via System Properties → Environment Variables):
   C:\Program Files\gstreamer\1.0\msvc_x86_64\bin
   
   # Add system environment variables:
   GSTREAMER_1_0_ROOT_MSVC_X86_64=C:\Program Files\gstreamer\1.0\msvc_x86_64
   PKG_CONFIG_PATH=C:\Program Files\gstreamer\1.0\msvc_x86_64\lib\pkgconfig
   ```

**Verification:**

```powershell
# Verify GStreamer installation
gst-launch-1.0 --version
pkg-config --cflags --libs gstreamer-1.0

# Verify CMake installation
cmake --version
```

### Common Installation Issues

#### Problem: "Cannot find gstreamer-1.0"

**Solution:**

```bash
# Ubuntu/Debian
sudo apt-get install libgstreamer1.0-dev pkg-config

# macOS
brew install gstreamer pkg-config
export PKG_CONFIG_PATH="$(brew --prefix)/lib/pkgconfig:$PKG_CONFIG_PATH"

# Windows (PowerShell)
# Ensure environment variables are set correctly:
# GSTREAMER_1_0_ROOT_MSVC_X86_64=C:\Program Files\gstreamer\1.0\msvc_x86_64
# PKG_CONFIG_PATH=C:\Program Files\gstreamer\1.0\msvc_x86_64\lib\pkgconfig

# Verify pkg-config can find GStreamer
pkg-config --exists gstreamer-1.0 && echo "GStreamer found" || echo "GStreamer NOT found"
```

#### Problem: Missing plugins (playbin/decodebin errors)

**Solution:**

```bash
# Ubuntu/Debian - install plugin packages
sudo apt-get install gstreamer1.0-plugins-{base,good,bad,ugly} gstreamer1.0-libav

# macOS - install plugin packages
brew install gst-plugins-{base,good,bad,ugly}

# List available plugins
gst-inspect-1.0 | grep -i plugin
```

#### Problem: "Permission denied" on Linux

**Solution:**

```bash
# Add user to audio/video groups
sudo usermod -a -G audio,video $USER
# Logout and login again

# Or install PulseAudio for audio support
sudo apt-get install pulseaudio pulseaudio-utils
```

### Docker Installation

For containerized applications:

```dockerfile
# Ubuntu-based container
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    nodejs npm \
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    libgstreamer-plugins-bad1.0-dev \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good \
    gstreamer1.0-plugins-bad \
    gstreamer1.0-plugins-ugly \
    gstreamer1.0-libav \
    cmake build-essential pkg-config \
    && rm -rf /var/lib/apt/lists/*

# Install your application
COPY package*.json ./
RUN npm ci
COPY . .
RUN npm run build
```

## Quick Start

### Basic Pipeline

```javascript
import { Pipeline } from '@gst/kit';

const pipeline = new Pipeline('videotestsrc ! autovideosink');
await pipeline.play();

// Stop after 5 seconds
setTimeout(async () => {
  await pipeline.stop();
}, 5000);
```

### Working with AppSink (Pull-based Approach)

```javascript
import { Pipeline } from '@gst/kit';

const pipeline = new Pipeline('videotestsrc num-buffers=10 ! videoconvert ! appsink name=sink');
const sink = pipeline.getElementByName('sink');

if (sink?.type === 'app-sink-element') {
  await pipeline.play();
  
  while (true) {
    const sample = await sink.getSample(); // Explicitly request samples
    if (!sample) break;
  
    console.log('Received frame:', sample.buffer?.length, 'bytes');
  }
  
  await pipeline.stop();
}
```

### Working with AppSink (Event-Driven/Push Approach)

```javascript
import { Pipeline } from '@gst/kit';

const pipeline = new Pipeline('videotestsrc num-buffers=10 ! videoconvert ! appsink name=sink');
const sink = pipeline.getElementByName('sink');

if (sink?.type === 'app-sink-element') {
  let frameCount = 0;
  
  // Set up reactive callback - samples are pushed automatically
  const unsubscribe = sink.onSample((sample) => {
    frameCount++;
    console.log(`Frame ${frameCount}:`, sample.buffer?.length, 'bytes');
  
    if (frameCount === 10) {
      unsubscribe();
      pipeline.stop();
    }
  });

  await pipeline.play();
}
```

### Working with AppSrc (Source Input)

```javascript
import { Pipeline } from '@gst/kit';

const pipeline = new Pipeline('appsrc name=source ! videoconvert ! autovideosink');
const source = pipeline.getElementByName('source');

if (source?.type === 'app-src-element') {
  source.setElementProperty('caps', 'video/x-raw,format=RGB,width=320,height=240');
  source.setElementProperty('is-live', true);
  
  await pipeline.play();
  
  // Push random RGB frames
  setInterval(() => {
    const buffer = Buffer.alloc(320 * 240 * 3);
    buffer.fill(Math.floor(Math.random() * 255));
    source.push(buffer);
  }, 33); // ~30 FPS
}
```

### Extracting Buffer Data with Pad Probes

```javascript
import { Pipeline } from '@gst/kit';

const pipeline = new Pipeline(
  'videotestsrc ! videoconvert ! x264enc ! rtph264pay name=pay ! fakesink'
);
const payloader = pipeline.getElementByName('pay');

if (payloader) {
  // Add probe to capture comprehensive buffer data from pad
  const unsubscribe = payloader.addPadProbe('src', (bufferData) => {
    console.log('Buffer Data:', {
      // Raw buffer data
      buffer: bufferData.buffer, // Buffer object with raw data
      size: bufferData.buffer?.length, // Buffer size in bytes
  
      // Timing information (nanoseconds)
      pts: bufferData.pts,       // Presentation timestamp
      dts: bufferData.dts,       // Decode timestamp
      duration: bufferData.duration,
      offset: bufferData.offset,
      offsetEnd: bufferData.offsetEnd,
  
      // Buffer metadata
      flags: bufferData.flags,   // GStreamer buffer flags
  
      // Stream format information
      caps: bufferData.caps,     // Caps object with format details
  
      // RTP data (only for RTP streams)
      rtp: bufferData.rtp ? {
        timestamp: bufferData.rtp.timestamp,
        sequence: bufferData.rtp.sequence,
        ssrc: bufferData.rtp.ssrc,
        payloadType: bufferData.rtp.payloadType
      } : undefined
    });
  });

  await pipeline.play();
  
  // ... later, remove the probe
  unsubscribe();
  await pipeline.stop();
}
```

### Pipeline State Management

```javascript
import { Pipeline } from '@gst/kit';

const pipeline = new Pipeline('videotestsrc ! autovideosink');

// State change operations return detailed results
const playResult = await pipeline.play();
console.log('Play result:', playResult.result); // 'success', 'async', 'failure', etc.
console.log('Current state:', playResult.finalState);

// Check if pipeline is playing
console.log('Is playing:', pipeline.playing());

// Pause and resume
await pipeline.pause();
await pipeline.play();

// Stop pipeline
await pipeline.stop();
```

### Position and Duration Queries

```javascript
import { Pipeline } from '@gst/kit';

const pipeline = new Pipeline('videotestsrc ! timeoverlay ! autovideosink');
await pipeline.play();

setInterval(() => {
  const position = pipeline.queryPosition(); // Position in seconds
  const duration = pipeline.queryDuration(); // Duration in seconds
  console.log(`Position: ${position}s / Duration: ${duration}s`);
}, 1000);
```

### Seeking

```javascript
import { Pipeline } from '@gst/kit';

const pipeline = new Pipeline('videotestsrc ! timeoverlay ! autovideosink');
await pipeline.play();

// Seek to 60 seconds
const seekSuccess = pipeline.seek(60);
console.log('Seek successful:', seekSuccess);
```

### Message Bus Handling

```javascript
import { Pipeline } from '@gst/kit';

const pipeline = new Pipeline('videotestsrc num-buffers=100 ! autovideosink');
await pipeline.play();

// Listen for bus messages
while (true) {
  const message = await pipeline.busPop(1000); // 1 second timeout
  
  if (message) {
    console.log('Message:', message.type, message.srcElementName);
  
    if (message.type === 'eos') {
      console.log('End of stream');
      break;
    } else if (message.type === 'error') {
      console.error('Error:', message.errorMessage);
      break;
    }
  }
}
```

### Element Property Manipulation

```javascript
import { Pipeline } from '@gst/kit';

const pipeline = new Pipeline('videotestsrc name=source ! capsfilter name=filter ! autovideosink');

const source = pipeline.getElementByName('source');
const filter = pipeline.getElementByName('filter');

// Set various property types
source?.setElementProperty('pattern', 'ball');
source?.setElementProperty('is-live', true);
source?.setElementProperty('num-buffers', 100);

filter?.setElementProperty('caps', 'video/x-raw,width=1280,height=720,framerate=30/1');

// Get property values
const patternResult = source?.getElementProperty('pattern');
const capsResult = filter?.getElementProperty('caps');

// Access the actual values using the standardized format
const pattern = patternResult?.value;
const caps = capsResult?.value;
```

### Pad Manipulation

```javascript
import { Pipeline } from '@gst/kit';

const pipeline = new Pipeline(
  'input-selector name=sel ! autovideosink videotestsrc pattern=0 ! sel.sink_0 videotestsrc pattern=1 ! sel.sink_1'
);

const selector = pipeline.getElementByName('sel');
await pipeline.play();

// Switch between input pads
setInterval(() => {
  const activePad = Math.random() > 0.5 ? 'sink_0' : 'sink_1';
  selector?.setPad('active-pad', activePad);
  console.log('Switched to:', activePad);
}, 2000);

// Get pad information
const srcPad = selector?.getPad('src');
console.log('Pad info:', srcPad?.name, srcPad?.direction, srcPad?.caps);
```

## Complete Feature Set

### Core Pipeline Features

- **Pipeline Management**: Create, play, pause, stop GStreamer pipelines
- **State Management**: Comprehensive state change handling with detailed results
- **Element Access**: Get elements by name with proper typing
- **Property System**: Get/set element properties with type safety

### Advanced Data Access

- **App Sources & Sinks**: Two approaches for data access:
  - **Pull-based**: Explicitly request samples with `getSample()` (async, controlled timing)
  - **Push-based**: Reactive callbacks with `onSample()` (automatic, real-time)
- **Pad Probes**: Add/remove event-driven callbacks to intercept comprehensive buffer data
- **Buffer Analysis**: Extract raw data, timing information, flags, caps, and metadata

### Media Processing

- **RTP Support**: Extract RTP metadata including timestamps, sequence numbers, SSRC, payload type
- **Seeking**: Frame-accurate seeking with success feedback
- **Query System**: Position and duration queries in seconds
- **Message Bus**: Handle GStreamer messages (EOS, errors, warnings, state changes)

### Runtime Features

- **Multi-format Support**: Video, audio, containers, streaming protocols
- **Codec Support**: H.264, H.265, VP8, VP9, AV1, and more through GStreamer plugins
- **Network Streaming**: RTP, RTSP, HLS, DASH, WebRTC protocols
- **Hardware Acceleration**: GPU-accelerated encoding/decoding where available

## Build System & Tools

### Native Code (C++)

- **CMake**: Modern build system with proper dependency detection
- **N-API**: Node-API for runtime-independent native bindings
- **GStreamer Integration**: Full integration with GStreamer 1.0 ecosystem
- **Compiler Support**: GCC, Clang with C++17 standard

### TypeScript/JavaScript

- **Rollup**: Module bundler generating both ESM and CJS outputs
- **TypeScript**: Full type definitions and compilation
- **Dual Packaging**: Supports both `import` and `require` statements
- **Source Maps**: Full debugging support

### Testing & Quality

- **Vitest**: Modern, fast test runner with concurrent execution
- **ESLint**: Code linting with TypeScript support
- **Prettier**: Code formatting
- **Coverage**: Built-in test coverage reporting

## Project Structure

```txt
gst-kit/
├── src/
│   ├── cpp/                    # C++ native implementation
│   │   ├── addon.cpp          # N-API module entry point
│   │   ├── pipeline.cpp       # Pipeline class implementation
│   │   ├── element.cpp        # Element class implementation
│   │   ├── async-workers.cpp  # Async operation workers
│   │   └── type-conversion.cpp # Type conversion utilities
│   └── ts/                    # TypeScript implementation
│       ├── index.ts           # Main API exports and types
│       └── *.test.ts          # Comprehensive test suite
├── examples/                  # Usage examples
│   ├── basic-pipeline.mjs     # Simple pipeline example
│   ├── appsink.mjs           # AppSink usage
│   ├── appsrc.mjs            # AppSrc usage
│   ├── rtp-timestamp.mjs     # RTP handling
│   ├── bus.mjs               # Message bus handling
│   ├── seek.mjs              # Seeking functionality
│   ├── query.mjs             # Position/duration queries
│   ├── set-pad.mjs           # Pad manipulation
│   ├── fakesink.mjs          # Fakesink usage
│   └── glshader.mjs          # OpenGL shader example
├── build/                     # CMake build output
├── dist/                      # Rollup build output
│   ├── esm/                  # ES modules
│   ├── cjs/                  # CommonJS modules
│   └── index.d.ts            # Type definitions
├── scripts/                   # Build and utility scripts
├── CMakeLists.txt            # CMake configuration
├── rollup.config.mjs         # Rollup bundler configuration
├── vitest.config.ts          # Vitest test configuration
├── tsconfig.json             # TypeScript configuration
└── package.json              # Node.js package configuration
```

### Contributing Guide

#### For TypeScript/JavaScript changes

- Edit files in `src/ts/`
- Add tests alongside implementation
- Run `npm run test:unit` for testing
- Build with `npm run build:ts`

#### For C++ native changes

- Edit files in `src/cpp/`
- Update CMakeLists.txt if adding new files
- Build with `npm run build:native`
- Test with full `npm test`

#### For new features

1. Add implementation in appropriate `src/` directory
2. Add comprehensive tests in `src/ts/*.test.ts`
3. Add usage example in `examples/`
4. Update type definitions in `src/ts/index.ts`
5. Update documentation in README.md

## Buffer Flags Reference

```javascript
import { GstBufferFlags } from '@gst/kit';

// Check buffer flags
if (bufferData.flags & GstBufferFlags.GST_BUFFER_FLAG_DELTA_UNIT) {
  console.log('This is a delta frame (not a keyframe)');
}

if (bufferData.flags & GstBufferFlags.GST_BUFFER_FLAG_HEADER) {
  console.log('This buffer contains header data');
}
```

Available flags include:

- `GST_BUFFER_FLAG_LIVE`: Buffer from live source
- `GST_BUFFER_FLAG_DECODE_ONLY`: Buffer should only be decoded, not displayed
- `GST_BUFFER_FLAG_DISCONT`: Buffer represents discontinuity
- `GST_BUFFER_FLAG_DELTA_UNIT`: Buffer is delta unit (not keyframe)
- `GST_BUFFER_FLAG_HEADER`: Buffer contains header information
- And more...

## Property System

### Standardized Property Results

The `getElementProperty()` method returns a standardized object with type information:

```javascript
// Property result format
const result = element.getElementProperty('property-name');

if (result === null) {
  console.log('Property value is null');
} else {
  console.log('Property type:', result.type);  // "primitive" | "array" | "object" | "buffer" | "sample"
  console.log('Property value:', result.value); // The actual value
}
```

### Property Types

- **`primitive`**: Strings, numbers, booleans, enums
- **`bigint`**: BigInts (typically uint64s used to store nanoseconds)
- **`array`**: Arrays of values
- **`object`**: GStreamer structures (like stats)
- **`buffer`**: Raw binary data
- **`sample`**: Media samples with buffer, caps, and metadata

### Usage Examples

```javascript
// String property
const patternResult = videotestsrc.getElementProperty('pattern');
if (patternResult?.type === 'primitive') {
  console.log('Pattern:', patternResult.value); // e.g., "ball"
}

// Boolean property
const isLiveResult = videotestsrc.getElementProperty('is-live');
if (isLiveResult?.type === 'primitive') {
  console.log('Is live:', isLiveResult.value); // true/false
}

// Structure/Object property (like stats)
const statsResult = rtpdepay.getElementProperty('stats');
if (statsResult?.type === 'object') {
  const stats = statsResult.value;
  console.log('RTP timestamp:', stats.timestamp);
}

// Sample property
const sampleResult = fakesink.getElementProperty('last-sample');
if (sampleResult?.type === 'sample') {
  const sample = sampleResult.value;
  console.log('Buffer size:', sample.buffer.length);
  console.log('Caps:', sample.caps);
}
```

## API Reference

### Pipeline Class

```typescript
class Pipeline {
  constructor(description: string)
  
  // State management
  play(timeoutMs?: number): Promise<StateChangeResult>
  pause(timeoutMs?: number): Promise<StateChangeResult>
  stop(timeoutMs?: number): Promise<StateChangeResult>
  playing(): boolean
  
  // Element access
  getElementByName(name: string): Element | AppSinkElement | AppSrcElement | null
  
  // Position and seeking
  queryPosition(): number
  queryDuration(): number
  seek(positionSeconds: number): boolean
  
  // Message handling
  busPop(timeoutMs?: number): Promise<GstMessage | null>
}
```

### Element Types

```typescript
// Base element with common functionality
interface Element {
  readonly type: "element"
  getElementProperty(key: string): GStreamerPropertyResult
  setElementProperty(key: string, value: GStreamerPropertyValue): void
  addPadProbe(padName: string, callback: (bufferData: BufferData) => void): () => void
  setPad(attribute: string, padName: string): void
  getPad(padName: string): GstPad | null
}

// AppSink element for receiving data
interface AppSinkElement extends Element {
  readonly type: "app-sink-element"
  getSample(timeoutMs?: number): Promise<GStreamerSample | null>
  onSample(callback: (sample: GStreamerSample) => void): () => void
}

// AppSrc element for providing data
interface AppSrcElement extends Element {
  readonly type: "app-src-element"
  push(buffer: Buffer, pts?: Buffer | number): void
}
```

## Performance Considerations

- **Concurrent Testing**: All tests run concurrently for faster execution
- **Efficient Memory Management**: Proper buffer lifecycle management
- **Async Operations**: Non-blocking operations for better performance
- **Type Safety**: Compile-time error detection reduces runtime overhead

## Runtime Compatibility

| Runtime     | Support Level    | Notes                       |
| ----------- | ---------------- | --------------------------- |
| Node.js 16+ | ✅ Full          | Minimal version             |
| Node.js 22+ | ✅ Full          | Latest stable support       |
| Bun 1.0+    | ✅ Full          | Alternative runtime support |
| Deno        | ❌ Not supported | Native module limitations   |

## Platform Compatibility

| Platform        | Local Development | Production Ready | Notes                           |
| --------------- | ----------------- | ---------------- | ------------------------------- |
| Ubuntu/Linux    | ✅ Full           | ✅ Full          | Excellent for servers           |
| macOS           | ✅ Full           | ✅ Full          | Intel and Apple Silicon         |
| Windows         | ✅ Full           | ✅ Full          | Requires environment setup     |
| Docker          | ✅ Full           | ✅ Full          | Ubuntu-based containers         |

## License

MIT License - see LICENSE file for details.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for development setup and contribution guidelines.
