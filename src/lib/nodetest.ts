
export class ImageCanvas
{
    canvas:HTMLCanvasElement;
    context:CanvasRenderingContext2D;

    constructor(width:number = 1024, height:number = 1024)
    {
        this.canvas = document.createElement("canvas");
        this.canvas.width = width;
        this.canvas.height = height;
        this.context = this.canvas.getContext("2d");
    }

    // copies image from source
    // scales the image to fit dest canvas
    copyFromCanvas(canvas:HTMLCanvasElement)
    {
        this.context.clearRect(0,0, this.canvas.width, this.canvas.height);
        this.context.drawImage(canvas, 0, 0, this.canvas.width, this.canvas.height);
    }

    copyFromImageCanvas(imageCanvas:ImageCanvas)
    {
        this.context.clearRect(0,0, this.canvas.width, this.canvas.height);
        this.context.drawImage(imageCanvas.canvas, 0, 0, this.canvas.width, this.canvas.height);
    }

    resize(width:number, height:number)
    {
        this.canvas.width = width;
        this.canvas.height = height;
    }

    width():number
    {
        return this.canvas.width;
    }

    height():number
    {
        return this.canvas.height;
    }

    toImage():HTMLImageElement
    {
        var img:HTMLImageElement = <HTMLImageElement>document.createElement("image");
        //var img:HTMLImageElement = new Image(this.width, this.height);
        img.src = this.canvas.toDataURL("image/png");

        return img;
    }

    createTexture(gl:WebGLRenderingContext): WebGLTexture
    {
        var texture = gl.createTexture();
        gl.bindTexture(gl.TEXTURE_2D, texture);

        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.REPEAT);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.REPEAT);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);

        // Upload the image into the texture.
        gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, this.canvas);

        return texture;
    }
}

export class Guid {
    static newGuid() {
        return 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, function(c) {
            var r = Math.random()*16|0, v = c == 'x' ? r : (r&0x3|0x8);
            return v.toString(16);
        });
    }
}

function createTexture(gl:WebGLRenderingContext, width:number, height:number):WebGLTexture
{
    var texture = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, texture);

    // Set the parameters so we can render any size image.
    //gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    //gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.REPEAT);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.REPEAT);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);

    // Upload the image into the texture.
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, null);

    return texture;
}

function getShaderSource(id) {
    var shaderScript = document.getElementById(id);
    if (!shaderScript) {
        return null;
    }
 
    var str = "";
    var k = shaderScript.firstChild;
    while (k) {
        if (k.nodeType == 3) {
            str += k.textContent;
        }
        k = k.nextSibling;
    }
 
    return str;
}

function compileShader(gl, source, shaderType) {
    var shader = gl.createShader(shaderType);
 
    gl.shaderSource(shader, source);
    gl.compileShader(shader);
 
    if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
        alert(gl.getShaderInfoLog(shader));
        return null;
    }
 
    return shader;
}

function buildShaderProgram(gl, vertSource, fragSource) {
    var vertexShader = compileShader(gl, vertSource, gl.VERTEX_SHADER);
    var fragmentShader = compileShader(gl, fragSource, gl.FRAGMENT_SHADER);
 
    var shaderProgram = gl.createProgram();
    gl.attachShader(shaderProgram, vertexShader);
    gl.attachShader(shaderProgram, fragmentShader);
    gl.linkProgram(shaderProgram);
 
    if (!gl.getProgramParameter(shaderProgram, gl.LINK_STATUS)) {
        alert("Could not initialise shaders");
    }
 
    gl.useProgram(shaderProgram);
    /*
    shaderProgram.samplerLoc = gl.getUniformLocation(shaderProgram, "tex");
 
    shaderProgram.vertexPositionAttribute = gl.getAttribLocation(shaderProgram, "aVertexPosition");
    gl.enableVertexAttribArray(shaderProgram.vertexPositionAttribute);
 
    shaderProgram.vertexUVAttribute = gl.getAttribLocation(shaderProgram, "aVertexUV");
    gl.enableVertexAttribArray(shaderProgram.vertexUVAttribute);
    */

    return shaderProgram;
}

// for use in code after build
export enum PropertyType
{
    Float = "float",
    Int = "int",
    Bool = "bool",
    Color = "color",
    Enum = "enum",
    String = "string",
}

export class Property
{
    public name : string;
    public displayName : string;
    public type : string;

    // to be overriden
    public getValue():any
    {
        return null;
    }

    public setValue(val:any)
    {

    }

    public clone():Property
    {
        return null;
    }
}

export class FloatProperty extends Property
{
    value : number;
    minValue: number = 0;
    maxValue:number = 1;
    step:number = 1;
    public constructor(name:string, displayName:string, value:number, step:number = 1)
    {
        super();
        this.name = name;
        this.displayName = displayName;
        this.value = value;
        this.step = step;
        this.type = PropertyType.Float;
    }

    public getValue():any
    {
        return this.value;
    }

    public setValue(val:any)
    {
        // todo: validate
        this.value = val;
    }

    public clone():Property
    {
        var prop = new FloatProperty(this.name, this.displayName, this.value, this.step);
        prop.minValue = this.minValue;
        prop.maxValue = this.maxValue;

        return prop;
    }

    public copyValuesFrom(prop:FloatProperty)
    {
        this.minValue = prop.minValue;
        this.maxValue = prop.maxValue;
        this.value    = prop.value;
        this.step     = prop.step;
    }
}

