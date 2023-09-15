 // All "ABOVE***" variable are used to know above which connectors the mouse is positionned
var ABOVESENDER = false;
var ABOVERECEIVER= false;
var ABOVEINFO = null;
var WORKINGCOPY = "static/json/dummyFile.json"; // Temporary JSON file to work on
var TEMPORARYINFO = null; // To store information needed to be accessed globally

class Module {
    constructor(name,host,type,port,position){
        this.ID = moduleID;

        if(name == null){
            this.name = String(moduleID);
        }
        else{
            this.name = String(name);
        }

        this.host = String(host)
        this.type = String(type)

        if(port==null){
            this.port = portRange("module",-1)
        }
        else{
            this.port = port
        }

        this.moduleCont = new createjs.Container();
        moduleID += 1;

        this.drag = new createjs.Container(); //Part of the module to grab when the user wants to drag it
        this.bottom = new createjs.Container();

        this.connections = new Object();
        this.connections["senders"] = new Object();
        this.connections["receivers"] = new Object();

        this.connectionLine = new createjs.Shape(); // Object used to draw a line before it is connected to two connectors

        this.chidSendArray = [];
        this.chidReceiveArray = [];

        this.chidCount = 0;

        this.ioSize = 30; // size of the connectors

        this.initModule(position);
    }

    initModule(position){
        this.moduleCont.name = "module" + String(name); // Debugging information
        this.moduleCont.x = position[0];
        this.moduleCont.y = position[1];

        this.drag.name = "drag"; // Debugging information
        this.drag.x = this.drag.y = 0;

        // Build the module shape
        var topSquare = new createjs.Shape();
        this.drawTopSquare(topSquare);
        if(this.name == String(null)){
            var labelName = new createjs.Text("Module " + String(this.ID), "bold 14px Arial", "#000000");
        }
        else{
            var labelName = new createjs.Text(String(this.name), "bold 14px Arial", "#000000");
        }
        labelName.textAlign = "center";
        labelName.y = 20;
        labelName.x = 100;

        var labelHost = new createjs.Text("Host:"+String(this.host), "bold 14px Arial", "#000000");
        labelHost.textAlign = "center";
        labelHost.y = 50;
        labelHost.x = 100;

        this.drag.addChild(topSquare, labelName, labelHost);

        this.bottom.name = "bottomBlock";
        this.bottom.x = 0;
        this.bottom.y = this.drag.getBounds().height;

        var botSquare = new createjs.Shape();
        botSquare.name = "botSquare";
        botSquare.graphics.beginFill('#5755d9');
        botSquare.graphics.beginStroke("#000");
        botSquare.graphics.setStrokeStyle(1);
        botSquare.graphics.drawRect(0, 0, 200, 150);
        botSquare.graphics.endFill();
        botSquare.setBounds(0, 0, 200, 150);

        var buttonsContainer = new createjs.Container();
        buttonsContainer.name = "buttonsContainer";
        let widthButtons = 60;
        let heightButtons = 40;
        let space = 7;
        buttonsContainer.x = botSquare.getBounds().width/2-widthButtons/2;
        buttonsContainer.y = 0;

        var addSenderButton = new createjs.Shape();
        addSenderButton.graphics.beginFill('#32b643');
        addSenderButton.graphics.drawRoundRectComplex(0,space,widthButtons,heightButtons,10,10,10,10);
        addSenderButton.graphics.endFill();

        var senderIcon = new createjs.Text("S", "bold 14px Arial", "#000000");
        senderIcon.textAlign = "center";
        senderIcon.x = widthButtons/2;
        senderIcon.y = space + 15;

        var addReceiverButton = new createjs.Shape();
        var outCounts = 0;
        addReceiverButton.graphics.beginFill('#ffb700');
        addReceiverButton.graphics.drawRoundRectComplex(0,heightButtons+2*space,widthButtons,heightButtons,10,10,10,10);
        addReceiverButton.graphics.endFill();

        var receiverIcon = new createjs.Text("R", "bold 14px Arial", "#000000");
        receiverIcon.textAlign = "center";
        receiverIcon.x = widthButtons/2;
        receiverIcon.y = heightButtons + space + 20;

        // Add all the new shapes to the main stage
        buttonsContainer.addChild(addSenderButton,addReceiverButton,senderIcon,receiverIcon);

        this.bottom.addChild(botSquare,buttonsContainer);

        this.moduleCont.addChild(this.bottom,this.drag);
        stage.addChild(this.moduleCont,this.connectionLine);

        //-----------------------------Add event listeners---------------------------

        // ---------------------Drag and drop listeners------------------------------
        this.drag.addEventListener("mousedown",  event => {
            var sX = Math.floor(event.stageX);
            var sY = Math.floor(event.stageY);
            this.moduleCont.dX = sX - this.moduleCont.x;
            this.moduleCont.dY = sY - this.moduleCont.y;
        });

        this.drag.addEventListener("pressmove",  event => {
            var sX = Math.floor(event.stageX);
            var sY = Math.floor(event.stageY);
            this.moduleCont.x = sX - this.moduleCont.dX;
            this.moduleCont.y = sY - this.moduleCont.dY;
            stage.update();
        });
        // --------------------------------------------------------------------

        // -----------------Changes color when beging dragged----------------------------
        this.drag.addEventListener("mouseover", event => {
            this.drawTopSquare(topSquare);
            topSquare.alpha = 0.5;
            stage.update();
        });

        this.drag.addEventListener("mouseout", event =>{
            this.drawTopSquare(topSquare);
            topSquare.alpha = 1;
            stage.update();
        });
        // ---------------------------------------------------------------

        // ---------------New connectors listeners------------------------
        addSenderButton.addEventListener("click", event => {
            this.makeSenderContainer();
            writeJSON();
            stage.update();
        });

        addReceiverButton.addEventListener("click", event => {
            this.makeReceiverContainer();
            writeJSON();
            stage.update();
        });

        //---------------------------------------------------------------------------
    }

    drawTopSquare(square){  // Draw top part of the module
        square.graphics.clear();
        square.graphics.beginStroke("#000");
        square.graphics.setStrokeStyle(1);
        square.graphics.beginFill('#e85600');
        square.graphics.drawRoundRectComplex(0, 0, 200, 80,20,20,0,0);
        square.graphics.endFill();
        square.setBounds(0, 0, 200, 80);
    }

