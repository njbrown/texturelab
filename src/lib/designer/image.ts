import { NativeImage } from "electron";

export class Image
{
    canvas: HTMLCanvasElement;
    path: string;

    constructor(path:string,
                canvasSource: CanvasImageSource,
                width:number,
                height:number){

        let canvas = document.createElement('canvas') as HTMLCanvasElement;
        canvas.width = width;
        canvas.height = height;

        let ctx = canvas.getContext("2d");
        ctx.drawImage(canvasSource, 0, 0);

        this.canvas = canvas;
        this.path = path;
    }

    static load(path:string): Image
    {
        let nativeImage = NativeImage.createFromPath(path);
        let img:HTMLImageElement = document.createElement("image") as HTMLImageElement;

        img.src = nativeImage.toDataURL();

        return new Image(path, img, img.width, img.height);
    }

    clone(): Image
    {
        return new Image(this.path,
                         this.canvas,
                         this.width,
                         this.height);
    } 

    get width()
    {
        return this.canvas.width;
    }

    get height()
    {
        return this.canvas.height;
    }
}