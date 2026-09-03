#include "systeminfo.h"

#include "telemetry.h"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QProcessEnvironment>
#include <QScreen>
#include <QString>
#include <QSurfaceFormat>
#include <QSysInfo>
#include <QThread>
#include <QtGlobal>

#if defined(Q_OS_LINUX)
#include <sys/sysinfo.h>
#elif defined(Q_OS_WIN)
#include <windows.h>
#elif defined(Q_OS_MACOS)
#include <sys/sysctl.h>
#include <sys/types.h>
#endif

namespace {

// Not in QOpenGLFunctions' enum set; both extensions are queried through the
// plain glGetIntegerv the base functions object already gives us.
constexpr GLenum kGpuMemInfoDedicatedVidmemNvx = 0x9047;
constexpr GLenum kGpuMemInfoCurrentAvailableVidmemNvx = 0x9049;
constexpr GLenum kTextureFreeMemoryAti = 0x87FC;

// Windows' GL/gl.h stops at 1.1, so these aren't guaranteed to exist even
// though every context we ever create is 3.2 core.
#ifndef GL_MAX_RENDERBUFFER_SIZE
#define GL_MAX_RENDERBUFFER_SIZE 0x84E8
#endif
#ifndef GL_MAX_SAMPLES
#define GL_MAX_SAMPLES 0x8D57
#endif
#ifndef GL_SHADING_LANGUAGE_VERSION
#define GL_SHADING_LANGUAGE_VERSION 0x8B8C
#endif

std::string glStringOr(QOpenGLFunctions* f, GLenum name, const char* fallback)
{
    const GLubyte* s = f->glGetString(name);
    return s ? reinterpret_cast<const char*>(s) : fallback;
}

int64_t physicalMemoryKb()
{
#if defined(Q_OS_LINUX)
    struct sysinfo info;
    if (sysinfo(&info) == 0)
        return (int64_t)info.totalram * info.mem_unit / 1024;
#elif defined(Q_OS_WIN)
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    if (GlobalMemoryStatusEx(&status))
        return (int64_t)(status.ullTotalPhys / 1024);
#elif defined(Q_OS_MACOS)
    int64_t bytes = 0;
    size_t len = sizeof(bytes);
    if (sysctlbyname("hw.memsize", &bytes, &len, nullptr, 0) == 0)
        return bytes / 1024;
#endif
    return 0;
}

int64_t freeMemoryKb()
{
#if defined(Q_OS_LINUX)
    struct sysinfo info;
    if (sysinfo(&info) == 0)
        return (int64_t)(info.freeram + info.bufferram) * info.mem_unit / 1024;
#elif defined(Q_OS_WIN)
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    if (GlobalMemoryStatusEx(&status))
        return (int64_t)(status.ullAvailPhys / 1024);
#endif
    return 0;
}

// How this build was delivered, which decides whether a driver-side crash is
// even our code's fault — an AppImage carries its own Qt and libstdc++ into a
// host GL stack it was never built against.
std::string packaging()
{
    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (env.contains(QStringLiteral("APPIMAGE")))
        return "appimage";
#if defined(Q_OS_MACOS)
    if (QCoreApplication::applicationDirPath().contains(QStringLiteral(".app/Contents/")))
        return "macos-bundle";
#endif
    return "native";
}

} // namespace

SystemInfo::GpuMemory SystemInfo::queryGpuMemory()
{
    GpuMemory mem;

    QOpenGLContext* ctx = QOpenGLContext::currentContext();
    if (!ctx)
        return mem;

    QOpenGLFunctions* f = ctx->functions();

    if (ctx->hasExtension(QByteArrayLiteral("GL_NVX_gpu_memory_info"))) {
        GLint total = 0;
        GLint available = 0;
        f->glGetIntegerv(kGpuMemInfoDedicatedVidmemNvx, &total);
        f->glGetIntegerv(kGpuMemInfoCurrentAvailableVidmemNvx, &available);
        mem.known = true;
        mem.totalKb = total;
        mem.availableKb = available;
    }
    else if (ctx->hasExtension(QByteArrayLiteral("GL_ATI_meminfo"))) {
        // Returns 4 ints; the first is the free pool in KB. Total isn't
        // exposed, so it stays 0 and callers fall back to the free figure.
        GLint values[4] = {0, 0, 0, 0};
        f->glGetIntegerv(kTextureFreeMemoryAti, values);
        mem.known = true;
        mem.availableKb = values[0];
    }

    // Whatever the query left behind is not our caller's problem to notice.
    while (f->glGetError() != GL_NO_ERROR) {
    }

    return mem;
}

