"""
 Copyright (C) 2019-2021 CERN
 
 DAQling is free software: you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 DAQling is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public License
 along with DAQling. If not, see <http://www.gnu.org/licenses/>.
"""

from flask import (
    Blueprint, flash, render_template, request, url_for, redirect, current_app
)
from os import listdir,remove
from os.path import isfile, join
import json

import os

bp = Blueprint('initPage',__name__)
@bp.route('/',methods=('GET','POST'))
def index():
    # Path for the temporary file json and its position on the canvas
    path_dummy = current_app.root_path + url_for('static',filename='json/dummyFile.json')
    path_position = current_app.root_path + url_for('static',filename='json/positiondummyFile.json')
    path_schema = current_app.root_path + url_for('static',filename='schema/config-schema.json')
    DAQ_CONFIG_DIR = os.environ.get("DAQ_CONFIG_DIR")
    # List files in the DAQ_CONFIG_DIR directory
    jsonFiles = [f for f in listdir(DAQ_CONFIG_DIR) if isfile(join(DAQ_CONFIG_DIR, f)) and join(DAQ_CONFIG_DIR, f).endswith(".json")]
    schemaFiles = [schema for schema in listdir(DAQ_CONFIG_DIR+"/schemas/resolved") if isfile(join(DAQ_CONFIG_DIR+"/schemas/resolved", schema))]
    if request.method == 'POST':
            if request.form["actionType"] == "New": # Working on a new configuration file
                currentFileName = request.form["fileName"]
                with open(path_dummy,'w') as f:
                    f.write("{\"common\": {},\"configuration\": {\"components\": []}}") # Erases the temporary file
                if os.path.exists(path_position):
                    os.remove(path_position)
                return redirect(url_for('initPage.canvas',currentFileName=currentFileName))
            elif(request.form["actionType"] == "Edit"): # Editing a pre-existing configuration file.
                currentFileName = request.form["fileName"]
                with open(DAQ_CONFIG_DIR+'/'+currentFileName,'r') as data, open(path_dummy,'w') as f:
                    f.write(data.read()) # Copy the pre-existing file into the temporary file
                return redirect(url_for('initPage.canvas',currentFileName=currentFileName))
            elif(request.form["actionType"] == "Select"):
                currentSchemaName = request.form["fileName"]
                with open(DAQ_CONFIG_DIR+"/schemas/"+currentSchemaName,'r') as schema,open(path_schema,'w') as internalSchema:
                    internalSchema.write(schema.read())

    return render_template('menu.html',jsonFiles=jsonFiles,schemaFiles=schemaFiles)

@bp.route('/canvas',methods=('GET','POST'))
def canvas():
    if request.args.get('currentFileName') is None: # Current file name for saving purposes
        currentFileName = ""
    else:
        currentFileName = request.args['currentFileName']
    if request.args.get('save') == "true": # Saves position on the canvas
        data = request.data
        path = current_app.root_path + url_for('static',filename='json/position'+currentFileName)
        with open(path,'w') as f:
            f.write(data.decode("utf-8"))

    return render_template('canvas.html',currentFileName=currentFileName)