    makeSenderContainer(){ // Build and set up the sender container
        let index = this.chidSendArray.length;
        if(this.checkAddInOut(this.ID,index,"send")){
            this.chidSendArray.push(new createjs.Container());
            this.chidSendArray[index].x = this.moduleCont.getBounds().width - 40;
            this.chidSendArray[index].y = index*this.ioSize;
            this.chidSendArray[index].name = "sender"+String(index);

            var sendSquare = new createjs.Shape();
            this.drawSendSquare('#32b643',sendSquare);

            var sendLabel = new createjs.Text(String(index), "bold 14px Arial", "#000000");
            sendLabel.textAlign = "center";
            sendLabel.y = this.ioSize/2 - 5;
            sendLabel.x = 20;

            this.chidSendArray[index].addChild(sendSquare,sendLabel);
            this.bottom.addChild(this.chidSendArray[index])
            this.initDrawingSend(index,this.chidSendArray[index])

            //----------------Event listeners--------------
            // Color is changed when the mouse is above a sender container
            this.chidSendArray[index].addEventListener("mouseover", event=>{
                this.drawSendSquare('#8AF598',sendSquare);
                // Infomation to be shared with other modules
                ABOVEINFO = [this.ID,event.currentTarget["name"][6],this.host];
                ABOVESENDER = true;
                stage.update();
            });
            this.chidSendArray[index].addEventListener("mouseout", event=>{
                this.drawSendSquare('#32b643',sendSquare);
                ABOVEINFO = null;
                ABOVESENDER = false;
                stage.update();
            });
            //---------------------------------------------
        }
    }

    makeSenderContainerREADINGONLY(){
        // Same as above, but without the check that the previous sender container has a connection.
        // It is used when reading a pre-existing JSON file
        let index = this.chidSendArray.length;
        this.chidSendArray.push(new createjs.Container());
        this.chidSendArray[index].x = this.moduleCont.getBounds().width - 40;
        this.chidSendArray[index].y = index*this.ioSize;
        this.chidSendArray[index].name = "sender"+String(index);

        var sendSquare = new createjs.Shape();
        this.drawSendSquare('#32b643',sendSquare);

        var sendLabel = new createjs.Text(String(index), "bold 14px Arial", "#000000");
        sendLabel.textAlign = "center";
        sendLabel.y = this.ioSize/2 - 5;
        sendLabel.x = 20;

        this.chidSendArray[index].addChild(sendSquare,sendLabel);
        this.bottom.addChild(this.chidSendArray[index])
        this.initDrawingSend(index,this.chidSendArray[index])

        //----------------Event listeners--------------
        this.chidSendArray[index].addEventListener("mouseover", event=>{
            this.drawSendSquare('#8AF598',sendSquare);
            ABOVEINFO = [this.ID,event.currentTarget["name"][6],this.host];
            ABOVESENDER = true;
            stage.update();
        });
        this.chidSendArray[index].addEventListener("mouseout", event=>{
            this.drawSendSquare('#32b643',sendSquare);
            ABOVEINFO = null;
            ABOVESENDER = false;
            stage.update();
        });
        //---------------------------------------------
    }

    drawSendSquare(color,square){
        square.graphics.clear();
        square.graphics.beginStroke("#000");
        square.graphics.setStrokeStyle(1);
        square.graphics.beginFill(color);
        square.graphics.drawRect(0, 0, 40, this.ioSize);
        square.graphics.endFill();
    }

    makeReceiverContainer(){ // Build and set up the receiver container
        let index = this.chidReceiveArray.length;
        if(this.checkAddInOut(this.ID,index,"receiver")){
            this.chidReceiveArray.push(new createjs.Container());
            this.chidReceiveArray[index].x = 0;
            this.chidReceiveArray[index].y = index*this.ioSize;
            this.chidReceiveArray[index].name = "receiver"+String(index);


            var receiveSquare = new createjs.Shape();
            this.drawReceiveSquare('#ffb700',receiveSquare);

            var receiveLabel = new createjs.Text(String(index), "bold 14px Arial", "#000000");
            receiveLabel.textAlign = "center";
            receiveLabel.y = this.ioSize/2 - 5;
            receiveLabel.x = 20;

            this.chidReceiveArray[index].addChild(receiveSquare,receiveLabel);
            this.bottom.addChild(this.chidReceiveArray[index]);
            this.initDrawingReceive(index,this.chidReceiveArray[index]);

            //----------------Event listeners--------------
            // Color is changed when the mouse is above a receiver container
            this.chidReceiveArray[index].addEventListener("mouseover", event=>{
                this.drawReceiveSquare('#E0D5B8',receiveSquare);
                // Infomation to be shared with other modules
                ABOVEINFO = [this.ID,event.currentTarget["name"][8],this.host];
                ABOVERECEIVER = true;
                stage.update();
            });
            this.chidReceiveArray[index].addEventListener("mouseout", event=>{
                this.drawReceiveSquare('#ffb700',receiveSquare);
                ABOVEINFO = null;
                ABOVERECEIVER = false;
                stage.update();
            });
            //---------------------------------------------
        }
    }

    makeReceiverContainerREADINGONLY(){
        // Same as above, but without the check that the previous receiver container has a connection.
        // It is used when reading a pre-existing JSON file
        let index = this.chidReceiveArray.length;
        this.chidReceiveArray.push(new createjs.Container());
        this.chidReceiveArray[index].x = 0;
        this.chidReceiveArray[index].y = index*this.ioSize;
        this.chidReceiveArray[index].name = "receiver"+String(index);

        var receiveSquare = new createjs.Shape();
        this.drawReceiveSquare('#ffb700',receiveSquare);

        var receiveLabel = new createjs.Text(String(index), "bold 14px Arial", "#000000");
        receiveLabel.textAlign = "center";
        receiveLabel.y = this.ioSize/2 - 5;
        receiveLabel.x = 20;

        this.chidReceiveArray[index].addChild(receiveSquare,receiveLabel);
        this.bottom.addChild(this.chidReceiveArray[index]);
        this.initDrawingReceive(index,this.chidReceiveArray[index]);

        //----------------Event listeners--------------
        this.chidReceiveArray[index].addEventListener("mouseover", event=>{
            this.drawReceiveSquare('#E0D5B8',receiveSquare);
            ABOVEINFO = [this.ID,event.currentTarget["name"][8],this.host];
            ABOVERECEIVER = true;
            stage.update();
        });
        this.chidReceiveArray[index].addEventListener("mouseout", event=>{
            this.drawReceiveSquare('#ffb700',receiveSquare);
            ABOVEINFO = null;
            ABOVERECEIVER = false;
            stage.update();
        });
        //---------------------------------------------
    }

    drawReceiveSquare(color,square){
        square.graphics.clear();
        square.graphics.beginStroke("#000");
        square.graphics.setStrokeStyle(1);
        square.graphics.beginFill(color);
        square.graphics.drawRect(0, 0, 40, this.ioSize);
        square.graphics.endFill();
    }

