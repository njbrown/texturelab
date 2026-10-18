// Image-based lighting for the viewer. Nothing is bundled: the default is a
// procedurally generated gradient sky, `studio` is three.js's generated
// RoomEnvironment, and HDR/EXR maps are loaded from URLs the host serves.

import {
    BackSide,
    Color,
    EquirectangularReflectionMapping,
    Mesh,
    PMREMGenerator,
    Scene,
    ShaderMaterial,
    SphereGeometry,
    Vector3,
    type ColorRepresentation,
    type Texture,
} from "three";
import { RoomEnvironment } from "three/examples/jsm/environments/RoomEnvironment.js";
import { EXRLoader } from "three/examples/jsm/loaders/EXRLoader.js";
import { HDRLoader } from "three/examples/jsm/loaders/HDRLoader.js";

export interface SunOptions {
    /** Direction towards the sun. Normalised for you. */
    direction?: [number, number, number];
    color?: ColorRepresentation;
    /** Radiance multiplier for the sun disc. Default 40. */
    intensity?: number;
    /** Angular radius in radians. Default 0.04. */
    size?: number;
}

export interface GradientEnvironment {
    type: "gradient";
    zenith?: ColorRepresentation;
    horizon?: ColorRepresentation;
    ground?: ColorRepresentation;
    /** A bright spot gives glossy and metallic surfaces something to reflect. `false` turns it off. */
    sun?: SunOptions | false;
}

export interface StudioEnvironment {
    type: "studio";
}

export interface ImageEnvironment {
    type: "hdr" | "exr";
    url: string;
}

export type Environment = GradientEnvironment | StudioEnvironment | ImageEnvironment;

export type SkyPreset = "default" | "overcast" | "sunset" | "night";

export const SKY_PRESETS: Record<SkyPreset, GradientEnvironment> = {
    default: {
        type: "gradient",
        zenith: "#7d93b8",
        horizon: "#e6e8ee",
        ground: "#3a3632",
        sun: { direction: [0.5, 0.65, 0.55], color: "#fff4e0", intensity: 40, size: 0.04 },
    },
    overcast: {
        type: "gradient",
        zenith: "#b9bec6",
        horizon: "#d9dbde",
        ground: "#4a4845",
        sun: false,
    },
    sunset: {
        type: "gradient",
        zenith: "#2b3a67",
        horizon: "#f2a65a",
        ground: "#2a201c",
        sun: { direction: [0.8, 0.12, 0.3], color: "#ffb36b", intensity: 60, size: 0.05 },
    },
    night: {
        type: "gradient",
        zenith: "#05070d",
        horizon: "#1b2233",
        ground: "#08080a",
        sun: { direction: [-0.4, 0.5, 0.6], color: "#c9d6ff", intensity: 12, size: 0.03 },
    },
};

const skyVertex = /* glsl */ `
varying vec3 vDir;
void main() {
    vDir = normalize(position);
    gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0);
}
`;

const skyFragment = /* glsl */ `
uniform vec3 zenith;
uniform vec3 horizon;
uniform vec3 ground;
uniform vec3 sunDir;
uniform vec3 sunColor;
uniform float sunIntensity;
uniform float sunSize;
varying vec3 vDir;
void main() {
    vec3 d = normalize(vDir);
    float y = d.y;
    vec3 col = y >= 0.0
        ? mix(horizon, zenith, pow(y, 0.6))
        : mix(horizon, ground, pow(min(-y * 4.0, 1.0), 0.5));
    if (sunIntensity > 0.0) {
        float angle = acos(clamp(dot(d, sunDir), -1.0, 1.0));
        // Soft-edged disc plus a little glow.
        float disc = 1.0 - smoothstep(sunSize * 0.7, sunSize, angle);
        float glow = exp(-angle * 6.0) * 0.08;
        col += sunColor * (disc * sunIntensity + glow * sunIntensity * 0.1);
    }
    gl_FragColor = vec4(col, 1.0);
}
`;

/** Render the gradient sky into a PMREM environment texture. */
export function createGradientEnvironment(pmrem: PMREMGenerator, env: GradientEnvironment): Texture {
    const preset = SKY_PRESETS.default;
    const sun = env.sun === false ? null : { ...(preset.sun as SunOptions), ...(env.sun ?? {}) };
    const dir = new Vector3(...(sun?.direction ?? [0, 1, 0])).normalize();

    const material = new ShaderMaterial({
        vertexShader: skyVertex,
        fragmentShader: skyFragment,
        side: BackSide,
        depthWrite: false,
        toneMapped: false,
        uniforms: {
            zenith: { value: new Color(env.zenith ?? preset.zenith) },
            horizon: { value: new Color(env.horizon ?? preset.horizon) },
            ground: { value: new Color(env.ground ?? preset.ground) },
            sunDir: { value: dir },
            sunColor: { value: new Color(sun?.color ?? "#ffffff") },
            sunIntensity: { value: sun ? (sun.intensity ?? 40) : 0 },
            sunSize: { value: sun?.size ?? 0.04 },
        },
    });
    const geometry = new SphereGeometry(10, 64, 32);
    const scene = new Scene();
    scene.add(new Mesh(geometry, material));

    const target = pmrem.fromScene(scene, 0, 0.1, 100);
    geometry.dispose();
    material.dispose();
    return target.texture;
}

export function createStudioEnvironment(pmrem: PMREMGenerator): Texture {
    const room = new RoomEnvironment();
    const target = pmrem.fromScene(room, 0.04);
    room.dispose();
    return target.texture;
}

export async function loadImageEnvironment(pmrem: PMREMGenerator, env: ImageEnvironment): Promise<Texture> {
    const loader = env.type === "exr" ? new EXRLoader() : new HDRLoader();
    const equirect = await loader.loadAsync(env.url);
    equirect.mapping = EquirectangularReflectionMapping;
    const target = pmrem.fromEquirectangular(equirect);
    equirect.dispose();
    return target.texture;
}