export class IntProperty extends Property
{
    value : number;
    minValue: number = 0;
    maxValue:number = 100;
    step:number = 1;
    public constructor(name:string, displayName:string, value:number, step:number = 1)
    {
        super();
        this.name = name;
        this.displayName = displayName;
        this.value = value;
        this.step = step;
        this.type = PropertyType.Int;
    }

    public getValue():any
    {
        return this.value;
    }

    public setValue(val:any)
    {
        // todo: validate
        this.value = val;
    }

    public clone():Property
    {
        var prop = new IntProperty(this.name, this.displayName, this.value, this.step);
        prop.minValue = this.minValue;
        prop.maxValue = this.maxValue;
        
        return prop;
    }

    public copyValuesFrom(prop:IntProperty)
    {
        this.minValue = prop.minValue;
        this.maxValue = prop.maxValue;
        this.value    = prop.value;
        this.step     = prop.step;
    }
}

export class BoolProperty extends Property
{
    value : boolean;
    public constructor(name:string, displayName:string, value:boolean)
    {
        super();
        this.name = name;
        this.displayName = displayName;
        this.value = value;
        this.type = PropertyType.Bool;
    }

    public getValue():any
    {
        return this.value;
    }

    public setValue(val:any)
    {
        // todo: validate
        this.value = val;
    }

    public clone():Property
    {
        var prop = new BoolProperty(this.name, this.displayName, this.value);
        
        return prop;
    }

    public copyValuesFrom(prop:BoolProperty)
    {
        this.value    = prop.value;
    }
}

export class EnumProperty extends Property
{
    values : string[];
    index:number = 0;
    public constructor(name:string, displayName:string, values:string[])
    {
        super();
        this.name = name;
        this.displayName = displayName;
        this.values = values;
        this.type = PropertyType.Enum;
    }

    public getValues():string[]
    {
        return this.values;
    }

    public getValue():any
    {
        return this.index;
    }

    public setValue(val:any)
    {
        // todo: validate
        this.index = val;
    }

    public clone():Property
    {
        var prop = new EnumProperty(this.name, this.displayName, this.values.slice(0));
        prop.index = this.index;

        return prop;
    }

    public copyValuesFrom(prop:EnumProperty)
    {
        this.values    = prop.values;
        this.index     = prop.index;
    }
}

// values range from 0 to 1
export class Color
{
    public r:number = 0.0;
    public g:number = 0.0;
    public b:number = 0.0;
    public a:number = 1.0;

    public constructor()
    {

    }

    public static parse(hex:string):Color
    {
        var result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
        if (result) {
        var c = new Color();
        c.r = parseInt(result[1], 16) / 255;
        c.g = parseInt(result[2], 16) / 255;
        c.b = parseInt(result[3], 16) / 255;
        return c;
        } else {
            return new Color();
        }
    }

    public toHex():string
    {
        //https://stackoverflow.com/questions/596467/how-do-i-convert-a-float-number-to-a-whole-number-in-javascript
        var r = ~~(this.r * 255);
        var g = ~~(this.g * 255);
        var b = ~~(this.b * 255);
        return "#" + ((1 << 24) + (r << 16) + (g << 8) + b).toString(16).slice(1);
    }
}

export class ColorProperty extends Property
{
    value : Color;
    public constructor(name:string, displayName:string, value:Color)
    {
        super();
        this.name = name;
        this.displayName = displayName;
        this.value = value;
        this.type = PropertyType.Color;
    }

    public getValue():any
    {
        return this.value;
    }

    public setValue(val:any)
    {
        // todo: validate
        console.log("got color: "+val);
        if (val instanceof Color)
            this.value = val;
        else if (typeof val == "string")
            this.value = Color.parse(val);
        else if (typeof val == "object")
        {
            var value = new Color();
            value.r = val.r || 0;
            value.g = val.g || 0;
            value.b = val.b || 0;
            value.a = val.a || 1.0;

            this.value = value;
        }
    }

    public clone():Property
    {
        var prop = new ColorProperty(this.name, this.displayName, this.value);
    
        return prop;
    }

    public copyValuesFrom(prop:ColorProperty)
    {
        this.setValue(prop.value);
    }
}

export class StringProperty extends Property
{
    value : string;
    public constructor(name:string, displayName:string, value:string="")
    {
        super();
        this.name = name;
        this.displayName = displayName;
        this.value = value;
        this.type = PropertyType.String;
    }

    public getValue():any
    {
        return this.value;
    }

    public setValue(val:any)
    {
        // todo: validate
        this.value = val;
    }

    public clone():Property
    {
        var prop = new StringProperty(this.name, this.displayName, this.value);
        
        return prop;
    }

    public copyValuesFrom(prop:StringProperty)
    {
        this.value    = prop.value;
    }
}



export class DesignerNodeConn
{
    public id : string = Guid.newGuid();

    leftNode:DesignerNode;
    leftNodeOutput:string = ""; // if null, use first output

    rightNode:DesignerNode;
    rightNodeInput:string;
}


export class DesignerNode
{
    public id : string = Guid.newGuid();
    public title:string;
    public typeName:string;// added when node is created from library

    public gl:WebGLRenderingContext;
    public designer:Designer;
    tex:WebGLTexture;
    //program:WebGLShader;
    source:string;// shader code
    shaderProgram:WebGLProgram;
    exportName:string;

    inputs:string[] = new Array();;
    properties:Property[] = new Array();

    // tells scene to update the texture next frame
    needsUpdate:boolean = true;

    // callbacks
    onthumbnailgenerated:(DesignerNode,HTMLImageElement)=>void;

