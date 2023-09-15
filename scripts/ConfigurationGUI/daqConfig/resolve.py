#!/usr/bin/env python3.6
import sys
from os import listdir,remove
from os.path import isfile, join
import jsonref
import os
from os import environ as env
from pathlib import Path
import pprint

DAQ_CONFIG_DIR=env['DAQ_CONFIG_DIR']

schemaFiles = [schema for schema in listdir(DAQ_CONFIG_DIR+"/schemas") if isfile(join(DAQ_CONFIG_DIR+"/schemas", schema))]
os.makedirs(DAQ_CONFIG_DIR+"/schemas/resolved", exist_ok=True)
base_dir_uri = Path(DAQ_CONFIG_DIR).as_uri() + '/schemas/'

for schema in schemaFiles:
    val = (DAQ_CONFIG_DIR+"schemas/resolved/"+schema)
    tmpval = (DAQ_CONFIG_DIR+"schemas/resolved/tmp"+schema)
    Path(val).touch()
    Path(tmpval).touch()

    with open(DAQ_CONFIG_DIR+"schemas/"+schema, "r") as r, open(val,"w") as w, open(tmpval,"r+") as rw:
        original_stdout = sys.stdout
        sys.stdout = rw
        jsonref_obj = jsonref.load(r, base_uri=base_dir_uri, loader=jsonref.JsonLoader())
        pprint.pprint(jsonref_obj,width=sys.maxsize)
        rw.seek(0)
        for line in rw:
            w.write(line.replace("'",'"').replace("False",'false').replace("True",'true'))
            sys.stdout = original_stdout
        os.remove(tmpval)
