//import * as scene from "./scene";
import { Designer } from "./designer";
import { DesignerNode } from "./designer/designernode";
import { Property, PropertyType } from "./designer/properties";
import { Guid } from "./utils";
import { DesignerVariableType } from "./designer/designervariable";
import { DesignerLibrary } from "./designer/library";
import { NodeScene } from "./scene";
import { ConnectionGraphicsItem } from "./scene/connectiongraphicsitem";
import { NodeGraphicsItem } from "./scene/nodegraphicsitem";
import { SocketType } from "./scene/socketgraphicsitem";

function hexToRgb(hex) {
  var result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
  return result
    ? {
        r: parseInt(result[1], 16),
        g: parseInt(result[2], 16),
        b: parseInt(result[3], 16)
      }
    : null;
}

function rgbToHex(r, g, b) {
  return "#" + ((1 << 24) + (r << 16) + (g << 8) + b).toString(16).slice(1);
}

// stores the IDs for the display nodes
export class DisplayNodes {
  public albedoNode: string;
  public normalNode: string;
  public roughnessNode: string;
  public heightNode: string;
  public metallicNode: string;
  /*
    public albedoCanvas : ImageCanvas = new ImageCanvas();
    public normalCanvas : ImageCanvas = new ImageCanvas();
    public roughnessCanvas : ImageCanvas = new ImageCanvas();
    public heightCanvas : ImageCanvas = new ImageCanvas();
    public metallicCanvas : ImageCanvas = new ImageCanvas();

    resize(width:number, height:number)
    {
        this.albedoCanvas.resize(width, height);
        this.normalCanvas.resize(width, height);
        this.roughnessCanvas.resize(width, height);
        this.heightCanvas.resize(width, height);
        this.metallicCanvas.resize(width, height);
    }
    */
}

export enum DisplayChannel {
  Albedo,
  Metallic,
  Roughness,
  Normal,
  Height
}

export class Editor {
  canvas: HTMLCanvasElement;

  graph: NodeScene;
  designer: Designer;
  selectedDesignerNode: DesignerNode;

  preview2D: HTMLCanvasElement;
  preview2DCtx: CanvasRenderingContext2D;

  scene3D: any; // todo: set a type

  //   propGen: PropertyGenerator;
  //   varGen: VariableGenerator;
  displayNodes: DisplayNodes;

  onnodeselected?: (item: DesignerNode) => void;
  onpreviewnode?: (item: DesignerNode, image: HTMLCanvasElement) => void;

  constructor() {
    this.displayNodes = new DisplayNodes();
    this.selectedDesignerNode = null;
  }
  /*
    constructor(canvas:HTMLCanvasElement, preview2D:HTMLCanvasElement, propHolder : HTMLElement, varHolder : HTMLElement, scene3D:any)
    {
        this.canvas = canvas;

        this.displayNodes = new DisplayNodes();

        this.preview2D = preview2D;
        this.preview2DCtx = preview2D.getContext("2d");

        this.scene3D = scene3D;
        this.selectedDesignerNode = null;

        this.propGen = new PropertyGenerator(this, propHolder);
        this.varGen = new VariableGenerator(this, varHolder);
        

        //this.setDesigner(new Designer());
        //this.setScene(new NodeScene(canvas));
    }
    */

  // creates new texture
  // requires canvas to be already set
  createNewTexture() {
    this.setDesigner(new Designer());
    this.setScene(new NodeScene(this.canvas));
  }

  set2DPreview(preview2D: HTMLCanvasElement) {
    this.preview2D = preview2D;
    this.preview2DCtx = preview2D.getContext("2d");
  }

  setSceneCanvas(canvas: HTMLCanvasElement) {
    this.canvas = canvas;
    this.setScene(new NodeScene(canvas));
  }

  resizeScene(width: number, height: number) {
    this.canvas.width = width;
    this.canvas.height = height;
  }

  // setPropertyHolder(propHolder : HTMLElement)
  // {
  //     this.propGen = new PropertyGenerator(this, propHolder);
  // }

  // setVariableHolder(varHolder : HTMLElement)
  // {
  //     this.varGen = new VariableGenerator(this, varHolder);
  //     if (this.designer)
  //         this.varGen.setDesigner(this.designer);
  // }

  set3DScene(scene3D: any) {
    this.scene3D = scene3D;
  }