    initDrawingSend(index,item){ // Set up the drawing lines between a sender container and a receiver container
        this.connections["senders"][index] = [];

        var initX,initY,endX,endY;

        item.addEventListener("mousedown", event => {
            initX = this.moduleCont.x + this.drag.getBounds().width;
            initY = this.moduleCont.y + this.drag.getBounds().height + index*this.ioSize + 15;
        });

        item.addEventListener("pressmove", event => {
            var mouseX = Math.floor(event.stageX);
            var mouseY = Math.floor(event.stageY);
            drawLine(this.connectionLine,initX,initY,mouseX,mouseY);
        });

        item.addEventListener("pressup", event => {
            // Checks is the final position of the mouse is a valid connecting point
            var stateConnection;
            if(ABOVEINFO != null){
              stateConnection = checkConnection(String(this.ID),String(index),String(ABOVEINFO[0]),String(ABOVEINFO[1]));
            }
            if(!ABOVERECEIVER || stateConnection == "exists"){
                this.connectionLine.graphics.clear();
                stage.update();
            }
            else{
                // Draws the connection and saves it
                this.connectionLine.graphics.clear();
                TEMPORARYINFO = [String(this.ID),String(index),String(ABOVEINFO[0]),String(ABOVEINFO[1])];
                if(stateConnection == "new") {
                    // It is a new connection, displays a form to fill the receiver informations
                    callConnectForm(String(ABOVEINFO[0]),String(ABOVEINFO[1]),String(ABOVEINFO[2]));
                }
                else if(stateConnection == "receiver_full"){
                    copyConnectors("receiver");
                }
                else if(stateConnection == "sender_full"){
                    copyConnectors("sender");
                }
                else{
                    console.log("Error: ???")
                }
                connectLines(String(this.ID),String(index),String(ABOVEINFO[0]),String(ABOVEINFO[1]));
            }
        });
        // Updates lines when the module is dragged
        this.drag.addEventListener("pressmove",  event =>{
            var i;
            for(i=0;i<this.connections["senders"][index].length;i++){
                let up = this.connections["senders"][index][i];
                updateLines(overallConnections[up],up.split(":")[0],up.split(":")[1],up.split(":")[2],up.split(":")[3]);
            }
        });
    }

    initDrawingReceive(index,item){ // Set up the drawing lines between a reciever container and a sender container
        this.connections["receivers"][index] = [];

        var initX,initY,endX,endY;

        item.addEventListener("mousedown", event => {
            initX = this.moduleCont.x ;
            initY = this.moduleCont.y + this.drag.getBounds().height + index*this.ioSize + 15;
        });

        item.addEventListener("pressmove", event => {
            var mouseX = Math.floor(event.stageX);
            var mouseY = Math.floor(event.stageY);
            drawLine(this.connectionLine,initX,initY,mouseX,mouseY);
        });

        item.addEventListener("pressup", event => {
            // It is a new connection, displays a form to fill the receiver informations
            var stateConnection;
            if(ABOVEINFO != null){
              stateConnection = checkConnection(String(ABOVEINFO[0]),String(ABOVEINFO[1]),String(this.ID),String(index));
            }
            if(!ABOVESENDER || stateConnection == "exists"){
                this.connectionLine.graphics.clear();
                stage.update();
            }
            else{
                // Draws the conneciton and saves it
                this.connectionLine.graphics.clear();
                                    TEMPORARYINFO = [String(ABOVEINFO[0]),String(ABOVEINFO[1]),String(this.ID),String(index)];
                if(stateConnection == "new"){
                    // If no connection already exists on this receiver, displays a form to fill this informations
                    callConnectForm(String(this.ID),String(index),String(this.host));
                }
                else if(stateConnection == "receiver_full"){
                    copyConnectors("receiver");
                }
                else if(stateConnection == "sender_full"){
                    copyConnectors("sender");
                }
                else{
                    console.log("Error: ???")
                }
                connectLines(String(ABOVEINFO[0]),String(ABOVEINFO[1]),String(this.ID),String(index));
            }
        });
        // Updates lines when the module is dragged
        this.drag.addEventListener("pressmove",  event =>{
            var i;
            for(i=0;i<this.connections["receivers"][index].length;i++){
                let up = this.connections["receivers"][index][i];
                updateLines(overallConnections[up],up.split(":")[0],up.split(":")[1],up.split(":")[2],up.split(":")[3]);
            }
        });
    }

    checkAddInOut(currentM,connectNum,type){
        // Checks if the previous sender/receiver is used. If not the connection cannot be created
        var i,posM;
        for(i=0;i<moduleArray.length;i++){
            if(moduleArray[i].ID == currentM){
                posM = i;
                break;
            }
        }
        if(type=="send"){
            for(i=0;i<connectNum;i++){
                if(moduleArray[posM].connections["senders"][i].length == 0){
                    return false;
                }
            }
            return true;
        }else{
            for(i=0;i<connectNum;i++){
                if(moduleArray[posM].connections["receivers"][i].length == 0){
                    return false;
                }
            }
            return true;
        }
    }

    getPosition(){
        return [this.moduleCont.x,this.moduleCont.y]
    }
}

"use strict";

var canvas = document.getElementById('canvas');
var stage = new createjs.Stage("canvas");
var moduleID = 0;
var moduleArray = [];
var overallConnections = new Object();

let jsoneditor = null;
let jsoneditorModule = null;
let jsoneditorMultiModule = null;

init();

// reload the page when it is accessed from the browser history(backward arrow)
window.addEventListener( "pageshow", function ( event ) {
  var historyTraversal = event.persisted ||
                         ( typeof window.performance != "undefined" &&
                              window.performance.navigation.type === 2 );
  if ( historyTraversal ) {
    // Handle page restore.
    window.location.reload(true);
  }
});


function init(){ // Initialize the page: create all the forms needed and update the canvas with the current JSON information
    resize(null,null);
    createjs.Touch.enable(stage);
    stage.enableMouseOver(20);
    stage.mouseMoveOutside = true;
    initConnectionForm();
    initNewModuleForm();
    readJSON();
};

function resize(width,height){
    // Resizes the canvas
    if(width==null || height==null){
        canvas.setAttribute('width', window.innerWidth);
        canvas.setAttribute('height', window.innerHeight);
    }
    else{
        canvas.setAttribute('width', width);
        canvas.setAttribute('height', height);
    }
};

function connectLines(mSend,ioSend,mReceive,ioReceive){
    // Creates the logical connection between a sender and a receiver
    let connectName = String(mSend) + ":" + String(ioSend) + ":" + String(mReceive) + ":" + String(ioReceive);
    // Connections are stored in the overallConnections array with the following convention:
    // "module ID sender:sender ID:module ID receiver:receiver ID"
    var i;
    var keepGoing = true;
    for(i=0;i<moduleArray.length;i++){
        if(moduleArray[i].ID == mReceive){
            moduleArray[i].connections["receivers"][ioReceive].push(connectName);
            if(keepGoing){
                keepGoing = false;
            }else{
                break;
            }
        }
        if(moduleArray[i].ID == mSend){
            moduleArray[i].connections["senders"][ioSend].push(connectName);
            if(keepGoing){
                keepGoing = false;
            }else{
                break;
            }
        }
    }

    overallConnections[connectName] = new createjs.Shape();
    overallConnections[connectName].name = connectName;
    stage.addChild(overallConnections[connectName]);
    updateLines(overallConnections[connectName],mSend,ioSend,mReceive,ioReceive);
};

