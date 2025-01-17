import os

sourcepath = os.path.join(os.path.dirname(os.path.realpath(__file__)), "paraview_utils")

fibers = ProgrammableSource(
    PythonPath="'{}'".format(sourcepath),
    Script=open(os.path.join(sourcepath, "fiber_reader.py"), "r").read(),
    ScriptRequestInformation=open(os.path.join(sourcepath, "fiber_reader_request.py"), "r").read(),
    OutputDataSetType="vtkPolyData",
    guiName="Fibers",
)

tf = Tube(fibers)
tf.Radius = 0.025
tf.Capping = 1
tf.NumberofSides = 10

Show(tf)
SetDisplayProperties(DiffuseColor=[0 / 255, 255 / 255, 127 / 255])

bodies = ProgrammableSource(
    PythonPath="'{}'".format(sourcepath),
    Script=open(os.path.join(sourcepath, "body_reader.py"), "r").read(),
    ScriptRequestInformation=open(os.path.join(sourcepath, "body_reader_request.py"), "r").read(),
    OutputDataSetType="vtkMultiblockDataSet",
    guiName="Bodies",
)

sf = Smooth(bodies)
Show(sf)
SetDisplayProperties(DiffuseColor=[143 / 255, 255 / 255, 246 / 255])
