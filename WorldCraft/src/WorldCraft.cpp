// WorldCraft.cpp : Application entry point � Minecraft-style multi-chunk world.

#include <WorldCraft.h>
#include <Renderer/Shader.h>
#include <Renderer/Window.h>
#include <Renderer/Camera.h>
#include <Renderer/WireframeCube.h>
#include <Renderer/ItemEntityRenderer.h>
#include <Texture/BlockTextures.h>
#include <Chunk/ChunkWorld.h>
#include <WorldGen/TerrainGen.h>
#include <World/WaterSimulation.h>
#include <World/ItemEntityManager.h>
#include <UI/ImGuiManager.h>
#include <UI/SettingsDialog.h>
#include <UI/LoadingScreen.h>
#include <UI/ChaseCameraHUD.h>
#include <UI/AnalogClock.h>
#include <UI/GameMenu.h>
#include <UI/WorldSelectionDialog.h>
#include <UI/WorldNameDialog.h>
#include <UI/SaveConfirmationDialog.h>
#include <UI/Hotbar.h>
#include <UI/RadialMenu.h>
#include <UI/FabricatorUI.h>
#include <UI/InventoryUI.h>
#include <Inventory/Inventory.h>
#include <WorldGen/WorldSettings.h>
#include <Persistence/WorldPersistence.h>
#include <Utils/Raycast.h>
#include <imgui.h>
#include <thread>
#include <chrono>
#include <ctime>
#include <algorithm>
#include <set>
#include <tuple>

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
uniform float uTime;          // Absolute time for cloud animation

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

// Simple 2D noise for clouds
float hash(vec2 p)
{
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise2D(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);  // Smoothstep

    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));

    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