function deleteConnection(mSend,ioSend,mReceive,ioReceive){
    // Deletes an existing connection
    let connectName = String(mSend) + ":" + String(ioSend) + ":" + String(mReceive) + ":" + String(ioReceive);
    var i;
    var keepGoing = true;
    for(i=0;i<moduleArray.length;i++){
        if(moduleArray[i].ID == mReceive){
            const index = moduleArray[i].connections["receivers"][ioReceive].indexOf(connectName)
            if(index > -1){
                moduleArray[i].connections["receivers"][ioReceive].splice(index,1)
            }
            if(keepGoing){
                keepGoing = false;
            }else{
                break;
            }
        }
        if(moduleArray[i].ID == mSend){
            const index = moduleArray[i].connections["senders"][ioSend].indexOf(connectName)
            if(index > -1){
                moduleArray[i].connections["senders"][ioSend].splice(index,1)
            }
            if(keepGoing){
                keepGoing = false;
            }else{
                break;
            }
        }
    }
    stage.removeChild(overallConnections[connectName]);
    delete overallConnections[connectName];
    stage.update();
}

function updateLines(connectionLine,mSend,ioSend,mReceive,ioReceive){
    // Updates the connection line visual on the canvas between two modules
    var i;
    var sendPosMod,receivePosMod;
    var keepGoing = true;

    for(i=0;i<moduleArray.length;i++){
        if(moduleArray[i].ID == mSend){
            sendPosMod = i;
            if(keepGoing){
                keepGoing = false;
            }else{
                break;
            }
        }
        if(moduleArray[i].ID == mReceive){
            receivePosMod = i;
            if(keepGoing){
                keepGoing = false;
            }else{
                break;
            }
        }
    }

    let fromX = moduleArray[sendPosMod].moduleCont.x + moduleArray[sendPosMod].drag.getBounds().width;
    let fromY = moduleArray[sendPosMod].moduleCont.y + moduleArray[sendPosMod].drag.getBounds().height + ioSend*(moduleArray[sendPosMod].ioSize) + moduleArray[sendPosMod].ioSize/2;

    let toX = moduleArray[receivePosMod].moduleCont.x;
    let toY = moduleArray[receivePosMod].moduleCont.y + moduleArray[receivePosMod].drag.getBounds().height + ioReceive*(moduleArray[receivePosMod].ioSize) + moduleArray[receivePosMod].ioSize/2;

    drawLine(connectionLine,fromX,fromY,toX,toY);
};

function drawLine(connectionLine,fromX,fromY,toX,toY){
    connectionLine.graphics.clear();
    connectionLine.graphics.setStrokeStyle(2).beginStroke("#000000");
    connectionLine.graphics.moveTo(fromX,fromY);
    connectionLine.graphics.curveTo(((fromX+toX)/2),((fromY+toY)/2)+100,toX,toY);
    connectionLine.graphics.endStroke();
    stage.update();
};

function dragAndDrop(name,host,type,port,position){
    // Create a new module
    if(position == null){
        moduleArray.push(new Module(name,host,type,port,[0,0]))
    }
    else{
        moduleArray.push(new Module(name,host,type,port,position))
    }
    stage.update();
}

function readJSON(){
    // Reads an existing JSON file to display its content on the GUI
    var json = new Object();
    $.ajax({
        url: WORKINGCOPY,
        type:'GET',
        dataType: 'json',
        data: json,
        cache: false,
        success: function(json){
            var jsonPosition = new Object();
            var i,j;
            // Reads the json position file for the modules
            $.ajax({
                url: "static/json/position"+String(currentFileName),
                type:'GET',
                dataType: 'json',
                data: jsonPosition,
                cache: false,
                success: function(jsonPosition){
                    // If the positon file assciated to the JSON exists
                    for(i=0;i<json["configuration"]["components"].length;i++){
                        var found = false;
                        for(j=0;j<jsonPosition["module"].length;j++){
                            if(json["configuration"]["components"][i]["name"] == jsonPosition["module"][j]["name"]){
                                dragAndDrop(json["configuration"]["components"][i]["name"],json["configuration"]["components"][i]["host"],json["configuration"]["components"][i]["type"],json["configuration"]["components"][i]["port"],jsonPosition["module"][j]["position"]);
                                found = true;
                                break;
                            }
                        }
                        if(!found){
                            dragAndDrop(json["configuration"]["components"][i]["name"],json["configuration"]["components"][i]["host"],json["configuration"]["components"][i]["type"],json["configuration"]["components"][i]["port"],[50+50*i,75+75*i]);
                        }
                    }
                    readJSON_helper(json);
                },
                error: function(){
                    // If the position file does not exist
                    for(i=0;i<json["configuration"]["components"].length;i++){
                        dragAndDrop(json["configuration"]["components"][i]["name"],json["configuration"]["components"][i]["host"],json["configuration"]["components"][i]["type"],json["configuration"]["components"][i]["port"],[50+50*i,75+75*i]);
                    }
                    readJSON_helper(json);
                }});
    }});
}