  setDesigner(designer: Designer) {
    this.designer = designer;
    var self = this;

    designer.onnodetextureupdated = function(dnode) {
      var graphNode = self.graph.getNodeById(dnode.id);
      self.designer.copyNodeTextureToImageCanvas(dnode, graphNode.imageCanvas);

      if (self.onpreviewnode) {
        if (dnode == self.selectedDesignerNode)
          self.onpreviewnode(dnode, graphNode.imageCanvas.canvas);
      }
      // if(node == self.selectedDesignerNode) {
      //     requestAnimationFrame(function(){
      //         self.preview2DCtx.clearRect(0,0,self.preview2D.width, self.preview2D.height);
      //         self.preview2DCtx.drawImage(graphNode.imageCanvas.canvas,
      //             0,0,
      //         self.preview2D.width, self.preview2D.height);
      //     });
      // }

      self.updateDisplayNode(graphNode);
    };

    /*
        designer.onthumbnailgenerated = function(node, thumb) {
            console.log(self.selectedDesignerNode);
            console.log("onthumbnailgenerated generated for: "+node.title);
            // refresh right node image
            var graphNode = self.graph.getNodeById(node.id);
            graphNode.setThumbnail(thumb);
            self.updateDisplayNode(graphNode);

            if(node == self.selectedDesignerNode) {
                requestAnimationFrame(function(){
                    self.preview2DCtx.clearRect(0,0,self.preview2D.width, self.preview2D.height);
                    self.preview2DCtx.drawImage(thumb,
                        0,0,
                    self.preview2D.width, self.preview2D.height);
                });

                
            }
        }
        */

    //if (this.varGen) this.varGen.setDesigner(designer);
    //this.propGen.setDesigner(designer);
  }

  setScene(scene: NodeScene) {
    this.graph = scene;

    var self = this;
    this.graph.onconnectioncreated = function(con: ConnectionGraphicsItem) {
      // get node from graph
      var leftNode = con.socketA.node;
      var rightNode = con.socketB.node;

      // get node from designer and connect them
      var leftDNode = self.designer.getNodeById(leftNode.id);
      var rightDNode = self.designer.getNodeById(rightNode.id);

      // make connection
      // switch from `title` to `name`
      self.designer.addConnection(leftDNode, rightDNode, con.socketB.title);

      // refresh right node image
      //var thumb = self.designer.generateImageFromNode(rightDNode);
      //rightNode.setThumbnail(thumb);
    };

    this.graph.onconnectiondestroyed = function(con: ConnectionGraphicsItem) {
      // get node from graph
      var leftNode = con.socketA.node;
      var rightNode = con.socketB.node;

      // get node from designer and connect them
      var leftDNode = self.designer.getNodeById(leftNode.id);
      var rightDNode = self.designer.getNodeById(rightNode.id);

      // remove connection
      // switch from `title` to `name`
      self.designer.removeConnection(leftDNode, rightDNode, con.socketB.title);

      // clear right node image
      rightNode.setThumbnail(null);
    };

    this.graph.onnodeselected = function(node: NodeGraphicsItem) {
      var dnode = self.designer.getNodeById(node.id);
      self.selectedDesignerNode = dnode;
      //console.log(dnode);

      if (true) {
        if (self.preview2DCtx) {
          self.preview2DCtx.drawImage(
            node.imageCanvas.canvas,
            0,
            0,
            self.preview2D.width,
            self.preview2D.height
          );
        }

        // todo: move to double click
        if (self.onpreviewnode) {
          self.onpreviewnode(dnode, node.imageCanvas.canvas);
        }

        //console.log(this.scene3D);
        if (self.scene3D) {
          //console.log("setting height texture");
          //self.scene3D.setHeightTexture(node.thumbnail);
          self.updateDisplayNode(node);
        }
      }

      //var thumb = self.designer.generateImageFromNode(dnode);
      // thumbnail will be generated eventually

      // display node properties
      // if (self.propGen) self.propGen.setNode(dnode);

      if (self.onnodeselected) self.onnodeselected(dnode);
    };

    // property changes
    /*
        this.propGen.onnodepropertychanged = function(dnode:DesignerNode, prop:Property) {
            //var node = self.graph.getNodeById(node.id);
            //self.graph.refreshNode()
            
            // todo: do this properly
            var thumb = self.designer.generateImageFromNode(dnode);
            var node = self.graph.getNodeById(dnode.id);
            node.thumbnail = thumb;

            //console.log(node.thumbnail);
            requestAnimationFrame(function(){
                self.preview2DCtx.clearRect(0,0,self.preview2D.width, self.preview2D.height);
                self.preview2DCtx.drawImage(thumb,
                    0,0,
                self.preview2D.width, self.preview2D.height);
            });
            
            // just a stest
            //self.scene3D.setHeightTexture(node.thumbnail);
            self.updateDisplayNode(node);
        }
        */
  }

  // adds node
  // x and y are screen space
  addNode(
    dNode: DesignerNode,
    screenX: number = 0,
    screenY: number = 0
  ): NodeGraphicsItem {
    // must add to designer first
    this.designer.addNode(dNode);

    // create node from designer
    var node = new NodeGraphicsItem(dNode.title);
    for (let input of dNode.getInputs()) {
      node.addSocket(input, input, SocketType.In);
    }
    node.addSocket("output", "output", SocketType.Out);
    this.graph.addNode(node);
    node.id = dNode.id;

    // generate thumbnail
    var thumb = this.designer.generateImageFromNode(dNode);
    node.setThumbnail(thumb);

    var pos = this.graph.canvasToScene(screenX, screenY);
    node.setCenter(pos.x, pos.y);

    return node;
  }

