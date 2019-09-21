import * as THREE from "three";
import { OBJLoader } from "three/examples/jsm/loaders/OBJLoader";
import { OrbitControls } from "three-orbitcontrols-ts";
import { DesignerNode } from "./designer/designernode";
import { ImageCanvas } from "./designer/imagecanvas";

// https://www.bostonbiomotion.com/
// https://blog.subvertallmedia.com/2018/06/25/three-js-imports.html
// https://areknawo.com/building-3d-2048-game-with-vue-and-three-js-setup/
// https://github.com/nicolaspanel/three-orbitcontrols-ts/issues/1
// https://github.com/nicolaspanel/three-orbitcontrols-ts/issues/7
// "three-orbitcontrols-ts": "git+https://git@github.com/nicolaspanel/three-orbitcontrols-ts.git",
// https://stackoverflow.com/questions/16334505/how-to-load-obj-model-with-three-js-in-typescript?rq=1

export class View3D {
  private camera!: THREE.PerspectiveCamera;
  private renderer!: THREE.WebGLRenderer;
  private scene: THREE.Scene = new THREE.Scene();
  private material: THREE.MeshStandardMaterial = new THREE.MeshStandardMaterial(
    {
      //color: 0x3F51B5,
      color: 0xffffff,
      roughness: 0.5,
      metalness: 0.0,
      side: THREE.DoubleSide
    }
  );

  private model: THREE.Object3D;
  private controls: OrbitControls;
  // texture repeat
  private repeat: number = 4;

  // geometry
  private sphereGeom = new THREE.SphereGeometry(0.7, 64, 64);
  private cubeGeom = new THREE.BoxGeometry();
  private planeGeom = new THREE.PlaneGeometry(1, 1, 100, 100);
  private cylinderGeom = new THREE.CylinderGeometry(0.5, 0.5, 1, 32, 32);

  setCanvas(el: HTMLCanvasElement) {
    this.setupRenderer(el);
    this.camera = new THREE.PerspectiveCamera(
      45,
      el.width / el.height,
      0.1,
      1000
    );
    this.camera.position.z = 2.6;
    this.camera.position.y = 1;
    this.camera.position.x = 1;

    this.setupOrbitControls(el);
    this.setupLighting();

    //const geometry = new THREE.SphereGeometry(1, 64, 64);
    this.model = new THREE.Mesh(this.sphereGeom, this.material);

    // let loader = new THREE.ObjectLoader();
    // loader.load("app://./")

    this.scene.add(this.model);

    const animate = () => {
      requestAnimationFrame(animate);
      this.controls.update();
      this.renderer.render(this.scene, this.camera);
    };

    animate();
  }

  setupRenderer(el: HTMLCanvasElement) {
    let renderer = new THREE.WebGLRenderer({
      alpha: true,
      canvas: el,
      preserveDrawingBuffer: true,
      antialias: true
    });
    renderer.setClearColor(0x000000, 0);
    renderer.physicallyCorrectLights = true;
    renderer.gammaInput = true;
    renderer.gammaOutput = true;
    renderer.shadowMap.enabled = true;
    renderer.shadowMap.type = THREE.PCFSoftShadowMap;
    renderer.setSize(el.width, el.height);

    this.renderer = renderer;
  }

  setupLighting() {
    var container = new THREE.Object3D();

    var brightness = 2;

    var object3d = new THREE.DirectionalLight("white", 0.225 * brightness);
    object3d.position.set(2.6, 1, 3);
    object3d.name = "Back light";
    container.add(object3d);

    var object3d = new THREE.DirectionalLight("white", 0.375 * brightness);
    object3d.position.set(-2, -1, 0);
    object3d.name = "Key light";
    container.add(object3d);

    var object3d = new THREE.DirectionalLight("white", 0.75 * brightness);
    object3d.position.set(3, 3, 2);
    object3d.name = "Fill light";
    container.add(object3d);

    this.scene.add(container);
  }

  setupOrbitControls(el: HTMLElement) {
    const controls = new OrbitControls(this.camera, el);

    controls.enableZoom = true;
    controls.enableRotate = true;

    //controls.autoRotate = true;
    controls.enablePan = true;
    controls.keyPanSpeed = 7.0;
    controls.enableKeys = true;
    controls.target = new THREE.Vector3(0, 0, 0);
    controls.mouseButtons.PAN = null;
    controls.keys = {
      LEFT: 0,
      UP: 0,
      RIGHT: 0,
      BOTTOM: 0
    };

    this.controls = controls;
  }

  loadEnvironment(basePath: string) {}

  resize(width: number, height: number) {
    this.camera.aspect = width / height;
    this.camera.updateProjectionMatrix();
    this.renderer.setSize(width, height);
  }
  private _init() {}

  setTexture(imageCanvas: ImageCanvas, channelName: string) {
    if (channelName == "albedo")
      this.setAlbedoTexture(imageCanvas, channelName);
    if (channelName == "normal")
      this.setNormalTexture(imageCanvas, channelName);
    if (channelName == "metalness")
      this.setMetalnessTexture(imageCanvas, channelName);
    if (channelName == "roughness")
      this.setRoughnessTexture(imageCanvas, channelName);
    if (channelName == "height")
      this.setHeightTexture(imageCanvas, channelName);
  }