void SystemInfo::reportSystemContext()
{
    const int64_t totalRamKb = physicalMemoryKb();

    Telemetry::Fields device = {
        {"arch", QSysInfo::currentCpuArchitecture().toStdString()},
        {"cpu_count", (int64_t)QThread::idealThreadCount()},
        {"memory_size", totalRamKb * 1024},
        {"free_memory", freeMemoryKb() * 1024},
        {"model", QSysInfo::prettyProductName().toStdString()},
        {"kernel_version", QSysInfo::kernelVersion().toStdString()},
    };

    if (QScreen* screen = QGuiApplication::primaryScreen()) {
        device.emplace_back("screen_width_pixels",
                            (int64_t)screen->geometry().width());
        device.emplace_back("screen_height_pixels",
                            (int64_t)screen->geometry().height());
        device.emplace_back("screen_density", screen->devicePixelRatio());
    }
    device.emplace_back("screen_count",
                        (int64_t)QGuiApplication::screens().size());

    Telemetry::setContext("device", device);

    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const std::string pkg = packaging();

    Telemetry::setContext(
        "runtime",
        {
            {"name", std::string("Qt")},
            {"version", std::string(qVersion())},
            {"build_version", std::string(QT_VERSION_STR)},
            {"platform_plugin", QGuiApplication::platformName().toStdString()},
            {"session_type",
             env.value(QStringLiteral("XDG_SESSION_TYPE"), QStringLiteral("unknown"))
                 .toStdString()},
            {"packaging", pkg},
        });

    Telemetry::setTag("qt.platform", QGuiApplication::platformName().toStdString());
    Telemetry::setTag("packaging", pkg);
}

void SystemInfo::reportGpuContext()
{
    QOpenGLContext* ctx = QOpenGLContext::currentContext();
    if (!ctx) {
        Telemetry::breadcrumb("gpu", "reportGpuContext called with no current context");
        return;
    }

    QOpenGLFunctions* f = ctx->functions();

    const std::string vendor = glStringOr(f, GL_VENDOR, "unknown");
    const std::string renderer = glStringOr(f, GL_RENDERER, "unknown");
    const std::string version = glStringOr(f, GL_VERSION, "unknown");

    GLint maxTextureSize = 0;
    GLint maxRenderbufferSize = 0;
    GLint maxSamples = 0;
    f->glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    f->glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &maxRenderbufferSize);
    f->glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);

    const QSurfaceFormat fmt = ctx->format();
    const GpuMemory mem = queryGpuMemory();

    Telemetry::Fields gpu = {
        {"name", renderer},
        {"vendor_name", vendor},
        {"version", version},
        {"api_type", std::string("OpenGL")},
        {"shading_language_version",
         glStringOr(f, GL_SHADING_LANGUAGE_VERSION, "unknown")},
        {"granted_version",
         std::to_string(fmt.majorVersion()) + "." + std::to_string(fmt.minorVersion())},
        {"granted_profile",
         std::string(fmt.profile() == QSurfaceFormat::CoreProfile ? "core"
                     : fmt.profile() == QSurfaceFormat::CompatibilityProfile
                         ? "compatibility"
                         : "none")},
        {"max_texture_size", (int64_t)maxTextureSize},
        {"max_renderbuffer_size", (int64_t)maxRenderbufferSize},
        {"max_samples", (int64_t)maxSamples},
        {"memory_reporting", mem.known},
    };

    if (mem.known) {
        // Sentry renders gpu.memory_size as MB.
        gpu.emplace_back("memory_size", mem.totalKb / 1024);
        gpu.emplace_back("free_memory_mb", mem.availableKb / 1024);
    }

    Telemetry::setContext("gpu", gpu);

    Telemetry::setTag("gpu.vendor", vendor);
    Telemetry::setTag("gpu.renderer", renderer);
    Telemetry::setTag("gl.version", version);

    Telemetry::breadcrumb("gpu", renderer + " / " + version,
                          {{"vendor", vendor},
                           {"max_texture_size", (int64_t)maxTextureSize},
                           {"vram_total_mb", mem.known ? mem.totalKb / 1024 : (int64_t)-1}});
}
