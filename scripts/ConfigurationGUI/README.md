# DAQling configuration GUI

A GUI to help writing JSON configuration files for DAQling

## Install

You might want to make the installation in a python3 virtual environment.

1) `pip install daqConfig-\*.\*.\*-py3-none-any.whl`

## Run

You might also want to run the server in a python3 virtual environment.

1) `./run.sh`

## Utilisation

An help button is available to give information about the interface. When the help button is highlighted in green, passing the mouse over the interface elements give explanations about them.

When a module is created, it can be dragged on the whiteboard by holding the top square. The "S" button adds a new sender connector, the "R" button adds a new receiver connector. Two connectors can be linked to each other by pressing with a left click on the first connector and releasing on the second.

## Limitations

1) While creating a new connection, connection information are asked to the user. Most of the fields are automatically filled, but selecting a transport type with less fields and going back to another one with more fields will leave empty fields. The connection can be canceled and recreated to fix the problem.

2) The settings of a module are related to its type, but when a type is chosen, the settings are not automatically set to the correct format. The user has to select the correct one using the JSON editor.

3) Adding any type of connectors(sender or receiver), not connecting the connector to another one and then going under the JSON editor page will have the result to partially fill the connector information. Going back to the whiteboard and then linking the connector will erase the previously written unwanted information.
