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

import functools
import json
import os
from flask import (
    Blueprint, flash, render_template, request, url_for, current_app, redirect
)


bp = Blueprint('input',__name__)

@bp.route('/editModule',methods=('GET','POST'))
def editModule():
    if request.method == 'POST':
        # Saves the json file either internally to work on it or externally on the directory given by DAQ_CONFIGS_DIR
        if request.args.get('type') == "outFile": # External save
            data = request.data
            DAQ_CONFIG_DIR = os.environ.get("DAQ_CONFIG_DIR")
            fileName = DAQ_CONFIG_DIR+'/'+request.args.get('currentFileName')
            with open(fileName,'w') as f:
                f.write(data.decode("utf-8"))
        elif request.args.get('type') == "tempFile": # Internal save
            data = request.data
            path_dummy = current_app.root_path + url_for('static',filename='json/dummyFile.json')
            with open(path_dummy,'w') as f:
                f.write(data.decode("utf-8"))
        elif request.args.get("type") == "selectSchema": # Change selected schema
            path_schema = current_app.root_path + url_for('static',filename='schema/config-schema.json')
            DAQ_CONFIG_DIR = os.environ.get("DAQ_CONFIG_DIR")
            currentSchemaName = request.args.get("schema")
            with open(DAQ_CONFIG_DIR+"schemas/resolved/"+currentSchemaName,'r') as schema,open(path_schema,'w') as internalSchema:
                internalSchema.write(schema.read())

    return render_template('/editModule.html',currentFileName=request.args.get('currentFileName'))
