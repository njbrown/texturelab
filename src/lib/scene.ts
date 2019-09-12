import { ImageCanvas } from "./designer/imagecanvas";
import { Designer } from "./designer";
import { NodeGraphicsItem } from "./scene/nodegraphicsitem";
import { ConnectionGraphicsItem } from "./scene/connectiongraphicsitem";
import { SocketGraphicsItem, SocketType } from "./scene/socketgraphicsitem";
import { GraphicsItem } from "./scene/graphicsitem";

export class NodeScene {
  canvas: HTMLCanvasElement;
  context!: CanvasRenderingContext2D;
  contextExtra: any;

  nodes: NodeGraphicsItem[];
  conns: ConnectionGraphicsItem[];

  draggedNode?: NodeGraphicsItem;
  hitSocket?: SocketGraphicsItem;
  hitConnection?: ConnectionGraphicsItem;

  mouseX!: number;
  mouseY!: number;

  panning!: boolean;
  panStart!: SVGPoint;

  // callbacks
  onconnectioncreated?: (item: ConnectionGraphicsItem) => void;
  onconnectiondestroyed?: (item: ConnectionGraphicsItem) => void;
  onnodeselected?: (item: NodeGraphicsItem) => void;

  constructor(canvas: HTMLCanvasElement) {
    this.canvas = canvas;
    this.context = this.canvas.getContext("2d");
    addTransformExtrasToContext(this.context);
    this.contextExtra = this.context;
    this.nodes = new Array();
    this.conns = new Array();

    // bind event listeners
    var self = this;
    canvas.addEventListener("mousemove", function(evt: MouseEvent) {
      self.onMouseMove(evt);
    });
    canvas.addEventListener("mousedown", function(evt: MouseEvent) {
      self.onMouseDown(evt);
    });
    canvas.addEventListener("mouseup", function(evt: MouseEvent) {
      self.onMouseUp(evt);
    });
    canvas.addEventListener("mousewheel", function(evt: WheelEvent) {
      self.onMouseScroll(evt);
    });
    canvas.addEventListener("contextmenu", function(evt: MouseEvent) {
      evt.preventDefault();
    });
  }

  canvasToScene(x: number, y: number): SVGPoint {
    // var pt = new SVGPoint();
    // pt.x = x;
    // pt.y = y;

    // var t = this.currentContextTransform().inverse();
    return this.contextExtra.transformedPoint(x, y);
    //return this.contextExtra.screenToScene(x,y);
  }

  // assumes x and y are in canvas space (not scene space)
  zoom(x: number, y: number, level: number) {
    var scaleFactor = 1.01;
    var pt = this.contextExtra.transformedPoint(x, y);
    this.contextExtra.translate(pt.x, pt.y);
    var factor = Math.pow(scaleFactor, level);
    this.contextExtra.scale(factor, factor);
    this.contextExtra.translate(-pt.x, -pt.y);
  }

  currentContextTransform(): SVGMatrix {
    // black magic to make it work
    // `currentTransform` might still not be a part of the spec
    return (<any>this.context).currentTransform;
  }

  getHitItem(x: number, y: number): GraphicsItem {
    return null;
  }

  addNode(item: NodeGraphicsItem) {
    this.nodes.push(item);
  }

  getNodeById(id: string): NodeGraphicsItem {
    for (let node of this.nodes) {
      if (node.id == id) return node;
    }
    return null;
  }

  //todo: integrity check
  addConnection(con: ConnectionGraphicsItem) {
    this.conns.push(con);

    // link the sockets
    con.socketA.addConnection(con);
    con.socketB.addConnection(con);

    // callback
    if (this.onconnectioncreated) this.onconnectioncreated(con);
  }

  removeConnection(con: ConnectionGraphicsItem) {
    console.log("removing connection in scene");
    console.log(con);

    this.conns.splice(this.conns.indexOf(con), 1);
    //con.socketA.con = null;
    //con.socketB.con = null;
    con.socketA.removeConnection(con);
    con.socketB.removeConnection(con);

    // callback
    if (this.onconnectiondestroyed) this.onconnectiondestroyed(con);
  }