    // an update is requested when:
    // a property is changed
    // a new connection is made
    // a connection is removed
    //
    // all output connected nodes are invalidated as well
    private requestUpdate()
    {
        this.designer.requestUpdate(this);
    }

    public render(inputs:NodeInput[])
    {
        var gl = this.gl;
        // bind texture to fbo
        //gl.clearColor(0,0,1,1);
        gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

        // bind shader
        gl.useProgram(this.shaderProgram);

        // pass textures
        var texIndex = 0;
        for(let input of inputs) {
            gl.activeTexture(gl.TEXTURE0 + texIndex);
            gl.bindTexture(gl.TEXTURE_2D, input.node.tex);
            gl.uniform1i(gl.getUniformLocation(this.shaderProgram,input.name), texIndex);
            texIndex++;
        }

        // pass seed
        gl.uniform1f(gl.getUniformLocation(this.shaderProgram,"_seed"),
                    this.designer.getRandomSeed());

        // texture size
        gl.uniform2f(gl.getUniformLocation(this.shaderProgram,"_textureSize"),
                    this.designer.width, this.designer.height);

        // pass properties
        for(let prop of this.properties) {
            if (prop instanceof FloatProperty) {
                gl.uniform1f(gl.getUniformLocation(this.shaderProgram,"prop_"+prop.name),
                    (prop as FloatProperty).value);
            }
            if (prop instanceof IntProperty) {
                gl.uniform1i(gl.getUniformLocation(this.shaderProgram,"prop_"+prop.name),
                    (prop as IntProperty).value);
            }
            if (prop instanceof BoolProperty) {
                gl.uniform1i(gl.getUniformLocation(this.shaderProgram,"prop_"+prop.name),
                    (prop as BoolProperty).value == false ? 0:1);
            }
            if (prop instanceof EnumProperty) {
                gl.uniform1i(gl.getUniformLocation(this.shaderProgram,"prop_"+prop.name),
                    (prop as EnumProperty).index);
            }
            if (prop instanceof ColorProperty) {
                var col = (prop as ColorProperty).value;
                gl.uniform4f(gl.getUniformLocation(this.shaderProgram,"prop_"+prop.name),
                    col.r, col.g, col.b, col.a);
            }
        }

        // bind mesh
        var posLoc = gl.getAttribLocation(this.shaderProgram, "a_pos");
        var texCoordLoc = gl.getAttribLocation(this.shaderProgram, "a_texCoord");
 
        // provide texture coordinates for the rectangle.
        gl.bindBuffer(gl.ARRAY_BUFFER, this.designer.posBuffer);
        gl.enableVertexAttribArray(posLoc);
        gl.vertexAttribPointer(posLoc, 3, gl.FLOAT, false, 0, 0);

        gl.bindBuffer(gl.ARRAY_BUFFER, this.designer.texCoordBuffer);
        gl.enableVertexAttribArray(texCoordLoc);
        gl.vertexAttribPointer(texCoordLoc, 2, gl.FLOAT, false, 0, 0);

        gl.drawArrays(gl.TRIANGLES,0,6);

        gl.disableVertexAttribArray(posLoc);
        gl.disableVertexAttribArray(texCoordLoc);

        // render
    }

    public getInputs():string[]
    {
        return this.inputs;
    }

    protected addInput(name:string)
    {
        this.inputs.push(name);
    }

    public setProperty(name:string, value:any)
    {
        //console.log(this.properties);
        for(let prop of this.properties) {
            if (prop.name == name) {
                prop.setValue(value);
                this.requestUpdate();
            }
        }
    }

    public _init()
    {
        //this.inputs = new Array();
        //this.properties = new Array();
        this.createTexture();

        this.init();
    }

    protected init()
    {
        
        /*
        this.source = `
        vec4 sample(vec2 uv)
        {
        return vec4(uv,x, uv.y, 0, 0);
        }
        `;

        this.buildShader(this.source);
        */
    }

    // #source gets appended to fragment shader
    buildShader(source:string)
    {
        var vertSource:string = `
        precision highp float;

        attribute vec3 a_pos;
        attribute vec2 a_texCoord;
            
        // the texCoords passed in from the vertex shader.
        varying vec2 v_texCoord;
            
        void main() {
            gl_Position = vec4(a_pos,1.0);
            v_texCoord = a_texCoord;
        }`;

        var fragSource:string = `
        precision highp float;
        varying vec2 v_texCoord;

        vec4 sample(vec2 uv);
        void initRandom();

        uniform vec2 _textureSize;
            
        void main() {
            initRandom();
            gl_FragColor = sample(v_texCoord);
        }

        `;

        fragSource = fragSource +
                     this.createRandomLib() + 
                     this.createCodeForInputs() + 
                     this.createCodeForProps() +
                     "#line 0\n" +
                     source;

        this.shaderProgram = buildShaderProgram(this.gl, vertSource, fragSource);
    }

