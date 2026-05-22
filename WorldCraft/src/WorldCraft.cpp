// WorldCraft.cpp : Application entry point � Minecraft-style multi-chunk world.

#include <WorldCraft.h>
#include <Renderer/Shader.h>
#include <Renderer/Window.h>
#include <Renderer/Camera.h>
#include <Texture/BlockTextures.h>
#include <Chunk/ChunkWorld.h>
#include <WorldGen/TerrainGen.h>
#include <UI/ImGuiManager.h>
#include <UI/SettingsDialog.h>
#include <WorldGen/WorldSettings.h>
#include <imgui.h>
#include <thread>
#include <chrono>

// ---------------------------------------------------------------------------
// Window
// ---------------------------------------------------------------------------
static constexpr int SCREEN_W = 1280;
static constexpr int SCREEN_H = 800;

// ---------------------------------------------------------------------------
// Sky shader — fullscreen background drawn before chunks
// ---------------------------------------------------------------------------
static const char* kSkyVert = R"glsl(
#version 330 core
// Full-screen triangle trick: no VBO needed.
void main()
{
    // Emit a triangle that covers the whole clip space.
    vec2 pos[3];
    pos[0] = vec2(-1.0, -1.0);
    pos[1] = vec2( 3.0, -1.0);
    pos[2] = vec2(-1.0,  3.0);
    gl_Position = vec4(pos[gl_VertexID], 0.0, 1.0);
}
)glsl";

static const char* kSkyFragFinal = R"glsl(
#version 330 core

uniform mat4  uInvViewProj;
uniform vec2  uResolution;
uniform float uTimeOfDay;     // [0,1)

out vec4 FragColor;

// ---- time-of-day colour palettes ------------------------------------------
// tod: 0=midnight, 0.25=dawn, 0.5=noon, 0.75=dusk

float dayWeight(float tod)
{
    // Smooth pulse centred on noon (0.5), zero at midnight (0 or 1).
    float t = abs(tod - 0.5) * 2.0;      // 0 at noon, 1 at midnight
    return 1.0 - smoothstep(0.0, 0.85, t);
}

vec3 zenithColour(float tod)
{
    vec3 night  = vec3(0.01, 0.01, 0.06);
    vec3 dawn   = vec3(0.35, 0.30, 0.55);
    vec3 day    = vec3(0.22, 0.48, 0.90);
    vec3 dusk   = vec3(0.45, 0.25, 0.45);

    float t = tod;
    // Dawn 0.18-0.32, Day 0.32-0.68, Dusk 0.68-0.82, Night otherwise.
    if (t < 0.18)  return mix(night, night,  smoothstep(0.0,  0.18, t));
    if (t < 0.28)  return mix(night, dawn,   smoothstep(0.18, 0.28, t));
    if (t < 0.38)  return mix(dawn,  day,    smoothstep(0.28, 0.38, t));
    if (t < 0.62)  return day;
    if (t < 0.72)  return mix(day,   dusk,   smoothstep(0.62, 0.72, t));
    if (t < 0.82)  return mix(dusk,  night,  smoothstep(0.72, 0.82, t));
    return night;
}

vec3 horizonColour(float tod)
{
    // Ground horizon slightly darker than sky zenith for depth
    vec3 nightH = vec3(0.01, 0.01, 0.05);  // Slightly darker than zenith night
    vec3 dawnH  = vec3(0.30, 0.25, 0.45);  // Slightly darker/warmer than zenith dawn
    vec3 dayH   = vec3(0.18, 0.40, 0.75);  // Slightly darker than zenith day blue
    vec3 duskH  = vec3(0.38, 0.20, 0.35);  // Slightly darker than zenith dusk

    float t = tod;
    if (t < 0.18)  return nightH;
    if (t < 0.28)  return mix(nightH, dawnH,  smoothstep(0.18, 0.28, t));
    if (t < 0.38)  return mix(dawnH,  dayH,   smoothstep(0.28, 0.38, t));
    if (t < 0.62)  return dayH;
    if (t < 0.72)  return mix(dayH,   duskH,  smoothstep(0.62, 0.72, t));
    if (t < 0.82)  return mix(duskH,  nightH, smoothstep(0.72, 0.82, t));
    return nightH;
}

