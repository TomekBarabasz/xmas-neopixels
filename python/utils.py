import json
from types import SimpleNamespace

def loadConfigFile(filename):
    with open(filename, "r") as your_file:
        your_dict = json.load(your_file)
        your_file.seek(0)
        return json.load(your_file, object_hook= lambda x: SimpleNamespace(**x))