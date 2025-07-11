class OBJParser:

    def __init__(self):
        self.vertices = []
        self.faces = []
        self.normals = []

    def read(self, f_in):
        f = open(f_in)
        for line in f:
            if line[0:2] == "v ":
                parameters = line.split()
                self.vertices.append([parameters[1], parameters[2], parameters[3]])
            elif line[0:2] == "vn":
                parameters = line.split()
                self.normals.append([parameters[1], parameters[2], parameters[3]])
            elif line[0:2] == "f ":
                parameters = line.split()
                self.faces.append([parameters[1], parameters[2], parameters[3]])
        f.close()

    def write(self, f_out):
        with open(f_out, "w") as f:
            for vertex in self.vertices:
                f.write("v " + vertex[0] + " " + vertex[1] + " " + vertex[2] + "\n")
            f.write("\n")
            for normal in self.normals:
                f.write("vn " + normal[0] + " " + normal[1] + " " + normal[2] + "\n")
            f.write("\n")
            for face in self.faces:
                f.write("f " + face[0] + " " + face[1] + " " + face[2] + "\n")
            f.close()

parser = OBJParser()
f_in = input("Enter input obj file: ")
f_out = input("Enter ouptut obj file: ")
parser.read(f_in)
parser.write(f_out)