  // DISPLAY NODE FUNCTIONS

  // updates appropriate image if set
  updateDisplayNode(node: NodeGraphicsItem) {
    if (!this.scene3D) return;

    //console.log(node.id);
    //console.log(this.displayNodes.normalNode);

    // TODO: create custom CanvasImage that resizes with
    // the texture size. NodeGraphicsItem's CanvasImage is fixed
    // to 1024x1024. Another option is to give each DesignerNode a
    // CanvasImage that updates when its texture updates then pass
    // it to NodeGraphicsitem. That way it gets used one place and
    // gets updated everywhere else all at once.
    if (node.id == this.displayNodes.albedoNode) {
      //this.scene3D.setAlbedoTexture(node.thumbnail);
      this.scene3D.setAlbedoCanvasTexture(node.imageCanvas.canvas);
    }

    if (node.id == this.displayNodes.metallicNode) {
      //this.scene3D.setMetallicTexture(node.thumbnail);
      this.scene3D.setMetallicCanvasTexture(node.imageCanvas.canvas);
    }

    if (node.id == this.displayNodes.normalNode) {
      //this.scene3D.setNormalTexture(node.thumbnail);
      this.scene3D.setNormalCanvasTexture(node.imageCanvas.canvas);
    }

    if (node.id == this.displayNodes.roughnessNode) {
      //this.scene3D.setRoughnessTexture(node.thumbnail);
      this.scene3D.setRoughnessCanvasTexture(node.imageCanvas.canvas);
    }

    if (node.id == this.displayNodes.heightNode) {
      //this.scene3D.setHeightTexture(node.thumbnail);
      this.scene3D.setHeightCanvasTexture(node.imageCanvas.canvas);
    }
  }

  setDisplayChannelNode(channel: DisplayChannel, nodeId: string) {
    var node = this.graph.getNodeById(nodeId);
    if (channel == DisplayChannel.Albedo) {
      this.displayNodes.albedoNode = nodeId;
    }
    if (channel == DisplayChannel.Metallic) {
      this.displayNodes.metallicNode = nodeId;
    }
    if (channel == DisplayChannel.Normal) {
      this.displayNodes.normalNode = nodeId;
    }
    if (channel == DisplayChannel.Roughness) {
      this.displayNodes.roughnessNode = nodeId;
    }
    if (channel == DisplayChannel.Height) {
      this.displayNodes.heightNode = nodeId;
    }

    this.updateDisplayNode(node);
  }

  exposeVariable(node: DesignerNode, prop: Property, varDisplayName: string) {
    // create new variable
    var varName = Guid.newGuid();
    var dvar = this.designer.addVariable(
      varName,
      varDisplayName,
      this.evalDesignerVariableType(prop)
    );
    // copy over important props
    // todo:make more elegant
    dvar.property = prop.clone();
    dvar.property.name = varName;
    dvar.property.displayName = varDisplayName;

    // add it to scene and bind prop
    this.designer.mapNodePropertyToVariable(varName, node, prop.name);

    // copy property props

    // refresh var ui
    // this.varGen.refreshUi();
  }

  evalDesignerVariableType(prop: Property): DesignerVariableType {
    if (prop.type == PropertyType.Float) {
      return DesignerVariableType.Float;
    } else if (prop.type == PropertyType.Int) {
      return DesignerVariableType.Int;
    } else if (prop.type == PropertyType.Bool) {
      return DesignerVariableType.Bool;
    } else if (prop.type == PropertyType.Enum) {
      return DesignerVariableType.Enum;
    } else if (prop.type == PropertyType.Color) {
      return DesignerVariableType.Color;
    } else {
      console.log("error: invalid property type for variable", prop);
    }

    return null;
  }

  update() {
    if (this.designer) this.designer.update();
  }

  draw() {
    if (this.graph) this.graph.draw();
  }

  load(data: any, lib: DesignerLibrary) {
    // load scene
    var d = Designer.load(data, lib);

    // load graph
    var g = NodeScene.load(d, data["scene"], this.canvas);

    // load editor data
    if (data["editor"] != null) {
      var e = data["editor"];
      console.log("loading editor data");
      console.log(e.displayNodes);

      this.displayNodes.albedoNode = e.displayNodes.albedoNode;
      this.displayNodes.metallicNode = e.displayNodes.metallicNode;
      this.displayNodes.normalNode = e.displayNodes.normalNode;
      this.displayNodes.roughnessNode = e.displayNodes.roughnessNode;
      this.displayNodes.heightNode = e.displayNodes.heightNode;
    }

    //todo: properly destroy existing graph

    //this.designer = d;
    //this.graph = g;
    this.setDesigner(d);
    this.setScene(g);
  }

  save(): any {
    var data = this.designer.save();
    data["scene"] = this.graph.save();
    data["editor"] = {
      displayNodes: this.displayNodes
    };

    return data;
  }
}
