// values range from 0 to 1
export class Color {
	public r = 0.0;
	public g = 0.0;
	public b = 0.0;
	public a = 1.0;

	public constructor(r = 0.0, g = 0.0, b = 0.0, a = 1.0) {
		this.r = r;
		this.g = g;
		this.b = b;
		this.a = a;
	}
	public clone(): Color {
		return new Color(this.r, this.g, this.b, this.a);
	}

	public copy(col: Color) {
		this.r = col.r;
		this.g = col.g;
		this.b = col.b;
		this.a = col.a;
	}

	public lerp(to: Color, t: number) {
		this.r = this.r * t + to.r * (1.0 - t);
		this.g = this.g * t + to.g * (1.0 - t);
		this.b = this.b * t + to.b * (1.0 - t);
		this.a = this.a * t + to.a * (1.0 - t);
	}

	public static parse(hex: string): Color {
		const result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
		if (result) {
			const c = new Color();
			c.r = parseInt(result[1], 16) / 255;
			c.g = parseInt(result[2], 16) / 255;
			c.b = parseInt(result[3], 16) / 255;
			return c;
		} else {
			return new Color();
		}
	}

	public toHex(): string {
		//https://stackoverflow.com/questions/596467/how-do-i-convert-a-float-number-to-a-whole-number-in-javascript
		const r = ~~(this.r * 255);
		const g = ~~(this.g * 255);
		const b = ~~(this.b * 255);
		return "#" + ((1 << 24) + (r << 16) + (g << 8) + b).toString(16).slice(1);
	}
}