    // creates opengl texture for this node
    // gets the height from the scene
    // if the texture is already created, delete it and recreate it
    createTexture()
    {
        var gl = this.gl;

        if(this.tex) {
            gl.deleteTexture(this.tex);
            this.tex = null;
        }

        var tex = gl.createTexture();
        gl.bindTexture(gl.TEXTURE_2D, tex);

        const level = 0;
        const internalFormat = gl.RGBA;
        const border = 0;
        const format = gl.RGBA;
        const type = gl.UNSIGNED_BYTE;
        const data = null;
        gl.texImage2D(gl.TEXTURE_2D, level, internalFormat,
                        this.designer.width, this.designer.height, border,
                        format, type, data);
        
        // set the filtering so we don't need mips
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.REPEAT);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.REPEAT);

        gl.bindTexture(gl.TEXTURE_2D, null);
        
        this.tex = tex;
    }

    createRandomLibOld():string 
    {
        // float _seed = `+this.designer.getRandomSeed().toFixed(1)+`;
        var code:string = `
        // this offsets the random start (should be a uniform)
        uniform float _seed;
        // this is the starting number for the rng
        // (should be set from the uv coordinates so it's unique per pixel)
        vec2 _randomStart;

        float _rand(vec2 co){
            return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
        }

        //todo: test variance!
        vec2 _rand2(vec2 co){
            return vec2(_rand(co), _rand(co + vec2(0.0001, 0.0001)));
        }

        float randomFloat(int index) 
        {
            return _rand(_randomStart + vec2(_seed) + vec2(index));
        }

        float randomVec2(int index) 
        {
            return _rand(_randomStart + vec2(_seed) + vec2(index));
        }

        float randomFloat(int index, float start, float end)
        {
            float r = _rand(_randomStart + vec2(_seed) + vec2(index));
            return start + r*(end-start);
        }

        int randomInt(int index, int start, int end)
        {
            float r = _rand(_randomStart + vec2(_seed) + vec2(index));
            return start + int(r*float(end-start));
        }

        bool randomBool(int index)
        {
            return _rand(_randomStart + vec2(_seed) + vec2(index)) > 0.5;
        }

        void initRandom()
        {
            _randomStart = v_texCoord;
        }
        `;

        return code;
    }

    createRandomLib():string
    {
        // float _seed = `+this.designer.getRandomSeed().toFixed(1)+`;
        var code:string = `
        // this offsets the random start (should be a uniform)
        uniform float _seed;
        // this is the starting number for the rng
        // (should be set from the uv coordinates so it's unique per pixel)
        vec2 _randomStart;

        // gives a much better distribution at 1
        #define RANDOM_ITERATIONS 1

        #define HASHSCALE1 443.8975
        #define HASHSCALE3 vec3(443.897, 441.423, 437.195)
        #define HASHSCALE4 vec4(443.897, 441.423, 437.195, 444.129)

        //  1 out, 2 in...
        float hash12(vec2 p)
        {
            vec3 p3  = fract(vec3(p.xyx) * HASHSCALE1);
            p3 += dot(p3, p3.yzx + 19.19);
            return fract((p3.x + p3.y) * p3.z);
        }

        ///  2 out, 2 in...
        vec2 hash22(vec2 p)
        {
            vec3 p3 = fract(vec3(p.xyx) * HASHSCALE3);
            p3 += dot(p3, p3.yzx+19.19);
            return fract((p3.xx+p3.yz)*p3.zy);

        }


        float _rand(vec2 uv)
        {
            float a = 0.0;
            for (int t = 0; t < RANDOM_ITERATIONS; t++)
            {
                float v = float(t+1)*.152;
                // 0.005 is a good value
                vec2 pos = (uv * v);
                a += hash12(pos);
            }

            return a/float(RANDOM_ITERATIONS);
        }

        vec2 _rand2(vec2 uv)
        {
            vec2 a = vec2(0.0);
            for (int t = 0; t < RANDOM_ITERATIONS; t++)
            {
                float v = float(t+1)*.152;
                // 0.005 is a good value
                vec2 pos = (uv * v);
                a += hash22(pos);
            }

            return a/float(RANDOM_ITERATIONS);
        }

        float randomFloat(int index) 
        {
            return _rand(_randomStart + vec2(_seed) + vec2(index));
        }

        float randomVec2(int index) 
        {
            return _rand(_randomStart + vec2(_seed) + vec2(index));
        }

        float randomFloat(int index, float start, float end)
        {
            float r = _rand(_randomStart + vec2(_seed) + vec2(index));
            return start + r*(end-start);
        }

        int randomInt(int index, int start, int end)
        {
            float r = _rand(_randomStart + vec2(_seed) + vec2(index));
            return start + int(r*float(end-start));
        }

        bool randomBool(int index)
        {
            return _rand(_randomStart + vec2(_seed) + vec2(index)) > 0.5;
        }

        void initRandom()
        {
            _randomStart = v_texCoord;
        }
        `;

        return code;
    }

    createCodeForInputs()
    {
        var code:string = "";

        for(let input of this.inputs) {
            code += "uniform sampler2D " + input + ";\n";
        }

        return code;
    }

    createCodeForProps()
    {
        var code:string = "";

        //console.log(this.properties);
        //console.log(typeof FloatProperty);

        for(let prop of this.properties) {
            //code += "uniform sampler2D " + input + ";\n";
            if (prop instanceof FloatProperty) {
                code += "uniform float prop_"+prop.name+";\n";
            }
            if (prop instanceof IntProperty) {
                code += "uniform int prop_"+prop.name+";\n";
            }
            if (prop instanceof BoolProperty) {
                code += "uniform bool prop_"+prop.name+";\n";
            }
            if (prop instanceof EnumProperty) {
                code += "uniform int prop_"+prop.name+";\n";
            }
            if (prop instanceof ColorProperty) {
                code += "uniform vec4 prop_"+prop.name+";\n";
            }
        }

        code += "\n";

        return code;
    }

    // PROPERTY FUNCTIONS
    addIntProperty(id:string, displayName:string, defaultVal:number = 1, minVal:number = 1, maxVal:number = 100,increment:number = 1):IntProperty
    {
        var prop = new IntProperty(id,displayName, defaultVal);
        prop.minValue = minVal;
        prop.maxValue = maxVal;
        prop.step = increment;

        this.properties.push(prop);
        return prop;
    }

    addFloatProperty(id:string, displayName:string, defaultVal:number = 1, minVal:number = 1, maxVal:number = 100,increment:number = 1):FloatProperty
    {
        var prop = new FloatProperty(id,displayName, defaultVal);
        prop.minValue = minVal;
        prop.maxValue = maxVal;
        prop.step = increment;

        this.properties.push(prop);
        return prop;
    }

    addBoolProperty(id:string, displayName:string, defaultVal:boolean = false):BoolProperty
    {
        var prop = new BoolProperty(id,displayName, defaultVal);

        this.properties.push(prop);
        return prop;
    }

    addEnumProperty(id:string, displayName:string, defaultVal:string[] = new Array()):EnumProperty
    {
        var prop = new EnumProperty(id,displayName, defaultVal);

        this.properties.push(prop);
        return prop;
    }

    addColorProperty(id:string, displayName:string, defaultVal:Color):ColorProperty
    {
        var prop = new ColorProperty(id,displayName, defaultVal);

        this.properties.push(prop);
        return prop;
    }

    addStringProperty(id:string, displayName:string, defaultVal:string = ""):StringProperty
    {
        var prop = new StringProperty(id,displayName, defaultVal);

        this.properties.push(prop);
        return prop;
    }
}

