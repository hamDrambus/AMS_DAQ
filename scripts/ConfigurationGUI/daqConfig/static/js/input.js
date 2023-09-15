let jsoneditor = null;
// Set the options for the JSON editor
let data = {ajax:true,schema: {$ref: "/static/schema/config-schema.json"},disable_collapse:false,disable_properties:false,disable_edit_json:true,disable_array_reorder:true,keep_oneof_values:true,show_errors:"always"};
const jsonEditorForm = document.getElementById("json-editor-form");

var xhr = new XMLHttpRequest();
var xhr2 = new XMLHttpRequest();

var submitButtonState = null;
var validateButtonState = null;
var validIconState = null;

var HELPACTIVATED = false;

function initJsoneditor(){
  // destroy old JSONEditor instance if exists
  if (jsoneditor) {
    jsoneditor.destroy()
  }

  JSONEditor.defaults.options.theme = 'spectre';
  JSONEditor.defaults.options.iconlib = 'spectre';
  JSONEditor.defaults.options.ajax = true;

  // new instance of JSONEditor
  jsoneditor = new JSONEditor(jsonEditorForm, data);
}

$(document).ready(function() { // Prevent ajax to fetch cached(and not updated) json
  $.ajaxSetup({ cache: false });
});

initJsoneditor();

jsoneditor.on('ready',() => {
    $.getJSON("/static/json/dummyFile.json", function(json){
        // Set the editor to the pre-existing values
        jsoneditor.setValue(json);
    });
    jsoneditor.on('change', () =>{
        document.getElementById("textSubmission").className = "d-hide";
        let validationErrors = jsoneditor.validate();

        /*  The help button modifies the state of the other buttons. Since the
            submit and validation buttons color depends of their state,
            this states are saved to be correctly reset to their value once
            the help button is "un-clicked".*/
        if (validationErrors.length) {
            document.getElementById("validate-textarea").value = JSON.stringify(validationErrors, null, 2);
            if(!HELPACTIVATED){
                document.getElementById("submit").className = "btn btn-primary disabled";
                document.getElementById("validationField").className = "btn btn-error";
                document.getElementById("validIcon").className = "icon icon-cross";
            }
            document.getElementById("submit2").className = "btn btn-primary disabled";
            document.getElementById("validationField2").className = "btn btn-error";
            document.getElementById("validIcon2").className = "icon icon-cross";
            validateButtonState = "btn btn-error";
            submitButtonState = "btn btn-primary disabled";
            validIconState = "icon icon-cross";
        }
        else {
            document.getElementById("validate-textarea").value = 'valid';
            if(!HELPACTIVATED){
                document.getElementById("submit").className = "btn btn-primary";
                document.getElementById("validationField").className = "btn btn-success";
                document.getElementById("validIcon").className = "icon icon-check";
            }
            document.getElementById("submit2").className = "btn btn-primary";
            document.getElementById("validationField2").className = "btn btn-success";
            document.getElementById("validIcon2").className = "icon icon-check";
            submitButtonState = "btn btn-primary";
            validateButtonState = "btn btn-success";
            validIconState = "icon icon-check";
        }
    });
    document.getElementById('submit').addEventListener("click", event => {
        submit_helper();
    });
    document.getElementById('submit2').addEventListener("click", event => {
        submit_helper();
    });
    document.getElementById('submitNewJSON').addEventListener("click", event =>{
        if(jsoneditor.validate().length){ // Second validation after file(if nedded) has been named
            console.log("Validation error")
        }
        else{
            // POST request to write the file on disk
            const data = JSON.stringify(jsoneditor.getValue(),null,2);
            const fileNameNew = document.getElementById('fileNameNew').value;
            // Changes the URL and reloads the page to remember the name of the file given by the user
            window.location.href = "/editModule?currentFileName="+fileNameNew;
            const url = "/editModule?type=outFile&currentFileName="+fileNameNew;
            xhr.open("POST",url,true);
            xhr.send(data);
            document.getElementById("textSubmission").className = "text-success";
        }
    });
    document.getElementById('returnToCanvas').addEventListener("click", event =>{
        returnToCanvas_helper()
    });
    document.getElementById('returnToCanvas2').addEventListener("click", event =>{
        returnToCanvas_helper()
    });
});

function submit_helper(){
    // Saves the modifications made on the JSON file when returning to the canvas
    const data = JSON.stringify(jsoneditor.getValue(),null,2);
    const url = "/editModule?type=tempFile";
    xhr.open("POST",url,true);
    xhr.send(data);
    if(jsoneditor.validate().length){ // Validation error
        console.log("Validation error")
    }
    else{
        if(currentFileName != "dummyFile.json" && currentFileName != "None"){
            // POST request to the server to write the validated JSON file if the file is already named
            const data2 = JSON.stringify(jsoneditor.getValue(),null,2);
            const url2 = "/editModule?type=outFile&currentFileName="+currentFileName;
            xhr2.open("POST",url2,true);
            xhr2.send(data2);
            document.getElementById("textSubmission").className = "text-success";
        }
        else{
            // Ask the user to name the file that is being written
            document.getElementById("hiddenSubmit").click()
        }
    }
}

function returnToCanvas_helper(){
    // Saves the modifications made on the JSON file when returning to the canvas
    const data = JSON.stringify(jsoneditor.getValue(),null,2);
    const url = "/editModule?type=tempFile";
    xhr.open("POST",url,true);
    xhr.send(data);
}

document.getElementById("helpButton").addEventListener("click", event =>{
    // Activate or deactivate the help pop-up when the user clicks on the help button
    if(HELPACTIVATED){
        HELPACTIVATED = false;
        document.getElementById("helpButton").className = "btn btn-primary";
        document.getElementById("submit").className = submitButtonState;
        document.getElementById("popover1").className = "d-none";
        document.getElementById("validationField").className = validateButtonState;
        document.getElementById("popover2").className = "d-none";
        document.getElementById("returnToCanvas").className = "btn btn-primary";
        document.getElementById("popover3").className = "d-none";
        document.getElementById("menuButton").className = "btn btn-primary";
        document.getElementById("popover4").className = "d-none";
        document.getElementById("validIcon").className = validIconState;

    }
    else{
        HELPACTIVATED = true;
        document.getElementById("helpButton").className = "btn btn-success";
        document.getElementById("submit").className = "d-none";
        document.getElementById("popover1").className = "popover popover-bottom";
        document.getElementById("validationField").className = "d-none";
        document.getElementById("popover2").className = "popover popover-bottom";
        document.getElementById("returnToCanvas").className = "d-none";
        document.getElementById("popover3").className = "popover popover-bottom";
        document.getElementById("menuButton").className = "d-none";
        document.getElementById("popover4").className = "popover popover-bottom";
    }
})