vec3 sunDirection(float tod)
{
    float angle = (tod - 0.25) * 6.28318530;
    return normalize(vec3(cos(angle), sin(angle), 0.0));
}

vec3 moonDirection(float tod) { return -sunDirection(tod); }

float starField(vec3 dir)
{
    vec3 d = floor(normalize(dir) * 200.0);
    float h  = fract(sin(dot(d, vec3(127.1, 311.7,  74.9))) * 43758.5453);
    float h2 = fract(sin(dot(d, vec3(269.5, 183.3, 246.1))) * 43758.5453);
    return (h > 0.993 && h2 > 0.5) ? pow(h2, 3.0) : 0.0;
}

void main()
{
    // Reconstruct normalised view ray.
    vec2 ndc    = (gl_FragCoord.xy / uResolution) * 2.0 - 1.0;
    vec4 rayClip = vec4(ndc, 1.0, 1.0);
    vec4 rayWorld = uInvViewProj * rayClip;
    vec3 rayDir  = normalize(rayWorld.xyz / rayWorld.w);

    float tod = uTimeOfDay;

    // Base sky gradient: zenith -> horizon based on ray elevation.
    float elevation = rayDir.y;  // -1 below, +1 above
    float t         = clamp(elevation * 1.6 + 0.15, 0.0, 1.0);
    vec3  sky       = mix(horizonColour(tod), zenithColour(tod), t);

    // Sun disk.
    vec3 sd   = sunDirection(tod);
    float sunDot = dot(rayDir, sd);
    float sunDisk = smoothstep(0.9985, 0.9995, sunDot);
    // Sun corona / glow.
    float sunGlow = pow(max(sunDot, 0.0), 48.0) * 0.6;
    // Only show sun above horizon.
    float sunVis = clamp(sd.y * 4.0 + 0.5, 0.0, 1.0);
    vec3  sunColor = vec3(1.0, 0.95, 0.75);
    sky += sunColor * (sunDisk + sunGlow) * sunVis;

    // Moon disk (simple white circle).
    vec3 md   = moonDirection(tod);
    float moonDot  = dot(rayDir, md);
    float moonDisk = smoothstep(0.9992, 0.9998, moonDot);
    float moonVis  = clamp(md.y * 4.0 + 0.5, 0.0, 1.0);
    vec3  moonColor = vec3(0.85, 0.90, 1.0);
    sky += moonColor * moonDisk * moonVis;

    // Stars — visible only at night (when sun is below horizon).
    float nightness = clamp(-sd.y * 3.0, 0.0, 1.0);
    // Rotate star field slowly with time to simulate Earth rotation.
    float starAngle = tod * 6.28318;
    float cs = cos(starAngle), ss = sin(starAngle);
    vec3 starDir = vec3(
        rayDir.x * cs - rayDir.z * ss,
        rayDir.y,
        rayDir.x * ss + rayDir.z * cs
    );
    float star = starField(starDir);
    sky += vec3(star) * nightness * 1.4;

    // Below-horizon: fill with the horizon colour so any pixel gap between
    // world geometry and the sky edge is invisible (matches the fog colour).
    if (rayDir.y < -0.02)
        sky = horizonColour(tod);

    // Dawn/dusk orange blush on horizon near sun.
    float horizonBlush = pow(max(sunDot * sunVis, 0.0), 6.0);
    vec3  blushColor   = (tod < 0.5)
        ? vec3(1.0, 0.55, 0.20)   // dawn = warm orange
        : vec3(1.0, 0.35, 0.10);  // dusk = deep red-orange
    float blushBand = smoothstep(0.0, 0.3, 1.0 - abs(elevation));
    sky += blushColor * horizonBlush * blushBand * 0.7;

    FragColor = vec4(sky, 1.0);
}
)glsl";