export class DesignerNodeFactory
{
    public name:string;
    public displayName:string;
    public create:()=>DesignerNode;
}

// holds list of node factories
export class DesignerLibrary
{
    nodes = new Array();

    // https://www.snip2code.com/Snippet/685188/Create-instance-of-generic-type-on-TypeS
    public addNode<T extends DesignerNode>(name:string, displayName:string, type:{new():T;})
    {
        var factory = new DesignerNodeFactory();
        factory.name = name;
        factory.displayName = displayName;
        factory.create = ():DesignerNode => {
            return new type();
        };

        //this.nodes.push(factory);
        this.nodes[name] = factory;
    }
    
    public create(name:string):DesignerNode
    {
        //if (this.nodes.indexOf(name) == -1)
        //    return null;

        var node = this.nodes[name].create();
        node.typeName = name;

        return node;
    }

}

export class Designer
{
    canvas:HTMLCanvasElement;
    gl:WebGLRenderingContext;
    public texCoordBuffer:WebGLBuffer;
    public posBuffer:WebGLBuffer;
    vertexShaderSource:string;
    fbo:WebGLFramebuffer;
    thumbnailProgram:WebGLProgram;

    randomSeed:number;
    width:number;
    height:number;

    nodes:DesignerNode[];
    conns:DesignerNodeConn[];

    // list of nodes yet to be designed
    updateList:DesignerNode[];

    library:DesignerLibrary;

    //variables
    variables:DesignerVariable[];

    // callbacks
    onthumbnailgenerated:(DesignerNode,HTMLImageElement)=>void;
    
    // called everytime a node's texture gets updated
    // listeners can use this update their CanvasTextures
    // by rendering the node's texture with renderNodeTextureToCanvas(node, imageCanvas)
    onnodetextureupdated:(DesignerNode)=>void;

    public constructor()
    {
        this.width = 1024;
        this.height = 1024;
        this.randomSeed = 32;

        this.canvas = <HTMLCanvasElement>document.createElement("canvas");
        //document.body.appendChild(this.canvas);
        this.canvas.width = this.width;
        this.canvas.height = this.height;
        this.gl = this.canvas.getContext("webgl");

        this.nodes = new Array();
        this.conns = new Array();

        this.updateList = new Array();
        this.variables = new Array();
        this.init();
    }

    public setTextureSize(width:number, height:number)
    {
        //todo: is resizing the canvas even necessary?
        this.width = width;
        this.height = height;
        this.canvas.width = this.width;
        this.canvas.height = this.height;

        for(let node of this.nodes)
        {
            node.createTexture();
            this.requestUpdate(node);
        }
    }

    public randomizeSeed()
    {
        this.setRandomSeed(Math.floor(Math.random() * 256) );
    }

    public setRandomSeed(newSeed:number)
    {
        this.randomSeed = newSeed;
        // invalidate all nodes
        this.invalidateAllNodes();
    }

    public getRandomSeed():number
    {
        return this.randomSeed;
    }

    init()
    {
        this.createVertexBuffers();
        this.createFBO();
        this.createThumbmailProgram();
    }

    update()
    {
        var updateQuota = 10000000000000;
        // fetch random node from update list (having all in sockets that have been updated) and update it
        // todo: do only on per update loop
        while(this.updateList.length != 0) {
            for(let node of this.updateList) {
                if (this.haveAllUpdatedLeftNodes(node)) {
                    // update this node's texture and thumbnail

                    // a note about this:
                    // technically all the child nodes should be updated here, so this.updateList
                    // wont be touched in this function
                    // so we avoid messing up our loop since the length of this.updateList wont change
                    this.generateImageFromNode(node);

                    // remove from list
                    this.updateList.splice(this.updateList.indexOf(node), 1);
                    node.needsUpdate = false;
                    //break;// one per update loop

                    updateQuota--;
                    if (updateQuota < 0)
                        break;
                }
            }
        }
    }

    // checks if all input nodes have needsUpdate set to false
    haveAllUpdatedLeftNodes(node:DesignerNode) : boolean
    {
        for(let con of this.conns) {
            // get connections to this node
            if (con.rightNode == node) {
                if (con.leftNode.needsUpdate == true) {
                    // found a node that needs update itself
                    return false;
                }
            }
        }

        return true;
    }

