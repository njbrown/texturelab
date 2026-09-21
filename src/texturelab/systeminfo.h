#pragma once

#include <cstdint>

// Hardware and environment reporting for crash triage.
//
// Exists because a Sentry report that says only "it aborted somewhere in the GL
// driver" is unactionable: the GTX 750 / 4K report that prompted this had no
// GPU, no VRAM figure, and no clue that a resolution change had just happened.
// Everything here funnels into Telemetry contexts/tags and no-ops when crash
// reporting is off.
namespace SystemInfo {

// What the driver will tell us about video memory. `known` is false when
// neither GL_NVX_gpu_memory_info nor GL_ATI_meminfo is advertised — which is
// common enough that callers must handle it rather than assume a number.
struct GpuMemory {
    bool known = false;
    int64_t totalKb = 0;
    int64_t availableKb = 0;
};

// Requires a current OpenGL context. Cheap enough to call per user action.
GpuMemory queryGpuMemory();

// Register the non-GL half: RAM, CPU, screens, Qt/platform/packaging.
// Call once, before any GL work — the crash we're chasing happened *during* GL
// init, so this has to already be on the scope by then.
void reportSystemContext();

// Register the GPU half. Requires a current OpenGL context; call once from
// TextureRenderer::setup() where that's guaranteed.
void reportGpuContext();

} // namespace SystemInfo
