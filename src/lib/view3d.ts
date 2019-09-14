import * as THREE from "three";
import { OrbitControls } from "three-orbitcontrols-ts";

// https://www.bostonbiomotion.com/
// https://blog.subvertallmedia.com/2018/06/25/three-js-imports.html
// https://areknawo.com/building-3d-2048-game-with-vue-and-three-js-setup/
// https://github.com/nicolaspanel/three-orbitcontrols-ts/issues/1
// https://github.com/nicolaspanel/three-orbitcontrols-ts/issues/7
// "three-orbitcontrols-ts": "git+https://git@github.com/nicolaspanel/three-orbitcontrols-ts.git",

export class View3D {
  private camera!: THREE.PerspectiveCamera;
  private renderer!: THREE.WebGLRenderer;
  private scene: THREE.Scene = new THREE.Scene();
  private material: THREE.MeshStandardMaterial = new THREE.MeshStandardMaterial(
    {
      //color: 0x3F51B5,
      color: 0xffffff,
      roughness: 0.5,
      metalness: 0.0
    }
  );

  private model: THREE.Object3D;
  private controls: OrbitControls;

  setCanvas(el: HTMLCanvasElement) {
    this.setupRenderer(el);
    this.camera = new THREE.PerspectiveCamera(
      45,
      el.width / el.height,
      0.1,
      1000
    );
    this.camera.position.z = 3.1;
    this.camera.position.y = 1;
    this.camera.position.x = 1;

    this.setupOrbitControls(el);
    this.setupLighting();

    const geometry = new THREE.SphereGeometry(1, 64, 64);
    this.model = new THREE.Mesh(geometry, this.material);

    this.scene.add(this.model);
    this.camera.position.z = 5;

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
}