    // adds node to update list
    // add subsequent (output) nodes in tree to update list a well, recursively
    requestUpdate(node:DesignerNode) 
    {
        if(this.updateList.indexOf(node)==-1) {
            // not yet in the list, add to list and add dependent nodes
            node.needsUpdate = true;// just in case...
            this.updateList.push(node);
        }

        // add all right connections
        for(let con of this.conns) {
            if (con.leftNode == node) {
                this.requestUpdate(con.rightNode);
            }
        }
    }

    public invalidateAllNodes()
    {
        for(let node of this.nodes)
        {
            this.requestUpdate(node);
        }
    }

    setLibrary(lib:DesignerLibrary)
    {
        this.library = lib;
    }

    // creates node and adds it to scene
    createNode(name:string): DesignerNode
    {
        var node = this.library.create(name);

        this.addNode(node);
        return node;
    }

    createFBO()
    {
        var gl = this.gl;

        this.fbo = gl.createFramebuffer();

        // gotta create at least a renderbuffer
    }

    createVertexBuffers()
    {
        var gl = this.gl;
        //var texCoordLocation = gl.getAttribLocation(program, "a_texCoord");
 
        // provide texture coordinates for the rectangle.
        this.texCoordBuffer = gl.createBuffer();
        gl.bindBuffer(gl.ARRAY_BUFFER, this.texCoordBuffer);
        gl.bufferData(gl.ARRAY_BUFFER, new Float32Array([
            0.0,  0.0,
            1.0,  0.0,
            0.0,  1.0,
            0.0,  1.0,
            1.0,  0.0,
            1.0,  1.0]), gl.STATIC_DRAW);
        //gl.enableVertexAttribArray(texCoordLocation);
        //gl.vertexAttribPointer(texCoordLocation, 2, gl.FLOAT, false, 0, 0);


        this.posBuffer = gl.createBuffer();
        gl.bindBuffer(gl.ARRAY_BUFFER, this.posBuffer);
        gl.bufferData(gl.ARRAY_BUFFER, new Float32Array([
            -1.0,  -1.0, 0.0,
            1.0,  -1.0, 0.0,
            -1.0,  1.0, 0.0,
            -1.0,  1.0, 0.0,
            1.0,  -1.0, 0.0,
            1.0,  1.0, 0.0]), gl.STATIC_DRAW);

        gl.bindBuffer(gl.ARRAY_BUFFER, null);
    }

    // adds node to scene
    // adds it to this.updateList
    addNode(node:DesignerNode)
    {
        this.nodes.push(node);
        node.gl = this.gl;
        node.designer = this;
        node._init();
        this.requestUpdate(node);
    }

    getNodeById(nodeId:string):DesignerNode
    {
        for(let node of this.nodes) {
            if (node.id == nodeId)
                return node;
        }
        return null;
    }

    addConnection(leftNode:DesignerNode, rightNode:DesignerNode, rightIndex:string)
    {
        let con = new DesignerNodeConn();
        con.leftNode = leftNode;

        con.rightNode = rightNode;
        con.rightNodeInput = rightIndex;

        this.conns.push(con);

        // right node needs to be updated
        this.requestUpdate(rightNode);

        return con;
    }

    removeConnection(leftNode:DesignerNode, rightNode:DesignerNode, rightIndex:string)
    {
        for(let con of this.conns) {
            if (con.leftNode == leftNode &&
                con.rightNode == rightNode &&
                con.rightNodeInput == rightIndex)
                {
                    // right node needs to be updated
                    this.requestUpdate(rightNode);

                    // found our connection, remove
                    this.conns.splice(this.conns.indexOf(con), 1);
                    //console.log("removed connection in designer");
                    //console.log(con);

                    return con;
                }
        }

        return null;
    }

    generateImage(name:string) : HTMLImageElement
    {
        var node:DesignerNode = this.getNodeByName(name);
        return this.generateImageFromNode(node);
    }

    // this function generates the image of the node given its input nodes
    // if the input nodes arent updated then it will update them
    // for every node updated in this function, it emits onthumbnailgenerated(node, thumbnail)
    // it returns a thumbnail (an html image)

    generateImageFromNode(node:DesignerNode) : HTMLImageElement 
    {
        //console.log("generating node "+node.exportName);
        // process input nodes
        var inputs:NodeInput[] = this.getNodeInputs(node);
        for(let input of inputs) {
            if (input.node.needsUpdate) {
                this.generateImageFromNode(input.node);

                // remove from update list since thumbnail has now been generated
                input.node.needsUpdate = false;
                this.updateList.splice(this.updateList.indexOf(input.node),1);
            }
        }

        var gl = this.gl;

        // todo: move to node maybe
        gl.bindFramebuffer(gl.FRAMEBUFFER, this.fbo);
        gl.activeTexture(gl.TEXTURE0);
        gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, node.tex, 0);

        gl.viewport(0, 0, this.width, this.height);
        node.render(inputs);

        gl.bindFramebuffer(gl.FRAMEBUFFER, null);

        if (this.onnodetextureupdated) {
            this.onnodetextureupdated(node);
        }

        var thumb = this.generateThumbnailFromNode(node);
        if (this.onthumbnailgenerated) {
            this.onthumbnailgenerated(node, thumb);
        }