// ---------------------------------------------------------------------------
// Chunk vertex shader — position / UV / texLayer / light
// ---------------------------------------------------------------------------
static const char* kChunkVert = R"glsl(
#version 330 core

layout(location = 0) in vec3  aPos;
layout(location = 1) in vec2  aUV;
layout(location = 2) in float aTexLayer;
layout(location = 3) in float aLight;

uniform mat4 uMVP;
uniform vec3 uCamPos;
uniform vec3 uChunkOffset;
uniform float uSunHeight;  // -1..1 from sky shader

out vec2  vUV;
out float vTexLayer;
out float vLight;
out float vFogDist;
out float vWorldY;
out float vCamY;
out vec3  vWorldPos;

void main()
{
    vec3 worldPos = aPos + uChunkOffset;
    gl_Position   = uMVP * vec4(aPos, 1.0);
    vUV           = aUV;
    vTexLayer     = aTexLayer;
    // Scale geometry light by sun height so world darkens at night.
    float sunFactor = clamp(uSunHeight * 2.0 + 0.15, 0.08, 1.0);
    vLight        = aLight * sunFactor;
    vFogDist      = length(worldPos - uCamPos);
    vWorldY       = worldPos.y;
    vCamY         = uCamPos.y;
    vWorldPos     = worldPos;
}
)glsl";

// ---------------------------------------------------------------------------
// Chunk fragment shader — samples the texture array, applies light
// ---------------------------------------------------------------------------
static const char* kChunkFrag = R"glsl(
#version 330 core

in  vec2  vUV;
in  float vTexLayer;
in  float vLight;
in  float vFogDist;
in  float vWorldY;
in  float vCamY;
in  vec3  vWorldPos;

uniform sampler2DArray uTexArray;
uniform float uFogStart;
uniform float uFogEnd;
uniform vec3  uFogColour;
uniform float uSeaLevel;
uniform float uTime;
uniform bool  uInWater;  // NEW: Is camera actually in water (not just below sea level)?

out vec4 FragColor;

// Simple animated caustics pattern for underwater ripple effect
float caustics(vec2 uv, float time)
{
    vec2 p = uv * 0.5 + time * 0.05;
    float a = sin(p.x * 3.0 + time * 2.0) * cos(p.y * 3.0 + time * 1.5);
    float b = sin((p.x + p.y) * 2.0 - time * 1.8) * 0.5;
    return clamp(a + b, 0.0, 1.0) * 0.3 + 0.7;  // Range 0.7-1.0
}

// Heat haze distortion for distant terrain
vec2 heatHaze(vec2 uv, vec3 worldPos, float time, float intensity)
{
    // Multiple sine waves at different frequencies for organic motion
    float wave1 = sin(worldPos.x * 0.3 + time * 1.2) * cos(worldPos.z * 0.25 + time * 0.8);
    float wave2 = sin(worldPos.x * 0.5 - time * 0.9) * sin(worldPos.z * 0.4 + time * 1.5);
    float wave3 = cos(worldPos.x * 0.15 + worldPos.z * 0.2 + time * 0.5);

    // Combine waves and scale by intensity
    vec2 distortion = vec2(wave1 + wave2 * 0.5, wave2 + wave3 * 0.5) * intensity;
    return uv + distortion * 0.002;  // Subtle UV shift
}