  // if the user click drags on a socket then it's making a connection
  drawActiveConnection() {
    let ctx = this.context;
    if (this.hitSocket) {
      ctx.beginPath();
      ctx.strokeStyle = "rgb(200, 200, 0)";
      ctx.lineWidth = 4;
      ctx.moveTo(this.hitSocket.centerX(), this.hitSocket.centerY());

      if (this.hitSocket.socketType == SocketType.Out) {
        ctx.bezierCurveTo(
          this.hitSocket.centerX() + 60,
          this.hitSocket.centerY(), // control point 1
          this.mouseX - 60,
          this.mouseY,
          this.mouseX,
          this.mouseY
        );
      } else {
        ctx.bezierCurveTo(
          this.hitSocket.centerX() - 60,
          this.hitSocket.centerY(), // control point 1
          this.mouseX + 60,
          this.mouseY,
          this.mouseX,
          this.mouseY
        );
      }
      ctx.stroke();
    }
  }

  clearAndDrawGrid() {
    //this.context.scale(2,2);
    this.context.fillStyle = "rgb(120, 120, 120)";
    var topCorner = this.canvasToScene(0, 0);
    var bottomCorner = this.canvasToScene(
      this.canvas.clientWidth,
      this.canvas.clientHeight
    );
    this.context.fillRect(
      topCorner.x,
      topCorner.y,
      bottomCorner.x - topCorner.x,
      bottomCorner.y - topCorner.y
    );
    //this.context.fillRect(0,0,this.canvas.width, this.canvas.height);

    // todo: draw grid
  }

  draw() {
    this.clearAndDrawGrid();
    // draw connections
    for (let con of this.conns) {
      con.draw(this.context);
    }

    if (this.hitSocket) {
      this.drawActiveConnection();
    }

    // draw nodes
    for (let item of this.nodes) {
      item.draw(this.context);
    }
  }

  // mouse events
  onMouseDown(evt: MouseEvent) {
    var pos = this.getScenePos(evt);
    this.mouseX = pos.x;
    this.mouseY = pos.y;

    if (evt.button == 1 || evt.button == 2) {
      this.panning = true;
      this.panStart = this.getScenePos(evt); // need its own copy
    }
    //console.log("mouse down: ",evt.button);
    else if (evt.button == 0) {
      // check for a hit socket first
      let hitSock: SocketGraphicsItem = this.getHitSocket(
        this.mouseX,
        this.mouseY
      );
      if (hitSock) {
        // if socket is an in socket with a connection, make hitsocket the connected out socket
        if (hitSock.socketType == SocketType.In && hitSock.hasConnections()) {
          this.hitSocket = hitSock.getConnection(0).socketA; // insockets should only have one connection
          // store connection for removal as well
          this.hitConnection = hitSock.getConnection(0);
        } else this.hitSocket = hitSock;
      } else {
        // if there isnt a hit socket then check for a hit node
        let hitNode: NodeGraphicsItem = this.getHitNode(
          this.mouseX,
          this.mouseY
        );
        this.draggedNode = hitNode;
        if (hitNode && this.onnodeselected) this.onnodeselected(hitNode);
      }
    }
  }