// Minecraft-style clouds: blocky, flat-bottomed
float minecraftClouds(vec3 rayDir, float time)
{
    // Only render clouds in upper hemisphere
    if (rayDir.y < 0.05) return 0.0;

    // Cloud layer height
    float cloudHeight = 120.0;

    // Ray-plane intersection to find where ray hits cloud layer
    float t = cloudHeight / rayDir.y;
    vec3 cloudPos = rayDir * t;

    // Horizontal position with slow drift
    vec2 cloudUV = cloudPos.xz * 0.008 + vec2(time * 0.005, time * 0.002);

    // Layered noise for blocky clouds
    float n = 0.0;
    n += noise2D(cloudUV * 2.0) * 0.5;
    n += noise2D(cloudUV * 4.0) * 0.25;
    n += noise2D(cloudUV * 8.0) * 0.125;

    // Threshold to create blocky appearance
    float cloudDensity = smoothstep(0.45, 0.55, n);

    // Fade out near horizon
    float horizonFade = smoothstep(0.05, 0.25, rayDir.y);

    return cloudDensity * horizonFade;
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

    // Minecraft-style clouds (only during day)
    float dayAmount = clamp(sd.y * 2.0, 0.0, 1.0);
    float cloudAmount = minecraftClouds(rayDir, uTime);
    vec3 cloudColor = vec3(1.0, 1.0, 1.0) * 0.95;  // Slightly off-white
    sky = mix(sky, cloudColor, cloudAmount * dayAmount);

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
layout(location = 4) in float aFaceType;  // 0=side/bottom, 1=top
layout(location = 5) in float aSkyLight;  // Sky light level 0-15

uniform mat4 uMVP;
uniform vec3 uCamPos;
uniform vec3 uChunkOffset;
uniform float uSunHeight;  // -1..1 from sky shader
uniform float uTime;
uniform bool uEnableWaves;  // Water wave toggle

out vec2  vUV;
out float vTexLayer;
out float vLight;
out float vFogDist;
out float vWorldY;
out float vCamY;
out vec3  vWorldPos;
out float vSkyLight;  // Pass through sky light

void main()
{
    vec3 worldPos = aPos + uChunkOffset;

    // Apply wave displacement to water blocks (texture layer 11)
    if (uEnableWaves && abs(aTexLayer - 11.0) < 0.1)
    {
        // Multi-frequency wave for natural ocean motion
        float wave1 = sin(worldPos.x * 0.5 + uTime * 1.2) * cos(worldPos.z * 0.4 + uTime * 0.8);
        float wave2 = sin(worldPos.x * 0.3 - uTime * 0.9) * sin(worldPos.z * 0.5 + uTime * 1.1);
        float wave3 = cos((worldPos.x + worldPos.z) * 0.2 + uTime * 0.6);

        // Combine waves with different amplitudes
        float waveHeight = (wave1 * 0.12 + wave2 * 0.08 + wave3 * 0.06);

        // Displace vertices that are part of the water surface:
        // - Top faces: aFaceType = 1.0
        // - Side face top edges: aFaceType = 0.5
        // - Side face bottom edges: aFaceType = 0.0 (not displaced)
        // - Bottom faces: aFaceType = 0.0 (not displaced)
        float shouldDisplace = step(0.4, aFaceType);  // 1.0 if aFaceType >= 0.5

        // Small offset prevents z-fighting with solid block tops
        worldPos.y += (waveHeight + 0.002) * shouldDisplace;
    }

    gl_Position   = uMVP * vec4(worldPos - uChunkOffset, 1.0);
    vUV           = aUV;
    vTexLayer     = aTexLayer;
    // Scale geometry light by sun height so world darkens at night.
    float sunFactor = clamp(uSunHeight * 2.0 + 0.15, 0.08, 1.0);
    vLight        = aLight * sunFactor;
    vFogDist      = length(worldPos - uCamPos);
    vWorldY       = worldPos.y;
    vCamY         = uCamPos.y;
    vWorldPos     = worldPos;
    vSkyLight     = aSkyLight;
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
in  float vSkyLight;  // Sky light level 0-15

uniform sampler2DArray uTexArray;
uniform float uFogStart;
uniform float uFogEnd;
uniform vec3  uFogColour;
uniform float uSeaLevel;
uniform float uTime;
uniform bool  uInWater;  // NEW: Is camera actually in water (not just below sea level)?
uniform vec3  uTorchPos;  // Character torch position
uniform bool  uTorchEnabled;  // Character torch on/off

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
    vec3 baseColour = tex.rgb;

    // Sky light brightness: 0-15 scale to 0.0-1.0
    // Very gentle curve to minimize dark patches
    float skyBrightness = pow(vSkyLight / 15.0, 1.0);  // Linear response (was 1.2)
    skyBrightness = clamp(skyBrightness, 0.20, 1.0);  // 20% minimum brightness (was 0.12)

    // Combine AO/face lighting with sky light
    vec3 ambient    = baseColour * 0.12;              // Higher ambient (was 0.08)
    vec3 diffuse    = baseColour * vLight * skyBrightness;  // AO + face + sky light
    vec3 lit        = clamp(ambient + diffuse, vec3(0.0), vec3(1.0));

    // Subtle gamma-space lift: raise lit to ~1/1.8 so mid-tones look richer.
    lit = pow(lit, vec3(1.0 / 1.8));

    // Character torch lighting (if enabled) - applied AFTER cave darkening
    if (uTorchEnabled)
    {
        float torchDist = length(vWorldPos - uTorchPos);
        float torchRadius = 10.0;  // Maximum light reach (reduced from 12)
        float torchAttenuation = clamp(1.0 - (torchDist / torchRadius), 0.0, 1.0);
        torchAttenuation = torchAttenuation * torchAttenuation * torchAttenuation;  // Cubic falloff for faster dropoff

        // Softer warm torch light - less intense, more natural
        vec3 torchColor = vec3(1.0, 0.7, 0.4) * 0.5;  // Reduced from 1.2 to 0.5
        lit += torchColor * torchAttenuation;
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
        GLint  enableWavesLoc = -1;  // Water wave toggle
        GLint  torchPosLoc = -1;  // Character torch position
        GLint  torchEnabledLoc = -1;  // Character torch on/off

        GLuint skyProg = 0;
        GLint  skyInvVPLoc = -1;
        GLint  skyResLoc = -1;
        GLint  skyTodLoc = -1;
        GLint  skyTimeLoc = -1;
        GLuint skyVAO = 0;

        Renderer::WireframeCube wireframeCube;  // Block highlight renderer
        Renderer::ItemEntityRenderer itemRenderer;  // Dropped item renderer
    };

    auto destroyRenderResources = [](RenderResources& rr)
    {
        rr.wireframeCube.cleanup();
        rr.itemRenderer.destroy();
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
        rr.enableWavesLoc = glGetUniformLocation(rr.shaderProg, "uEnableWaves");
        rr.torchPosLoc = glGetUniformLocation(rr.shaderProg, "uTorchPos");
        rr.torchEnabledLoc = glGetUniformLocation(rr.shaderProg, "uTorchEnabled");

        rr.skyProg = Renderer::buildProgram(kSkyVert, kSkyFragFinal);
        rr.skyInvVPLoc = glGetUniformLocation(rr.skyProg, "uInvViewProj");
        rr.skyResLoc = glGetUniformLocation(rr.skyProg, "uResolution");
        rr.skyTodLoc = glGetUniformLocation(rr.skyProg, "uTimeOfDay");
        rr.skyTimeLoc = glGetUniformLocation(rr.skyProg, "uTime");

        glGenVertexArrays(1, &rr.skyVAO);

        // Initialize wireframe cube for block highlighting
        rr.wireframeCube.init();

        // Initialize item entity renderer
        rr.itemRenderer.init();
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
    UI::LoadingScreen loadingScreen;
    UI::ChaseCameraHUD chaseCameraHUD;
    UI::AnalogClock analogClock;
    UI::GameMenu gameMenu;
    UI::WorldSelectionDialog worldSelectionDialog;
    UI::WorldNameDialog worldNameDialog;
    UI::SaveConfirmationDialog saveConfirmationDialog;
    UI::Hotbar hotbar;
    UI::RadialMenu radialMenu;
    UI::FabricatorUI fabricatorUI;
    UI::InventoryUI inventoryUI;
    bool isInitialLoadComplete = false;

    // ---- Inventory System ---------------------------------------------------
    Inventory::PlayerInventory playerInventory;

    // ---- Camera System ------------------------------------------------------
    // Dual camera mode: Free-fly (noclip) and Character (walking/jumping)
    enum class CameraMode { FreeFly, Character, Chase };
    CameraMode cameraMode = CameraMode::FreeFly;
    CameraMode previousCameraMode = CameraMode::FreeFly;  // For returning from Chase mode

    // ---- Build Mode System --------------------------------------------------
    // Toggle between placement and removal modes
    enum class BuildMode { Placement, Removal };
    BuildMode buildMode = BuildMode::Placement;

    Renderer::FlyCamera flyCamera({ 8.0f, 100.0f, 8.0f });
    Renderer::CharacterCamera charCamera({ 8.0f, 100.0f, 8.0f });
    Renderer::ChaseCamera chaseCamera({ 8.0f, 100.0f, 8.0f });

    flyCamera.init(window);
    charCamera.init(window);

    // ---- Persistence System -------------------------------------------------
    auto persistence = std::make_unique<Persistence::WorldPersistence>("saves");
    Persistence::WorldMetadata worldMetadata = Persistence::WorldMetadata::createDefault();

    // Check if we should load the last played world
    bool loadedLastWorld = false;
    WorldGen::WorldSettings defaultSettings = WorldGen::WorldSettings::createDefault();

    if (defaultSettings.loadLastWorldOnStartup)
    {
        // Try to load the most recently played world
        auto worlds = persistence->listWorlds();
        if (!worlds.empty())
        {
            // The worlds are already sorted by last played time (most recent first)
            const auto& lastWorld = worlds[0];
            std::cout << "Loading last played world: " << lastWorld.worldName << std::endl;

            if (persistence->loadWorld(lastWorld.worldName, worldMetadata))
            {
                loadedLastWorld = true;
                std::cout << "Successfully loaded world: " << lastWorld.worldName << std::endl;
            }
            else
            {
                std::cout << "Failed to load last world, creating new default world" << std::endl;
            }
        }
    }

    // If we didn't load a world, create a default one
    if (!loadedLastWorld)
    {
        worldMetadata.worldName = "WorldCraft_World";
        persistence->createWorld(worldMetadata);
    }

    // Restore camera from saved metadata
    cameraMode = static_cast<CameraMode>(worldMetadata.cameraMode);
    flyCamera.setPosition(worldMetadata.playerPosition);
    flyCamera.setYaw(worldMetadata.playerYaw);
    flyCamera.setPitch(worldMetadata.playerPitch);
    charCamera.setPosition(worldMetadata.playerPosition);
    charCamera.setYaw(worldMetadata.playerYaw);
    charCamera.setPitch(worldMetadata.playerPitch);

    // Restore inventory and blueprint state from saved metadata
    playerInventory.loadFromMetadata(worldMetadata);

    // ---- World --------------------------------------------------------------
    // Start with settings from metadata
    WorldGen::WorldSettings currentSettings = worldMetadata.settings;
    auto world = std::make_unique<Chunk::ChunkWorld>(currentSettings, persistence.get());

    // Sync loaded settings to the settings dialog UI
    settingsDialog.setSettings(currentSettings);

    // ---- Water Simulation ---------------------------------------------------
    World::WaterSimulation waterSim;
    float waterTickAccumulator = 0.0f;

    // ---- Item Entity Manager (dropped items) ----------------------------------
    World::ItemEntityManager itemManager;

    // Callback for when user generates a new world
    bool needsWorldRegeneration = false;
    WorldGen::WorldSettings pendingSettings;

    settingsDialog.setGenerateWorldCallback([&](const WorldGen::WorldSettings& newSettings) {
        pendingSettings = newSettings;
        needsWorldRegeneration = true;
        settingsDialog.hide();  // Close dialog after generating
    });

    // Callback for when user clicks "Go To Position"
    settingsDialog.setGoToPositionCallback([&](float x, float y, float z) {
        // Get current camera position based on active mode
        glm::vec3 currentPos;
        if (cameraMode == CameraMode::FreeFly)
            currentPos = flyCamera.position();
        else if (cameraMode == CameraMode::Character)
            currentPos = charCamera.position();
        else
            currentPos = chaseCamera.position();

        // Initialize chase camera from current position
        chaseCamera.setPosition(currentPos);
        chaseCamera.setTarget(glm::vec3(x, y, z));

        // Save previous mode to return to after chase completes
        if (cameraMode != CameraMode::Chase)
            previousCameraMode = cameraMode;

        // Switch to chase mode
        cameraMode = CameraMode::Chase;

        // Show chase camera HUD
        chaseCameraHUD.show();

        // Close settings dialog
        settingsDialog.hide();
    });

    // ---- Day/night ---------------------------------------------------------
    // One full day = 480 real seconds (8 minutes).  Change kDayLength to taste.
    static constexpr float kDayLength  = 480.0f;
    // Restore time from saved metadata
    float timeOfDay = worldMetadata.timeOfDay;
    bool timePaused = worldMetadata.timePaused;
    bool useLiveTime = worldMetadata.useLiveTime;

    // Set time settings callback
    settingsDialog.setTimeSettingsCallback([&](float newTime, bool paused, bool liveTime) {
        timeOfDay = newTime;
        timePaused = paused;
        useLiveTime = liveTime;
    });

    // Set save world callback
    settingsDialog.setSaveWorldCallback([&]() {
        int savedCount = world->saveModifiedChunks();

        // Always update metadata with current state from settings dialog
        currentSettings = settingsDialog.getSettings();
        worldMetadata.settings = currentSettings;
        worldMetadata.lastPlayedTime = std::time(nullptr);

        // Save camera position and orientation
        glm::vec3 camPos;
        if (cameraMode == CameraMode::FreeFly)
            camPos = flyCamera.position();
        else if (cameraMode == CameraMode::Character)
            camPos = charCamera.position();
        else
            camPos = chaseCamera.position();

        worldMetadata.playerPosition = camPos;
        worldMetadata.playerYaw = flyCamera.getYaw();
        worldMetadata.playerPitch = flyCamera.getPitch();
        worldMetadata.cameraMode = static_cast<int>(cameraMode);

        // Save time settings
        worldMetadata.timeOfDay = timeOfDay;
        worldMetadata.timePaused = timePaused;
        worldMetadata.useLiveTime = useLiveTime;

        // Save inventory and blueprint state
        playerInventory.saveToMetadata(worldMetadata);

        persistence->saveMetadata(worldMetadata);

        if (savedCount > 0)
        {
            std::cout << "Saved " << savedCount << " modified chunks to disk" << std::endl;
        }
        else
        {
            std::cout << "No modified chunks to save (metadata updated)" << std::endl;
        }
    });

    // Set game menu action callback
    gameMenu.setActionCallback([&](UI::MenuAction action) {
        switch (action)
        {
        case UI::MenuAction::Continue:
            gameMenu.hide();
            break;

        case UI::MenuAction::SaveWorld:
        {
            int savedCount = world->saveModifiedChunks();

            // Update metadata with current state from settings dialog
            currentSettings = settingsDialog.getSettings();
            worldMetadata.settings = currentSettings;
            worldMetadata.lastPlayedTime = std::time(nullptr);

            // Save camera position and orientation
            glm::vec3 camPos;
            if (cameraMode == CameraMode::FreeFly)
                camPos = flyCamera.position();
            else if (cameraMode == CameraMode::Character)
                camPos = charCamera.position();
            else
                camPos = chaseCamera.position();

            worldMetadata.playerPosition = camPos;
            worldMetadata.playerYaw = flyCamera.getYaw();  // Use fly camera's yaw as reference
            worldMetadata.playerPitch = flyCamera.getPitch();

            // Save camera mode
            worldMetadata.cameraMode = static_cast<int>(cameraMode);

            // Save time settings
            worldMetadata.timeOfDay = timeOfDay;
            worldMetadata.timePaused = timePaused;
            worldMetadata.useLiveTime = useLiveTime;

            // Save inventory and blueprint state
            playerInventory.saveToMetadata(worldMetadata);

            persistence->saveMetadata(worldMetadata);

            // Show confirmation dialog
            saveConfirmationDialog.show(savedCount);

            if (savedCount > 0)
            {
                std::cout << "[Menu] Saved " << savedCount << " modified chunks" << std::endl;
            }
            else
            {
                std::cout << "[Menu] No modified chunks to save (world saved with current settings)" << std::endl;
            }
            gameMenu.hide();
            break;
        }

        case UI::MenuAction::CreateNewWorld:
        {
            gameMenu.hide();
            // Get list of existing world names for validation
            auto worlds = persistence->listWorlds();
            std::vector<std::string> worldNames;
            for (const auto& w : worlds)
                worldNames.push_back(w.worldName);
            worldNameDialog.setExistingWorldNames(worldNames);

            // Generate a unique suggested name
            std::string suggestedName = "New World";
            int counter = 1;
            while (std::find(worldNames.begin(), worldNames.end(), suggestedName) != worldNames.end())
            {
                suggestedName = "New World " + std::to_string(counter);
                counter++;
            }

            worldNameDialog.show(suggestedName);
            break;
        }

        case UI::MenuAction::LoadWorld:
        {
            gameMenu.hide();
            worldSelectionDialog.setPersistence(persistence.get());
            worldSelectionDialog.refreshWorldList();
            worldSelectionDialog.show();
            break;
        }
        }
    });

    // Set world name dialog callback (for creating new world)
    worldNameDialog.setConfirmCallback([&](const std::string& worldName) {
        std::cout << "[WorldName] Creating new world: " << worldName << std::endl;

        // Create new world with the specified name
        Persistence::WorldMetadata newMetadata = Persistence::WorldMetadata::createDefault(worldName);
        newMetadata.settings = currentSettings;  // Use current settings as default

        if (persistence->createWorld(newMetadata))
        {
            worldMetadata = newMetadata;
            std::cout << "[WorldName] World created successfully" << std::endl;

            // Open settings dialog to let user configure world generation
            settingsDialog.show();
        }
        else
        {
            std::cout << "[WorldName] Failed to create world" << std::endl;
        }
    });

    // Set world selection dialog callback (for loading existing world)
    worldSelectionDialog.setLoadWorldCallback([&](const std::string& worldName) {
        std::cout << "[WorldSelection] Loading world: " << worldName << std::endl;

        bool isReloadingCurrent = (persistence->getCurrentWorldName() == worldName);

        // Save current world before switching/reloading
        if (persistence->isWorldOpen())
        {
            int savedCount = world->saveModifiedChunks();
            if (savedCount > 0 || isReloadingCurrent)
            {
                // Save current camera state
                glm::vec3 camPos;
                if (cameraMode == CameraMode::FreeFly)
                    camPos = flyCamera.position();
                else if (cameraMode == CameraMode::Character)
                    camPos = charCamera.position();
                else
                    camPos = chaseCamera.position();

                worldMetadata.playerPosition = camPos;
                worldMetadata.playerYaw = flyCamera.getYaw();
                worldMetadata.playerPitch = flyCamera.getPitch();
                worldMetadata.cameraMode = static_cast<int>(cameraMode);
                worldMetadata.timeOfDay = timeOfDay;
                worldMetadata.timePaused = timePaused;
                worldMetadata.useLiveTime = useLiveTime;
                worldMetadata.lastPlayedTime = std::time(nullptr);

                // Save current settings from dialog
                currentSettings = settingsDialog.getSettings();
                worldMetadata.settings = currentSettings;

                // Save inventory and blueprint state
                playerInventory.saveToMetadata(worldMetadata);

                persistence->saveMetadata(worldMetadata);

                if (isReloadingCurrent)
                {
                    std::cout << "[WorldSelection] Saved current state before reload" << std::endl;
                }
                else
                {
                    std::cout << "[WorldSelection] Saved " << savedCount << " chunks from previous world" << std::endl;
                }
            }
        }

        // Load the selected world
        if (persistence->loadWorld(worldName, worldMetadata))
        {
            std::cout << "[WorldSelection] Loaded world metadata" << std::endl;

            // Apply loaded settings
            currentSettings = worldMetadata.settings;
            settingsDialog.setSettings(currentSettings);

            // Restore camera position and mode
            cameraMode = static_cast<CameraMode>(worldMetadata.cameraMode);
            flyCamera.setPosition(worldMetadata.playerPosition);
            flyCamera.setYaw(worldMetadata.playerYaw);
            flyCamera.setPitch(worldMetadata.playerPitch);
            charCamera.setPosition(worldMetadata.playerPosition);
            charCamera.setYaw(worldMetadata.playerYaw);
            charCamera.setPitch(worldMetadata.playerPitch);

            std::cout << "[WorldSelection] Restored camera: pos(" 
                      << worldMetadata.playerPosition.x << ", "
                      << worldMetadata.playerPosition.y << ", "
                      << worldMetadata.playerPosition.z << ") mode=" 
                      << worldMetadata.cameraMode << std::endl;

            // Trigger world regeneration
            pendingSettings = currentSettings;
            needsWorldRegeneration = true;

            // Restore time settings
            timeOfDay = worldMetadata.timeOfDay;
            timePaused = worldMetadata.timePaused;
            useLiveTime = worldMetadata.useLiveTime;
            settingsDialog.setCurrentTime(timeOfDay, timePaused, useLiveTime);

            // Restore inventory and blueprint state
            playerInventory.loadFromMetadata(worldMetadata);
        }
        else
        {
            std::cout << "[WorldSelection] Failed to load world" << std::endl;
        }
    });

    // Initialize settings dialog with current time
    settingsDialog.setCurrentTime(timeOfDay, timePaused, useLiveTime);

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
    bool characterTorchEnabled = false;  // Toggle for character's torch light

    // Auto-save system
    float autoSaveTimer = 0.0f;
    constexpr float AUTO_SAVE_INTERVAL = 300.0f;  // Auto-save every 5 minutes (300 seconds)

    while (!shouldQuit)
    {
        const Uint64 now = SDL_GetPerformanceCounter();
        const float delta = static_cast<float>(now - lastTime) / static_cast<float>(perfFreq);
        lastTime = now;

        // Check if initial loading is complete
        if (!isInitialLoadComplete && world->isInitialLoadComplete())
        {
            isInitialLoadComplete = true;
            loadingScreen.setActive(false);
        }

        // Control mouse cursor and relative mouse mode based on dialog/menu state or loading
        bool dialogOpen = settingsDialog.isOpen() || worldSelectionDialog.isOpen() || worldNameDialog.isOpen() || saveConfirmationDialog.isOpen();
        bool menuOpen = gameMenu.isVisible();
        bool fabricatorOpen = fabricatorUI.isOpen();
        bool inventoryOpen = inventoryUI.isOpen();
        bool radialOpen = radialMenu.isVisible();
        bool loading = loadingScreen.isActive();
        bool uiOpen = dialogOpen || menuOpen || radialOpen || loading || fabricatorOpen || inventoryOpen;
        SDL_SetRelativeMouseMode(uiOpen ? SDL_FALSE : SDL_TRUE);
        SDL_ShowCursor(uiOpen ? SDL_ENABLE : SDL_DISABLE);

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
                // Block all keyboard input during loading (except ESC to quit)
                if (loading)
                {
                    if (event.key.keysym.sym == SDLK_ESCAPE)
                    {
                        shouldQuit = true;
                    }
                    continue;
                }

                // Always allow F10, F11 and Alt+Enter, even if ImGui wants keyboard
                if (event.key.keysym.sym == SDLK_F10)
                {
                    // F10 toggles game menu
                    gameMenu.toggle();
                }
                else if (event.key.keysym.sym == SDLK_F11)
                {
                    // F11 toggles settings dialog
                    settingsDialog.toggle();
                }
                else if (event.key.keysym.sym == SDLK_i && !dialogOpen && !menuOpen && !loading)
                {
                    // 'I' key toggles inventory UI (works even when inventory is open to close it)
                    inventoryUI.toggle();
                }
                else if (event.key.keysym.sym == SDLK_e && !dialogOpen && !menuOpen && !inventoryOpen && !fabricatorOpen && !loading)
                {
                    // 'E' key toggles build mode (Placement/Removal) - blocked during dialogs and UI screens
                    if (buildMode == BuildMode::Placement)
                    {
                        buildMode = BuildMode::Removal;
                    }
                    else
                    {
                        buildMode = BuildMode::Placement;
                    }
                }
                else if (event.key.keysym.sym == SDLK_RETURN && (event.key.keysym.mod & KMOD_ALT))
                {
                    // Alt+Enter toggles fullscreen (standard game shortcut)
                    Renderer::toggleFullscreen(window);
                }
                else if (event.key.keysym.sym == SDLK_c && !dialogOpen && !menuOpen && !inventoryOpen && !fabricatorOpen && !loading)
                {
                    // 'C' key toggles camera mode - blocked during dialogs and UI screens
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
                else if (event.key.keysym.sym == SDLK_t && !dialogOpen && !menuOpen && !inventoryOpen && !fabricatorOpen && !loading)
                {
                    // 'T' key toggles character torch light - blocked during dialogs and UI screens
                    characterTorchEnabled = !characterTorchEnabled;
                }
                else if (event.key.keysym.sym == SDLK_q && !dialogOpen && !menuOpen && !inventoryOpen && !fabricatorOpen && !loading)
                {
                    // 'Q' key opens radial menu (hold to use) - blocked during dialogs and UI screens
                    if (!radialMenu.isVisible())
                    {
                        radialMenu.show();
                    }
                }
                // Hotbar number keys (1-9) - blocked during dialogs and UI screens
                else if (event.key.keysym.sym >= SDLK_1 && event.key.keysym.sym <= SDLK_9 && !dialogOpen && !menuOpen && !inventoryOpen && !fabricatorOpen)
                {
                    int slot = event.key.keysym.sym - SDLK_1;  // 0-8
                    playerInventory.setSelectedSlot(slot);
                }
                else if (event.key.keysym.sym == SDLK_ESCAPE)
                {
                    // ESC priority: inventory -> fabricator -> menu -> quit
                    if (inventoryOpen)
                    {
                        inventoryUI.close();
                    }
                    else if (fabricatorOpen)
                    {
                        fabricatorUI.close();
                    }
                    else if (!dialogOpen && gameMenu.isVisible())
                    {
                        gameMenu.hide();
                    }
                    else if (!dialogOpen)
                    {
                        shouldQuit = true;
                    }
                }
                else if (event.key.keysym.sym == SDLK_u && !dialogOpen && !menuOpen && !inventoryOpen && !fabricatorOpen && !loading)
                {
                    // DEBUG: U key - Expand inventory (test feature) - blocked during dialogs and UI screens
                    playerInventory.expandInventory(5);
                }
                else if (event.key.keysym.sym == SDLK_b && !dialogOpen && !menuOpen && !loading)
                {
                    // 'B' key toggles blueprint crafting UI
                    fabricatorUI.toggle();
                }
                else if (gameMenu.isVisible())
                {
                    // Handle menu navigation
                    gameMenu.handleKeyPress(event.key.keysym.sym);
                }
            }
            else if (event.type == SDL_KEYUP)
            {
                // Q key release - close radial menu and select hovered material
                if (event.key.keysym.sym == SDLK_q && radialMenu.isVisible())
                {
                    radialMenu.selectHoveredMaterial(playerInventory);
                    radialMenu.hide();
                }
            }
            else if (event.type == SDL_MOUSEMOTION && gameMenu.isVisible())
            {
                // Handle mouse movement for menu hover
                gameMenu.handleMouseMove(static_cast<float>(event.motion.x), static_cast<float>(event.motion.y));
            }
            else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT && gameMenu.isVisible())
            {
                // Handle mouse click for menu selection
                gameMenu.handleMouseClick(static_cast<float>(event.button.x), static_cast<float>(event.button.y));
            }
        }

        // Handle world regeneration
        if (needsWorldRegeneration)
        {
            currentSettings = pendingSettings;
            world.reset();  // Destroy old world
            world = std::make_unique<Chunk::ChunkWorld>(currentSettings, persistence.get());
            waterSim.clear();  // Clear water simulation state
            itemManager.clear();  // Clear dropped items
            waterTickAccumulator = 0.0f;
            needsWorldRegeneration = false;

            // Reset loading screen for new world
            isInitialLoadComplete = false;
            loadingScreen.setActive(true);
        }

        // Only update camera when not loading and (dialog/menu is closed OR chase mode is active)
        if (!loading && !menuOpen && !radialOpen && (!dialogOpen || cameraMode == CameraMode::Chase))
        {
            if (cameraMode == CameraMode::FreeFly)
                flyCamera.update(window, delta);
            else if (cameraMode == CameraMode::Character)
                charCamera.update(window, delta, world.get());
            else if (cameraMode == CameraMode::Chase)
            {
                // Update chase camera, check if it reached target
                bool stillMoving = chaseCamera.update(window, delta, world.get(), currentSettings.seaLevel);
                if (!stillMoving)
                {
                    // Chase complete - return to previous camera mode
                    glm::vec3 finalPos = chaseCamera.position();

                    if (previousCameraMode == CameraMode::FreeFly)
                    {
                        flyCamera.setPosition(finalPos);
                        flyCamera.setOrientation(chaseCamera.yaw(), chaseCamera.pitch());
                    }
                    else if (previousCameraMode == CameraMode::Character)
                    {
                        charCamera.setPosition(finalPos);
                        charCamera.setOrientation(chaseCamera.yaw(), chaseCamera.pitch());
                    }

                    cameraMode = previousCameraMode;

                    // Hide chase camera HUD
                    chaseCameraHUD.hide();
                }
            }
        }

        // ---- Block targeting and removal (Character camera only) ----------------
        std::optional<Utils::RaycastHit> targetBlock;
        if (!loading && !dialogOpen && !radialOpen && cameraMode == CameraMode::Character && world)
        {
            // Perform raycast from camera eye position along look direction
            glm::vec3 rayOrigin = charCamera.position();
            glm::vec3 rayDir = charCamera.forward();
            constexpr float maxReach = 5.0f;  // Minecraft-style reach distance

            // Raycast through voxels to find target block
            targetBlock = Utils::raycastVoxel(rayOrigin, rayDir, maxReach,
                [&world](int x, int y, int z) -> bool {
                    // Check if this voxel is solid (not air/water)
                    return world->isBlockSolid(static_cast<float>(x),
                                               static_cast<float>(y),
                                               static_cast<float>(z));
                });

            // Handle mouse buttons based on build mode
            Uint32 mouseState = SDL_GetMouseState(nullptr, nullptr);
            static bool wasLeftPressed = false;
            bool isLeftPressed = (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;

            // In Removal mode: left mouse button removes blocks
            if (buildMode == BuildMode::Removal && isLeftPressed && !wasLeftPressed && targetBlock.has_value())
            {
                // Remove the targeted block (set to air)
                const glm::ivec3& blockPos = targetBlock->blockPos;

                // Prevent removal of bedrock layer at Y=0 (indestructible bottom)
                if (blockPos.y == 0)
                {
                    wasLeftPressed = isLeftPressed;
                    continue;
                }

                // Check if we're removing water (need to clean up flow)
                Voxel::BlockID removedBlock = world->getBlockAt(
                    static_cast<float>(blockPos.x),
                    static_cast<float>(blockPos.y),
                    static_cast<float>(blockPos.z));
                bool wasWater = (removedBlock == Voxel::BlockID::Water);

                world->setBlockAt(static_cast<float>(blockPos.x),
                                 static_cast<float>(blockPos.y),
                                 static_cast<float>(blockPos.z),
                                 Voxel::BlockID::Air);

                // Spawn item entity for the removed block (Minecraft-style pickup)
                // Don't spawn items for air or water
                if (removedBlock != Voxel::BlockID::Air && removedBlock != Voxel::BlockID::Water)
                {
                    glm::vec3 spawnPos(
                        static_cast<float>(blockPos.x) + 0.5f,
                        static_cast<float>(blockPos.y) + 0.5f,
                        static_cast<float>(blockPos.z) + 0.5f
                    );
                    itemManager.spawnItem(spawnPos, removedBlock, 1);
                }

                // If we removed water, clean up its source and flow
                if (wasWater && currentSettings.enableWaterFlow)
                {
                    waterSim.removeSource(blockPos.x, blockPos.y, blockPos.z, world.get());
                }

                // Notify water simulation that a block changed - nearby water may need to flow
                if (currentSettings.enableWaterFlow)
                {
                    // When a block is removed, scan for ALL nearby water blocks
                    // (including world-generated lakes/oceans) and register them as sources
                    // This ensures large bodies of water can flow when terrain changes

                    // Scan a 7x7x7 cube centered on the removed block
                    const int SCAN_RADIUS = 3;

                    for (int dy = -SCAN_RADIUS; dy <= SCAN_RADIUS; ++dy)
                    {
                        for (int dx = -SCAN_RADIUS; dx <= SCAN_RADIUS; ++dx)
                        {
                            for (int dz = -SCAN_RADIUS; dz <= SCAN_RADIUS; ++dz)
                            {
                                int checkX = blockPos.x + dx;
                                int checkY = blockPos.y + dy;
                                int checkZ = blockPos.z + dz;

                                Voxel::BlockID block = world->getBlockAt(
                                    static_cast<float>(checkX),
                                    static_cast<float>(checkY),
                                    static_cast<float>(checkZ)
                                );

                                // If we find water and it doesn't already have a source, register it
                                if (block == Voxel::BlockID::Water && !waterSim.hasSource(checkX, checkY, checkZ))
                                {
                                    waterSim.registerSource(checkX, checkY, checkZ);
                                }
                            }
                        }
                    }

                    waterSim.notifyBlockChange(blockPos.x, blockPos.y, blockPos.z);

                    // Force an immediate water update after block removal to start flow instantly
                    glm::vec3 playerPos;
                    if (cameraMode == CameraMode::FreeFly)
                        playerPos = flyCamera.position();
                    else if (cameraMode == CameraMode::Character)
                        playerPos = charCamera.position();
                    else
                        playerPos = chaseCamera.position();

                    waterSim.update(world.get(), playerPos, 64.0f);
                }
            }
            // In Placement mode: left mouse button places blocks
            else if (buildMode == BuildMode::Placement && isLeftPressed && !wasLeftPressed && targetBlock.has_value())
            {
                // Place block adjacent to targeted block (in the direction of the hit face)
                const glm::ivec3& hitBlock = targetBlock->blockPos;
                Voxel::FaceDir hitFace = targetBlock->face;

                // Calculate placement position using face offsets
                glm::ivec3 placePos = hitBlock;
                int faceIdx = static_cast<int>(hitFace);
                placePos.x += Voxel::FaceOffsetX[faceIdx];
                placePos.y += Voxel::FaceOffsetY[faceIdx];
                placePos.z += Voxel::FaceOffsetZ[faceIdx];

                // Don't place blocks in the same position as the player
                glm::vec3 playerPos = charCamera.position();
                glm::ivec3 playerBlockPos(
                    static_cast<int>(std::floor(playerPos.x)),
                    static_cast<int>(std::floor(playerPos.y)),
                    static_cast<int>(std::floor(playerPos.z))
                );

                // Check if placement would overlap player (current block or block above)
                bool wouldOverlapPlayer = (placePos == playerBlockPos) ||
                                         (placePos == playerBlockPos + glm::ivec3(0, 1, 0));

                if (!wouldOverlapPlayer)
                {
                    // Get selected block from inventory
                    Voxel::BlockID selectedBlock = playerInventory.getSelectedBlockID();

                    // Check if player has items in inventory (creative mode: always allow)
                    // For now: only place if we have items in the selected slot
                    const Inventory::InventorySlot& selectedSlot = playerInventory.getSlot(playerInventory.getSelectedSlot());

                    if (!selectedSlot.isEmpty())
                    {
                        // Place the block
                        world->setBlockAt(static_cast<float>(placePos.x),
                                         static_cast<float>(placePos.y),
                                         static_cast<float>(placePos.z),
                                         selectedBlock);

                        // Consume one item from inventory
                        playerInventory.consumeSelectedItem(1);

                        // If placing water, register it as a source
                        if (selectedBlock == Voxel::BlockID::Water)
                        {
                            waterSim.registerSource(placePos.x, placePos.y, placePos.z);
                        }
                    }
                }
            }

            wasLeftPressed = isLeftPressed;
        }

        // ---- Water Simulation Tick ----------------------------------------------
        if (currentSettings.enableWaterFlow && !dialogOpen && world)
        {
            waterTickAccumulator += delta;
            float tickInterval = 1.0f / currentSettings.waterFlowRate;

            if (waterTickAccumulator >= tickInterval)
            {
                waterTickAccumulator -= tickInterval;

                // Run water simulation update (within player range)
                glm::vec3 playerPos;
                if (cameraMode == CameraMode::FreeFly)
                    playerPos = flyCamera.position();
                else if (cameraMode == CameraMode::Character)
                    playerPos = charCamera.position();
                else
                    playerPos = chaseCamera.position();

                // Scale discovery rate with flow rate setting
                // Higher flow rate = discover more sources per tick
                // Range: 0.1 to 2.0 → 1 to 20 sources per tick
                int maxNewSources = static_cast<int>(currentSettings.waterFlowRate * 10.0f);
                waterSim.update(world.get(), playerPos, 64.0f, maxNewSources); // Update water within 64 blocks
            }
        }

        // ---- Item Entity Updates (dropped items) ---------------------------------
        if (!dialogOpen && world)
        {
            glm::vec3 playerPos;
            if (cameraMode == CameraMode::FreeFly)
                playerPos = flyCamera.position();
            else if (cameraMode == CameraMode::Character)
                playerPos = charCamera.position();
            else
                playerPos = chaseCamera.position();

            // Update all dropped items (physics, collection, despawn)
            // When items are collected, add them to inventory
            itemManager.update(delta, playerPos, 2.5f,
                // Collision check callback: returns true if the position is solid
                [&](float x, float y, float z) -> bool {
                    if (!world) return false;
                    Voxel::BlockID block = world->getBlockAt(x, y, z);
                    return Voxel::BlockRegistry::get().isSolid(block);
                },
                // Collection callback: add picked up items to inventory
                [&](Voxel::BlockID blockType, int stackCount) {
                    playerInventory.addPickedUpItem(blockType, stackCount);
                });
        }

        // Time progression control
        if (!dialogOpen && !loading)
        {
            if (useLiveTime)
            {
                // Use real-world local time
                auto now = std::chrono::system_clock::now();
                auto now_time_t = std::chrono::system_clock::to_time_t(now);
                std::tm local_tm;
                #ifdef _WIN32
                    localtime_s(&local_tm, &now_time_t);
                #else
                    localtime_r(&now_time_t, &local_tm);
                #endif

                float hours = static_cast<float>(local_tm.tm_hour);
                float minutes = static_cast<float>(local_tm.tm_min);
                float seconds = static_cast<float>(local_tm.tm_sec);

                // Convert to 0.0-1.0 range (24 hours = 1.0)
                timeOfDay = (hours + minutes / 60.0f + seconds / 3600.0f) / 24.0f;

                // Update the settings dialog
                settingsDialog.setCurrentTime(timeOfDay, timePaused, useLiveTime);
            }
            else if (!timePaused)
            {
                // Normal game time progression
                timeOfDay = std::fmod(timeOfDay + delta / kDayLength, 1.0f);

                // Update the settings dialog periodically
                settingsDialog.setCurrentTime(timeOfDay, timePaused, useLiveTime);
            }
        }

        // Auto-save system - save modified chunks periodically
        if (!loading && currentSettings.enableAutoSave)
        {
            autoSaveTimer += delta;
            if (autoSaveTimer >= AUTO_SAVE_INTERVAL)
            {
                autoSaveTimer = 0.0f;
                int modifiedCount = world->getModifiedChunkCount();
                if (modifiedCount > 0)
                {
                    int savedCount = world->saveModifiedChunks();
                    if (savedCount > 0)
                    {
                        // Update metadata with current state from settings dialog
                        currentSettings = settingsDialog.getSettings();
                        worldMetadata.settings = currentSettings;
                        worldMetadata.lastPlayedTime = std::time(nullptr);

                        // Save camera state
                        glm::vec3 camPos;
                        if (cameraMode == CameraMode::FreeFly)
                            camPos = flyCamera.position();
                        else if (cameraMode == CameraMode::Character)
                            camPos = charCamera.position();
                        else
                            camPos = chaseCamera.position();

                        worldMetadata.playerPosition = camPos;
                        worldMetadata.playerYaw = flyCamera.getYaw();
                        worldMetadata.playerPitch = flyCamera.getPitch();
                        worldMetadata.cameraMode = static_cast<int>(cameraMode);
                        worldMetadata.timeOfDay = timeOfDay;
                        worldMetadata.timePaused = timePaused;
                        worldMetadata.useLiveTime = useLiveTime;

                        // Save inventory and blueprint state
                        playerInventory.saveToMetadata(worldMetadata);

                        persistence->saveMetadata(worldMetadata);

                        std::cout << "[Auto-save] Saved " << savedCount << " modified chunks" << std::endl;
                    }
                }
            }
        }

        if (kRenderIsolationMode)
        {
            SDL_Delay(10);
            continue;
        }

        // Get active camera position
        glm::vec3 activePos;
        if (cameraMode == CameraMode::FreeFly)
            activePos = flyCamera.position();
        else if (cameraMode == CameraMode::Character)
            activePos = charCamera.position();
        else
            activePos = chaseCamera.position();

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

        // Update radial menu if visible (needs fbW, fbH)
        if (radialMenu.isVisible())
        {
            int mouseX, mouseY;
            SDL_GetMouseState(&mouseX, &mouseY);
            radialMenu.update(playerInventory, static_cast<float>(mouseX), static_cast<float>(mouseY), fbW, fbH);
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const float aspect = (fbH > 0)
            ? static_cast<float>(fbW) / static_cast<float>(fbH)
            : 1.0f;

        const glm::mat4 proj    = glm::perspective(glm::radians(70.0f), aspect, 0.1f, 3000.0f);
        glm::mat4 view;
        if (cameraMode == CameraMode::FreeFly)
            view = flyCamera.viewMatrix();
        else if (cameraMode == CameraMode::Character)
            view = charCamera.viewMatrix();
        else
            view = chaseCamera.viewMatrix();
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
        glUniform1f(rr.skyTimeLoc, static_cast<float>(SDL_GetTicks64()) / 1000.0f);
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
        glUniform1i(rr.enableWavesLoc, currentSettings.enableWaterWaves ? 1 : 0);
        glUniform3f(rr.torchPosLoc, camPos.x, camPos.y, camPos.z);  // Torch at camera position
        glUniform1i(rr.torchEnabledLoc, characterTorchEnabled ? 1 : 0);

        if (world)
            world->render(rr.mvpLoc, rr.chunkOffsetLoc, camPos, view, proj);

        // ---- Item entities (dropped blocks) -------------------------------------
        // Render all active item entities as small textured cubes
        rr.itemRenderer.beginBatch(view, proj);
        for (const auto& item : itemManager.getItems())
        {
            rr.itemRenderer.renderItemBatch(
                item->getPosition(),
                item->getBlockType(),
                item->getRotation(),
                0.25f  // Small scale for dropped items
            );
        }
        rr.itemRenderer.endBatch();

        // ---- Block highlight (wireframe cube) -----------------------------------
        if (targetBlock.has_value())
        {
            const glm::mat4 mvp = proj * view;

            // In Removal mode: show white highlight on target block
            if (buildMode == BuildMode::Removal)
            {
                const glm::vec4 highlightColor(1.0f, 1.0f, 1.0f, 0.8f);  // White, more opaque
                rr.wireframeCube.render(targetBlock->blockPos, mvp, highlightColor);
            }
            // In Placement mode: show ghost block preview
            else if (buildMode == BuildMode::Placement && !dialogOpen)
            {
                // Calculate placement position using face offsets
                glm::ivec3 placePos = targetBlock->blockPos;
                int faceIdx = static_cast<int>(targetBlock->face);
                placePos.x += Voxel::FaceOffsetX[faceIdx];
                placePos.y += Voxel::FaceOffsetY[faceIdx];
                placePos.z += Voxel::FaceOffsetZ[faceIdx];

                // Check if placement is valid (not overlapping player)
                glm::vec3 playerPos = charCamera.position();
                glm::ivec3 playerBlockPos(
                    static_cast<int>(std::floor(playerPos.x)),
                    static_cast<int>(std::floor(playerPos.y)),
                    static_cast<int>(std::floor(playerPos.z))
                );

                bool wouldOverlapPlayer = (placePos == playerBlockPos) ||
                                         (placePos == playerBlockPos + glm::ivec3(0, 1, 0));

                // Render preview with color indicating validity
                if (wouldOverlapPlayer)
                {
                    // Red ghost for invalid placement
                    const glm::vec4 invalidColor(1.0f, 0.0f, 0.0f, 0.6f);
                    rr.wireframeCube.render(placePos, mvp, invalidColor);
                }
                else
                {
                    // Green ghost for valid placement
                    const glm::vec4 validColor(0.0f, 1.0f, 0.0f, 0.6f);
                    rr.wireframeCube.render(placePos, mvp, validColor);
                }
            }
        }

        // ---- ImGui overlay --------------------------------------------------
        imguiManager.newFrame();

        // Show loading screen if still loading
        if (loading)
        {
            float progress = world->getLoadingProgress();
            int loadedChunks = world->getLoadedChunkCount();
            int targetChunks = world->getTargetChunkCount();
            loadingScreen.render(progress, loadedChunks, targetChunks);
        }

        // Update settings dialog with modified chunks count
        settingsDialog.setModifiedChunksCount(world->getModifiedChunkCount());
        settingsDialog.render();

        // Render world management dialogs
        worldSelectionDialog.render(persistence.get());
        worldNameDialog.render();
        saveConfirmationDialog.render();

        // Render inventory UI (I key)
        inventoryUI.render(playerInventory);

        // Render fabricator UI (blueprint crafting - B key)
        fabricatorUI.render(playerInventory);

        // Chase camera HUD - show when in chase mode
        if (cameraMode == CameraMode::Chase && chaseCameraHUD.isVisible())
        {
            chaseCameraHUD.render(
                chaseCamera.position(),
                chaseCamera.getTarget(),
                chaseCamera.getCurrentSpeed(),
                chaseCamera.getDistanceToTarget()
            );
        }

        // HUD overlay (camera mode and world position) - only when not loading
        if (!loading && !dialogOpen)
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

            // Modified chunks count (unsaved changes)
            ImGui::Separator();
            int modCount = world->getModifiedChunkCount();
            if (modCount > 0)
            {
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.2f, 1.0f), "Unsaved: %d chunk%s", modCount, modCount == 1 ? "" : "s");
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Press F10 to save");
            }
            else
            {
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No unsaved changes");
            }

            ImGui::End();
        }

        // Render analog clock in top-left (hidden when inventory or fabricator is open)
        if (!inventoryOpen && !fabricatorOpen)
        {
            analogClock.render(timeOfDay, fbW, fbH);
        }

        // Render hotbar (always visible except during loading)
        if (!loading)
        {
            hotbar.render(playerInventory, fbW, fbH);

            // Render build mode indicator above hotbar
            ImGui::SetNextWindowPos(ImVec2(fbW * 0.5f, fbH - 120), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowBgAlpha(0.0f); // Transparent background
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

            ImGui::Begin("BuildModeIndicator", nullptr, 
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | 
                ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize);

            // Mode text with color coding
            if (buildMode == BuildMode::Placement)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f)); // Green
                ImGui::Text("MODE: PLACEMENT");
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f)); // Red
                ImGui::Text("MODE: REMOVAL");
            }
            ImGui::PopStyleColor();

            ImGui::End();
            ImGui::PopStyleVar(2);
        }

        // Render radial menu (if visible) - on top of hotbar
        if (radialMenu.isVisible())
        {
            radialMenu.render(fbW, fbH);
        }

        // Render game menu (if visible) - should be on top of everything
        if (gameMenu.isVisible())
        {
            gameMenu.setModifiedChunksCount(world->getModifiedChunkCount());
            gameMenu.render(delta, fbW, fbH);
        }

        imguiManager.render();

        SDL_GL_SwapWindow(window);
    }

    // ---- Cleanup ------------------------------------------------------------
    // Final save on exit
    std::cout << "Saving world before exit..." << std::endl;
    int finalSaveCount = world->saveModifiedChunks();
    if (finalSaveCount > 0)
    {
        // Update metadata with current state from settings dialog
        currentSettings = settingsDialog.getSettings();
        worldMetadata.settings = currentSettings;
        worldMetadata.lastPlayedTime = std::time(nullptr);

        // Save inventory and blueprint state
        playerInventory.saveToMetadata(worldMetadata);

        persistence->saveMetadata(worldMetadata);
        std::cout << "Final save: " << finalSaveCount << " chunks saved" << std::endl;
    }

    imguiManager.shutdown();
    rr.texArray.destroy();
    if (rr.skyVAO) glDeleteVertexArrays(1, &rr.skyVAO);
    if (rr.skyProg) glDeleteProgram(rr.skyProg);
    if (rr.shaderProg) glDeleteProgram(rr.shaderProg);
    Renderer::destroyWindow(window);
    Renderer::shutdown();
    return 0;
}