function readJSON_helper(json){
    var i,j,k,l;
    var toConnectObj = {};

    for(i=0;i<json["configuration"]["components"].length;i++){
        var moduleID;
        for(j=0;j<moduleArray.length;j++){
            if(json["configuration"]["components"][i]["name"] == moduleArray[j].name){
                moduleID = moduleArray[j].ID;
            }
        }
        if(json["configuration"]["components"][i]["connections"] != undefined){
            if(json["configuration"]["components"][i]["connections"]["senders"] != undefined){
                for(j=0;j<json["configuration"]["components"][i]["connections"]["senders"].length;j++){
                    // Makes the the Sender containers
                    moduleArray[moduleID].makeSenderContainerREADINGONLY();
                    // If port is zero, it means it has not been filled and we do not want any connections to be made
                    if(json["configuration"]["components"][i]["connections"]["senders"][j]["port"] != 0){
                        let connectInfo;
                        // check the connection type
                        if(json["configuration"]["components"][i]["connections"]["senders"][j]["transport"] == "tcp" || json["configuration"]["components"][i]["connections"]["senders"][j]["transport"] == "tcp (w/ filter)"){
                            connectInfo = String(json["configuration"]["components"][i]["connections"]["senders"][j]["type"]) + String(json["configuration"]["components"][i]["connections"]["senders"][j]["transport"]) + String(json["configuration"]["components"][i]["connections"]["senders"][j]["port"])
                        }
                        else if(json["configuration"]["components"][i]["connections"]["senders"][j]["transport"] == "ipc"){
                            connectInfo = String(json["configuration"]["components"][i]["connections"]["senders"][j]["type"]) + String(json["configuration"]["components"][i]["connections"]["senders"][j]["transport"]) + String(json["configuration"]["components"][i]["connections"]["senders"][j]["path"])
                        }
                        else if(json["configuration"]["components"][i]["connections"]["senders"][j]["transport"] == undefined){
                            console.log("Error: Transport protocol needed");
                            continue;
                        }
                        else{
                            console.log("Error: Unknown transport protocol");
                            continue;
                        }

                        // Fill an temporary array to keep the connectors information in order to connect them later
                        if(toConnectObj[connectInfo] == undefined){
                            toConnectObj[connectInfo] = {}
                        }
                        if(toConnectObj[connectInfo]["senders"] == undefined){
                            toConnectObj[connectInfo]["senders"] = [[moduleID,json["configuration"]["components"][i]["connections"]["senders"][j]["chid"]]];
                        }
                        else{
                            toConnectObj[connectInfo]["senders"].push([moduleID,json["configuration"]["components"][i]["connections"]["senders"][j]["chid"]]);
                        }
                    }
                }
            }
            if(json["configuration"]["components"][i]["connections"]["receivers"] != undefined){
                for(j=0;j<json["configuration"]["components"][i]["connections"]["receivers"].length;j++){
                    // Makes the receivers containers
                    moduleArray[moduleID].makeReceiverContainerREADINGONLY();
                    // If port is zero, it means it has not been filled and we do not want any connections to be made
                    if(json["configuration"]["components"][i]["connections"]["receivers"][j]["port"] != 0){
                        let connectInfo
                        // check the connection type
                        if(json["configuration"]["components"][i]["connections"]["receivers"][j]["transport"] == "tcp" || json["configuration"]["components"][i]["connections"]["receivers"][j]["transport"] == "tcp (w/ filter)"){
                            if(String(json["configuration"]["components"][i]["connections"]["receivers"][j]["port"]).length!=0){
                                connectInfo = String(json["configuration"]["components"][i]["connections"]["receivers"][j]["type"]) + String(json["configuration"]["components"][i]["connections"]["receivers"][j]["transport"]) + String(json["configuration"]["components"][i]["connections"]["receivers"][j]["port"])
                            }
                        }
                        else if(json["configuration"]["components"][i]["connections"]["receivers"][j]["transport"] == "ipc"){
                            connectInfo = String(json["configuration"]["components"][i]["connections"]["receivers"][j]["type"]) + String(json["configuration"]["components"][i]["connections"]["receivers"][j]["transport"]) + String(json["configuration"]["components"][i]["connections"]["receivers"][j]["path"])
                        }
                        else if(json["configuration"]["components"][i]["connections"]["receivers"][j]["transport"] == undefined){
                            console.log("Error: Transport protocol needed");
                            continue;
                        }
                        else{
                            console.log("Error: Unknown transport protocol");
                            continue;
                        }

                        // Fill an temporary array to keep the connectors information in order to connect them later
                        if(toConnectObj[connectInfo] == undefined){
                            toConnectObj[connectInfo] = {}
                        }
                        if(toConnectObj[connectInfo]["receivers"] == undefined){
                            toConnectObj[connectInfo]["receivers"] = [[moduleID,json["configuration"]["components"][i]["connections"]["receivers"][j]["chid"]]];
                        }
                        else{
                            toConnectObj[connectInfo]["receivers"].push([moduleID,json["configuration"]["components"][i]["connections"]["receivers"][j]["chid"]]);
                        }
                    }
                }
            }
        }
    }
    // Checks which connectors need to be connected
    for(const connection in toConnectObj){
        if(toConnectObj[connection]["senders"]!= undefined && toConnectObj[connection]["receivers"]!=undefined){
            for(i=0;i<toConnectObj[connection]["senders"].length;i++){
                for(j=0;j<toConnectObj[connection]["receivers"].length;j++){
                    // Connect the modules/connectors
                    connectLines(toConnectObj[connection]["senders"][i][0],toConnectObj[connection]["senders"][i][1],toConnectObj[connection]["receivers"][j][0],toConnectObj[connection]["receivers"][j][1]);
                }
            }
        }
    }
    /* Updates the port generator to use only higher ports than the one used
     in the JSON file for the connectors */
    var higherPort = -1;
    for(i=0;i<json["configuration"]["components"].length;i++){
        if(json["configuration"]["components"][i]["connections"] != undefined){
            if(json["configuration"]["components"][i]["connections"]["senders"] != undefined){
                for(j=0;j<json["configuration"]["components"][i]["connections"]["senders"].length;j++){
                    if(json["configuration"]["components"][i]["connections"]["senders"][j]["port"] != undefined && Number.isInteger(json["configuration"]["components"][i]["connections"]["senders"][j]["port"]) && json["configuration"]["components"][i]["connections"]["senders"][j]["port"]>higherPort){
                        higherPort = json["configuration"]["components"][i]["connections"]["senders"][j]["port"]
                    }
                }
            }
            if(json["configuration"]["components"][i]["connections"]["receivers"]!= undefined){
                for(j=0;j<json["configuration"]["components"][i]["connections"]["receivers"].length;j++){
                    if(json["configuration"]["components"][i]["connections"]["receivers"][j]["port"] != undefined && Number.isInteger(json["configuration"]["components"][i]["connections"]["receivers"][j]["port"]) && json["configuration"]["components"][i]["connections"]["receivers"][j]["port"]>higherPort){
                        higherPort = json["configuration"]["components"][i]["connections"]["receivers"][j]["port"]
                    }
                }
            }
        }
    }
    portRange("connection",higherPort);
    // For the module port
    higherPort = -1;
    for(i=0;i<json["configuration"]["components"].length;i++){
        if(json["configuration"]["components"][i]["port"] != undefined && Number.isInteger(json["configuration"]["components"][i]["port"]) && json["configuration"]["components"][i]["port"]>higherPort){
            higherPort = json["configuration"]["components"][i]["port"];
        }
    }
    portRange("module",higherPort);
}

function writeJSON(){ // Updates the internal dummy file to match what is displayed on the GUI
    var json = new Object();
    $.ajax({
        url: WORKINGCOPY,
        type:'GET',
        dataType: 'json',
        data: json,
        cache: false,
        success: function(json){
        var collectExisting = [];
        var i,j,k;
        if(json["configuration"]["components"] != undefined){
            // If the module already exists on the JSON file, it will not overwrite it. Informations are copied
            for(i=0;i<json["configuration"]["components"].length;i++){
                collectExisting.push(json["configuration"]["components"][i]["name"]);
                if(json["configuration"]["components"][i]["connections"] == undefined){
                    json["configuration"]["components"][i]["connections"] = {};
                }
                for(j=0;j<moduleArray.length;j++){
                    if(moduleArray[j].name == json["configuration"]["components"][i]["name"]){
                        if(moduleArray[j].chidSendArray.length != 0 && json["configuration"]["components"][i]["connections"]["senders"] == undefined){
                            json["configuration"]["components"][i]["connections"]["senders"] = [];
                        }
                        if(moduleArray[j].chidReceiveArray.length != 0 && json["configuration"]["components"][i]["connections"]["receivers"] == undefined){
                            json["configuration"]["components"][i]["connections"]["receivers"] = [];
                        }
                        // Checks if new connections have been added to the module
                        if(json["configuration"]["components"][i]["connections"]["senders"] != undefined){
                            while(json["configuration"]["components"][i]["connections"]["senders"].length < moduleArray[j].chidSendArray.length){
                                let chid = json["configuration"]["components"][i]["connections"]["senders"].length;
                                json["configuration"]["components"][i]["connections"]["senders"].push({"chid":chid})
                            }
                        }
                        if(json["configuration"]["components"][i]["connections"]["receivers"] != undefined){
                            while(json["configuration"]["components"][i]["connections"]["receivers"].length < moduleArray[j].chidReceiveArray.length){
                                let chid = json["configuration"]["components"][i]["connections"]["receivers"].length;
                                json["configuration"]["components"][i]["connections"]["receivers"].push({"chid":chid})
                            }
                        }
                    }
                }
            }
        }
        else{
            json["configuration"]["components"] = [];
        }
        // For all the new modules, add them to the file and retreive their connection information
        for(i=0;i<moduleArray.length;i++){
            if(!collectExisting.includes(moduleArray[i].name)){
                json["configuration"]["components"].push({"name":moduleArray[i].name,"host":moduleArray[i].host,"type":moduleArray[i].type,"port":moduleArray[i].port});
                json["configuration"]["components"][json["configuration"]["components"].length-1]["connections"]= new Object();
                if(moduleArray[i].chidSendArray.length!= 0){
                    json["configuration"]["components"][json["configuration"]["components"].length-1]["connections"]["senders"] = [];
                    for(j=0;j<moduleArray[i].chidSendArray.length;j++){
                        json["configuration"]["components"][json["configuration"]["components"].length-1]["connections"]["senders"].push({"chid":j})
                    }
                }
                if(moduleArray[i].chidReceiveArray.length!= 0){
                    json["configuration"]["components"][json["configuration"]["components"].length-1]["connections"]["receivers"] = [];
                    for(j=0;j<moduleArray[i].chidReceiveArray.length;j++){
                        json["configuration"]["components"][json["configuration"]["components"].length-1]["connections"]["receivers"].push({"chid":j})
                    }
                }
            }
        }
        // POST request to write the file on the disk
        var xhr = new XMLHttpRequest();
        const data = JSON.stringify(json,null,2);
        const url = "/editModule?type=tempFile";
        xhr.open("POST",url,true);
        xhr.send(data);
    }});
}