  onMouseUp(evt: MouseEvent) {
    var pos = this.getScenePos(evt);
    this.mouseX = pos.x;
    this.mouseY = pos.y;

    if (evt.button == 1 || evt.button == 2) {
      this.panning = false;
    } else if (evt.button == 0) {
      if (this.hitSocket) {
        let closeSock: SocketGraphicsItem = this.getHitSocket(
          this.mouseX,
          this.mouseY
        );

        if (
          closeSock &&
          closeSock != this.hitSocket &&
          closeSock.socketType != this.hitSocket.socketType &&
          closeSock.node != this.hitSocket.node
        ) {
          // close socket
          var con: ConnectionGraphicsItem = new ConnectionGraphicsItem();
          // out socket should be on the left, socketA
          if (this.hitSocket.socketType == SocketType.Out) {
            // out socket
            con.socketA = this.hitSocket;
            con.socketB = closeSock;

            // close sock is an inSocket which means it should only have one connection
            // remove current connection from inSocket
            if (closeSock.hasConnections())
              this.removeConnection(closeSock.getConnection(0));
          } else {
            // in socket
            con.socketA = closeSock;
            con.socketB = this.hitSocket;
          }

          // link connection
          //con.socketA.con = con;
          //con.socketB.con = con;

          this.addConnection(con);
        } else if (!closeSock) {
          // delete connection if hit node is an insock
          // if we're here it means one of 2 things:
          // 1: a new connection failed to form
          // 2: we're breaking a previously formed connection, which can only be done
          // by dragging from an insock that already has a connection

          if (this.hitSocket.socketType == SocketType.Out) {
            /*
                        if (this.hitSocket.hasConnections()) {
                            // remove connection
                            //let con = this.hitSocket.con;
                            this.removeConnection(this.hitSocket.getConnectionFrom(this.hitSocket));
                        }
                        */

            if (this.hitConnection) this.removeConnection(this.hitConnection);
          }
        }
      }

      this.draggedNode = null;
      this.hitSocket = null;
      this.hitConnection = null;
    }
  }

  onMouseMove(evt: MouseEvent) {
    var lastX = this.mouseX;
    var lastY = this.mouseY;
    var pos = this.getScenePos(evt);
    this.mouseX = pos.x;
    this.mouseY = pos.y;

    if (this.panning) {
      // convert to scene space first
      //var lastPt = this.contextExtra.transformedPoint(lastX, lastY);
      //var pt = this.contextExtra.transformedPoint(this.mouseX, this.mouseY);
      //this.context.translate(pt.x - lastPt.x, pt.y - lastPt.y);
      //console.log(pt.x - this.panStart.x, pt.y - this.panStart.y);
      console.log(this.mouseX - this.panStart.x, this.mouseY - this.panStart.y);
      this.context.translate(
        this.mouseX - this.panStart.x,
        this.mouseY - this.panStart.y
      );
      //this.panStart = pos;
    }

    // handle dragged socket
    if (this.hitSocket) {
    }

    // handle dragged node
    if (this.draggedNode != null) {
      var diff = this.canvasToScene(evt.movementX, evt.movementY);
      //console.log("move: ",evt.movementX,evt.movementY);
      this.draggedNode.move(evt.movementX, evt.movementY);
    }
  }

  onMouseScroll(evt: WheelEvent) {
    var pos = _getMousePos(this.canvas, evt);
    var delta = evt.wheelDelta / 40;
    this.zoom(pos.x, pos.y, delta);

    evt.preventDefault();
    return false;
  }

  // hit detection
  getHitNode(x: number, y: number): NodeGraphicsItem {
    // todo: sort items from front to back
    for (let node of this.nodes) {
      if (node.isPointInside(x, y)) return node;
    }

    return null;
  }

  getHitSocket(x: number, y: number): SocketGraphicsItem {
    // todo: sort items from front to back
    for (let node of this.nodes) {
      for (let sock of node.sockets) {
        if (sock.isPointInside(x, y)) return sock;
      }
    }

    return null;
  }

  // UTILITY

  // returns the scene pos from the mouse event
  getScenePos(evt: MouseEvent) {
    var canvasPos = _getMousePos(this.canvas, evt);
    return this.canvasToScene(canvasPos.x, canvasPos.y);
  }

  // SAVE/LOAD

  // only save position data to associative array
  save(): any {
    var data: any = {};

    var nodes = {};
    for (let node of this.nodes) {
      var n: any = {};
      n["id"] = node.id;
      n["x"] = node.centerX();
      n["y"] = node.centerY();

      nodes[node.id] = n;
    }
    data["nodes"] = nodes;

    return data;
  }

