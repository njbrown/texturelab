export {
    parseTexture,
    supportedNodes,
    getVariable,
    setVariable,
    resetVariables,
    libraryForVersion,
    TextureParseError,
    TEXTURE_CHANNELS,
    type TextureDocument,
    type TextureConnection,
    type TextureChannel,
    type LibraryVersion,
    type ParseOptions,
} from "./document.js";
export {
    createRuntime,
    TextureRuntime,
    EvaluationResult,
    type RuntimeOptions,
    type EvaluateOptions,
    type EvaluateProgress,
    type EvaluationWarning,
    type WarningKind,
    type ChannelPixels,
    type TextureMaps,
} from "./runtime.js";
export type { RenderTargetFormat } from "./gl/context.js";
export { TextureNode, MultiPassNode } from "./node.js";
export * from "./props.js";
export { Gradient, type GradientPoint } from "./gradient.js";
export { Curve, type CurvePoint } from "./curve.js";
export { QColor, type Color } from "./color.js";
export {
    accepts as variableAcceptsProp,
    clampValue as clampVariableValue,
    sanitizeName as sanitizeVariableName,
    type TextureVariable,
    type VariableType,
    type VariableValue,
    type VariableLink,
    type EnumOption,
} from "./variables.js";

/** Library versions this runtime can render. */
export const SUPPORTED_LIBRARY_VERSIONS = ["v1", "v2", "v3"] as const;