function initConnectionForm(){
    // Prepares the form to add a new connection
    var json = new Object();
    $.ajax({
        url: "static/schema/config-schema.json",
        type:'GET',
        dataType: 'json',
        data: json,
        cache: false,
        success: function(json){
            let path = json["properties"]["configuration"]["properties"]["components"]["items"]["properties"]["connections"]["properties"]["receivers"]["items"]
            let data = {schema: path,disable_collapse:true,disable_properties:true,disable_edit_json:true,disable_array_reorder:true,keep_oneof_values:true};
            JSONEditor.defaults.options.theme = 'spectre';
            JSONEditor.defaults.options.iconlib = 'spectre';
            const jsonEditorForm = document.getElementById("json-editor-form");
            if (jsoneditor) {
              jsoneditor.destroy()
            }
            jsoneditor = new JSONEditor(jsonEditorForm, data);
        }});
}

function initNewModuleForm(){
    // Prepares the form to add a new module
    var json = new Object();
    $.ajax({
        url: "static/schema/config-schema.json",
        type:'GET',
        dataType: 'json',
        data: json,
        cache: false,
        success: function(json){
            let path = json["properties"]["configuration"]["properties"]["components"]["items"]
            var schema = {};
            var schemaMulti = {};
            // Init the schema to add one new module
            schema["type"] = "object"
            schema["title"] = "Component"
            schema["properties"] = {}
            schema["properties"]["name"] = path["properties"]["name"]
            schema["properties"]["host"] = path["properties"]["host"]
            schema["properties"]["type"] = path["properties"]["type"]
            schema["definitions"] = json["definitions"]
            // Init the schema to add multiple modules
            schemaMulti["type"] = "object"
            schemaMulti["title"] = "Component"
            schemaMulti["properties"] = {}
            schemaMulti["properties"]["name"] = path["properties"]["name"]
            schemaMulti["properties"]["number of modules"] = {"type":"integer"}
            schemaMulti["properties"]["host"] = path["properties"]["host"]
            schemaMulti["properties"]["type"] = path["properties"]["type"]
            schemaMulti["definitions"] = json["definitions"]

            let data = {schema: schema,disable_collapse:true,disable_properties:true,disable_edit_json:true,disable_array_reorder:true};
            let dataMulti = {schema: schemaMulti,disable_collapse:true,disable_properties:true,disable_edit_json:true,disable_array_reorder:true};
            JSONEditor.defaults.options.theme = 'spectre';
            JSONEditor.defaults.options.iconlib = 'spectre';

            const jsonEditorForm2 = document.getElementById("json-editor-form2");
            const jsonEditorForm3 = document.getElementById("json-editor-form3");
            if (jsoneditorModule) {
              jsoneditorModule.destroy()
            }
            if (jsoneditorMultiModule) {
              jsoneditorMultiModule.destroy()
            }
            jsoneditorModule = new JSONEditor(jsonEditorForm2, data);
            jsoneditorMultiModule = new JSONEditor(jsonEditorForm3, dataMulti);
        }});
}

function submitNewModule(){
    // Used in the HTML template to add a new module
    var json = jsoneditorModule.getValue();
    dragAndDrop(json["name"],json["host"],json["type"],null,[50, 75]);
    writeJSON();
}

function submitNewMultiModules(){
  // Used in the HTML template to add a multiple new modules
  var json = jsoneditorMultiModule.getValue();
  var nb = json["number of modules"]
  var name = json["name"];
  var i;
  var count = 0;

  for(i=0;i<moduleArray.length;i++){
    /* Check if modules with the same name already exist to put
    the correct number at the end of the name(e.g. readoutinterface5) */
    if(moduleArray[i]["name"].toLowerCase().includes(name)){
      var value = parseInt(moduleArray[i]["name"].replace(name,''));
      if(!isNaN(value)){
        if(count<value){
          count = value;
        }
      }
    }
  }
  for(i=0;i<nb;i++){
    count = count + 1;
    dragAndDrop(name+String(("0" + count).slice(-2)),json["host"],json["type"],null,[60*count, 75*count]);
  }
  writeJSON();
}