void main()
{
    // Calculate distance-based heat haze intensity - starts much closer to camera
    float distNormForHaze = clamp((vFogDist - uFogStart * 0.3) / (uFogEnd - uFogStart * 0.3), 0.0, 1.0);
    float hazeIntensity = pow(distNormForHaze, 0.6) * 6.0;  // More aggressive curve, stronger effect

    // Apply heat haze distortion to UV coordinates for distant blocks
    vec2 distortedUV = heatHaze(vUV, vWorldPos, uTime, hazeIntensity);

    vec4 tex = texture(uTexArray, vec3(distortedUV, vTexLayer));
    if (tex.a < 0.1)
        discard;

    // vLight already contains: kFaceLight * AO_factor * sunFactor (from vertex shader).
    // Split into a small constant ambient (sky-light bounce) plus the AO-baked diffuse.
    // The ambient ensures blocks are never pitch-black even in deep shadow.
    vec3 baseColour = tex.rgb;
    vec3 ambient    = baseColour * 0.12;          // dim sky-bounce, always present
    vec3 diffuse    = baseColour * vLight;        // AO + face directional + sun height
    vec3 lit        = clamp(ambient + diffuse, vec3(0.0), vec3(1.0));

    // Subtle gamma-space lift: raise lit to ~1/1.8 so mid-tones look richer.
    lit = pow(lit, vec3(1.0 / 1.8));

    // Cave lighting: blocks below Y=60 progressively darken
    // This creates proper cave darkness without needing surface height data
    if (vWorldY < 60.0)
    {
        float caveDepth = 60.0 - vWorldY;  // 0 at Y=60, increases downward
        float caveDarkness = exp(-caveDepth * 0.08);  // Exponential darkening
        caveDarkness = clamp(caveDarkness, 0.15, 1.0);  // Never completely black (0.15 minimum)
        lit *= caveDarkness;
    }

    // Underwater darkening: blocks below sea level darken with depth.
    if (vWorldY < uSeaLevel)
    {
        float depth      = uSeaLevel - vWorldY;           // 0 at surface, positive downward
        float darkFactor = exp(-depth * 0.07);            // exponential falloff
        darkFactor       = clamp(darkFactor, 0.08, 1.0);
        // Tint toward deep blue-green as depth increases.
        vec3 waterTint = mix(vec3(0.0, 0.15, 0.3), vec3(1.0), darkFactor);
        lit *= waterTint;
    }

    // Check if camera is underwater - trigger at the top surface of water blocks
    // BUT only if actually in water, not in an air pocket/cave below sea level
    bool cameraUnderwater = uInWater;

    vec3 finalColor;

    if (cameraUnderwater)
    {
        // Underwater rendering - add water fog and caustics ripple effect
        vec3 waterFogColor = vec3(0.0, 0.25, 0.45);  // Blue-green underwater fog

        // Reduced underwater fog density
        float underwaterFogStart = 0.0;
        float underwaterFogEnd = 80.0;  // Increased from 64 for less dense fog
        float underwaterDistNorm = clamp((vFogDist - underwaterFogStart) / (underwaterFogEnd - underwaterFogStart), 0.0, 1.0);

        // Apply blue-green water tint to everything
        lit *= vec3(0.5, 0.75, 1.0);  // Underwater color tint - lighter/brighter

        // Animated caustics ripple effect
        float causticsPattern = caustics(vWorldPos.xz, uTime);
        lit *= causticsPattern;

        // Less dense underwater fog
        float underwaterFog = pow(underwaterDistNorm, 1.0) * 0.85;  // Reduced from 0.95
        finalColor = mix(lit, waterFogColor, underwaterFog);
    }
    else
    {
        // Above water - very aggressive atmospheric blur with heat haze
        float distNorm = clamp((vFogDist - uFogStart) / (uFogEnd - uFogStart), 0.0, 1.0);

        // Much stronger blur from heat shimmer - heavy detail loss at distance
        float shimmerBlur = pow(distNorm, 0.9) * 0.35;  // Earlier, stronger blur
        lit = mix(lit, vec3(dot(lit, vec3(0.333))), shimmerBlur);

        // Very aggressive desaturation - starts early, ramps hard
        float desatAmount = pow(distNorm, 1.4) * 0.99;  // Earlier onset, near-complete desaturation
        float luminance = dot(lit, vec3(0.299, 0.587, 0.114));
        vec3 desaturated = mix(lit, vec3(luminance), desatAmount);

        // Extremely strong haze - rapid transition to fog color, complete at horizon
        float hazeAmount = pow(distNorm, 1.0) * 1.0;  // Linear at end to ensure full fog color at horizon
        finalColor = mix(desaturated, uFogColour, hazeAmount);
    }

    FragColor = vec4(finalColor, tex.a);
}
)glsl";

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main()
{
    constexpr bool kNoSdlIsolationMode = false;
    if (kNoSdlIsolationMode)
    {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::_Exit(0);
    }

    constexpr bool kRenderIsolationMode = false;

    struct RenderResources
    {
        Texture::TextureArray texArray;
        GLuint shaderProg = 0;
        GLint  mvpLoc = -1;
        GLint  texArrayLoc = -1;
        GLint  camPosLoc = -1;
        GLint  chunkOffsetLoc = -1;
        GLint  fogStartLoc = -1;
        GLint  fogEndLoc = -1;
        GLint  fogColLoc = -1;
        GLint  seaLevelLoc = -1;
        GLint  sunHeightLoc = -1;
        GLint  timeLoc = -1;
        GLint  inWaterLoc = -1;  // NEW: Is camera actually in water?

        GLuint skyProg = 0;
        GLint  skyInvVPLoc = -1;
        GLint  skyResLoc = -1;
        GLint  skyTodLoc = -1;
        GLuint skyVAO = 0;
    };

    auto destroyRenderResources = [](RenderResources& rr)
    {
        if (rr.skyVAO)
        {
            glDeleteVertexArrays(1, &rr.skyVAO);
            rr.skyVAO = 0;
        }
        if (rr.skyProg)
        {
            glDeleteProgram(rr.skyProg);
            rr.skyProg = 0;
        }
        if (rr.shaderProg)
        {
            glDeleteProgram(rr.shaderProg);
            rr.shaderProg = 0;
        }
        rr.texArray.destroy();
    };

    auto initRenderResources = [&](RenderResources& rr)
    {
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);

        rr.texArray.build();

        rr.shaderProg = Renderer::buildProgram(kChunkVert, kChunkFrag);
        rr.mvpLoc = glGetUniformLocation(rr.shaderProg, "uMVP");
        rr.texArrayLoc = glGetUniformLocation(rr.shaderProg, "uTexArray");
        rr.camPosLoc = glGetUniformLocation(rr.shaderProg, "uCamPos");
        rr.chunkOffsetLoc = glGetUniformLocation(rr.shaderProg, "uChunkOffset");
        rr.fogStartLoc = glGetUniformLocation(rr.shaderProg, "uFogStart");
        rr.fogEndLoc = glGetUniformLocation(rr.shaderProg, "uFogEnd");
        rr.fogColLoc = glGetUniformLocation(rr.shaderProg, "uFogColour");
        rr.seaLevelLoc = glGetUniformLocation(rr.shaderProg, "uSeaLevel");
        rr.sunHeightLoc = glGetUniformLocation(rr.shaderProg, "uSunHeight");
        rr.timeLoc = glGetUniformLocation(rr.shaderProg, "uTime");
        rr.inWaterLoc = glGetUniformLocation(rr.shaderProg, "uInWater");

        rr.skyProg = Renderer::buildProgram(kSkyVert, kSkyFragFinal);
        rr.skyInvVPLoc = glGetUniformLocation(rr.skyProg, "uInvViewProj");
        rr.skyResLoc = glGetUniformLocation(rr.skyProg, "uResolution");
        rr.skyTodLoc = glGetUniformLocation(rr.skyProg, "uTimeOfDay");

        glGenVertexArrays(1, &rr.skyVAO);
    };

    // ---- Window & GL context ------------------------------------------------
    bool isFullscreen = true;
    SDL_Window* window = Renderer::createWindow(SCREEN_W, SCREEN_H, "WorldCraft", isFullscreen);
    if (!window) return -1;

    RenderResources rr;
    if (!kRenderIsolationMode)
        initRenderResources(rr);

    // ---- ImGui Setup --------------------------------------------------------
    UI::ImGuiManager imguiManager;
    if (!imguiManager.initialize(window))
    {
        std::cerr << "Failed to initialize ImGui\n";
        Renderer::destroyWindow(window);
        return -1;
    }

    UI::SettingsDialog settingsDialog;
    settingsDialog.show();  // Show on startup

    // ---- Camera System ------------------------------------------------------
    // Dual camera mode: Free-fly (noclip) and Character (walking/jumping)
    enum class CameraMode { FreeFly, Character };
    CameraMode cameraMode = CameraMode::FreeFly;

    Renderer::FlyCamera flyCamera({ 8.0f, 100.0f, 8.0f });
    Renderer::CharacterCamera charCamera({ 8.0f, 100.0f, 8.0f });

    flyCamera.init(window);
    charCamera.init(window);

    // ---- World --------------------------------------------------------------
    // Start with default settings
    WorldGen::WorldSettings currentSettings = WorldGen::WorldSettings::createDefault();
    auto world = std::make_unique<Chunk::ChunkWorld>(currentSettings);

    // Callback for when user generates a new world
    bool needsWorldRegeneration = false;
    WorldGen::WorldSettings pendingSettings;

    settingsDialog.setGenerateWorldCallback([&](const WorldGen::WorldSettings& newSettings) {
        pendingSettings = newSettings;
        needsWorldRegeneration = true;
        settingsDialog.hide();  // Close dialog after generating
    });

    // ---- Day/night ---------------------------------------------------------
    // One full day = 480 real seconds (8 minutes).  Change kDayLength to taste.
    static constexpr float kDayLength  = 480.0f;
    // Start near dawn so the player immediately sees colour.
    float timeOfDay = 0.22f;  // 0=midnight, 0.25=dawn, 0.5=noon, 0.75=dusk

    // Helper: compute sun Y component from timeOfDay.
    auto calcSunHeight = [](float tod) -> float {
        float angle = (tod - 0.25f) * 6.28318530f;
        return std::sin(angle);  // +1 at noon, -1 at midnight
    };

    // Helper: horizon fog colour interpolated from time-of-day.
    auto calcFogColour = [](float tod) -> glm::vec3 {
        // Ground horizon slightly darker than sky - provides depth without being too dark
        const glm::vec3 night(0.01f, 0.01f, 0.05f);  // Slightly darker than sky
        const glm::vec3 dawn (0.30f, 0.25f, 0.45f);  // Slightly darker/warmer than sky
        const glm::vec3 day  (0.18f, 0.40f, 0.75f);  // Slightly darker than sky blue
        const glm::vec3 dusk (0.38f, 0.20f, 0.35f);  // Slightly darker than sky
        auto sm = [](float e0, float e1, float x) -> float {
            float t = glm::clamp((x - e0) / (e1 - e0), 0.0f, 1.0f);
            return t * t * (3.0f - 2.0f * t);
        };
        if (tod < 0.18f) return night;
        if (tod < 0.28f) return glm::mix(night, dawn,  sm(0.18f, 0.28f, tod));
        if (tod < 0.38f) return glm::mix(dawn,  day,   sm(0.28f, 0.38f, tod));
        if (tod < 0.62f) return day;
        if (tod < 0.72f) return glm::mix(day,   dusk,  sm(0.62f, 0.72f, tod));
        if (tod < 0.82f) return glm::mix(dusk,  night, sm(0.72f, 0.82f, tod));
        return night;
    };

    // ---- Render loop --------------------------------------------------------
    Uint64 lastTime = SDL_GetPerformanceCounter();
    const Uint64 perfFreq = SDL_GetPerformanceFrequency();
    bool shouldQuit = false;

    while (!shouldQuit)
    {
        const Uint64 now = SDL_GetPerformanceCounter();
        const float delta = static_cast<float>(now - lastTime) / static_cast<float>(perfFreq);
        lastTime = now;

        // Control mouse cursor and relative mouse mode based on dialog state
        bool dialogOpen = settingsDialog.isOpen();
        SDL_SetRelativeMouseMode(dialogOpen ? SDL_FALSE : SDL_TRUE);
        SDL_ShowCursor(dialogOpen ? SDL_ENABLE : SDL_DISABLE);

        // Poll SDL events
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            // Let ImGui process the event first
            imguiManager.processEvent(&event);

            if (event.type == SDL_QUIT)
            {
                shouldQuit = true;
            }
            else if (event.type == SDL_KEYDOWN)
            {
                // Always allow F11 and Alt+Enter, even if ImGui wants keyboard
                if (event.key.keysym.sym == SDLK_F11)
                {
                    // F11 toggles settings dialog
                    settingsDialog.toggle();
                }
                else if (event.key.keysym.sym == SDLK_RETURN && (event.key.keysym.mod & KMOD_ALT))
                {
                    // Alt+Enter toggles fullscreen (standard game shortcut)
                    Renderer::toggleFullscreen(window);
                }
                else if (event.key.keysym.sym == SDLK_c && !dialogOpen)
                {
                    // 'C' key toggles camera mode (when dialog closed)
                    if (cameraMode == CameraMode::FreeFly)
                    {
                        // Switch to Character mode
                        cameraMode = CameraMode::Character;
                        // Transfer position and orientation
                        charCamera.setPosition(flyCamera.position());
                        charCamera.setOrientation(flyCamera.yaw(), flyCamera.pitch());
                    }
                    else
                    {
                        // Switch to FreeFly mode
                        cameraMode = CameraMode::FreeFly;
                        // Transfer position and orientation
                        flyCamera.setPosition(charCamera.position());
                        flyCamera.setOrientation(charCamera.yaw(), charCamera.pitch());
                    }
                }
                else if (event.key.keysym.sym == SDLK_ESCAPE && !dialogOpen)
                {
                    // ESC quits only if dialog is closed (ESC within dialog is handled by ImGui)
                    shouldQuit = true;
                }
            }
        }

        // Handle world regeneration
        if (needsWorldRegeneration)
        {
            currentSettings = pendingSettings;
            world.reset();  // Destroy old world
            world = std::make_unique<Chunk::ChunkWorld>(currentSettings);
            needsWorldRegeneration = false;
        }

        // Only update camera when dialog is closed (pauses game)
        if (!dialogOpen)
        {
            if (cameraMode == CameraMode::FreeFly)
                flyCamera.update(window, delta);
            else
                charCamera.update(window, delta, world.get());
        }

        // Pause time progression when dialog is open
        if (!dialogOpen)
        {
            timeOfDay = std::fmod(timeOfDay + delta / kDayLength, 1.0f);
        }

        if (kRenderIsolationMode)
        {
            SDL_Delay(10);
            continue;
        }

        // Get active camera position
        glm::vec3 activePos = (cameraMode == CameraMode::FreeFly)
            ? flyCamera.position()
            : charCamera.position();

        // Load/evict chunks around the camera.
        if (world)
            world->update(activePos);

        // Viewport size (may change)
        int fbW, fbH;
        SDL_GL_GetDrawableSize(window, &fbW, &fbH);

        // Fullscreen/window transitions can briefly report a zero-sized framebuffer.
        // Skip rendering that frame and wait for a valid surface size.
        if (fbW <= 0 || fbH <= 0)
        {
            SDL_Delay(10);
            continue;
        }

        glViewport(0, 0, fbW, fbH);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const float aspect = (fbH > 0)
            ? static_cast<float>(fbW) / static_cast<float>(fbH)
            : 1.0f;

        const glm::mat4 proj    = glm::perspective(glm::radians(70.0f), aspect, 0.1f, 3000.0f);
        const glm::mat4 view    = (cameraMode == CameraMode::FreeFly)
            ? flyCamera.viewMatrix()
            : charCamera.viewMatrix();
        // For sky we need a view matrix without translation (pure rotation).
        const glm::mat4 viewRot = glm::mat4(glm::mat3(view));
        const glm::mat4 invVP   = glm::inverse(proj * viewRot);

        // ---- Sky pass (depth write OFF so it stays behind geometry) ----------
        glDepthMask(GL_FALSE);
        glDisable(GL_DEPTH_TEST);
        glUseProgram(rr.skyProg);
        glUniformMatrix4fv(rr.skyInvVPLoc, 1, GL_FALSE, glm::value_ptr(invVP));
        glUniform2f(rr.skyResLoc, static_cast<float>(fbW), static_cast<float>(fbH));
        glUniform1f(rr.skyTodLoc, timeOfDay);
        glBindVertexArray(rr.skyVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);
        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);

        // ---- Chunk pass -----------------------------------------------------
        const float sunHeight  = calcSunHeight(timeOfDay);
        const glm::vec3 fogCol = calcFogColour(timeOfDay);
        const float chunkSize  = static_cast<float>(Chunk::CHUNK_SIZE_X);

        // Dynamic fog based on render distance setting - prevents hard edges at custom distances
        const float renderDistChunks = static_cast<float>(currentSettings.renderDistance);
        const float fogEnd     = renderDistChunks * chunkSize;  // Match user's render distance
        const float fogStart   = fogEnd * 0.30f;  // Start at 30% - gentle fog onset

        const glm::vec3 camPos = activePos;  // Use active camera position

        // Check if camera is actually in water (not just below sea level - could be in a cave!)
        bool cameraInWater = false;
        if (world && camPos.y < currentSettings.seaLevel + 1.0f)
        {
            // Check the block at camera position
            Voxel::BlockID blockAtCamera = world->getBlockAt(camPos.x, camPos.y, camPos.z);
            cameraInWater = (blockAtCamera == Voxel::BlockID::Water);
        }

        glUseProgram(rr.shaderProg);
        glUniform1i(rr.texArrayLoc, 0);
        rr.texArray.bind(0);
        glUniform3f(rr.camPosLoc,   camPos.x, camPos.y, camPos.z);
        glUniform1f(rr.fogStartLoc, fogStart);
        glUniform1f(rr.fogEndLoc,   fogEnd);
        glUniform3f(rr.fogColLoc,   fogCol.r, fogCol.g, fogCol.b);
        glUniform1f(rr.seaLevelLoc, static_cast<float>(currentSettings.seaLevel));
        glUniform1f(rr.sunHeightLoc, sunHeight);
        glUniform1f(rr.timeLoc, static_cast<float>(SDL_GetTicks64()) / 1000.0f);
        glUniform1i(rr.inWaterLoc, cameraInWater ? 1 : 0);

        if (world)
            world->render(rr.mvpLoc, rr.chunkOffsetLoc, camPos, view, proj);

        // ---- ImGui overlay --------------------------------------------------
        imguiManager.newFrame();
        settingsDialog.render();

        // HUD overlay (camera mode and world position)
        if (!dialogOpen)
        {
            // Use DPI-aware scaling for UI elements
            ImGuiIO& io = ImGui::GetIO();
            float uiScale = io.DisplayFramebufferScale.x;  // DPI scale factor

            // Position in top-right corner with proper scaling
            float hudWidth = 280.0f;
            float hudPadding = 10.0f;
            ImGui::SetNextWindowPos(ImVec2(static_cast<float>(fbW) - hudWidth - hudPadding, hudPadding), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(hudWidth, 0.0f), ImGuiCond_Always);
            ImGui::Begin("HUD", nullptr,
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_AlwaysAutoResize |
                ImGuiWindowFlags_NoBackground);

            const char* modeText = (cameraMode == CameraMode::FreeFly)
                ? "Mode: Free-Fly (Noclip)"
                : "Mode: Character (Walk/Jump)";

            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "%s", modeText);
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Press 'C' to toggle");

            // World coordinates
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.2f, 1.0f), "Position:");
            ImGui::Text("X: %.1f", camPos.x);
            ImGui::Text("Y: %.1f", camPos.y);
            ImGui::Text("Z: %.1f", camPos.z);

            ImGui::End();
        }

        imguiManager.render();

        SDL_GL_SwapWindow(window);
    }

    // ---- Cleanup ------------------------------------------------------------
    imguiManager.shutdown();
    rr.texArray.destroy();
    if (rr.skyVAO) glDeleteVertexArrays(1, &rr.skyVAO);
    if (rr.skyProg) glDeleteProgram(rr.skyProg);
    if (rr.shaderProg) glDeleteProgram(rr.shaderProg);
    Renderer::destroyWindow(window);
    Renderer::shutdown();
    return 0;
}
