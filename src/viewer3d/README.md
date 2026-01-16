reference:
https://github.com/SaschaWillems/Vulkan-glTF-PBR
https://github.com/syoyo/tinygltf/tree/release/examples/glview

PBR shader reference:
https://github.com/KhronosGroup/glTF-Sample-Renderer/tree/main/source/Renderer/shaders

original branch:
https://github.com/KhronosGroup/glTF-Sample-Renderer/blob/47a191931461a6f2e14de48d6da0f0eb6ec2d147/source/Renderer/shaders/pbr.frag

https://raw.githubusercontent.com/KhronosGroup/glTF-Sample-Viewer/47a191931461a6f2e14de48d6da0f0eb6ec2d147/source/Renderer/shaders/material_info.glsl



steps:
load gltf model with tangents
load hdr texture
generate hdr textures
pass textures to shaders
