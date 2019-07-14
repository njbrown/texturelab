import * as THREE from "three";
import { OrbitControls } from "three-orbitcontrols-ts";

// https://www.bostonbiomotion.com/
// https://blog.subvertallmedia.com/2018/06/25/three-js-imports.html
// https://areknawo.com/building-3d-2048-game-with-vue-and-three-js-setup/

export class View3D {
	private camera!: THREE.PerspectiveCamera;
	private renderer!: THREE.WebGLRenderer;
	private scene: THREE.Scene = new THREE.Scene();

	setCanvas(el: HTMLCanvasElement) {
		this.renderer = new THREE.WebGLRenderer({
			canvas: el
		});

		this.camera = new THREE.PerspectiveCamera(
			75,
			el.width / el.height,
			0.1,
			1000
		);
		this.renderer.setSize(el.width, el.height);
		//el.appendChild(this.renderer.domElement);

		const geometry = new THREE.BoxGeometry(1, 1, 1);
		const material = new THREE.MeshBasicMaterial({ color: 0x00ff00 });
		const cube = new THREE.Mesh(geometry, material);

		this.scene.add(cube);
		this.camera.position.z = 5;

		// orbital controls
		const controls = new OrbitControls(this.camera, this.renderer.domElement);

		// How far you can orbit vertically, upper and lower limits.
		controls.minPolarAngle = 0;
		controls.maxPolarAngle = Math.PI;

		controls.rotateSpeed = 0.3;

		// How far you can dolly in and out ( PerspectiveCamera only )
		controls.minDistance = 0;
		controls.maxDistance = Infinity;

		controls.enableZoom = true; // Set to false to disable zooming
		controls.zoomSpeed = 1.0;

		controls.enablePan = true; // Set to false to disable panning (ie vertical and horizontal translations)

		controls.enableDamping = true; // Set to false to disable damping (ie inertia)
		controls.dampingFactor = 0.25;

		const animate = () => {
			requestAnimationFrame(animate);

			//cube.rotation.x += 0.01;
			//cube.rotation.y += 0.01;

			this.renderer.render(this.scene, this.camera);
		};

		animate();
	}

	resize(width: number, height: number) {
		this.camera.aspect = width / height;
		this.camera.updateProjectionMatrix();
		this.renderer.setSize(width, height);
	}
	private _init() {}
}