  clearTexture(channelName: string) {
    if (channelName == "albedo") {
      this.material.map = null;
    }
    if (channelName == "normal") {
      this.material.normalMap = null;
    }
    if (channelName == "metalness") {
      this.material.metalnessMap = null;
    }
    if (channelName == "roughness") {
      this.material.roughnessMap = null;
    }
    if (channelName == "height") {
      this.material.displacementMap = null;
    }
    this.material.needsUpdate = true;
  }

  updateTexture(channelName: string) {
    if (channelName == "albedo" && this.material.map != null) {
      this.material.map.needsUpdate = true;
    }
    if (channelName == "normal" && this.material.normalMap != null) {
      this.material.normalMap.needsUpdate = true;
    }
    if (channelName == "metalness" && this.material.metalnessMap != null) {
      this.material.metalnessMap.needsUpdate = true;
    }
    if (channelName == "roughness" && this.material.roughnessMap != null) {
      this.material.roughnessMap.needsUpdate = true;
    }
    if (channelName == "height" && this.material.displacementMap != null) {
      this.material.displacementMap.needsUpdate = true;
    }
    this.material.needsUpdate = true;
  }

  setAlbedoTexture(imageCanvas: ImageCanvas, channelName: string) {
    var tex = new THREE.CanvasTexture(imageCanvas.canvas);
    tex.wrapS = tex.wrapT = THREE.RepeatWrapping;
    tex.repeat.set(this.repeat, this.repeat);
    tex.anisotropy = this.renderer.capabilities.getMaxAnisotropy();

    tex.needsUpdate = true;
    this.material.map = tex;
    this.material.needsUpdate = true;
  }

  setNormalTexture(imageCanvas: ImageCanvas, channelName: string) {
    var tex = new THREE.CanvasTexture(imageCanvas.canvas);
    tex.wrapS = tex.wrapT = THREE.RepeatWrapping;
    tex.repeat.set(this.repeat, this.repeat);
    tex.anisotropy = this.renderer.capabilities.getMaxAnisotropy();

    tex.needsUpdate = true;
    this.material.normalMap = tex;
    this.material.needsUpdate = true;
  }

  setMetalnessTexture(imageCanvas: ImageCanvas, channelName: string) {
    var tex = new THREE.CanvasTexture(imageCanvas.canvas);
    tex.wrapS = tex.wrapT = THREE.RepeatWrapping;
    tex.repeat.set(this.repeat, this.repeat);
    tex.anisotropy = this.renderer.capabilities.getMaxAnisotropy();

    tex.needsUpdate = true;
    this.material.metalnessMap = tex;
    this.material.metalness = 1.0;
    this.material.needsUpdate = true;
  }

  setRoughnessTexture(imageCanvas: ImageCanvas, channelName: string) {
    var tex = new THREE.CanvasTexture(imageCanvas.canvas);
    tex.wrapS = tex.wrapT = THREE.RepeatWrapping;
    tex.repeat.set(this.repeat, this.repeat);
    tex.anisotropy = this.renderer.capabilities.getMaxAnisotropy();

    tex.needsUpdate = true;
    this.material.roughnessMap = tex;
    this.material.roughness = 1.0;
    this.material.needsUpdate = true;
  }

  setHeightTexture(imageCanvas: ImageCanvas, channelName: string) {
    var tex = new THREE.CanvasTexture(imageCanvas.canvas);
    tex.wrapS = tex.wrapT = THREE.RepeatWrapping;
    tex.repeat.set(this.repeat, this.repeat);
    tex.anisotropy = this.renderer.capabilities.getMaxAnisotropy();

    tex.needsUpdate = true;
    this.material.displacementMap = tex;
    this.material.displacementScale = 0.5;
    this.material.needsUpdate = true;
  }

  setRepeat(repeat: number) {
    this.repeat = repeat;

    const mat = this.material;
    if (mat.map) {
      mat.map.repeat.set(repeat, repeat);
      mat.map.needsUpdate = true;
    }

    if (mat.normalMap) {
      mat.normalMap.repeat.set(repeat, repeat);
      mat.normalMap.needsUpdate = true;
    }

    if (mat.metalnessMap) {
      mat.metalnessMap.repeat.set(repeat, repeat);
      mat.metalnessMap.needsUpdate = true;
    }

    if (mat.roughnessMap) {
      mat.roughnessMap.repeat.set(repeat, repeat);
      mat.roughnessMap.needsUpdate = true;
    }

    if (mat.displacementMap) {
      mat.displacementMap.repeat.set(repeat, repeat);
      mat.displacementMap.needsUpdate = true;
    }
  }

  setModel(modelName: string) {
    if (this.model) this.scene.remove(this.model);

    let geom: THREE.Geometry;
    if (modelName == "sphere") geom = this.sphereGeom;
    if (modelName == "cube") geom = this.cubeGeom;
    if (modelName == "plane") geom = this.planeGeom;
    if (modelName == "cylinder") geom = this.cylinderGeom;
    // crash if none is valid

    this.model = new THREE.Mesh(geom, this.material);
    this.scene.add(this.model);
  }
}