        return thumb;
    }

    // renders node's texture to an image object
    // ensure the node is updated before calling this function
    // this function doesnt try to update child nodes
    generateThumbnailFromNode(node:DesignerNode)
    {
        var gl = this.gl;

        gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

        // bind shader
        gl.useProgram(this.thumbnailProgram);

        // bind mesh
        var posLoc = gl.getAttribLocation(this.thumbnailProgram, "a_pos");
        var texCoordLoc = gl.getAttribLocation(this.thumbnailProgram, "a_texCoord");
 
        // provide texture coordinates for the rectangle.
        gl.bindBuffer(gl.ARRAY_BUFFER, this.posBuffer);
        gl.enableVertexAttribArray(posLoc);
        gl.vertexAttribPointer(posLoc, 3, gl.FLOAT, false, 0, 0);

        gl.bindBuffer(gl.ARRAY_BUFFER, this.texCoordBuffer);
        gl.enableVertexAttribArray(texCoordLoc);
        gl.vertexAttribPointer(texCoordLoc, 2, gl.FLOAT, false, 0, 0);

        // send texture
        gl.uniform1i(gl.getUniformLocation(this.thumbnailProgram,"tex"),0);
        gl.activeTexture(gl.TEXTURE0);
        gl.bindTexture(gl.TEXTURE_2D, node.tex);

        gl.drawArrays(gl.TRIANGLES,0,6);

        // cleanup
        gl.disableVertexAttribArray(posLoc);
        gl.disableVertexAttribArray(texCoordLoc);
        

        //var img:HTMLImageElement = <HTMLImageElement>document.createElement("image");
        //var img:HTMLImageElement = new Image(this.width, this.height);
        //img.src = this.canvas.toDataURL("image/png");

        // note: this called right after clears the image for some reason
        //gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
        //return img;
        return null;
    }

    // render's node's texture then draws it on the given canvas
    // used as an alternative to move textures since toDataUrl is
    // so computationally expensive
    copyNodeTextureToImageCanvas(node:DesignerNode, canvas:ImageCanvas)
    {
        var gl = this.gl;

        gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

        // bind shader
        gl.useProgram(this.thumbnailProgram);

        // bind mesh
        var posLoc = gl.getAttribLocation(this.thumbnailProgram, "a_pos");
        var texCoordLoc = gl.getAttribLocation(this.thumbnailProgram, "a_texCoord");
 
        // provide texture coordinates for the rectangle.
        gl.bindBuffer(gl.ARRAY_BUFFER, this.posBuffer);
        gl.enableVertexAttribArray(posLoc);
        gl.vertexAttribPointer(posLoc, 3, gl.FLOAT, false, 0, 0);

        gl.bindBuffer(gl.ARRAY_BUFFER, this.texCoordBuffer);
        gl.enableVertexAttribArray(texCoordLoc);
        gl.vertexAttribPointer(texCoordLoc, 2, gl.FLOAT, false, 0, 0);

        // send texture
        gl.uniform1i(gl.getUniformLocation(this.thumbnailProgram,"tex"),0);
        gl.activeTexture(gl.TEXTURE0);
        gl.bindTexture(gl.TEXTURE_2D, node.tex);

        gl.drawArrays(gl.TRIANGLES,0,6);

        // cleanup
        gl.disableVertexAttribArray(posLoc);
        gl.disableVertexAttribArray(texCoordLoc);

        // force rendering to be complete
        //gl.flush();
        
        canvas.copyFromCanvas(this.canvas);
    }

    createThumbmailProgram()
    {
        var prog = buildShaderProgram(this.gl,
        `precision mediump float;

        attribute vec3 a_pos;
        attribute vec2 a_texCoord;
            
        // the texCoords passed in from the vertex shader.
        varying vec2 v_texCoord;
            
        void main() {
            gl_Position = vec4(a_pos,1.0);
            v_texCoord = a_texCoord;
        }`,
        `precision mediump float;
        varying vec2 v_texCoord;
        uniform sampler2D tex;

        vec4 sample(vec2 uv);
            
        void main() {
            gl_FragColor = texture2D(tex,v_texCoord);
        }`);

        this.thumbnailProgram = prog;
    }

    getNodeByName(exportName:string):DesignerNode
    {
        for(let node of this.nodes) {
            if (node.exportName == exportName)
            return node;
        }

        return null;
    }

    getNodeInputs(node:DesignerNode) : NodeInput[]
    {
        var inputs : NodeInput[] = new Array();

        for(let con of this.conns) {
            if (con.rightNode == node) {
                let input = new NodeInput();
                input.name = con.rightNodeInput;
                input.node = con.leftNode;
                inputs.push(input);
            }
        }

        return inputs;
    }

    public addVariable(name:string, displayName:string, varType:DesignerVariableType) : DesignerVariable
    {
        //todo: throw exception if variable already exists?

        var variable = new DesignerVariable();
        variable.type = varType;
        variable.id = Guid.newGuid();
        
        switch(varType){
            case DesignerVariableType.Int:
                variable.property = new IntProperty(name, displayName, 0);
                break;
            case DesignerVariableType.Float:
                variable.property = new FloatProperty(name, displayName, 0);
                break;
            case DesignerVariableType.Bool:
                variable.property = new BoolProperty(name, displayName, false);
                break;
            case DesignerVariableType.Enum:
                variable.property = new EnumProperty(name, displayName, []);
                break;
            case DesignerVariableType.Color:
                variable.property = new ColorProperty(name, displayName, new Color());
                break;
        }

        this.variables.push(variable);
        return variable;
    }

    // todo: keep reference inside node's property
    public mapNodePropertyToVariable(varName:string, node:DesignerNode, nodePropName:string)
    {
        var variable = this.findVariable(varName);
        if (variable == null)
            return;//todo: throw exception?

        var map = new DesignerNodePropertyMap();
        map.node = node;
        map.propertyName = nodePropName;

        variable.nodes.push(map);
    }

    //todo: remove property map

    public setVariable(name:string, value:any)
    {
        var variable = this.findVariable(name);
        if (variable) {
            //todo: throw exception for invalid types being set
            variable.property.setValue(value);

            //update each node's variables
            for(let nodeMap of variable.nodes)
            {
                //if (nodeMap.node.hasProperty(nodeMap.propertyName))// just incase
                nodeMap.node.setProperty(nodeMap.propertyName, value);
            }
        } else {
            // throw exception?
        }
    }

    public findVariable(name:string)
    {
        for(let variable of this.variables)
            if (variable.property.name == name)
                return variable;
        return null;
    }

    public hasVariable(name:string):boolean
    {
        for(let variable of this.variables)
            if (variable.property.name == name)
                return true;
        return false;
    }

    public variableCount() : number
    {
        return this.variables.length;
    }
    
    public save():any
    {
        var nodes = new Array();
        for(let node of this.nodes) {
            var n = {};
            n["id"]     = node.id;
            n["typeName"]   = node.typeName;
            n["exportName"]   = node.exportName;
            //n["inputs"] = node.inputs;// not needed imo

            var props = {};
            for(let prop of node.properties) {
                props[prop.name] = prop.getValue();
            }
            n["properties"] = props;

            nodes.push(n);
        }

        var connections = new Array();
        for(let con of this.conns) {
            var c = {};
            c["id"]             = con.id;
            c["leftNodeId"]     = con.leftNode.id;
            c["leftNodeOutput"] = con.leftNodeOutput;
            c["rightNodeId"]    = con.rightNode.id;
            c["rightNodeInput"] = con.rightNodeInput;

            connections.push(c);
        }

        var variables = new Array();
        for(let dvar of this.variables) {
            var v = {};
            v["id"] = dvar.id;
            v["type"] = dvar.type;
            v["property"] = dvar.property;

            var nodeIds = new Array();
            for(let n of dvar.nodes) {
                nodeIds.push({
                    nodeId:n.node.id,
                    name:n.propertyName
                });
            }
            v["linkedProperties"] = nodeIds;
            variables.push(v);
            console.log(v);
        }

        var data = {};
        data["nodes"] = nodes;
        data["connections"] = connections;
        data["variables"] = variables;
        return data;
    }

    static load(data:any, lib:DesignerLibrary) : Designer
    {
        var d = new Designer();
        var nodes = data["nodes"];
        for(let node of nodes) {
            var n = lib.create(node["typeName"]);
            n.exportName = node["exportName"];
            n.id = node["id"];

            // add node to it's properties will be initialized
            // todo: separate setting properties and inputs from setting shader in node
            d.addNode(n);

            // add properties
            var properties = node["properties"];
            for(var prop in properties) {
                n.setProperty(prop, properties[prop]);  
            }
        }

        var connections = data["connections"];
        for(let con of connections) {
            //var c = d.addConnection()
            var left = d.getNodeById(con.leftNodeId);
            var right = d.getNodeById(con.rightNodeId);

            // todo: support left index
            d.addConnection(left, right, con.rightNodeInput);
        }
        /*
        for(let dvar of this.variables) {
            var v = {};
            v["id"] = dvar.id;
            v["type"] = dvar.type;
            v["property"] = dvar.property;

            var nodeIds = new Array();
            for(let n of dvar.nodes) {
                nodeIds.push({
                    nodeId:n.node.id,
                    name:n.propertyName
                });
            }
            v["linkedProperties"] = nodeIds;
            variables.push(v);
            console.log(v);
        }
        */
        if (data.variables) {
            var variables = <DesignerVariable[]>data.variables;
            for(let v of variables) {
                //this.addVariable(v.name, v.displayName, )
                
                var dvar = d.addVariable(v.property.name, v.property.displayName, v.type);
                dvar.id = v.id;
                // copy values over to the property
                switch(dvar.type) {
                    case DesignerVariableType.Float:
                        (<FloatProperty>dvar.property).copyValuesFrom(<FloatProperty>v.property);
                    break;

                    case DesignerVariableType.Int:
                        (<IntProperty>dvar.property).copyValuesFrom(<IntProperty>v.property);
                    break;

                    case DesignerVariableType.Bool:
                        (<BoolProperty>dvar.property).copyValuesFrom(<BoolProperty>v.property);
                    break;

                    case DesignerVariableType.Enum:
                        (<EnumProperty>dvar.property).copyValuesFrom(<EnumProperty>v.property);
                    break;

                    case DesignerVariableType.Color:
                        (<ColorProperty>dvar.property).copyValuesFrom(<ColorProperty>v.property);
                    break;
                }

                // link properties
                for(let lp of (<any>v).linkedProperties) {
                    let node = d.getNodeById(lp.nodeId);
                    d.mapNodePropertyToVariable(v.property.name, node, lp.name);
                }
            }
        }


        return d;
    }
}

class NodeInput
{
    public node:DesignerNode;
    public name:string;
}

// SCENE VARIABLES

export enum DesignerVariableType
{
    None = 0,
    Float = 1,
    Int = 2,
    Bool = 3,
    Enum = 4,
    Color = 5,
    //Gradient
}

class DesignerNodePropertyMap
{
    public node:DesignerNode;
    public propertyName:string;
}

export class DesignerVariable
{
    id:string;

    type:DesignerVariableType;
    // used to keep the value in bounds
    property:Property;
    
    nodes:DesignerNodePropertyMap[] = new Array();
}