  static load(
    designer: Designer,
    data: any,
    canvas: HTMLCanvasElement
  ): NodeScene {
    var s = new NodeScene(canvas);

    // add nodes one by one
    for (let dNode of designer.nodes) {
      // create node from designer
      var node = new NodeGraphicsItem(dNode.title);
      for (let input of dNode.getInputs()) {
        node.addSocket(input, input, SocketType.In);
      }
      node.addSocket("output", "output", SocketType.Out);
      s.addNode(node);
      node.id = dNode.id;

      // get position
      var x = data["nodes"][node.id].x;
      var y = data["nodes"][node.id].y;
      node.setCenter(x, y);
    }

    // add connection one by one
    for (let dcon of designer.conns) {
      var con = new ConnectionGraphicsItem();
      con.id = dcon.id;

      // get nodes
      var leftNode = s.getNodeById(dcon.leftNode.id);
      var rightNode = s.getNodeById(dcon.rightNode.id);

      // get sockets
      con.socketA = leftNode.getOutSocketByName(dcon.leftNodeOutput);
      con.socketB = rightNode.getInSocketByName(dcon.rightNodeInput);

      s.addConnection(con);
    }

    return s;
  }
}

// https://www.html5canvastutorials.com/advanced/html5-canvas-mouse-coordinates/
// https://stackoverflow.com/questions/17130395/real-mouse-position-in-canvas
function _getMousePos(canvas, evt) {
  var rect = canvas.getBoundingClientRect();
  return {
    x: evt.clientX - rect.left,
    y: evt.clientY - rect.top
  };
}

// https://codepen.io/techslides/pen/zowLd
// add functions that should have been a part of the 2d context api
//function trackTransforms(ctx){
function addTransformExtrasToContext(ctx: any) {
  var svg = document.createElementNS("http://www.w3.org/2000/svg", "svg");
  var xform = <SVGMatrix>svg.createSVGMatrix();
  ctx.getTransform = function() {
    return xform;
  };

  var savedTransforms: SVGMatrix[] = [];
  var save = ctx.save;
  ctx.save = function() {
    savedTransforms.push(xform.translate(0, 0));
    return save.call(ctx);
  };

  var restore = ctx.restore;
  ctx.restore = function() {
    xform = <SVGMatrix>savedTransforms.pop();
    return restore.call(ctx);
  };

  var scale = ctx.scale;
  ctx.scale = function(sx: number, sy: number) {
    xform = (<any>xform).scaleNonUniform(sx, sy);
    return scale.call(ctx, sx, sy);
  };

  var rotate = ctx.rotate;
  ctx.rotate = function(radians: number) {
    xform = xform.rotate((radians * 180) / Math.PI);
    return rotate.call(ctx, radians);
  };

  var translate = ctx.translate;
  ctx.translate = function(dx: number, dy: number) {
    xform = xform.translate(dx, dy);
    return translate.call(ctx, dx, dy);
  };

  var transform = ctx.transform;
  ctx.transform = function(
    a: number,
    b: number,
    c: number,
    d: number,
    e: number,
    f: number
  ) {
    var m2 = svg.createSVGMatrix();
    m2.a = a;
    m2.b = b;
    m2.c = c;
    m2.d = d;
    m2.e = e;
    m2.f = f;
    xform = xform.multiply(m2);
    return transform.call(ctx, a, b, c, d, e, f);
  };

  var setTransform = ctx.setTransform;
  ctx.setTransform = function(
    a: number,
    b: number,
    c: number,
    d: number,
    e: number,
    f: number
  ) {
    xform.a = a;
    xform.b = b;
    xform.c = c;
    xform.d = d;
    xform.e = e;
    xform.f = f;
    return setTransform.call(ctx, a, b, c, d, e, f);
  };

  var pt = svg.createSVGPoint();
  ctx.transformedPoint = function(x: number, y: number) {
    pt.x = x;
    pt.y = y;
    return pt.matrixTransform(xform.inverse());
  };

  ctx.screenToScene = function(x: number, y: number) {
    pt.x = x;
    pt.y = y;
    return pt.matrixTransform(xform.inverse());
  };

  ctx.screenToScene = function(x: number, y: number) {
    pt.x = x;
    pt.y = y;
    return pt.matrixTransform(xform);
  };
}