function submitNewConnection(){
    // Write the new connection information
    var json = new Object();
    var j,i,mSend,mRec;
    var keepGoing = true;
    for(i=0;i<moduleArray.length;i++){
        if(moduleArray[i].ID == TEMPORARYINFO[0]){
            mSend = i;
            if(keepGoing){
                keepGoing = false;
            }else{
                break;
            }
        }
        if(moduleArray[i].ID == TEMPORARYINFO[2]){
            mRec = i;
            if(keepGoing){
                keepGoing = false;
            }else{
                break;
            }
        }
    }
    $.ajax({
        url: WORKINGCOPY,
        type:'GET',
        dataType: 'json',
        data: json,
        cache: false,
        success: function(json){
            if(jsoneditor.validate().length){ // Error
                console.log("Validation error");
                deleteConnection(TEMPORARYINFO[0],TEMPORARYINFO[1],TEMPORARYINFO[2],TEMPORARYINFO[3])
            }
            else{
                var json2 = jsoneditor.getValue();
                for(i=0;i<json["configuration"]["components"].length;i++){
                    if(json["configuration"]["components"][i]["name"] == moduleArray[mRec].name){
                        for(j=0;j<json["configuration"]["components"][i]["connections"]["receivers"].length;j++){
                            if(json["configuration"]["components"][i]["connections"]["receivers"][j]["chid"] == TEMPORARYINFO[3]){
                                // Clean the json in case of previous unwanted information
                                for (var member in json["configuration"]["components"][i]["connections"]["receivers"][j]) delete json["configuration"]["components"][i]["connections"]["receivers"][j][member];
                                json["configuration"]["components"][i]["connections"]["receivers"][j]["chid"] = parseInt(TEMPORARYINFO[3],10)

                                // Copy the connection information into the one it is connected to
                                $.extend(json["configuration"]["components"][i]["connections"]["receivers"][j],json["configuration"]["components"][i]["connections"]["receivers"][j],json2)
                            }
                        }
                    }
                    if(json["configuration"]["components"][i]["name"] == moduleArray[mSend].name){
                        for(j=0;j<json["configuration"]["components"][i]["connections"]["senders"].length;j++){
                            if(json["configuration"]["components"][i]["connections"]["senders"][j]["chid"] == TEMPORARYINFO[1]){
                                // Clean the json in case of previous unwanted information
                                for (var member in json["configuration"]["components"][i]["connections"]["senders"][j]) delete json["configuration"]["components"][i]["connections"]["senders"][j][member];
                                json["configuration"]["components"][i]["connections"]["senders"][j]["chid"] = parseInt(TEMPORARYINFO[1],10)

                                // Check the connection type
                                if(json2["transport"] == "tcp" || json2["transport"] == "tcp (w/ filter)"){
                                    json["configuration"]["components"][i]["connections"]["senders"][j]["transport"] = json2["transport"];
                                    json["configuration"]["components"][i]["connections"]["senders"][j]["type"] = json2["type"];
                                    json["configuration"]["components"][i]["connections"]["senders"][j]["host"] = "*";
                                    json["configuration"]["components"][i]["connections"]["senders"][j]["port"] = json2["port"];
                                }
                                else if(json2["transport"] == "ipc"){
                                    json["configuration"]["components"][i]["connections"]["senders"][j]["transport"] = json2["transport"];
                                    json["configuration"]["components"][i]["connections"]["senders"][j]["type"] = json2["type"];
                                    json["configuration"]["components"][i]["connections"]["senders"][j]["path"] = json2["path"];
                                }
                                else {
                                    console.log("Error: unknown transport type")
                                }
                            }
                        }
                    }
                }
                var xhr = new XMLHttpRequest();
                const data = JSON.stringify(json,null,2);
                const url = "/editModule?type=tempFile";
                xhr.open("POST",url,true);
                xhr.send(data);
            }
        }});
}

function callConnectForm(moduleID,chid,host){
    // Displays the new connection form
    jsoneditor.setValue({"chid":chid,"port":portRange('connection',-1),"host":host});
    jsoneditor.on('change',() => {
        // check the connection type to disabled unused fields
        if(jsoneditor.getValue()["transport"] == "tcp" || jsoneditor.getValue()["transport"] == "tcp (w/ filter)"){
            jsoneditor.getEditor('root.host').disable();
            jsoneditor.getEditor('root.port').disable();
        }
        jsoneditor.getEditor('root.chid').disable();
    })
    document.getElementById("newConnectionLink").click()
}

function checkConnection(mSend,ioSend,mRec,ioRec){
    var i;
    let connectName = Object.getOwnPropertyNames(overallConnections);
    let tempName = String(mSend) + ":" + String(ioSend) + ":" + String(mRec) + ":" + String(ioRec)
    for(i=0;i<connectName.length;i++){
        if(tempName == connectName[i]){
            // The connection already exists
            return "exists";
        }
        else if(connectName[i].split(":")[2] == mRec && connectName[i].split(":")[3] == ioRec){
            // The receiver has already an incomming connection
            return "receiver_full";
        }
        else if(connectName[i].split(":")[0] == mSend && connectName[i].split(":")[1] == ioSend){
            // The sender has already an outgoing connection
            return "sender_full";
        }
    }
    return "new"; // It is a new connection
}

PORTMODULE = 5000;
PORTCONNECTION = 8000;
function portRange(type,port){
    // Auto-generates port number
    if(type=="module" && port == -1){
        PORTMODULE = PORTMODULE + 1;
        return PORTMODULE-1;
    }
    else if(type=="module" && port != -1){
        PORTMODULE = port + 1;
        return PORTMODULE-1;
    }
    else if(type=="connection" && port == -1){
        PORTCONNECTION = PORTCONNECTION + 1;
        return PORTCONNECTION-1;
    }
    else if(type=="connection" && port != -1){
        PORTCONNECTION = port + 1;
        return PORTCONNECTION-1;
    }
    else{
        throw new Error("Invalid type value(module or connection)")
    }
}

function cancelConnection(){
    deleteConnection(TEMPORARYINFO[0],TEMPORARYINFO[1],TEMPORARYINFO[2],TEMPORARYINFO[3]);
}

function saveModulesPosition(currentFileName){ // Saves the module positions to replace correctly when the page is reloaded
    var i;
    save = new Object();
    save["module"] = []
    for(i=0;i<moduleArray.length;i++){
        newObj = new Object();
        newObj["name"] = moduleArray[i].name;
        newObj["position"] = moduleArray[i].getPosition();
        save["module"].push(newObj);
    }
    var xhr = new XMLHttpRequest();
    const data = JSON.stringify(save,null,2);
    const url = "/canvas?save=true&currentFileName="+currentFileName;
    xhr.open("POST",url,true);
    xhr.send(data);
}

function readSavedPosition(currentFileName){ // Read the JSON file containing the saved positions
    var json = new Object();
    $.ajax({
        url: "static/json/position"+String(currentFileName),
        type:'GET',
        dataType: 'json',
        data: json,
        cache: false,
        success: function(json){
            return json;
        }});
}

