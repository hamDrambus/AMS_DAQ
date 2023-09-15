var HELPACTIVATED = false;

document.getElementById("helpButton").addEventListener("click", event =>{
    // Activate or deactivate the help pop-up when the user clicks on the help button
    if(HELPACTIVATED){
        HELPACTIVATED = false;
        document.getElementById("helpButton").className = "btn btn-primary btn-lg";
        document.getElementById("newConfigFile").className = "btn btn-primary btn-lg";
        document.getElementById("popover1").className = "d-none";
        document.getElementById("editConfigFile").className = "btn btn-primary btn-lg";
        document.getElementById("popover2").className = "d-none";
        document.getElementById("editPrevFile").className = "btn btn-primary btn-lg";
        document.getElementById("popover3").className = "d-none";
        document.getElementById("selectSchemaFile").className = "btn btn-primary btn-lg";
        document.getElementById("popover4").className = "d-none";

    }
    else{
        HELPACTIVATED = true;
        document.getElementById("helpButton").className = "btn btn-success btn-lg";
        document.getElementById("newConfigFile").className = "d-none";
        document.getElementById("popover1").className = "popover popover-right";
        document.getElementById("editConfigFile").className = "d-none";
        document.getElementById("popover2").className = "popover popover-right";
        document.getElementById("editPrevFile").className = "d-none";
        document.getElementById("popover3").className = "popover popover-right";
        document.getElementById("selectSchemaFile").className = "d-none";
        document.getElementById("popover4").className = "popover popover-right";
    }
})

document.getElementById("submitSchema").addEventListener("click", event =>{
    // Sends to the server the schema choosed by the user
    var i,value;
    const radioOut = document.getElementById("form3").elements;
    for(i=0;i<radioOut.length;i++){
        if(radioOut[i].checked){
            value = radioOut[i].value;
            break;
        }
    }

    var xhr = new XMLHttpRequest();
    const data = null;
    const url = "/editModule?type=selectSchema&schema="+String(value);
    xhr.open("POST",url,true);
    xhr.send(data);
})