function copyConnectors(from){
    // Copy the connector information into another one
    var json = new Object();
    var j,i,mSend,mRec;
    var keepGoing = true;
    for(i=0;i<moduleArray.length;i++){
        if(moduleArray[i].ID == TEMPORARYINFO[0]){
            mSend = i;
            if(keepGoing){
                keepGoing = false;
            }else{
                break;
            }
        }
        if(moduleArray[i].ID == TEMPORARYINFO[2]){
            mRec = i;
            if(keepGoing){
                keepGoing = false;
            }else{
                break;
            }
        }
    }
    $.ajax({
        url: WORKINGCOPY,
        type:'GET',
        dataType: 'json',
        data: json,
        cache: false,
        success: function(json){
            var posRec,posSend,posIoSend,posIoRec;
            var keepGoing = true;
            //------------- Find the modules and connectors in the array used for the internal representation----------------
            for(i=0;i<json["configuration"]["components"].length;i++){
                if(json["configuration"]["components"][i]["name"] == moduleArray[mRec].name){
                    posRec = i;
                    if(keepGoing){
                        keepGoing = false;
                    }
                    else{
                        break;
                    }
                }
                if(json["configuration"]["components"][i]["name"] == moduleArray[mSend].name){
                    posSend = i;
                    if(keepGoing){
                        keepGoing = false;
                    }
                    else{
                        break;
                    }
                }
            }
            for(i=0;i<json["configuration"]["components"][posSend]["connections"]["senders"].length;i++){
                if(json["configuration"]["components"][posSend]["connections"]["senders"][i]["chid"] == TEMPORARYINFO[1]){
                    posIoSend = i;
                }
            }
            for(i=0;i<json["configuration"]["components"][posRec]["connections"]["receivers"].length;i++){
                if(json["configuration"]["components"][posRec]["connections"]["receivers"][i]["chid"] == TEMPORARYINFO[3]){
                    posIoRec = i;
                }
            }
            // -------------------------------------------------------------------------------

            if(from=="receiver"){
                json = copyFromReceiver(posSend,posIoSend,posRec,posIoRec,json);
            }
            else if(from=="sender"){
                json = copyFromSender(posSend,posIoSend,posRec,posIoRec,json);
            }
            else{
                console.log("Error: to should be equal to sender or receiver");
            }
            var xhr = new XMLHttpRequest();
            const data = JSON.stringify(json,null,2);
            const url = "/editModule?type=tempFile";
            xhr.open("POST",url,true);
            xhr.send(data);
        }});
}

function copyFromReceiver(posSend,posIoSend,posRec,posIoRec,json){
    // Copy information from a receiver into a sender
    if(json["configuration"]["components"][posRec]["connections"]["receivers"][posIoRec]["transport"] == "tcp" || json["configuration"]["components"][posRec]["connections"]["receivers"][posIoRec]["transport"] == "tcp (w/ filter)"){
        json["configuration"]["components"][posSend]["connections"]["senders"][posIoSend]["transport"] = json["configuration"]["components"][posRec]["connections"]["receivers"][posIoRec]["transport"];
        json["configuration"]["components"][posSend]["connections"]["senders"][posIoSend]["type"] = json["configuration"]["components"][posRec]["connections"]["receivers"][posIoRec]["type"];
        json["configuration"]["components"][posSend]["connections"]["senders"][posIoSend]["host"] = "*";
        json["configuration"]["components"][posSend]["connections"]["senders"][posIoSend]["port"] = json["configuration"]["components"][posRec]["connections"]["receivers"][posIoRec]["port"];
    }
    else if(json["configuration"]["components"][posRec]["connections"]["receivers"][posIoRec]["transport"] == "ipc"){
        json["configuration"]["components"][posSend]["connections"]["senders"][posIoSend]["transport"] = json["configuration"]["components"][posRec]["connections"]["receivers"][posIoRec]["transport"];
        json["configuration"]["components"][posSend]["connections"]["senders"][posIoSend]["type"] = json["configuration"]["components"][posRec]["connections"]["receivers"][posIoRec]["type"];
        json["configuration"]["components"][posSend]["connections"]["senders"][posIoSend]["path"] = json["configuration"]["components"][posRec]["connections"]["receivers"][posIoRec]["path"];
    }
    else{
        console.log("Error: unknown transport protocol");
    }
    return json;
}

function copyFromSender(posSend,posIoSend,posRec,posIoRec,json){
    // Copy information from a sender into a receiver
    if(json["configuration"]["components"][posSend]["connections"]["senders"][posIoSend]["transport"] == "tcp" || json["configuration"]["components"][posSend]["connections"]["senders"][posIoSend]["transport"] == "tcp (w/ filter)"){
        json["configuration"]["components"][posRec]["connections"]["receivers"][posIoRec]["transport"] = json["configuration"]["components"][posSend]["connections"]["senders"][posIoSend]["transport"];
        json["configuration"]["components"][posRec]["connections"]["receivers"][posIoRec]["type"] = json["configuration"]["components"][posSend]["connections"]["senders"][posIoSend]["type"];
        json["configuration"]["components"][posRec]["connections"]["receivers"][posIoRec]["host"] = json["configuration"]["components"][posRec]["host"];
        json["configuration"]["components"][posRec]["connections"]["receivers"][posIoRec]["port"] = json["configuration"]["components"][posSend]["connections"]["senders"][posIoSend]["port"];
    }
    else if(json["configuration"]["components"][posSend]["connections"]["senders"][posIoSend]["transport"] == "ipc"){
        json["configuration"]["components"][posRec]["connections"]["receivers"][posIoRec]["transport"] = json["configuration"]["components"][posSend]["connections"]["senders"][posIoSend]["transport"];
        json["configuration"]["components"][posRec]["connections"]["receivers"][posIoRec]["type"] = json["configuration"]["components"][posSend]["connections"]["senders"][posIoSend]["type"];
        json["configuration"]["components"][posRec]["connections"]["receivers"][posIoRec]["path"] = json["configuration"]["components"][posSend]["connections"]["senders"][posIoSend]["path"];
    }
    else{
        console.log("Error: unknown transport protocol");
    }
    return json;
}

var HELPACTIVATED = false;

// -------------------------- Add event listeners ----------------------------
document.getElementById("helpButton").addEventListener("click", event =>{
    // Activate and deactivate the help pop-ups
    if(HELPACTIVATED){
        HELPACTIVATED = false;
        document.getElementById("helpButton").className = "btn btn-primary";
        document.getElementById("newModuleButton").className = "btn btn-primary";
        document.getElementById("popover1").className = "d-none";
        document.getElementById("editJSONButton").className = "btn btn-primary";
        document.getElementById("popover2").className = "d-none";
        document.getElementById("menuButton").className = "btn btn-primary";
        document.getElementById("popover3").className = "d-none";
        document.getElementById("canvasSize").className = "btn btn-primary";
        document.getElementById("popover4").className = "d-none";
        document.getElementById("newMultiModuleButton").className = "btn btn-primary";
        document.getElementById("popover5").className = "d-none";

    }
    else{
        HELPACTIVATED = true;
        document.getElementById("helpButton").className = "btn btn-success";
        document.getElementById("newModuleButton").className = "d-none";
        document.getElementById("popover1").className = "popover popover-bottom";
        document.getElementById("editJSONButton").className = "d-none";
        document.getElementById("popover2").className = "popover popover-bottom";
        document.getElementById("menuButton").className = "d-none";
        document.getElementById("popover3").className = "popover popover-bottom";
        document.getElementById("canvasSize").className = "d-none";
        document.getElementById("popover4").className = "popover popover-bottom";
        document.getElementById("newMultiModuleButton").className = "d-none";
        document.getElementById("popover5").className = "popover popover-bottom";
    }
})

document.getElementById("submitResize").addEventListener("click", event =>{
    const width = document.getElementById("canvasWidth").value;
    const height = document.getElementById("canvasHeight").value;
    resize(width,height)
    stage.update()
})

document.getElementById("canvasSize").addEventListener("click", event =>{
    const canvas = document.getElementById("canvas");
    document.getElementById("canvasWidth").value = canvas.width;
    document.getElementById("canvasHeight").value = canvas.height;
})
